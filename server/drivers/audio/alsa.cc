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
PLAYER_AUDIO_SAMPLE_LOAD_REQ - Store samples provided by remote clients (max 1MB)
PLAYER_AUDIO_SAMPLE_RETRIEVE_REQ - Send stored samples to remote clients (max 1MB)

Planned future support includes:
- Recording.
- The callback method of managing buffers (to allow for blocking/nonblocking).
- Mixer functionality.

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

- interface (string)
  - Default: "plughw:0,0"
  - The sound interface to use for playback/recording.
- samples (tuple of strings)
  - Default: empty
  - The path of wave files to have as locally stored samples.

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

#include "alsa.h"

#include <errno.h>

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
//	Utility functions
////////////////////////////////////////////////////////////////////////////////

// Adds a new sample (already initialised) to the linked list
bool Alsa::AddSample (AudioSample *newSample)
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

// Adds a new sample that is stored locally in a wave-format file
bool Alsa::AddFileSample (const char *path)
{
	AudioSample *newSample = NULL;
	if ((newSample = new AudioSample) == NULL)
	{
		PLAYER_WARN ("Failed to allocate memory for new audio sample");
		return false;
	}

	newSample->type = SAMPLE_TYPE_LOCAL;
	newSample->localPath = strdup (path);
	newSample->memData = NULL;
	newSample->index = nextSampleIdx++;
	newSample->next = NULL;

	printf ("ALSA: Added sample %s to list at index %d\n", newSample->localPath, newSample->index);

	return AddSample (newSample);
}

// Adds a new sample who's wave data is stored in memory
bool Alsa::AddMemorySample (player_audio_wav_t *sampleData)
{
	AudioSample *newSample = NULL;
	if ((newSample = new AudioSample) == NULL)
	{
		PLAYER_ERROR ("Failed to allocate memory for new audio sample");
		return false;
	}
	if ((newSample->memData = new WaveData) == NULL)
	{
		delete newSample;
		PLAYER_ERROR ("Failed to allocate memory for new audio sample");
		return false;
	}

	// Copy the wave data into the new WaveData struct
	memset (newSample->memData, 0, sizeof (WaveData));
	if (!PlayerToWaveData (sampleData, newSample->memData))
	{
		PLAYER_ERROR ("Error copying new sample over old");
		return false;
	}

	newSample->type = SAMPLE_TYPE_LOCAL;
	newSample->localPath = NULL;
	newSample->index = nextSampleIdx++;
	newSample->next = NULL;

//	printf ("ALSA: Added wave data to sample list at index %d\n", newSample->index);

	return AddSample (newSample);
}

// Finds the sample with the specified index
AudioSample* Alsa::GetSampleAtIndex (int index)
{
	if (samplesHead)
	{
		AudioSample *currentSample = samplesHead;
		while (currentSample != NULL)
		{
			if (currentSample->index == index)
				return currentSample;
			currentSample = currentSample->next;
		}
	}
	return NULL;
}

// Loads a wave file
WaveData* Alsa::LoadWaveFromFile (char *fileName)
{
	WaveData *wave = NULL;
	FILE *fin = NULL;
	char tag[5];
	uint16_t tempUShort = 0;
	uint32_t tempUInt = 0, subChunk1Size = 0;

	// Try to open the wave file
	if ((fin = fopen (fileName, "r")) == NULL)
	{
		PLAYER_ERROR1 ("Couldn't open wave file for reading: %s", strerror (errno));
		return NULL;
	}

	// Wave file should be in the format (header, format chunk, data chunk), where:
	// header = 4+4+4 bytes: "RIFF", size, "WAVE"
	// format = 4+4+2+2+4+4+2+2[+2] bytes:
	//          "fmt ", size, 1, numChannels, sampleRate, byteRate, blockAlign,
	//          bitsPerSample, [extraParamsSize] (not present for PCM)
	// data = 4+4+? bytes: "data", size, data bytes...

	// Read the header - first the RIFF tag
	if (fgets (tag, 5, fin) == NULL)
	{
		PLAYER_ERROR ("Error reading tag from wave file");
		return NULL;
	}
	if (strcmp (tag, "RIFF") != 0)
	{
		PLAYER_ERROR ("Bad WAV format: missing RIFF tag");
		return NULL;
	}
	// Get the size of the file
	if (fread (&tempUInt, 4, 1, fin) != 1)
	{
		if (feof (fin))
			PLAYER_ERROR ("End of file reading WAV header");
		else
			PLAYER_ERROR ("Error reading WAV header");
		return NULL;
	}
	// Check the file size isn't stupid - should at least have size of all
	// chunks (excluding 2 fields already done)
	if (tempUInt < 36)
	{
		PLAYER_ERROR ("WAV file too short: missing chunk information");
		return NULL;
	}
	// Next tag should say "WAVE"
	if (fgets (tag, 5, fin) == NULL)
	{
		PLAYER_ERROR ("Error reading tag from wave file");
		return NULL;
	}
	if (strcmp (tag, "WAVE") != 0)
	{
		PLAYER_ERROR ("Bad WAV format: missing WAVE tag");
		return NULL;
	}

	// Next is the format information chunk, starting with a "fmt " tag
	if (fgets (tag, 5, fin) == NULL)
	{
		PLAYER_ERROR ("Error reading tag from wave file");
		return NULL;
	}
	if (strcmp (tag, "fmt ") != 0)
	{
		PLAYER_ERROR ("Bad WAV format: missing fmt  tag");
		return NULL;
	}
	// Followed by size of this chunk - should be 16, may be 18 if not quite
	// following the format correctly
	if (fread (&subChunk1Size, 4, 1, fin) != 1)
	{
		if (feof (fin))
			PLAYER_ERROR ("End of file reading WAV format");
		else
			PLAYER_ERROR ("Error reading WAV format");
		return NULL;
	}
	if (subChunk1Size != 16 && subChunk1Size != 18)
	{
		PLAYER_ERROR ("WAV file too short: missing chunk information");
		return NULL;
	}
	// Audio format is next - if not 1, can't read this file cause it isn't PCM
	if (fread (&tempUShort, 2, 1, fin) != 1)
	{
		if (feof (fin))
			PLAYER_ERROR ("End of file reading WAV format");
		else
			PLAYER_ERROR ("Error reading WAV format");
		return NULL;
	}
	if (tempUShort != 1)
	{
		PLAYER_ERROR ("WAV file not in PCM format");
		return NULL;
	}
	// Having got this far, we can now start reading data we want to keep,
	// so allocate memory for the wave information
	if ((wave = new WaveData) == NULL)
	{
		PLAYER_ERROR ("Failed to allocate memory for new wave data");
		return NULL;
	}
	// Read the number of channels
	if (fread (&wave->numChannels, 2, 1, fin) != 1)
	{
		if (feof (fin))
			PLAYER_ERROR ("End of file reading WAV num channels");
		else
			PLAYER_ERROR ("Error reading WAV num channels");
		delete wave;
		return NULL;
	}
	// Read the sample rate
	if (fread (&wave->sampleRate, 4, 1, fin) != 1)
	{
		if (feof (fin))
			PLAYER_ERROR ("End of file reading WAV sample rate");
		else
			PLAYER_ERROR ("Error reading WAV sample rate");
		delete wave;
		return NULL;
	}
	// Read the byte rate
	if (fread (&wave->byteRate, 4, 1, fin) != 1)
	{
		if (feof (fin))
			PLAYER_ERROR ("End of file reading WAV byte rate");
		else
			PLAYER_ERROR ("Error reading WAV byte rate");
		delete wave;
		return NULL;
	}
	// Read the block align
	if (fread (&wave->blockAlign, 2, 1, fin) != 1)
	{
		if (feof (fin))
			PLAYER_ERROR ("End of file reading WAV block align");
		else
			PLAYER_ERROR ("Error reading WAV block align");
		delete wave;
		return NULL;
	}
	// Read the bits per sample
	if (fread (&wave->bitsPerSample, 2, 1, fin) != 1)
	{
		if (feof (fin))
			PLAYER_ERROR ("End of file reading WAV bits per sample");
		else
			PLAYER_ERROR ("Error reading WAV bits per sample");
		delete wave;
		return NULL;
	}
	// If the size of this chunk was 18, get those extra two bytes
	if (subChunk1Size == 18)
	{
		if (fread (&tempUShort, 2, 1, fin) != 1)
		{
			if (feof (fin))
				PLAYER_ERROR ("End of file reading blank 2 bytes");
			else
				PLAYER_ERROR ("Error reading WAV blank 2 bytes");
			delete wave;
			return NULL;
		}
	}

	// On to the data chunk, again starting with a tag
	if (fgets (tag, 5, fin) == NULL)
	{
		PLAYER_ERROR ("Error reading tag from wave file");
		delete wave;
		return NULL;
	}
	if (strcmp (tag, "data") != 0)
	{
		PLAYER_ERROR ("Bad WAV format: missing data tag");
		delete wave;
		return NULL;
	}
	// Size of the wave data
	if (fread (&wave->dataLength, 4, 1, fin) != 1)
	{
		if (feof (fin))
			PLAYER_ERROR ("End of file reading WAV data size");
		else
			PLAYER_ERROR ("Error reading WAV data size");
		delete wave;
		return NULL;
	}
	// Allocate the memory for the data
	if ((wave->data = new uint8_t[wave->dataLength]) == NULL)
	{
		PLAYER_ERROR ("Failed to allocate memory for wave data");
		delete wave;
		return NULL;
	}
	// Read the wave data
	if (fread (wave->data, 1, wave->dataLength, fin) != wave->dataLength)
	{
		if (feof (fin))
			PLAYER_ERROR ("End of file reading WAV data");
		else
			PLAYER_ERROR ("Error reading WAV data");
		delete[] wave->data;
		delete wave;
		return NULL;
	}

	// All done with reading, close the wave file
	fclose (fin);
	// Calculate the number of frames in the data
	wave->numFrames = wave->dataLength / (wave->numChannels * (wave->bitsPerSample / 8));
	// Set the format value
	switch (wave->bitsPerSample)
	{
	}

//	printf ("Loaded wave file:\n");
//	PrintWaveData (wave);

	// Return the result
	return wave;
}

// Copies the data from a loaded wave file to the player struct for wave data
// If the wave data doesn't match one of the possible formats supported by the
// player format flags, the copy isn't performed and an error is returned
bool Alsa::WaveDataToPlayer (WaveData *source, player_audio_wav_t *dest)
{
	// Set the format flags
	dest->format = PLAYER_AUDIO_FORMAT_RAW;
	if (source->numChannels == 2)
		dest->format |= PLAYER_AUDIO_STEREO;
	else if (source->numChannels != 1)
	{
		PLAYER_ERROR ("Cannot convert wave to player struct: wrong number of channels");
		return false;
	}
	switch (source->sampleRate)
	{
		case 11025:
			dest->format |= PLAYER_AUDIO_FREQ_11k;
			break;
		case 22050:
			dest->format |= PLAYER_AUDIO_FREQ_22k;
			break;
		case 44100:
			dest->format |= PLAYER_AUDIO_FREQ_44k;
			break;
		case 48000:
			dest->format |= PLAYER_AUDIO_FREQ_48k;
			break;
		default:
			PLAYER_ERROR ("Cannot convert wave to player struct: wrong sample rate");
			return false;
	}

	switch (source->bitsPerSample)
	{
		case 8:
			dest->format |= PLAYER_AUDIO_8BIT;
			break;
		case 16:
			dest->format |= PLAYER_AUDIO_16BIT;
			break;
		case 24:
			dest->format |= PLAYER_AUDIO_24BIT;
			break;
		default:
			PLAYER_ERROR ("Cannot convert wave to player struct: wrong format (bits per sample)");
			return false;
	}

	// Copy at most PLAYER_AUDIO_WAV_BUFFER_SIZE bytes of data
	uint32_t bytesToCopy = PLAYER_AUDIO_WAV_BUFFER_SIZE;
	if (source->dataLength < bytesToCopy)
		bytesToCopy = source->dataLength;
	memcpy (&dest->data, source->data, bytesToCopy);
	dest->data_count = source->dataLength;
	return true;
}

bool Alsa::PlayerToWaveData (player_audio_wav_t *source, WaveData *dest)
{
	// Set format information
	if ((source->format & PLAYER_AUDIO_FORMAT_BITS) != PLAYER_AUDIO_FORMAT_RAW)
	{
		// Can't handle non-raw data
		PLAYER_ERROR ("Cannot play non-raw audio data");
		return false;
	}

	if (source->format & PLAYER_AUDIO_STEREO)
		dest->numChannels = 2;
	else
		dest->numChannels = 1;

	if ((source->format & PLAYER_AUDIO_FREQ) == PLAYER_AUDIO_FREQ_11k)
		dest->sampleRate = 11025;
	else if ((source->format & PLAYER_AUDIO_FREQ) == PLAYER_AUDIO_FREQ_22k)
		dest->sampleRate = 22050;
	else if ((source->format & PLAYER_AUDIO_FREQ) == PLAYER_AUDIO_FREQ_44k)
		dest->sampleRate = 44100;
	else if ((source->format & PLAYER_AUDIO_FREQ) == PLAYER_AUDIO_FREQ_48k)
		dest->sampleRate = 48000;

	if ((source->format & PLAYER_AUDIO_BITS) == PLAYER_AUDIO_8BIT)
	{
		dest->bitsPerSample = 8;
	}
	else if ((source->format & PLAYER_AUDIO_BITS) == PLAYER_AUDIO_16BIT)
	{
		dest->bitsPerSample = 16;
	}
	else if ((source->format & PLAYER_AUDIO_BITS) == PLAYER_AUDIO_24BIT)
	{
		dest->bitsPerSample = 24;
	}

	// Calculate the other format info
	dest->blockAlign = dest->numChannels * (dest->bitsPerSample / 8);
	dest->byteRate = dest->sampleRate * dest->blockAlign;

	// Allocate memory for the data if necessary
	if (dest->data == NULL)
	{
		if ((dest->data = new uint8_t[source->data_count]) == NULL)
		{
			PLAYER_ERROR ("Failed to allocate memory for wave data");
			return false;
		}
	}
	// Copy the wave data across
	memcpy (dest->data, source->data, source->data_count);
	dest->dataLength = source->data_count;
	dest->numFrames = dest->dataLength / dest->blockAlign;

/*	printf ("Converted audio data to WaveData:\n");
	PrintWaveData (dest);*/
	return true;
}

// Print wave data info - it's a handy function to have around
void Alsa::PrintWaveData (WaveData *wave)
{
	printf ("Num channels:\t%d\n", wave->numChannels);
	printf ("Sample rate:\t%d\n", wave->sampleRate);
	printf ("Byte rate:\t%d\n", wave->byteRate);
	printf ("Block align:\t%d\n", wave->blockAlign);
	printf ("Format:\t\t");
	switch (wave->bitsPerSample)
	{
		case 8:
			printf ("Unsigned 8 bit\n");
			break;
		case 16:
			printf ("Signed 16 bit little-endian\n");
			break;
		case 24:
			if ((wave->blockAlign / wave->numChannels) == 3)
				printf ("Signed 24 bit 3-byte little-endian\n");
			else
				printf ("Signed 24 bit little-endian\n");
			break;
		case 32:
			printf ("Signed 32 bit little-endian\n");
			break;
		default:
			printf ("Unplayable format: %d bit\n", wave->bitsPerSample);
	}
	printf ("Num frames:\t%d\n", wave->numFrames);
	printf ("Data length:\t%d\n", wave->dataLength);
}

////////////////////////////////////////////////////////////////////////////////
//	Sound functions (setting params, writing data to the buffer, etc)
////////////////////////////////////////////////////////////////////////////////

// Sets the hardware parameters of the sound device to the provided wave data's format
bool Alsa::SetHWParams (WaveData *wave)
{
	snd_pcm_hw_params_t *hwparams;			// Hardware parameters
	snd_pcm_hw_params_alloca(&hwparams);	// Allocate params structure on stack

	// Init parameters
	if (snd_pcm_hw_params_any (pcmHandle, hwparams) < 0)
	{
		PLAYER_ERROR ("Cannot configure this PCM device");
		return false;
	}

	unsigned int exactRate;	// Sample rate returned by snd_pcm_hw_params_set_rate_near
	int periods = 2;		// Number of periods
	snd_pcm_uframes_t periodSize = 8192;	// Periodsize (bytes) of the output buffer

	// Use interleaved access
	if (snd_pcm_hw_params_set_access (pcmHandle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
	{
		PLAYER_ERROR ("Error setting interleaved access");
		return false;
	}
	// Set sound format
	snd_pcm_format_t format;
	switch (wave->bitsPerSample)
	{
		case 8:
			format = SND_PCM_FORMAT_U8;
			break;
		case 16:
			format = SND_PCM_FORMAT_S16_LE;
			break;
		case 24:
			if ((wave->blockAlign / wave->numChannels) == 3)
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
	if (snd_pcm_hw_params_set_format (pcmHandle, hwparams, format) < 0)
	{
		PLAYER_ERROR ("Error setting format");
		return false;
	}
	// Set sample rate
	exactRate = wave->sampleRate;
	if (snd_pcm_hw_params_set_rate_near (pcmHandle, hwparams, &exactRate, 0) < 0)
	{
		PLAYER_ERROR ("Error setting sample rate");
		return false;
	}
	if (exactRate != wave->sampleRate)
		PLAYER_WARN2 ("Rate %dHz not supported by hardware, using %dHz instead", wave->sampleRate, exactRate);

	// Set number of channels
	if (snd_pcm_hw_params_set_channels(pcmHandle, hwparams, wave->numChannels) < 0)
	{
		PLAYER_ERROR ("Error setting channels");
		return false;
	}

	// Set number of periods
	if (snd_pcm_hw_params_set_periods(pcmHandle, hwparams, periods, 0) < 0)
	{
		PLAYER_ERROR ("Error setting periods");
		return false;
	}

	// Set the buffer size in frames TODO: update this to take into account pbNumChannels
	if (snd_pcm_hw_params_set_buffer_size (pcmHandle, hwparams, (periodSize * periods) >> 2) < 0)
	{
		PLAYER_ERROR ("Error setting buffersize");
		return false;
	}

	// Apply hwparams to the pcm device
	if (snd_pcm_hw_params (pcmHandle, hwparams) < 0)
	{
		PLAYER_ERROR ("Error setting HW params");
		return false;
	}

	// Don't need this anymore (I think) TODO: figure out why this causes glib errors
// 	snd_pcm_hw_params_free (hwparams);

	return true;
}

// Plays some wave data
bool Alsa::PlayWave (WaveData *wave)
{
	int pcmReturn = 0;
	uint8_t *dataPos = wave->data;

	// Write the data until there is none left
	unsigned int framesToWrite = wave->numFrames;
	do
	{
		pcmReturn = snd_pcm_writei (pcmHandle, dataPos, framesToWrite);
		if (pcmReturn < 0)
		{
			snd_pcm_prepare (pcmHandle);
			printf ("BUFFER UNDERRUN!\t%s\n", snd_strerror (pcmReturn));
		}
		else
		{
			// Calculate how many frames remain unwritten
			framesToWrite = framesToWrite - pcmReturn;
			// Move the data pointer appropriately
			dataPos += pcmReturn;
		}

//		printf ("pcmReturn = %d\tframesToWrite = %d\n", pcmReturn, framesToWrite);
		fflush (stdout);
	}
	while (framesToWrite > 0);

	// Drain the remaining data once finished writing and stop playback
	snd_pcm_drain (pcmHandle);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//	Driver management
////////////////////////////////////////////////////////////////////////////////

// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
Alsa::Alsa (ConfigFile* cf, int section)
    : Driver (cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_AUDIO_CODE)
{
	pcmName = NULL;
	samplesHead = samplesTail = NULL;
	nextSampleIdx = 0;

	// Read the config file options - see header for descriptions if not here
	block = cf->ReadBool (section, "block", true);
	pcmName = strdup (cf->ReadString (section, "interface", "plughw:0,0"));
//	pbRate = cf->ReadInt (section, "pb_rate", 44100);
//	pbNumChannels = cf->ReadInt (section, "pb_numchannels", 2);
	// Read sample names
	int numSamples = cf->GetTupleCount (section, "samples");
	if (numSamples > 0)
	{
		for (int ii = 0; ii < numSamples; ii++)
		{
			if (!AddFileSample (cf->ReadTupleString (section, "samples", ii, "")))
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
	if (pcmName)
		free (pcmName);
	if (samplesHead)
	{
		AudioSample *currentSample = samplesHead;
		AudioSample *previousSample = currentSample;
		while (currentSample != NULL)
		{
			if (currentSample->localPath)
				free (currentSample->localPath);
			if (currentSample->memData)
			{
				if (currentSample->memData->data)
					delete[] currentSample->memData->data;
				delete currentSample->memData;
			}
			previousSample = currentSample;
			currentSample = currentSample->next;
			delete previousSample;
		}
	}
}

// Set up the device. Return 0 if things go well, and -1 otherwise.
int Alsa::Setup (void)
{
	pbStream = SND_PCM_STREAM_PLAYBACK;	// Make the playback stream

	// Open the pcm device for either blocking or non-blocking mode
	if (snd_pcm_open (&pcmHandle, pcmName, pbStream, block ? 0 : SND_PCM_NONBLOCK) < 0)
	{
		PLAYER_ERROR1 ("Error opening PCM device %s for playback", pcmName);
		return -1;
	}

	StartThread ();
//	printf ("Alsa driver initialised\n");
	return 0;
}


// Shutdown the device
int Alsa::Shutdown (void)
{
//	printf ("Alsa driver shutting down\n");

	StopThread ();
	snd_pcm_close (pcmHandle);

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

	    // Handle pending messages
		if (!InQueue->Empty ())
		{
			ProcessMessages ();
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//	Message handling
////////////////////////////////////////////////////////////////////////////////

int Alsa::HandleWavePlayCmd (player_audio_wav_t *data)
{
	WaveData wave;
	wave.data = NULL;

	if (!PlayerToWaveData (data, &wave))
		return -1;
	// Set hardware parameters for the wave data
	if (SetHWParams (&wave))
	{
		if (PlayWave (&wave))
		{
			delete[] wave.data;
			return 0;	// Success
		}
	}
	delete[] wave.data;
	// Failure
	return -1;
}

int Alsa::HandleSamplePlayCmd (player_audio_sample_item_t *data)
{
	AudioSample *sample;
	WaveData *wave = NULL;
	// Find the sample to be retrieved
	if ((sample = GetSampleAtIndex (data->index)) == NULL)
	{
		PLAYER_ERROR1 ("Couldn't find sample at index %d", data->index);
		return -1;
	}
	if (sample->type == SAMPLE_TYPE_LOCAL)
	{
		// Load the file
		if ((wave = LoadWaveFromFile (sample->localPath)) == NULL)
		{
			PLAYER_ERROR1 ("Couldn't load sample at index %d", data->index);
			return -1;
		}
		// Set hardware parameters for this wave data
		if (!SetHWParams (wave))
		{
			delete[] wave->data;
			delete wave;
			return -1;
		}
		// Play the file
		if (!PlayWave (wave))
		{
			delete[] wave->data;
			delete wave;
			return -1;
		}
	}
	else
	{
		// Set hardware parameters for the wave data
		if (!SetHWParams (sample->memData))
			return -1;
		// Wave file is in memory, play that
		if (!PlayWave (sample->memData))
			return -1;
	}
	return 0;
}

int Alsa::HandleSampleLoadReq (player_audio_sample_t *data, MessageQueue *resp_queue)
{
	// If the requested index to store at is at end or -1, append to the list
	if (data->index == nextSampleIdx || data->index == -1)
	{
		if (AddMemorySample (&data->sample))
			return 0;	// All happy
		else
		{
			PLAYER_ERROR ("Failed to load new sample");
			return -1;	// Error occured
		}
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
		AudioSample *oldSample;
		if ((oldSample = GetSampleAtIndex (data->index)) == NULL)
		{
			PLAYER_ERROR1 ("Couldn't find sample at index %d", data->index);
			return -1;
		}
		// Replace it with the new one, freeing the old one first
		if (oldSample->type == SAMPLE_TYPE_LOCAL)
		{
			// Create here first so don't delete the old one if have an error
			WaveData *newData;
			if ((newData = new WaveData) == NULL)
			{
				PLAYER_ERROR ("Failed to allocate memory for new audio sample");
				return -1;
			}

			free (oldSample->localPath);
			oldSample->memData = newData;
			memset (oldSample->memData, 0, sizeof (WaveData));
			if (!PlayerToWaveData (&data->sample, oldSample->memData))
			{
				PLAYER_ERROR ("Error copying new sample over old");
				return -1;
			}
			oldSample->type = SAMPLE_TYPE_REMOTE;
		}
		else
		{
			// Clear out the old sample
			delete[] oldSample->memData->data;
			memset (oldSample->memData, 0, sizeof (WaveData));
			// Copy the new sample into its place
			if (!PlayerToWaveData (&data->sample, oldSample->memData))
			{
				PLAYER_ERROR ("Error copying new sample over old");
				return -1;
			}
			oldSample->type = SAMPLE_TYPE_REMOTE;
		}
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
		AudioSample *sample;
		if ((sample = GetSampleAtIndex (data->index)) == NULL)
		{
			PLAYER_ERROR1 ("Couldn't find sample at index %d", data->index);
			return -1;
		}
		// Create a structure to return to the client if local, otherwise just
		// return the existing stored struct
		if (sample->type == SAMPLE_TYPE_LOCAL)
		{
			// Load the wave file from disc
			WaveData *wave = NULL;
			if ((wave = LoadWaveFromFile (sample->localPath)) == NULL)
			{
				PLAYER_ERROR1 ("Couldn't load sample at index %d", data->index);
				return -1;
			}

			// Build the result to return to the client
			player_audio_sample_t result;
			memset (&result, 0, sizeof (player_audio_sample_t));
			result.index = data->index;
			// Copy the loaded data into the result
			WaveDataToPlayer (wave, &result.sample);

			Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_AUDIO_SAMPLE_RETRIEVE_REQ, &result, sizeof (player_audio_sample_t), NULL);

			// Delete the loaded wave data
			delete[] wave->data;
			delete wave;
			return 0;
		}
		else
		{
			// Build the result to return to the client
			player_audio_sample_t result;
			memset (&result, 0, sizeof (player_audio_sample_t));
			result.index = data->index;
			// Copy the stored data into the result
			WaveDataToPlayer (sample->memData, &result.sample);

			Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_AUDIO_SAMPLE_RETRIEVE_REQ, &result, sizeof (player_audio_sample_t), NULL);
			return 0;
		}
	}
	return -1;
}

// Message processing
int Alsa::ProcessMessage (MessageQueue *resp_queue, player_msghdr *hdr, void *data)
{
	// Check for capabilities requests first
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILTIES_REQ);
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_AUDIO_WAV_PLAY_CMD);
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_AUDIO_SAMPLE_PLAY_CMD);
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_AUDIO_SAMPLE_LOAD_REQ);
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_AUDIO_SAMPLE_RETRIEVE_REQ);

	// Commands
	if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_AUDIO_WAV_PLAY_CMD, this->device_addr))
	{
		HandleWavePlayCmd (reinterpret_cast<player_audio_wav_t*> (data));
		return 0;
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_AUDIO_SAMPLE_PLAY_CMD, this->device_addr))
	{
		HandleSamplePlayCmd (reinterpret_cast<player_audio_sample_item_t*> (data));
		return 0;
	}
	// Requests
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_AUDIO_SAMPLE_LOAD_REQ, this->device_addr))
	{
		return HandleSampleLoadReq (reinterpret_cast<player_audio_sample_t*> (data), resp_queue);
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_AUDIO_SAMPLE_RETRIEVE_REQ, this->device_addr))
	{
		return HandleSampleRetrieveReq (reinterpret_cast<player_audio_sample_t*> (data), resp_queue);
	}

	return -1;
}
