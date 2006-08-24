/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003
 *     Brian Gerkey
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * A driver to provide access to the ALSA sound system.
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_alsa alsa
 * @brief Linux ALSA sound system driver

This driver provides access to sound playing and recording functionality through
the Advanced linux Sound Architecture (ALSA) system available on 2.6 series
kernels (and before via patches/separate libraries).

Not all of the audio interface is supported. Currently supported features are:

PLAYER_AUDIO_WAV_PLAY_CMD - Play raw PCM wave data
PLAYER_AUDIO_SAMPLE_PLAY_CMD - Play locally stored and remotely provided samples
PLAYER_AUDIO_MIXER_CHANNEL_CMD - Change volume levels
PLAYER_AUDIO_SAMPLE_LOAD_REQ - Store samples provided by remote clients (max 1MB)
PLAYER_AUDIO_MIXER_CHANNEL_LIST_REQ - Get channel details
PLAYER_AUDIO_MIXER_CHANNEL_LEVEL_REQ - Get volume levels
PLAYER_AUDIO_SAMPLE_RETRIEVE_REQ - Send stored samples to remote clients (max 1MB)

Known bugs:
- Sounds may skip just as they finish playing. This is something to do with the
  call to snd_pcm_drain, but I haven't figured out why yet.

@par Samples

Locally stored samples are preferred to samples loaded over the network or using
the PLAYER_AUDIO_WAV_PLAY_CMD message for a number of reasons:

- It takes time to transfer large quantities of wave data over the network.
- Remotely provided samples are stored in memory, local samples are only loaded
when played. If you have a lot of samples provided remotely, the memory use will
be high.
- Local samples are not limited to only the formats (bits per sample, sample
rate, etc) that player supports. They can be any standard WAV format file that
uses a format for the audio data supported by ALSA.
- Remote samples can only be up to 1MB in size. This limits you to about 6
seconds of audio data at 44100Hz, 16 bit, stereo. Local samples can be as big
as you have memory. A future version of the driver will implement play-on-read,
meaning local samples will only be limited by disc space to store them.

When using the PLAYER_AUDIO_SAMPLE_LOAD_REQ message to store samples, currently
only appending and overwriting existing samples is allowed. Trying to store at
a specific index greater than the number of currently stored samples will result
in an error. Note that the samples indices are 0-based, so if there are 5
samples stored and you request to store one at index 5 (technically beyond the
end of the list), it will append to the end and become the sample at index 5.
TODO: Talk to Toby to clarify his intentions for the index in this message.

@par Provides

The driver provides a single @ref interface_audio interface.

@par Configuration file options

- pbdevice (string)
  - Default: none
  - The device to use for playback.
  - If none, playback functionality will not be available.
  - e.g. "plughw:0,0"
  - The order of arguments in this string, according to the ALSA documentation,
    are card number or identifier, device number and subdevice.
- mixerdevice (string)
  - Default: none
  - The device to attach the mixer interface to
  - If none, mixer functionality will not be available.
  - e.g. "default"
- recdevice (string)
  - Default: none
  - The device to use for recording.
  - If none, record functionality will not be available.
  - e.g. "plughw:0,0"
  - The order of arguments in this string, according to the ALSA documentation,
    are card number or identifier, device number and subdevice.
- samples (tuple of strings)
  - Default: empty
  - The path of wave files to have as locally stored samples.
- pb_bufferlength (integer)
  - Default: 500ms
  - The length of the playback buffer. A longer buffer means less chance of
    skipping during playback.
- pb_periodlength (integer)
  - Default: 50ms
  - The length of a period. This is used to change how frequently the buffer is
    written to. The longer the period, the longer it takes to write, but also
    the less frequently it will be done.
- pb_silence (integer)
  - Default: 0ms
  - The length of silence to play between consecutive sounds. Useful if you
    don't want your sounds played right up next to eachother, but bad if you're
    streaming a sound that's bigger than a single wave data message.
  - If usequeue is false, this will be ignored.
- usequeue (boolean)
  - Default: true
  - Turns the queuing system on/off. When true, all PLAYER_AUDIO_WAV_PLAY_CMD
    and PLAYER_AUDIO_SAMPLE_PLAY_CMD commands will be added to a queue and
    played in order of request. When off, sending a new command will stop the
    currently playing sound and start the new one.
- rec_bufferlength (integer)
  - Default: 500ms
  - The length of the record buffer. A longer buffer means less chance of
    an underrun while recording.
- rec_periodlength (integer)
  - Default: 50ms
  - The length of a period for recording. This is used to change how frequently
    the buffer is read from. The longer the period, the longer between reads,
    but also the less frequently it will be done.
- rec_nch (integer)
  - Default: 1
  - Number of recording channels.
- rec_sr (integer)
  - Default: 44100
  - Recording sample rate.
- rec_bits (integer)
  - Default: 16
  - Bits per sample for recording.

@par Example

@verbatim
driver
(
  name "alsa"
  provides ["audio:0"]
  samples ["sample1.wav" "sample2.wav" "sample3.wav"]
)
@endverbatim

@author Geoffrey Biggs
 */
/** @} */

#include <libplayercore/playercore.h>
#include <sys/time.h>

#include "alsa.h"

// Initialisation function
Driver* Alsa_Init (ConfigFile* cf, int section)
{
	return reinterpret_cast<Driver*> (new Alsa (cf, section));
}

// Register function
void Alsa_Register (DriverTable* table)
{
	table->AddDriver("alsa", Alsa_Init);
}

////////////////////////////////////////////////////////////////////////////////
//	Stored sample functions
////////////////////////////////////////////////////////////////////////////////

// Adds a new stored sample (already initialised) to the linked list
bool Alsa::AddStoredSample (StoredSample *newSample)
{
	if (samplesHead == NULL)
	{
		samplesHead = samplesTail = newSample;
	}
	else
	{
		samplesTail->next = newSample;
		samplesTail = newSample;
	}

	return true;
}

bool Alsa::AddStoredSample (player_audio_wav_t *waveData)
{
	StoredSample *newSample = NULL;
	if ((newSample = new StoredSample) == NULL)
	{
		PLAYER_ERROR ("Failed to allocate memory for new stored sample");
		return false;
	}

	if ((newSample->sample = new AudioSample) == NULL)
	{
		PLAYER_ERROR ("Failed to allocate memory for new stored audio sample");
		delete newSample;
		return false;
	}

	if (!newSample->sample->FromPlayer (waveData))
	{
		delete newSample->sample;
		delete newSample;
		return false;
	}

	newSample->index = nextSampleIdx++;
	newSample->next = NULL;

	printf ("ALSA: Added stored sample to list at index %d\n", newSample->index);
	return AddStoredSample (newSample);
}

bool Alsa::AddStoredSample (const char *filePath)
{
	StoredSample *newSample = NULL;
	if ((newSample = new StoredSample) == NULL)
	{
		PLAYER_ERROR ("Failed to allocate memory for new stored sample");
		return false;
	}

	if ((newSample->sample = new AudioSample) == NULL)
	{
		PLAYER_ERROR ("Failed to allocate memory for new stored audio sample");
		delete newSample;
		return false;
	}

	if (!newSample->sample->LoadFile (filePath))
	{
		delete newSample->sample;
		delete newSample;
		return false;
	}

	newSample->index = nextSampleIdx++;
	newSample->next = NULL;

	printf ("ALSA: Added stored sample %s list at index %d\n", filePath, newSample->index);
	return AddStoredSample (newSample);
}

// Finds the sample with the specified index
StoredSample* Alsa::GetSampleAtIndex (int index)
{
	if (samplesHead)
	{
		StoredSample *currentSample = samplesHead;
		while (currentSample != NULL)
		{
			if (currentSample->index == index)
				return currentSample;
			currentSample = currentSample->next;
		}
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//	Queue management functions
////////////////////////////////////////////////////////////////////////////////

// Deletes all data stored in the queue
void Alsa::ClearQueue (void)
{
	QueueItem *currentItem = queueHead;
	QueueItem *previousItem = NULL;

	while (currentItem != NULL)
	{
		if (currentItem->temp)
			delete currentItem->sample;
		previousItem = currentItem;
		currentItem = currentItem->next;
		delete previousItem;
	}
	queueHead = NULL;
	queueTail = NULL;
}

bool Alsa::AddToQueue (QueueItem *newItem)
{
	if (!useQueue)	// If configured to not use a queue, clear out current queue first
	{
		StopPlayback ();	// Must stop playback before deleting the data being played
		ClearQueue ();
	}

	if (queueHead == NULL)
	{
		queueHead = queueTail = newItem;
	}
	else
	{
		queueTail->next = newItem;
		queueTail = newItem;
	}

	return true;
}

bool Alsa::AddToQueue (player_audio_wav_t *waveData)
{
	QueueItem *newItem = NULL;

	if ((newItem = new QueueItem) == NULL)
	{
		PLAYER_ERROR ("Failed to allocate memory for new queue item");
		return false;
	}

	if ((newItem->sample = new AudioSample) == NULL)
	{
		PLAYER_ERROR ("Failed to allocate memory for new audio sample");
		delete newItem;
		return false;
	}

	if (!newItem->sample->FromPlayer (waveData))
	{
		delete newItem->sample;
		delete newItem;
		return false;
	}

	newItem->temp = true;
	newItem->next = NULL;

	// If silence is wanted between samples, add it now (but only if not
	// the first thing in the queue)
	if (silenceTime != 0 && queueHead != NULL)
	{
		if (!AddSilence (silenceTime, newItem->sample))
		{
			delete newItem->sample;
			delete newItem;
			return false;
		}
	}

	return AddToQueue (newItem);
}

bool Alsa::AddToQueue (AudioSample *sample)
{
	QueueItem *newItem = NULL;

	if ((newItem = new QueueItem) == NULL)
	{
		PLAYER_ERROR ("Failed to allocate memory for new queue item");
		return false;
	}

	newItem->sample = sample;
	newItem->temp = false;
	newItem->next = NULL;

	// If silence is wanted between samples, add it now (but only if not
	// the first thing in the queue)
	if (silenceTime != 0 && queueHead != NULL)
	{
		if (!AddSilence (silenceTime, sample))
		{
			delete newItem;
			return false;
		}
	}

	return AddToQueue (newItem);
}

// Adds a block of silence into the queue as an audio sample
// time: The length of silence to add
// format: A pointer to another audio sample who's format should be copied
// Returns: true on success, false otherwise
bool Alsa::AddSilence (uint32_t time, AudioSample *format)
{
	QueueItem *newItem = NULL;

	if ((newItem = new QueueItem) == NULL)
	{
		PLAYER_ERROR ("Failed to allocate memory for silence queue item");
		return false;
	}

	if ((newItem->sample = new AudioSample) == NULL)
	{
		PLAYER_ERROR ("Failed to allocate memory for silence audio sample");
		delete newItem;
		return false;
	}

	newItem->temp = true;
	newItem->next = NULL;

	// Empty the new sample
	newItem->sample->ClearSample ();
	// Copy the format of the provided sample
	newItem->sample->CopyFormat (format);
	// Fill it up with silence
	if (!newItem->sample->FillSilence (time))
	{
		delete newItem->sample;
		delete newItem;
		return false;
	}
	// Add it to the queue
	return AddToQueue (newItem);
}

void Alsa::AdvanceQueue (void)
{
	// Move the queue head forward one
	QueueItem *oldHead = queueHead;
	queueHead = queueHead->next;

	// Delete the old head, including sample if necessary
	if (oldHead->temp)
		delete oldHead->sample;
	else
		// If the sample wasn't temp, rewind it
		oldHead->sample->SetDataPosition (0);
	delete oldHead;

/*	if (queueHead != NULL)
	{
		printf ("Playing sample:\n");
		queueHead->sample->PrintWaveInfo ();
	}*/
}

////////////////////////////////////////////////////////////////////////////////
//	Playback functions (setting params, writing data to the buffer, etc)
////////////////////////////////////////////////////////////////////////////////

bool Alsa::SetupPlayBack (void)
{
	// If no device configured, return
	if (!pbDevice)
		return false;

	// Open the pcm device in blocking mode
	if (snd_pcm_open (&pbHandle, pbDevice, SND_PCM_STREAM_PLAYBACK, 0) < 0)
	{
		PLAYER_ERROR1 ("Error opening PCM device %s for playback", pbDevice);
		return false;
	}

	// Set parameters for the pcm device
// 	if (!SetGeneralParams ())
// 		return -1;

	// Setup polling file descriptors
	numPBFDs = snd_pcm_poll_descriptors_count (pbHandle);
	if ((pbFDs = (struct pollfd*) new struct pollfd[numPBFDs]) == NULL)
	{
		PLAYER_ERROR ("Error allocating memory for playback file descriptors");
		return false;
	}
	snd_pcm_poll_descriptors (pbHandle, pbFDs, numPBFDs);

	return true;
}

// Sets the hardware parameters of the sound device to the provided wave data's format
bool Alsa::SetPBParams (AudioSample *sample)
{
	snd_pcm_hw_params_t *hwparams;			// Hardware parameters
	snd_pcm_sw_params_t *swparams;			// Software parameters

	// Allocate params structure on stack
	snd_pcm_hw_params_alloca(&hwparams);

	// Init parameters
	if (snd_pcm_hw_params_any (pbHandle, hwparams) < 0)
	{
		PLAYER_ERROR ("Cannot configure this playback device");
		return false;
	}

	unsigned int exactRate;	// Sample rate returned by snd_pcm_hw_params_set_rate_near

	// Use interleaved access
	if (snd_pcm_hw_params_set_access (pbHandle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
	{
		PLAYER_ERROR ("Error setting interleaved access for playback device");
		return false;
	}

	// Set sound format
	snd_pcm_format_t format;
	switch (sample->GetBitsPerSample ())
	{
		case 8:
			format = SND_PCM_FORMAT_U8;
			break;
		case 16:
			format = SND_PCM_FORMAT_S16_LE;
			break;
		case 24:
			if ((sample->GetBlockAlign () / sample->GetNumChannels ()) == 3)
				format = SND_PCM_FORMAT_S24_3LE;
			else
				format = SND_PCM_FORMAT_S24_LE;
			break;
		case 32:
			format = SND_PCM_FORMAT_S32_LE;
			break;
		default:
			PLAYER_ERROR ("Cannot play audio with this format");
			return false;
	}
	if (snd_pcm_hw_params_set_format (pbHandle, hwparams, format) < 0)
	{
		PLAYER_ERROR ("Error setting format for playback device");
		return false;
	}
	// Set sample rate
	exactRate = sample->GetSampleRate ();
	if (snd_pcm_hw_params_set_rate_near (pbHandle, hwparams, &exactRate, 0) < 0)
	{
		PLAYER_ERROR ("Error setting sample rate for playback device");
		return false;
	}
	if (exactRate != sample->GetSampleRate ())
		PLAYER_WARN2 ("Rate %dHz not supported by hardware for playback device, using %dHz instead", sample->GetSampleRate (), exactRate);

	// Set number of channels
	if (snd_pcm_hw_params_set_channels(pbHandle, hwparams, sample->GetNumChannels ()) < 0)
	{
		PLAYER_ERROR ("Error setting channels for playback device");
		return false;
	}

	// Set the length of the buffer
	actPBBufferTime = cfgPBBufferTime * 1000;
	if (snd_pcm_hw_params_set_buffer_time_near (pbHandle, hwparams, &actPBBufferTime, 0) < 0)
	{
		PLAYER_ERROR ("Error setting periods for playback device");
		return false;
	}
	if (actPBBufferTime < cfgPBBufferTime * 900)	// cfgPBBufferTime * 1000 * 9/10
		PLAYER_WARN2 ("Buffer length for playback device reduced from %dus to %dus", cfgPBBufferTime * 1000, actPBBufferTime);

// 	snd_pcm_hw_params_get_buffer_size (hwparams, &pbPeriodSize);

	// Set the length of a period
	actPBPeriodTime = cfgPBPeriodTime * 1000;
	if (actPBPeriodTime > (actPBBufferTime / 2))
	{
		actPBPeriodTime = (actPBBufferTime / 2);
		PLAYER_WARN1 ("Period time for playback device too long, reduced to %dms", actPBPeriodTime / 1000);
	}
	if (snd_pcm_hw_params_set_period_time_near (pbHandle, hwparams, &actPBPeriodTime, 0) < 0)
	{
		PLAYER_ERROR ("Error setting period time for playback device");
		return false;
	}
	if (actPBPeriodTime < cfgPBPeriodTime * 900)	// cfgPBPeriodTime * 1000 * 9/10
		PLAYER_WARN2 ("Period length for playback device reduced from %dms to %dms", cfgPBPeriodTime, actPBPeriodTime / 1000);

	snd_pcm_hw_params_get_period_size (hwparams, &pbPeriodSize, 0);

	// Allocate a buffer the size of one period
	if (periodBuffer != NULL)
		delete[] periodBuffer;
	if ((periodBuffer = new uint8_t[pbPeriodSize * sample->GetBlockAlign ()]) == NULL)
	{
		PLAYER_ERROR ("Failed to allocate memory for period buffer");
		return false;
	}

	// Apply hwparams to the pcm device
	if (snd_pcm_hw_params (pbHandle, hwparams) < 0)
	{
		PLAYER_ERROR ("Error setting HW params for playback device");
		return false;
	}

	// Set software parameters for the pcm device
	snd_pcm_sw_params_alloca (&swparams);	// Allocate params structure on stack
	// Get the current software parameters
	if (snd_pcm_sw_params_current (pbHandle, swparams) < 0)
	{
		PLAYER_ERROR ("Error getting current SW params for playback device");
		return false;
	}
	// Set notification of pbBufSize bytes available for writing
	if (snd_pcm_sw_params_set_avail_min (pbHandle, swparams, pbPeriodSize) < 0)
	{
		PLAYER_ERROR ("Error setting avil_min notification for playback device");
		return false;
	}
	// Set the paramters on the device
	if (snd_pcm_sw_params (pbHandle, swparams) < 0)
	{
		PLAYER_ERROR ("Error setting SW params for playback device");
		return false;
	}

	return true;
}

// // Sets the general hardware parameters of the sound device
// // (Those not affected by wave format)
// bool Alsa::SetGeneralParams (void)
// {
// 	snd_pcm_sw_params_t *swparams;			// Software parameters
// 	snd_pcm_hw_params_t *hwparams;			// Hardware parameters
// 	snd_pcm_hw_params_alloca (&hwparams);	// Allocate params structure on stack
//
// 	// Init parameters
// 	if (snd_pcm_hw_params_any (pbHandle, hwparams) < 0)
// 	{
// 		PLAYER_ERROR ("Cannot configure this PCM device");
// 		return false;
// 	}
//
// // 	int periods = 2;		// Number of periods
// // 	snd_pcm_uframes_t periodSize = 8192;	// Periodsize (bytes) of the output buffer
//
// 	// Use interleaved access
// 	if (snd_pcm_hw_params_set_access (pbHandle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
// 	{
// 		PLAYER_ERROR ("Error setting interleaved access");
// 		return false;
// 	}
//
// 	// Set number of periods
// 	if (snd_pcm_hw_params_set_periods(pbHandle, hwparams, pbNumPeriods, 0) < 0)
// 	{
// 		PLAYER_ERROR ("Error setting periods");
// 		return false;
// 	}
//
// 	// Set the size of a period
// 	if (snd_pcm_hw_params_set_period_size (pbHandle, hwparams, pbPeriodSize, 0) < 0)
// 	{
// 		PLAYER_ERROR ("Error setting period size");
// 		return false;
// 	}
//
// 	// Apply hwparams to the pcm device
// 	if (snd_pcm_hw_params (pbHandle, hwparams) < 0)
// 	{
// 		PLAYER_ERROR ("Error setting HW params");
// 		return false;
// 	}
//
// 	// Don't need this anymore (I think) TODO: figure out why this causes glib errors
// // 	snd_pcm_hw_params_free (hwparams);
//
// 	// Set software parameters for the pcm device
// 	snd_pcm_sw_params_alloca (&swparams);	// Allocate params structure on stack
// 	// Get the current software parameters
// 	if (snd_pcm_sw_params_current (pbHandle, swparams) < 0)
// 	{
// 		PLAYER_ERROR ("Error getting current SW params");
// 		return false;
// 	}
// 	// Set notification of pbBufSize bytes available for writing
// 	if (snd_pcm_sw_params_set_avail_min (pbHandle, swparams, pbPeriodSize) < 0)
// 	{
// 		PLAYER_ERROR ("Error setting avil_min notification");
// 		return false;
// 	}
// 	// Set the paramters on the device
// 	if (snd_pcm_sw_params (pbHandle, swparams) < 0)
// 	{
// 		PLAYER_ERROR ("Error setting SW params");
// 		return false;
// 	}
//
// 	return true;
// }
//
// // Sets the hardware parameters of the sound device to the provided wave data's format
// bool Alsa::SetWaveHWParams (AudioSample *sample)
// {
// 	snd_pcm_hw_params_t *hwparams;			// Hardware parameters
// 	snd_pcm_hw_params_alloca(&hwparams);	// Allocate params structure on stack
//
// 	// Init parameters
// 	if (snd_pcm_hw_params_any (pbHandle, hwparams) < 0)
// 	{
// 		PLAYER_ERROR ("Cannot configure this PCM device");
// 		return false;
// 	}
//
// 	unsigned int exactRate;	// Sample rate returned by snd_pcm_hw_params_set_rate_near
//
// 	printf ("Stream state is %d\n", snd_pcm_state (pbHandle));
//
// 	// Set sound format
// 	snd_pcm_format_t format;
// 	switch (sample->GetBitsPerSample ())
// 	{
// 		case 8:
// 			format = SND_PCM_FORMAT_U8;
// 			break;
// 		case 16:
// 			format = SND_PCM_FORMAT_S16_LE;
// 			break;
// 		case 24:
// 			if ((sample->GetBlockAlign () / sample->GetNumChannels ()) == 3)
// 				format = SND_PCM_FORMAT_S24_3LE;
// 			else
// 				format = SND_PCM_FORMAT_S24_LE;
// 			break;
// 		case 32:
// 			format = SND_PCM_FORMAT_S32_LE;
// 			break;
// 		default:
// 			PLAYER_ERROR ("Cannot play audio with this format");
// 			return false;
// 	}
// 	if (snd_pcm_hw_params_set_format (pbHandle, hwparams, format) < 0)
// 	{
// 		PLAYER_ERROR ("Error setting format");
// 		return false;
// 	}
// 	// Set sample rate
// 	exactRate = sample->GetSampleRate ();
// 	if (snd_pcm_hw_params_set_rate_near (pbHandle, hwparams, &exactRate, 0) < 0)
// 	{
// 		PLAYER_ERROR ("Error setting sample rate");
// 		return false;
// 	}
// 	if (exactRate != sample->GetSampleRate ())
// 		PLAYER_WARN2 ("Rate %dHz not supported by hardware, using %dHz instead", sample->GetSampleRate (), exactRate);
//
// 	// Set number of channels
// 	if (snd_pcm_hw_params_set_channels(pbHandle, hwparams, sample->GetNumChannels ()) < 0)
// 	{
// 		PLAYER_ERROR ("Error setting channels");
// 		return false;
// 	}
//
// 	// Apply hwparams to the pcm device
// 	if (snd_pcm_hw_params (pbHandle, hwparams) < 0)
// 	{
// 		PLAYER_ERROR ("Error setting HW params");
// 		return false;
// 	}
//
// 	return true;
// }

// Called to write data to the playback buffer when it is ready for writing
// numFrames: The number of frames that can be written
void Alsa::PlaybackCallback (int numFrames)
{
	int framesToWrite = 0;

	// Get frames from audio samples until filled the buffer, or hit a sample
	// with a different format to the current sample
	while (framesToWrite < numFrames && playState == PB_STATE_PLAYING)
	{
		int framesToCopy = numFrames - framesToWrite;
		// Request frames from the sample
		// Want to get the number of frames not yet filled in the buffer and
		// place them however far into the buffer the last lot got up to
		int framesCopied = queueHead->sample->GetData (framesToCopy, &periodBuffer[framesToWrite * queueHead->sample->GetBlockAlign ()]);
		// If got no frames, something went wrong with this sample
		if (framesCopied < 0)
		{
			// If no frames copied so far, nothing to write so drain
			// The write after this won't happen because of the while loop condition
			PLAYER_ERROR ("Error reading wave data");
			playState = PB_STATE_DRAIN;
		}
		// If got less than requested, end of the current sample
		else if (framesCopied < framesToCopy)
		{
			// If the next sample has the same format as the current one, advance
			// the queue and begin copying from that instead
			if (queueHead->next != NULL)
			{
				if (queueHead->sample->SameFormat (queueHead->next->sample))
					AdvanceQueue ();
				// If it doesn't, move to drain state
				else
					playState = PB_STATE_DRAIN;
			}
			// If it doesn't, move to drain state
			else
			{
				playState = PB_STATE_DRAIN;
			}
			// Add the number of frames copied to the number to write
			framesToWrite += framesCopied;
		}
		// Got the requested number, so not much to do
		else
			framesToWrite += framesCopied;
	}

	// Keep writing until all the data we got has been written to the playback buffer
	uint8_t *dataPos = periodBuffer;
	while (framesToWrite > 0)
	{
		int framesWritten = snd_pcm_writei (pbHandle, dataPos, framesToWrite);
		if (framesWritten > 0 && framesWritten <= framesToWrite)
		{	// Not all was written
			snd_pcm_wait (pbHandle, 100);
			// Calculate how many frames remain unwritten
			framesToWrite -= framesWritten;
			// Move the data pointer appropriately
			dataPos += framesWritten * queueHead->sample->GetBlockAlign ();
		}
		else if (framesWritten == -EAGAIN)
		{	// Nothing was written, but not a disasterous error?
			snd_pcm_wait (pbHandle, 100);
		}
		else if (framesWritten == -EPIPE)
		{
			PLAYER_WARN ("Buffer underrun occured during playback");
			// Need to prepare the device again after an xrun
			snd_pcm_prepare (pbHandle);
		}
		else
		{
			PLAYER_ERROR2 ("Error writing to playback buffer: (%d) %s", framesWritten, snd_strerror (framesWritten));
		}
	};

// 	struct timeval timeVal;
// 	gettimeofday (&timeVal, NULL);
// 	printf ("%d.%d: Wrote %d bytes in total\n", timeVal.tv_sec, timeVal.tv_usec, totalFrames);
// 	fflush (NULL);

	// If state has moved to drain
	if (playState == PB_STATE_DRAIN)
	{
		// Tell the pcm device to drain the buffer
		snd_pcm_drain (pbHandle);
// 		struct timeval timeVal;
// 		gettimeofday (&timeVal, NULL);
// 		printf ("%d.%d: Set to drain\n", timeVal.tv_sec, timeVal.tv_usec);
// 		fflush (NULL);
	}
}

////////////////////////////////////////////////////////////////////////////////
//	Record functions (setting params, reading data from the buffer, etc)
////////////////////////////////////////////////////////////////////////////////

bool Alsa::SetupRecord (void)
{
	// If no device configured, return
	if (!recDevice)
		return false;

	// Open the pcm device in blocking mode
	if (snd_pcm_open (&recHandle, recDevice, SND_PCM_STREAM_CAPTURE, 0) < 0)
	{
		PLAYER_ERROR1 ("Error opening PCM device %s for recording", recDevice);
		return false;
	}

	// Set hardware/software parameters
	if (!SetRecParams ())
		return false;

	// Setup polling file descriptors
	numRecFDs = snd_pcm_poll_descriptors_count (recHandle);
	if ((recFDs = (struct pollfd*) new struct pollfd[numRecFDs]) == NULL)
	{
		PLAYER_ERROR ("Error allocating memory for record file descriptors");
		return false;
	}
	snd_pcm_poll_descriptors (recHandle, recFDs, numRecFDs);

	return true;
}

// Sets the hardware parameters of the sound device to the provided wave data's format
bool Alsa::SetRecParams (void)
{
	snd_pcm_hw_params_t *hwparams;			// Hardware parameters
	snd_pcm_sw_params_t *swparams;			// Software parameters

	// Allocate params structure on stack
	snd_pcm_hw_params_alloca(&hwparams);

	// Init parameters
	if (snd_pcm_hw_params_any (recHandle, hwparams) < 0)
	{
		PLAYER_ERROR ("Cannot configure this recording device");
		return false;
	}

	unsigned int exactRate;	// Sample rate returned by snd_pcm_hw_params_set_rate_near

	// Use interleaved access
	if (snd_pcm_hw_params_set_access (recHandle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
	{
		PLAYER_ERROR ("Error setting interleaved access for recording device");
		return false;
	}

	// Set sound format
	snd_pcm_format_t format;
	switch (recBits)
	{
		case 8:
			format = SND_PCM_FORMAT_U8;
			break;
		case 16:
			format = SND_PCM_FORMAT_S16_LE;
			break;
		case 24:
			format = SND_PCM_FORMAT_S24_LE;
			break;
		case 32:
			format = SND_PCM_FORMAT_S32_LE;
			break;
		default:
			PLAYER_ERROR ("Cannot record audio with this format");
			return false;
	}
	if (snd_pcm_hw_params_set_format (recHandle, hwparams, format) < 0)
	{
		PLAYER_ERROR ("Error setting format for recording device");
		return false;
	}
	// Set sample rate
	exactRate = recSampleRate;
	if (snd_pcm_hw_params_set_rate_near (recHandle, hwparams, &exactRate, 0) < 0)
	{
		PLAYER_ERROR ("Error setting sample rate for recording device");
		return false;
	}
	if (exactRate != recSampleRate)
		PLAYER_WARN2 ("Rate %dHz not supported by hardware for recording device, using %dHz instead", recSampleRate, exactRate);
	recSampleRate = exactRate;

	// Set number of channels
	if (snd_pcm_hw_params_set_channels (recHandle, hwparams, recNumChannels) < 0)
	{
		PLAYER_ERROR ("Error setting channels for recording device");
		return false;
	}

	// Set the length of the buffer
	actRecBufferTime = cfgRecBufferTime * 1000;
	if (snd_pcm_hw_params_set_buffer_time_near (recHandle, hwparams, &actRecBufferTime, 0) < 0)
	{
		PLAYER_ERROR ("Error setting periods for recording device");
		return false;
	}
	if (actRecBufferTime < cfgRecBufferTime * 900)	// cfgPBBufferTime * 1000 * 9/10
		PLAYER_WARN2 ("Buffer length for recording device reduced from %dus to %dus", cfgRecBufferTime * 1000, actRecBufferTime);

// 	snd_pcm_hw_params_get_buffer_size (hwparams, &recPeriodSize);

	// Set the length of a period
	actRecPeriodTime = cfgRecPeriodTime * 1000;
	if (actRecPeriodTime > (actRecBufferTime / 2))
	{
		actRecPeriodTime = (actRecBufferTime / 2);
		PLAYER_WARN1 ("Period time for recording device too long, reduced to %dms", actRecPeriodTime / 1000);
	}
	if (snd_pcm_hw_params_set_period_time_near (recHandle, hwparams, &actRecPeriodTime, 0) < 0)
	{
		PLAYER_ERROR ("Error setting period time for recording device");
		return false;
	}
	if (actRecPeriodTime < cfgRecPeriodTime * 900)	// cfgPBPeriodTime * 1000 * 9/10
		PLAYER_WARN2 ("Period length for recording device reduced from %dms to %dms", cfgRecPeriodTime, actRecPeriodTime / 1000);

	snd_pcm_hw_params_get_period_size (hwparams, &recPeriodSize, 0);

	// Apply hwparams to the pcm device
	if (snd_pcm_hw_params (recHandle, hwparams) < 0)
	{
		PLAYER_ERROR ("Error setting HW params for recording device");
		return false;
	}

	// Set software parameters for the pcm device
	snd_pcm_sw_params_alloca (&swparams);	// Allocate params structure on stack
	// Get the current software parameters
	if (snd_pcm_sw_params_current (recHandle, swparams) < 0)
	{
		PLAYER_ERROR ("Error getting current SW params for recording device");
		return false;
	}
	// Set notification of pbBufSize bytes available for writing
	if (snd_pcm_sw_params_set_avail_min (recHandle, swparams, recPeriodSize) < 0)
	{
		PLAYER_ERROR ("Error setting avil_min notification for recording device");
		return false;
	}
	// Set the paramters on the device
	if (snd_pcm_sw_params (recHandle, swparams) < 0)
	{
		PLAYER_ERROR ("Error setting SW params for recording device");
		return false;
	}

	return true;
}

// Called to write data to the playback buffer when it is ready for writing
// numFrames: The number of frames that can be written
void Alsa::RecordCallback (int numFrames)
{
	int totalRead = 0;

	// If nowhere to save the data, return
	if (!recData)
	{
		PLAYER_ERROR ("Tried to record to NULL data buffer");
		return;
	}

	while (totalRead < numFrames)
	{
		int framesToRead = numFrames - totalRead;
		if (snd_pcm_frames_to_bytes (recHandle, framesToRead) + recData->data_count > PLAYER_AUDIO_WAV_BUFFER_SIZE)
			// Don't read past the end of the buffer
			framesToRead = snd_pcm_bytes_to_frames (recHandle, PLAYER_AUDIO_WAV_BUFFER_SIZE - recData->data_count);
		int framesRead = snd_pcm_readi (recHandle, &recData->data[recData->data_count], framesToRead);
		// If got data
		if (framesRead > 0)
		{
			recData->data_count += snd_pcm_frames_to_bytes (recHandle, framesRead);
			totalRead += framesRead;
			// If this buffer is full, publish the data (resetting the buffer to zero)
			if (recData->data_count == PLAYER_AUDIO_WAV_BUFFER_SIZE)
				PublishRecordedData ();
		}
		// Overrun
		else if (framesRead == -EPIPE)
		{
			PLAYER_WARN ("Buffer overrun occured during recording");
			// Need to prepare the device again after an xrun
			snd_pcm_prepare (recHandle);
		}
		// Some other error
		else
		{
			PLAYER_ERROR2 ("Error reading from record buffer: (%d) %s", framesRead, snd_strerror (framesRead));
			StopRecording ();
		}
	}
}

void Alsa::PublishRecordedData (void)
{
	// Don't do anything if there is no data
	if (!recData)
		return;
	if (recData->data_count == 0)
		return;
	// Publish the recorded data
	Publish (device_addr, NULL, PLAYER_MSGTYPE_DATA, PLAYER_AUDIO_WAV_REC_DATA, reinterpret_cast<void*> (recData), sizeof (player_audio_wav_t), NULL);
	// Reset record position
	recData->data_count = 0;
}

////////////////////////////////////////////////////////////////////////////////
//	Playback/record control functions
////////////////////////////////////////////////////////////////////////////////

// Start outputting sound (if there is some to output)
void Alsa::StartPlayback (void)
{
	// Don't do anything if already playing or no device
	if (playState != PB_STATE_STOPPED || !pbHandle)
		return;

	// If there is data in the queue
	if (queueHead != NULL)
	{
// 		printf ("Playing sample:\n");
// 		queueHead->sample->PrintWaveInfo ();
		// Set the parameters for the head of the queue
		SetPBParams (queueHead->sample);
		// Set playback state to PLAYING
		playState = PB_STATE_PLAYING;
	}

	// Update clients about state
	SendStateMessage ();
}

// Stop outputting sound - actually more like a pause, as doesn't reset the
// queue position
void Alsa::StopPlayback (void)
{
	// Set playback to false
	playState = PB_STATE_STOPPED;
	// Drop anything currently in the buffer and stop the card
	snd_pcm_drop (pbHandle);

	// Update clients about state
	SendStateMessage ();
}

// Start recording sound
void Alsa::StartRecording (void)
{
	// Don't do anything if already recording or no device
	if (recState != PB_STATE_STOPPED || !recHandle)
		return;

	// Allocate a data storage area
	if (recData)
		PLAYER_WARN ("recData not empty before starting recording");
	if ((recData = new player_audio_wav_t) == NULL)
	{
		PLAYER_ERROR ("Failed to allocate memory for recorded data buffer");
		return;
	}
	recData->data_count = 0;
	// Set the format field of the data structure
/*	recData->ClearSample ();
	recData->SetType (SAMPLE_TYPE_MEM);
	recData->SetNumChannels (recNumChannels);
	recData->SetSampleRate (recSampleRate);
	recData->SetBitsPerSample (recBits);
	recData->SetBlockAlign ((recBits * recNumChannels) / 8);*/
	recData->format = PLAYER_AUDIO_FORMAT_RAW;
	if (recNumChannels == 2)
		recData->format |= PLAYER_AUDIO_STEREO;
	else if (recNumChannels != 1)
	{
		PLAYER_ERROR ("Cannot convert wave to player struct: wrong number of channels");
		delete recData;
		recData = NULL;
		return;
	}
	switch (recSampleRate)
	{
		case 11025:
			recData->format |= PLAYER_AUDIO_FREQ_11k;
			break;
		case 22050:
			recData->format |= PLAYER_AUDIO_FREQ_22k;
			break;
		case 44100:
			recData->format |= PLAYER_AUDIO_FREQ_44k;
			break;
		case 48000:
			recData->format |= PLAYER_AUDIO_FREQ_48k;
			break;
		default:
			PLAYER_ERROR ("Cannot convert wave to player struct: wrong sample rate");
			delete recData;
			recData = NULL;
			return;
	}

	switch (recBits)
	{
		case 8:
			recData->format |= PLAYER_AUDIO_8BIT;
			break;
		case 16:
			recData->format |= PLAYER_AUDIO_16BIT;
			break;
		case 24:
			recData->format |= PLAYER_AUDIO_24BIT;
			break;
		default:
			PLAYER_ERROR ("Cannot convert wave to player struct: wrong format (bits per sample)");
			delete recData;
			recData = NULL;
			return;
	}
	// Prepare the recording device
	snd_pcm_prepare (recHandle);
	// Start the recording device
	int result = 0;
	if ((result = snd_pcm_start (recHandle)) < 0)
	{
		PLAYER_ERROR2 ("Error starting recording: (%d) %s", result, snd_strerror (result));
		delete recData;
		return;
	}
	// Move to recording state
	recState = PB_STATE_RECORDING;

	// Update clients about state
	SendStateMessage ();
}

// Stop recording sound
void Alsa::StopRecording (void)
{
	// Stop the device
	snd_pcm_drop (recHandle);
	// Move to stopped state
	recState = PB_STATE_STOPPED;
	// If there is data left over, publish it
	PublishRecordedData ();
	delete recData;
	recData = NULL;

	// Update clients about state
	SendStateMessage ();
}

////////////////////////////////////////////////////////////////////////////////
//	Mixer functions (finding channels, setting levels, etc)
////////////////////////////////////////////////////////////////////////////////

// Opens the mixer interface and enumerates the mixer capabilities
bool Alsa::SetupMixer (void)
{
	// Open the mixer interface
	if (snd_mixer_open (&mixerHandle, 0) < 0)
	{
		PLAYER_WARN ("Could not open mixer");
		return false;
	}

	// Attach it to the device
	if (snd_mixer_attach (mixerHandle, mixerDevice) < 0)
	{
		PLAYER_WARN ("Could not attach mixer");
		return false;
	}

	// Register... something
	if (snd_mixer_selem_register (mixerHandle, NULL, NULL) < 0)
	{
		PLAYER_WARN ("Could not register mixer");
		return false;
	}

	// Load elements
	if (snd_mixer_load (mixerHandle) < 0)
	{
		PLAYER_WARN ("Could not load mixer elements");
		return false;
	}

	// Enumerate the elements
	if (!EnumMixerElements ())
		return false;

	return true;
}

// Enumerates the mixer elements - i.e. finds out what each is
// Prepares the found data to be used with player
bool Alsa::EnumMixerElements (void)
{
	MixerElement *elements = NULL;
	snd_mixer_elem_t *elem = NULL;
	uint32_t count = 0;

	// Count the number of elements to store
	for (elem = snd_mixer_first_elem (mixerHandle); elem != NULL; elem = snd_mixer_elem_next (elem))
	{
		if (snd_mixer_elem_get_type (elem) == SND_MIXER_ELEM_SIMPLE && snd_mixer_selem_is_active (elem))
			count++;
		else
			printf ("Skipping element\n");
	}

// 	printf ("Found %d elements to enumerate\n", count);

	// Allocate space to store the elements
	if (count <= 0)
	{
		PLAYER_WARN ("Found zero or less mixer elements");
		return false;
	}
	if ((elements = new MixerElement[count]) == NULL)
	{
		PLAYER_WARN1 ("Failed to allocate memory to store %d elements", count);
		return false;
	}
	memset (elements, 0, sizeof (MixerElement) * count);

	// Get each element and its capabilities
	uint32_t ii = 0;
	for (elem = snd_mixer_first_elem (mixerHandle); elem != NULL; elem = snd_mixer_elem_next (elem), ii++)
	{
		if (snd_mixer_elem_get_type (elem) == SND_MIXER_ELEM_SIMPLE && snd_mixer_selem_is_active (elem))
		{
			elements[ii].elem = elem;
			if (!EnumElementCaps (&(elements[ii])))
			{
				CleanUpMixerElements (elements, count);
				return false;
			}
		}
	}
	uint32_t newCount = count;
	// Split channels capable of both playback and capture (makes it easier to manage via player)
	if ((mixerElements = SplitElements (elements, newCount)) == NULL)
	{
		PLAYER_WARN ("Error splitting mixer elements");
		CleanUpMixerElements (elements, count);
		return false;
	}
	numElements = newCount;

	CleanUpMixerElements (elements, count);

// 	PrintMixerElements (mixerElements, numElements);
	return true;
}

// Enumerates the capabilities of a single element
bool Alsa::EnumElementCaps (MixerElement *element)
{
	snd_mixer_elem_t *elem = element->elem;
	if (!elem)
	{
		PLAYER_WARN ("Attempted to enumerate NULL element pointer");
		return false;
	}

	// Get the element name
	element->name = strdup (snd_mixer_selem_get_name (elem));
	// Get capabilities
	// Volumes
	if (snd_mixer_selem_has_playback_volume (elem))
		element->caps |= ELEMCAP_PLAYBACK_VOL;
	if (snd_mixer_selem_has_capture_volume (elem))
		element->caps |= ELEMCAP_CAPTURE_VOL;
	if (snd_mixer_selem_has_common_volume (elem))
		element->caps |= ELEMCAP_COMMON_VOL;
	// Switches
	if (snd_mixer_selem_has_playback_switch (elem))
		element->caps |= ELEMCAP_PLAYBACK_SWITCH;
	if (snd_mixer_selem_has_capture_switch (elem))
		element->caps |= ELEMCAP_CAPTURE_SWITCH;
	if (snd_mixer_selem_has_common_switch (elem))
		element->caps |= ELEMCAP_COMMON_SWITCH;
	// Joined switches
// 	if (snd_mixer_selem_has_playback_switch (elem))
// 		mixerElements[index].caps |= ELEMCAP_PB_JOINED_SWITCH;
// 	if (snd_mixer_selem_has_capture_switch (elem))
// 		mixerElements[index].caps |= ELEMCAP_CAP_JOINED_SWITCH;

	element->playSwitch = 1;
	element->capSwitch = 1;
	element->comSwitch = 1;

// 	printf ("Found mixer element: %s\n", element->name);
	// Find channels for this element
	for (int ii = -1; ii <= (int) SND_MIXER_SCHN_LAST; ii++)
	{
		if (snd_mixer_selem_has_playback_channel (elem, static_cast<snd_mixer_selem_channel_id_t> (ii)))
		{
// 			printf ("Element has playback channel %d: %s\n", ii, snd_mixer_selem_channel_name (static_cast<snd_mixer_selem_channel_id_t> (ii)));
			element->caps |= ELEMCAP_CAN_PLAYBACK;
			// Get the current volume of this channel and make it the element one, if don't have that yet
			if (!element->curPlayVol)
				snd_mixer_selem_get_playback_volume (elem, static_cast<snd_mixer_selem_channel_id_t> (ii), &(element->curPlayVol));
			// Get the switch status of this channel
			if (element->caps & ELEMCAP_PLAYBACK_SWITCH)
				snd_mixer_selem_get_playback_switch (elem, static_cast<snd_mixer_selem_channel_id_t> (ii), &element->playSwitch);
		}
		if (snd_mixer_selem_has_capture_channel (elem, static_cast<snd_mixer_selem_channel_id_t> (ii)))
		{
// 			printf ("Element has capture channel %d: %s\n", ii, snd_mixer_selem_channel_name (static_cast<snd_mixer_selem_channel_id_t> (ii)));
			element->caps |= ELEMCAP_CAN_CAPTURE;
			// Get the current volume of this channel and make it the element one, if don't have that yet
			if (!element->curCapVol)
				snd_mixer_selem_get_capture_volume (elem, static_cast<snd_mixer_selem_channel_id_t> (ii), &(element->curCapVol));
			// Get the switch status of this channel
			if (element->caps & ELEMCAP_CAPTURE_SWITCH)
				snd_mixer_selem_get_capture_switch (elem, static_cast<snd_mixer_selem_channel_id_t> (ii), &element->capSwitch);
		}
	}

	// Get volume ranges
	if ((element->caps & ELEMCAP_CAN_PLAYBACK) && (element->caps & ELEMCAP_PLAYBACK_VOL))
	{
		snd_mixer_selem_get_playback_volume_range (elem, &(element->minPlayVol), &(element->maxPlayVol));
	}
	if ((element->caps & ELEMCAP_CAN_CAPTURE) && (element->caps & ELEMCAP_CAPTURE_VOL))
	{
		snd_mixer_selem_get_capture_volume_range (elem, &(element->minCapVol), &(element->maxCapVol));
	}
	if (element->caps & ELEMCAP_COMMON_VOL)
	{
		// If statement on next line isn't a typo, min vol will probably be zero whether it's been filled in or not, max won't
		element->minComVol = element->maxPlayVol ? element->minPlayVol : element->minCapVol;
		element->maxComVol = element->maxPlayVol ? element->maxPlayVol : element->maxCapVol;
	}

	// Common switch status
	if (element->caps & ELEMCAP_COMMON_SWITCH)
		element->comSwitch = element->playSwitch ? element->playSwitch : element->capSwitch;

// 	printf ("Element volume levels:\n");
// 	printf ("Playback:\t%ld, %ld, %ld, %s\n", element->minPlayVol, element->curPlayVol, element->maxPlayVol, element->playSwitch ? "Active" : "Inactive");
// 	printf ("Capture:\t%ld, %ld, %ld, %s\n", element->minCapVol, element->curCapVol, element->maxCapVol, element->capSwitch ? "Active" : "Inactive");
// 	printf ("Common:\t%ld, %ld, %ld, %s\n", element->minComVol, element->curComVol, element->maxComVol, element->comSwitch ? "Active" : "Inactive");

	return true;
}

// Splits elements into two separate elements for those elements that are capable
// of entirely separate playback and capture
MixerElement* Alsa::SplitElements (MixerElement *elements, uint32_t &count)
{
	MixerElement *result = NULL;
	// Count the number of elements we will get as a result:
	// Each current element adds 2 if it does both with separate controls, 1 otherwise
	uint32_t numSplitElements = 0;
	for (uint32_t ii = 0; ii < count; ii++)
	{
		if ((elements[ii].caps & ELEMCAP_CAN_PLAYBACK) && (elements[ii].caps & ELEMCAP_CAN_CAPTURE) &&
				!(elements[ii].caps & ELEMCAP_COMMON_VOL) && !(elements[ii].caps & ELEMCAP_COMMON_SWITCH))
			numSplitElements += 2;
		else
			numSplitElements += 1;
	}

	// Allocate space for the new array of elements
	if (numSplitElements <= 0)
	{
		PLAYER_WARN ("Found zero or less split mixer elements");
		return NULL;
	}
	if ((result = new MixerElement[numSplitElements]) == NULL)
	{
		PLAYER_WARN1 ("Failed to allocate memory to store %d split elements", numSplitElements);
		return NULL;
	}
	memset (result, 0, sizeof (MixerElement) * numSplitElements);

	// Copy relevant data across
	uint32_t currentIndex = 0;
	for (uint32_t ii = 0; ii < count; ii++)
	{
		// Element capable of separate playback and record
		if ((elements[ii].caps & ELEMCAP_CAN_PLAYBACK) && (elements[ii].caps & ELEMCAP_CAN_CAPTURE) &&
				!(elements[ii].caps & ELEMCAP_COMMON_VOL) && !(elements[ii].caps & ELEMCAP_COMMON_SWITCH))
		{
			// In this case, split the element, so will set data for currentIndex and currentIndex+1
			// Playback element
			result[currentIndex].elem = elements[ii].elem;
			result[currentIndex].caps = ELEMCAP_CAN_PLAYBACK;
			result[currentIndex].minPlayVol = elements[ii].minPlayVol;
			result[currentIndex].curPlayVol = elements[ii].curPlayVol;
			result[currentIndex].maxPlayVol = elements[ii].maxPlayVol;
			result[currentIndex].playSwitch = elements[ii].playSwitch;
			result[currentIndex].name = reinterpret_cast<char*> (malloc (strlen (elements[ii].name) + strlen (" (Playback)") + 1));
			strncpy (result[currentIndex].name, elements[ii].name, strlen (elements[ii].name) + 1);
			strncpy (&(result[currentIndex].name[strlen (elements[ii].name)]), " (Playback)", strlen (" (Playback)") + 1);

			// Capture element
			result[currentIndex + 1].elem = elements[ii].elem;
			result[currentIndex + 1].caps = ELEMCAP_CAN_CAPTURE;
			result[currentIndex + 1].minCapVol = elements[ii].minCapVol;
			result[currentIndex + 1].curCapVol = elements[ii].curCapVol;
			result[currentIndex + 1].maxCapVol = elements[ii].maxCapVol;
			result[currentIndex + 1].capSwitch = elements[ii].capSwitch;
			result[currentIndex + 1].name = reinterpret_cast<char*> (malloc (strlen (elements[ii].name) + strlen (" (Capture)") + 1));
			strncpy (result[currentIndex + 1].name, elements[ii].name, strlen (elements[ii].name) + 1);
			strncpy (&(result[currentIndex + 1].name[strlen (elements[ii].name)]), " (Capture)", strlen (" (Capture)") + 1);

			currentIndex += 2;
		}
		// Element that can only playback
		else if ((elements[ii].caps & ELEMCAP_CAN_PLAYBACK) && !(elements[ii].caps & ELEMCAP_CAN_CAPTURE))
		{
			// Just copy in this case
			result[currentIndex].elem = elements[ii].elem;
			result[currentIndex].caps = ELEMCAP_CAN_PLAYBACK;
			result[currentIndex].minPlayVol = elements[ii].minPlayVol;
			result[currentIndex].curPlayVol = elements[ii].curPlayVol;
			result[currentIndex].maxPlayVol = elements[ii].maxPlayVol;
			result[currentIndex].playSwitch = elements[ii].playSwitch;
			result[currentIndex].name = strdup (elements[ii].name);

			currentIndex += 1;
		}
		// Element that can only capture
		else if (!(elements[ii].caps & ELEMCAP_CAN_PLAYBACK) && (elements[ii].caps & ELEMCAP_CAN_CAPTURE))
		{
			// Just copy in this case
			result[currentIndex].elem = elements[ii].elem;
			result[currentIndex].caps = ELEMCAP_CAN_CAPTURE;
			result[currentIndex].minCapVol = elements[ii].minCapVol;
			result[currentIndex].curCapVol = elements[ii].curCapVol;
			result[currentIndex].maxCapVol = elements[ii].maxCapVol;
			result[currentIndex].capSwitch = elements[ii].capSwitch;
			result[currentIndex].name = strdup (elements[ii].name);

			currentIndex += 1;
		}
		// Element that can do both but cannot set independent volumes
		else
		{
			result[currentIndex].elem = elements[ii].elem;
			result[currentIndex].caps = ELEMCAP_CAN_PLAYBACK & ELEMCAP_CAN_CAPTURE & ELEMCAP_COMMON;
			result[currentIndex].minComVol = elements[ii].minComVol;
			result[currentIndex].curComVol = elements[ii].curComVol;
			result[currentIndex].maxComVol = elements[ii].maxComVol;
			result[currentIndex].comSwitch = elements[ii].comSwitch;
			result[currentIndex].name = strdup (elements[ii].name);

			currentIndex += 1;
		}
	}

	count = numSplitElements;
	return result;
}

// Cleans up mixer element data
void Alsa::CleanUpMixerElements (MixerElement *elements, uint32_t count)
{
	for (uint32_t ii = 0; ii < count; ii++)
	{
		if (elements[ii].name)
			free (elements[ii].name);
	}
	delete[] elements;
}

// Converts mixer information to player details
void Alsa::MixerDetailsToPlayer (player_audio_mixer_channel_list_detail_t *dest)
{
	memset (dest, 0, sizeof (player_audio_mixer_channel_list_detail_t));

	dest->details_count = numElements;
	dest->default_output = 0;
	dest->default_input = 0;	// TODO: figure out what the default is... driver option maybe?

	for (uint32_t ii = 0; ii < numElements; ii++)
	{
		dest->details[ii].name_count = strlen (mixerElements[ii].name);
		strncpy (dest->details[ii].name, mixerElements[ii].name, strlen (mixerElements[ii].name) + 1);
		if ((mixerElements[ii].caps & ELEMCAP_CAN_PLAYBACK) && !(mixerElements[ii].caps & ELEMCAP_CAN_CAPTURE))
			dest->details[ii].caps = PLAYER_AUDIO_MIXER_CHANNEL_TYPE_OUTPUT;
		else if (!(mixerElements[ii].caps & ELEMCAP_CAN_PLAYBACK) && (mixerElements[ii].caps & ELEMCAP_CAN_CAPTURE))
			dest->details[ii].caps = PLAYER_AUDIO_MIXER_CHANNEL_TYPE_INPUT;
		else
			dest->details[ii].caps = PLAYER_AUDIO_MIXER_CHANNEL_TYPE_INPUT & PLAYER_AUDIO_MIXER_CHANNEL_TYPE_OUTPUT;
	}
}

// Converts mixer information to player levels
void Alsa::MixerLevelsToPlayer (player_audio_mixer_channel_list_t *dest)
{
	memset (dest, 0, sizeof (player_audio_mixer_channel_list_t));

	dest->channels_count = numElements;

	for (uint32_t ii = 0; ii < numElements; ii++)
	{
		long min = 0, cur = 0, max = 0;
		int switchStatus = 0;
		if (mixerElements[ii].caps & ELEMCAP_CAN_PLAYBACK)
		{
			min = mixerElements[ii].minPlayVol;
			cur = mixerElements[ii].curPlayVol;
			max = mixerElements[ii].maxPlayVol;
			switchStatus = mixerElements[ii].playSwitch;
		}
		else if (mixerElements[ii].caps & ELEMCAP_CAN_CAPTURE)
		{
			min = mixerElements[ii].minCapVol;
			cur = mixerElements[ii].curCapVol;
			max = mixerElements[ii].maxCapVol;
			switchStatus = mixerElements[ii].capSwitch;
		}
		else if (mixerElements[ii].caps & ELEMCAP_COMMON)
		{
			min = mixerElements[ii].minComVol;
			cur = mixerElements[ii].curComVol;
			max = mixerElements[ii].maxComVol;
			switchStatus = mixerElements[ii].comSwitch;
		}
		dest->channels[ii].amplitude = LevelToPlayer (min, max, cur);
		dest->channels[ii].active.state = switchStatus;
		dest->channels[ii].index = ii;
	}
}

// Sets the volume level of an element
void Alsa::SetElementLevel (uint32_t index, float level)
{
	long newValue = 0;

	if (mixerElements[index].caps & ELEMCAP_CAN_PLAYBACK)
	{
		// Calculate the new level
		newValue = LevelFromPlayer (mixerElements[index].minPlayVol, mixerElements[index].maxPlayVol, level);
		// Set the volume for all channels in this element
		if (snd_mixer_selem_set_playback_volume_all (mixerElements[index].elem, newValue) < 0)
		{
			PLAYER_WARN1 ("Error setting playback level for element %d", index);
		}
		else
			mixerElements[index].curPlayVol = newValue;
	}
	else if (mixerElements[index].caps & ELEMCAP_CAN_CAPTURE)
	{
		// Calculate the new level
		newValue = LevelFromPlayer (mixerElements[index].minCapVol, mixerElements[index].maxCapVol, level);
		// Set the volume for all channels in this element
		if (snd_mixer_selem_set_capture_volume_all (mixerElements[index].elem, newValue) < 0)
		{
			PLAYER_WARN1 ("Error setting capture level for element %d", index);
		}
		else
			mixerElements[index].curCapVol = newValue;
	}
	else if (mixerElements[index].caps & ELEMCAP_COMMON)
	{
		// Calculate the new level
		newValue = LevelFromPlayer (mixerElements[index].minComVol, mixerElements[index].maxComVol, level);
		// Set the volume for all channels in this element
		if (snd_mixer_selem_set_playback_volume_all (mixerElements[index].elem, newValue) < 0)
		{
			PLAYER_WARN1 ("Error setting common level for element %d", index);
		}
		else
			mixerElements[index].curComVol = newValue;
	}
}

// Sets the switch for an element
void Alsa::SetElementSwitch (uint32_t index, player_bool_t active)
{
	if (mixerElements[index].caps & ELEMCAP_CAN_PLAYBACK)
	{
		// Set the switch for all channels in this element
		if (snd_mixer_selem_set_playback_switch_all (mixerElements[index].elem, active.state) < 0)
		{
			PLAYER_WARN1 ("Error setting playback switch for element %d", index);
		}
		else
			mixerElements[index].playSwitch = active.state;
	}
	else if (mixerElements[index].caps & ELEMCAP_CAN_CAPTURE)
	{
		// Set the switch for all channels in this element
		if (snd_mixer_selem_set_capture_switch_all (mixerElements[index].elem, active.state) < 0)
		{
			PLAYER_WARN1 ("Error setting capture switch for element %d", index);
		}
		else
			mixerElements[index].capSwitch = active.state;
	}
	else if (mixerElements[index].caps & ELEMCAP_COMMON)
	{
		// Set the switch for all channels in this element
		if (snd_mixer_selem_set_playback_switch_all (mixerElements[index].elem, active.state) < 0)
		{
			PLAYER_WARN1 ("Error setting common switch for element %d", index);
		}
		else
			mixerElements[index].comSwitch = active.state;
	}
}

// Publishes mixer information as a data message
void Alsa::PublishMixerData (void)
{
	player_audio_mixer_channel_list_t data;

	MixerLevelsToPlayer (&data);
	Publish (device_addr, NULL, PLAYER_MSGTYPE_DATA, PLAYER_AUDIO_MIXER_CHANNEL_DATA, reinterpret_cast<void*> (&data), sizeof (player_audio_mixer_channel_list_t), NULL);
}

// Converts an element level from a long to a float between 0 and 1
float Alsa::LevelToPlayer (long min, long max, long level)
{
	float result = 0.0f;
	if ((max - min) != 0)
		result = static_cast<float> (level - min) / static_cast<float> (max - min);
	return result;
}

// Converts an element level from a float between 0 and 1 to a long between min and max
long Alsa::LevelFromPlayer (long min, long max, float level)
{
	long result = static_cast<long> ((max - min) * level);
	return result;
}

// Handy debug function
void Alsa::PrintMixerElements (MixerElement *elements, uint32_t count)
{
	long min, cur, max;
	int switchStatus;
	printf ("Mixer elements:\n");
	for (uint32_t ii = 0; ii < count; ii++)
	{
		if (elements[ii].caps & ELEMCAP_CAN_PLAYBACK)
		{
			min = elements[ii].minPlayVol;
			cur = elements[ii].curPlayVol;
			max = elements[ii].maxPlayVol;
			switchStatus = elements[ii].playSwitch;
		}
		else if (elements[ii].caps & ELEMCAP_CAN_CAPTURE)
		{
			min = elements[ii].minCapVol;
			cur = elements[ii].curCapVol;
			max = elements[ii].maxCapVol;
			switchStatus = elements[ii].capSwitch;
		}
		else if (elements[ii].caps & ELEMCAP_COMMON)
		{
			min = elements[ii].minComVol;
			cur = elements[ii].curComVol;
			max = elements[ii].maxComVol;
			switchStatus = elements[ii].comSwitch;
		}
		printf ("Element %d:\t%s\n", ii, elements[ii].name);
		printf ("Capabilities:\t");
		if (elements[ii].caps & ELEMCAP_CAN_PLAYBACK)
			printf ("playback\t");
		if (elements[ii].caps & ELEMCAP_CAN_CAPTURE)
			printf ("capture\t");
		if (elements[ii].caps & ELEMCAP_COMMON)
			printf ("common");
		printf ("\n");
		printf ("Volume range:\t%ld->%ld\n", min, max);
		printf ("Current volume:\t%ld\n", cur);
		printf ("Active:\t%s\n", switchStatus ? "Yes" : "No");
	}
}

////////////////////////////////////////////////////////////////////////////////
//	Driver management
////////////////////////////////////////////////////////////////////////////////

// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
Alsa::Alsa (ConfigFile* cf, int section)
    : Driver (cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_AUDIO_CODE)
{
	pbDevice = mixerDevice = recDevice = NULL;
	pbHandle = NULL;
	recHandle = NULL;
	mixerHandle = NULL;
	samplesHead = samplesTail = NULL;
	queueHead = queueTail = NULL;
	nextSampleIdx = 0;
	mixerElements = NULL;
	periodBuffer = NULL;
	pbFDs = recFDs = NULL;
	recData = NULL;

	// Read the config file options - see header for descriptions if not here
	useQueue = cf->ReadBool (section, "usequeue", true);
	pbDevice = strdup (cf->ReadString (section, "pbdevice", NULL));
	mixerDevice = strdup (cf->ReadString (section, "mixerdevice", NULL));
	recDevice = strdup (cf->ReadString (section, "recdevice", NULL));
	cfgPBPeriodTime = cf->ReadInt (section, "pb_periodlength", 50);
	cfgPBBufferTime = cf->ReadInt (section, "pb_bufferlength", 500);
	// Don't have silence if not using the queue system
	silenceTime = useQueue? cf->ReadInt (section, "pb_silence", 0) : 0;
	cfgRecPeriodTime = cf->ReadInt (section, "rec_periodlength", 50);
	cfgRecBufferTime = cf->ReadInt (section, "rec_bufferlength", 500);
	recNumChannels = cf->ReadInt (section, "rec_nch", 1);
	recSampleRate = cf->ReadInt (section, "rec_sr", 44100);
	recBits = cf->ReadInt (section, "rec_bits", 16);

	// Check recording rates are sane
	if (recNumChannels != 1 && recNumChannels != 2)
	{
		PLAYER_WARN ("Recording channels must be 1 or 2; recording functionality will not be available");
		if (recDevice)
			free (recDevice);
		recDevice = NULL;
	}
	if (recSampleRate != 11025 && recSampleRate != 22050 && recSampleRate != 44100 && recSampleRate != 48000)
	{
		PLAYER_WARN ("Recording sample rate must be one of 11025Hz, 22050Hz, 44100Hz, 48000Hz; recording functionality will not be available");
		if (recDevice)
			free (recDevice);
		recDevice = NULL;
	}
	if (recBits != 8 && recBits != 16)
	{
		PLAYER_WARN ("Recording bits per sample must be 8 or 16; recording functionality will not be available");
		if (recDevice)
			free (recDevice);
		recDevice = NULL;
	}

	// Read sample names
	int numSamples = cf->GetTupleCount (section, "samples");
	if (numSamples > 0)
	{
		for (int ii = 0; ii < numSamples; ii++)
		{
			if (!AddStoredSample (cf->ReadTupleString (section, "samples", ii, "error_bad_sample_path")))
			{
				PLAYER_ERROR1 ("Could not add audio sample %d", cf->ReadTupleString (section, "samples", ii, ""));
				return;
			}
		}
	}

	return;
}

// Destructor
Alsa::~ Alsa (void)
{
	if (pbDevice)
		free (pbDevice);
	if (mixerDevice)
		free (mixerDevice);
	if (recDevice)
		free (recDevice);
	if (samplesHead)
	{
		StoredSample *currentSample = samplesHead;
		StoredSample *previousSample = currentSample;
		while (currentSample != NULL)
		{
			if (currentSample->sample)
				delete currentSample->sample;
			previousSample = currentSample;
			currentSample = currentSample->next;
			delete previousSample;
		}
	}
}

// Set up the device. Return 0 if things go well, and -1 otherwise.
int Alsa::Setup (void)
{
	// Clear queue and set to initial values
	ClearQueue ();

	// Only setup playback if a playback name was configured
	if (pbDevice)
	{
		if (!SetupPlayBack ())
		{
			PLAYER_WARN ("Error opening playback device, playback functionality will not be available");
			pbHandle = NULL;
		}
	}

	// Only setup mixer if a mixer name was configured
	if (mixerDevice)
	{
		if (!SetupMixer ())
		{
			PLAYER_WARN ("Error opening mixer, mixer functionality will not be available");
			mixerHandle = NULL;
		}
	}

	// Only setup recording if a recorder name was configured
	if (recDevice)
	{
		if (!SetupRecord ())
		{
			PLAYER_WARN ("Error opening record device, record functionality will not be available");
			recHandle = NULL;
		}
	}

	playState = PB_STATE_STOPPED;
	recState = PB_STATE_STOPPED;

	StartThread ();
	return 0;
}


// Shutdown the device
int Alsa::Shutdown (void)
{
	StopThread ();

	// Stop playback
	StopPlayback ();

	// Clean up PCM file descriptors
	if (pbFDs)
		delete[] pbFDs;
	pbFDs = NULL;
	if (recFDs)
		delete[] recFDs;
	recFDs = NULL;
	// Close the playback handle
	if (pbHandle)
		snd_pcm_close (pbHandle);
	// Clean up periodBuffer
	if (periodBuffer != NULL)
	{
		delete[] periodBuffer;
		periodBuffer = NULL;
	}
	// Close the record handle
	if (recHandle)
		snd_pcm_close (recHandle);
	// Clean up the record data buffer
	if (recData)
		delete recData;
	// Remove any queued sample data
	ClearQueue ();

	if (mixerHandle)
	{
		if (numElements > 0)
		{
			CleanUpMixerElements (mixerElements, numElements);
		}
		if (snd_mixer_detach (mixerHandle, mixerDevice) < 0)
			PLAYER_WARN ("Error detaching mixer interface");
		else
		{
			if (snd_mixer_close (mixerHandle) < 0)
				PLAYER_WARN ("Error closing mixer interface");
			else
			{
				// TODO: Figure out why this causes a segfault
// 				snd_mixer_free (mixerHandle);
			}
		}
	}

	return 0;
}


////////////////////////////////////////////////////////////////////////////////
//	Thread stuff
////////////////////////////////////////////////////////////////////////////////

void Alsa::Main (void)
{
	while (1)
	{
		pthread_testcancel ();

		// Check playback state
		// Check if draining the current sample
		if (playState == PB_STATE_DRAIN)
		{
			// If so, need to wait until it finishes
			if (snd_pcm_state (pbHandle) == SND_PCM_STATE_DRAINING)
			{
				// Do nothing if still draining
// 				printf ("Still draining\n");
			}
			// The state after draining is complete is SETUP
			else if (snd_pcm_state (pbHandle) == SND_PCM_STATE_SETUP || snd_pcm_state (pbHandle) == SND_PCM_STATE_PREPARED)
			{
				// Then move on to the next
				AdvanceQueue ();
				// If there is a next, set it up for playing
				if (queueHead != NULL)
				{
					// Set parameters for the new sample
					SetPBParams (queueHead->sample);
					// Finished draining, so set to playing (the next if will catch this and start immediately)
					playState = PB_STATE_PLAYING;
				}
				// If nothing left, moved to STOPPED state
				else
				{
					playState = PB_STATE_STOPPED;
					SendStateMessage ();
				}
			}
			else
			{
				PLAYER_WARN1 ("Unexpected PCM state for drain: %d", snd_pcm_state (pbHandle));
				playState = PB_STATE_STOPPED;
				SendStateMessage ();
			}
		}
		// If playing, check if the buffer is ready for more data
		if (playState == PB_STATE_PLAYING)
		{
			if (poll (pbFDs, numPBFDs, 5) > 0)
			{
				// If it is, check each file descriptor
				for (int ii = 0; ii < numPBFDs; ii++)
					if (pbFDs[ii].revents > 0)
						PlaybackCallback (pbPeriodSize);
			}
		}

		// Check record state
		if (recState == PB_STATE_RECORDING)
		{
			if (poll (recFDs, numRecFDs, 5) > 0)
			{
				// If it is, check each file descriptor
				for (int ii = 0; ii < numRecFDs; ii++)
					if (recFDs[ii].revents > 0)
						RecordCallback (recPeriodSize);
			}
		}

	    // Handle pending messages
		if (!InQueue->Empty ())
		{
			// Process one message at a time before checking sound buffer states
			ProcessMessages (1);
		}
	}
}


// Sends a PLAYER_AUDIO_STATE_DATA message describing the current state of the driver
void Alsa::SendStateMessage (void)
{
	player_audio_state_t msg;

	msg.state = 0;
	if (playState == PB_STATE_PLAYING || playState == PB_STATE_DRAIN)
		msg.state |= PLAYER_AUDIO_STATE_PLAYING;
	if (recState == PB_STATE_RECORDING)
		msg.state |= PLAYER_AUDIO_STATE_RECORDING;

	Publish (device_addr, NULL, PLAYER_MSGTYPE_DATA, PLAYER_AUDIO_STATE_DATA, reinterpret_cast<void*> (&msg), sizeof (player_audio_state_t), NULL);
}


////////////////////////////////////////////////////////////////////////////////
//	Message handling
////////////////////////////////////////////////////////////////////////////////

int Alsa::HandleWavePlayCmd (player_audio_wav_t *data)
{
	// Add the wave to the queue
	if (!AddToQueue (data))
	{
		PLAYER_WARN ("Unable to add wave data to queue");
		return -1;
	}
	// Start playback
	StartPlayback ();

	return 0;
}

int Alsa::HandleSamplePlayCmd (player_audio_sample_item_t *data)
{
	StoredSample *sample;
	// Find the sample to be retrieved
	if ((sample = GetSampleAtIndex (data->index)) == NULL)
	{
		PLAYER_ERROR1 ("Couldn't find sample at index %d", data->index);
		return -1;
	}

	// Add the sample to the queue
	if (!AddToQueue (sample->sample))
	{
		PLAYER_WARN ("Unable to add sample to queue");
		return -1;
	}

	// Start playback
	StartPlayback ();

	return 0;
}

int Alsa::HandleRecordCmd (player_bool_t *data)
{
	if (data->state)
		StartRecording ();
	else
		StopRecording ();

	return 0;
}

int Alsa::HandleMixerChannelCmd (player_audio_mixer_channel_list_t *data)
{
	for (uint32_t ii = 0; ii < data->channels_count; ii++)
	{
		SetElementLevel (data->channels[ii].index, data->channels[ii].amplitude);
		SetElementSwitch (data->channels[ii].index, data->channels[ii].active);
	}

	PublishMixerData ();

	return 0;
}

int Alsa::HandleSampleLoadReq (player_audio_sample_t *data, MessageQueue *resp_queue)
{
	// If the requested index to store at is at end or -1, append to the list
	if (data->index == nextSampleIdx || data->index == -1)
	{
		if (AddStoredSample (&data->sample))
			return 0;	// All happy
		else
			return -1;	// Error occured
	}
	// If the sample is negative (but not -1) or beyond the end, error
	else if (data->index < -1 || data->index > nextSampleIdx)
	{
		PLAYER_ERROR1 ("Can't add sample at negative index %d", data->index);
		return -1;
	}
	else
	{
		// Find the sample to be replaced
		StoredSample *oldSample;
		if ((oldSample = GetSampleAtIndex (data->index)) == NULL)
		{
			PLAYER_ERROR1 ("Couldn't find sample at index %d", data->index);
			return -1;
		}
		// Replace it with the new one, freeing the old one
		// First create the new sample
		AudioSample *newSample = NULL;
		if ((newSample = new AudioSample) == NULL)
		{
			PLAYER_ERROR ("Failed to allocate memory for new audio sample");
			return -1;
		}
		if (!newSample->FromPlayer (&data->sample))
		{
			PLAYER_ERROR ("Failed to copy new audio sample");
			return -1;
		}
		// Delete the old sample
		if (oldSample->sample)
			delete oldSample->sample;
		// Update the pointer
		oldSample->sample = newSample;
	}
	Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_AUDIO_SAMPLE_LOAD_REQ, NULL, 0, NULL);
	return 0;
}

int Alsa::HandleSampleRetrieveReq (player_audio_sample_t *data, MessageQueue *resp_queue)
{
	// If the requested index to retrieve is beyond the end or negative, error
	if (data->index >= nextSampleIdx || data->index < 0)
	{
		PLAYER_ERROR1 ("Can't retrieve sample from invalid index %d", data->index);
		return -1;
	}
	else
	{
		// Find the sample to be retrieved
		StoredSample *sample;
		if ((sample = GetSampleAtIndex (data->index)) == NULL)
		{
			PLAYER_ERROR1 ("Couldn't find sample at index %d", data->index);
			return -1;
		}
		// Convert the data to a player struct
		player_audio_sample_t result;
		memset (&result, 0, sizeof (player_audio_sample_t));
		result.index = data->index;
		sample->sample->ToPlayer (&result.sample);
		Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_AUDIO_SAMPLE_RETRIEVE_REQ, &result, sizeof (player_audio_sample_t), NULL);
	}
	return -1;
}

int Alsa::HandleMixerChannelListReq (player_audio_mixer_channel_list_detail_t *data, MessageQueue *resp_queue)
{
	player_audio_mixer_channel_list_detail_t result;
	MixerDetailsToPlayer (&result);
	Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_AUDIO_MIXER_CHANNEL_LIST_REQ, &result, sizeof (player_audio_mixer_channel_list_detail_t), NULL);

	return 0;
}

int Alsa::HandleMixerChannelLevelReq (player_audio_mixer_channel_list_t *data, MessageQueue *resp_queue)
{
	player_audio_mixer_channel_list_t result;
	MixerLevelsToPlayer (&result);
	Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_AUDIO_MIXER_CHANNEL_LEVEL_REQ, &result, sizeof (player_audio_mixer_channel_list_t), NULL);

	return 0;
}

// Message processing
int Alsa::ProcessMessage (MessageQueue *resp_queue, player_msghdr *hdr, void *data)
{
	// Check for capabilities requests first
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILTIES_REQ);
	if (pbHandle)
	{
		HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_AUDIO_WAV_PLAY_CMD);
		HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_AUDIO_SAMPLE_PLAY_CMD);
		HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_AUDIO_SAMPLE_LOAD_REQ);
		HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_AUDIO_SAMPLE_RETRIEVE_REQ);
	}
	if (recHandle)
	{
		HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_AUDIO_WAV_STREAM_REC_CMD);
	}
	if (mixerHandle)
	{
		HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_AUDIO_MIXER_CHANNEL_CMD);
		HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_AUDIO_MIXER_CHANNEL_LIST_REQ);
		HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_AUDIO_MIXER_CHANNEL_LEVEL_REQ);
	}

	// Commands
	if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_AUDIO_WAV_PLAY_CMD, device_addr) && pbHandle)
	{
		HandleWavePlayCmd (reinterpret_cast<player_audio_wav_t*> (data));
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_AUDIO_SAMPLE_PLAY_CMD, device_addr) && pbHandle)
	{
		HandleSamplePlayCmd (reinterpret_cast<player_audio_sample_item_t*> (data));
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_AUDIO_WAV_STREAM_REC_CMD, device_addr) && recHandle)
	{
		HandleRecordCmd (reinterpret_cast<player_bool_t*> (data));
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_AUDIO_MIXER_CHANNEL_CMD, device_addr) && mixerHandle)
	{
		HandleMixerChannelCmd (reinterpret_cast<player_audio_mixer_channel_list_t*> (data));
		return 0;
	}
	// Requests
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_AUDIO_SAMPLE_LOAD_REQ, device_addr) && pbHandle)
	{
		return HandleSampleLoadReq (reinterpret_cast<player_audio_sample_t*> (data), resp_queue);
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_AUDIO_SAMPLE_RETRIEVE_REQ, device_addr) && pbHandle)
	{
		return HandleSampleRetrieveReq (reinterpret_cast<player_audio_sample_t*> (data), resp_queue);
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_AUDIO_MIXER_CHANNEL_LIST_REQ, device_addr) && mixerHandle)
	{
		return HandleMixerChannelListReq (reinterpret_cast<player_audio_mixer_channel_list_detail_t*> (data), resp_queue);
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_AUDIO_MIXER_CHANNEL_LEVEL_REQ, device_addr) && mixerHandle)
	{
		return HandleMixerChannelLevelReq (reinterpret_cast<player_audio_mixer_channel_list_t*> (data), resp_queue);
	}


	return -1;
}
