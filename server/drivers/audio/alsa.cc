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

Planned future support includes:
- The callback method of managing buffers (to allow for blocking/nonblocking and
less skippy playback).
- Recording.

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
- mixerdevice (string)
  - Default: "default"
  - The device to attach the mixer interface to
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

	// Attach it to the device?
	if (snd_mixer_attach (mixerHandle, device) < 0)
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
	int temp = 0;
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

	element->playMute = true;
	element->capMute = true;
	element->comMute = true;

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
			// Get the mute status of this channel - if unmuted, then set element to unmuted
			if (element->caps & ELEMCAP_PLAYBACK_SWITCH)
			{
				snd_mixer_selem_get_playback_switch (elem, static_cast<snd_mixer_selem_channel_id_t> (ii), &temp);
				if (temp)
					element->playMute = false;
			}
		}
		if (snd_mixer_selem_has_capture_channel (elem, static_cast<snd_mixer_selem_channel_id_t> (ii)))
		{
// 			printf ("Element has capture channel %d: %s\n", ii, snd_mixer_selem_channel_name (static_cast<snd_mixer_selem_channel_id_t> (ii)));
			element->caps |= ELEMCAP_CAN_CAPTURE;
			// Get the current volume of this channel and make it the element one, if don't have that yet
			if (!element->curCapVol)
				snd_mixer_selem_get_capture_volume (elem, static_cast<snd_mixer_selem_channel_id_t> (ii), &(element->curCapVol));
			// Get the mute status of this channel - if unmuted, then set element to unmuted
			if (element->caps & ELEMCAP_CAPTURE_SWITCH)
			{
				snd_mixer_selem_get_playback_switch (elem, static_cast<snd_mixer_selem_channel_id_t> (ii), &temp);
				if (temp)
					element->capMute = false;
			}
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

	// Common mute status
	if (element->caps & ELEMCAP_COMMON_SWITCH)
		element->comMute = element->playMute ? element->playMute : element->capMute;

// 	printf ("Element volume levels:\n");
// 	printf ("Playback:\t%ld, %ld, %ld, %s\n", element->minPlayVol, element->curPlayVol, element->maxPlayVol, element->playMute ? "Muted" : "Unmuted");
// 	printf ("Capture:\t%ld, %ld, %ld, %s\n", element->minCapVol, element->curCapVol, element->maxCapVol, element->capMute ? "Muted" : "Unmuted");
// 	printf ("Common:\t%ld, %ld, %ld, %s\n", element->minComVol, element->curComVol, element->maxComVol, element->comMute ? "Muted" : "Unmuted");

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
			result[currentIndex].playMute = elements[ii].playMute;
			result[currentIndex].name = reinterpret_cast<char*> (malloc (strlen (elements[ii].name) + strlen (" (Playback)") + 1));
			strncpy (result[currentIndex].name, elements[ii].name, strlen (elements[ii].name) + 1);
			strncpy (&(result[currentIndex].name[strlen (elements[ii].name)]), " (Playback)", strlen (" (Playback)") + 1);

			// Capture element
			result[currentIndex + 1].elem = elements[ii].elem;
			result[currentIndex + 1].caps = ELEMCAP_CAN_CAPTURE;
			result[currentIndex + 1].minCapVol = elements[ii].minCapVol;
			result[currentIndex + 1].curCapVol = elements[ii].curCapVol;
			result[currentIndex + 1].maxCapVol = elements[ii].maxCapVol;
			result[currentIndex + 1].capMute = elements[ii].capMute;
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
			result[currentIndex].playMute = elements[ii].playMute;
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
			result[currentIndex].capMute = elements[ii].capMute;
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
			result[currentIndex].comMute = elements[ii].comMute;
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
		bool mute = false;
		if (mixerElements[ii].caps & ELEMCAP_CAN_PLAYBACK)
		{
			min = mixerElements[ii].minPlayVol;
			cur = mixerElements[ii].curPlayVol;
			max = mixerElements[ii].maxPlayVol;
			mute = mixerElements[ii].playMute;
		}
		else if (mixerElements[ii].caps & ELEMCAP_CAN_CAPTURE)
		{
			min = mixerElements[ii].minCapVol;
			cur = mixerElements[ii].curCapVol;
			max = mixerElements[ii].maxCapVol;
			mute = mixerElements[ii].capMute;
		}
		else if (mixerElements[ii].caps & ELEMCAP_COMMON)
		{
			min = mixerElements[ii].minComVol;
			cur = mixerElements[ii].curComVol;
			max = mixerElements[ii].maxComVol;
			mute = mixerElements[ii].comMute;
		}
		dest->channels[ii].amplitude = LevelToPlayer (min, max, cur);
		dest->channels[ii].active.state = mute ? 0 : 1;
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

// Sets mute for an element
void Alsa::SetElementMute (uint32_t index, player_bool_t mute)
{
	if (mixerElements[index].caps & ELEMCAP_CAN_PLAYBACK)
	{
		// Set the mute for all channels in this element
		if (snd_mixer_selem_set_playback_switch_all (mixerElements[index].elem, mute.state ? 1 : 0) < 0)
		{
			PLAYER_WARN1 ("Error setting playback mute for element %d", index);
		}
		else
			mixerElements[index].playMute = mute.state ? true : false;
	}
	else if (mixerElements[index].caps & ELEMCAP_CAN_CAPTURE)
	{
		// Set the mute for all channels in this element
		if (snd_mixer_selem_set_capture_volume_all (mixerElements[index].elem, mute.state ? 1 : 0) < 0)
		{
			PLAYER_WARN1 ("Error setting capture mute for element %d", index);
		}
		else
			mixerElements[index].capMute = mute.state ? true : false;
	}
	else if (mixerElements[index].caps & ELEMCAP_COMMON)
	{
		// Set the mute for all channels in this element
		if (snd_mixer_selem_set_playback_volume_all (mixerElements[index].elem, mute.state ? 1 : 0) < 0)
		{
			PLAYER_WARN1 ("Error setting common mute for element %d", index);
		}
		else
			mixerElements[index].comMute = mute.state ? true : false;
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
	bool mute;
	printf ("Mixer elements:\n");
	for (uint32_t ii = 0; ii < count; ii++)
	{
		if (elements[ii].caps & ELEMCAP_CAN_PLAYBACK)
		{
			min = elements[ii].minPlayVol;
			cur = elements[ii].curPlayVol;
			max = elements[ii].maxPlayVol;
			mute = elements[ii].playMute;
		}
		else if (elements[ii].caps & ELEMCAP_CAN_CAPTURE)
		{
			min = elements[ii].minCapVol;
			cur = elements[ii].curCapVol;
			max = elements[ii].maxCapVol;
			mute = elements[ii].capMute;
		}
		else if (elements[ii].caps & ELEMCAP_COMMON)
		{
			min = elements[ii].minComVol;
			cur = elements[ii].curComVol;
			max = elements[ii].maxComVol;
			mute = elements[ii].comMute;
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
		printf ("Active:\t%s\n", mute ? "No" : "Yes");
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
	pcmName = device = NULL;
	samplesHead = samplesTail = NULL;
	nextSampleIdx = 0;
	mixerElements = NULL;

	// Read the config file options - see header for descriptions if not here
	block = cf->ReadBool (section, "block", true);
	pcmName = strdup (cf->ReadString (section, "interface", "plughw:0,0"));
	device = strdup (cf->ReadString (section, "mixerdevice", "default"));
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
	if (device)
		free (device);
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

	if (!SetupMixer ())
	{
		PLAYER_WARN ("Error opening mixer, mixer interface will not be available");
		mixerHandle = NULL;
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

	if (mixerHandle)
	{
		if (numElements > 0)
		{
			CleanUpMixerElements (mixerElements, numElements);
		}
		if (snd_mixer_detach (mixerHandle, device) < 0)
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

	snd_pcm_close (pcmHandle);

	return 0;
}


////////////////////////////////////////////////////////////////////////////////
//	Thread stuff
////////////////////////////////////////////////////////////////////////////////

void Alsa::Main (void)
{
	// If mixer is enabled, send out some mixer data
/*	if (mixerHandle)
		PublishMixerData ();*/
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

int Alsa::HandleMixerChannelCmd (player_audio_mixer_channel_list_t *data)
{
	for (uint32_t ii = 0; ii < data->channels_count; ii++)
	{
		SetElementLevel (data->channels[ii].index, data->channels[ii].amplitude);
		SetElementMute (data->channels[ii].index, data->channels[ii].active);
	}

	PublishMixerData ();

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
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_AUDIO_WAV_PLAY_CMD);
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_AUDIO_SAMPLE_PLAY_CMD);
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_AUDIO_MIXER_CHANNEL_CMD);
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_AUDIO_SAMPLE_LOAD_REQ);
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_AUDIO_SAMPLE_RETRIEVE_REQ);
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_AUDIO_MIXER_CHANNEL_LIST_REQ);
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_AUDIO_MIXER_CHANNEL_LEVEL_REQ);

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
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_AUDIO_MIXER_CHANNEL_CMD, this->device_addr))
	{
		HandleMixerChannelCmd (reinterpret_cast<player_audio_mixer_channel_list_t*> (data));
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
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_AUDIO_MIXER_CHANNEL_LIST_REQ, this->device_addr))
	{
		return HandleMixerChannelListReq (reinterpret_cast<player_audio_mixer_channel_list_detail_t*> (data), resp_queue);
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_AUDIO_MIXER_CHANNEL_LEVEL_REQ, this->device_addr))
	{
		return HandleMixerChannelLevelReq (reinterpret_cast<player_audio_mixer_channel_list_t*> (data), resp_queue);
	}


	return -1;
}
