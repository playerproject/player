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

#include <libplayercore/playercore.h>

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
//	Driver management
////////////////////////////////////////////////////////////////////////////////

// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
Alsa::Alsa (ConfigFile* cf, int section)
    : Driver (cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_AUDIO_CODE)
{
	pcmName = NULL;

	// Read the config file options - see header for descriptions if not here
	block = cf->ReadBool (section, "block", true);
	pcmName = strdup (cf->ReadString (section, "interface", "plughw:0,0"));
	pbRate = cf->ReadInt (section, "pb_rate", 44100);
	pbNumChannels = cf->ReadInt (section, "pb_numchannels", 2);

	return;
}

// Destructor
Alsa::~ Alsa (void)
{
	if (pcmName)
		free (pcmName);
}

// Set up the device. Return 0 if things go well, and -1 otherwise.
int Alsa::Setup (void)
{
	pbStream = SND_PCM_STREAM_PLAYBACK;	// Make the playback stream
	snd_pcm_hw_params_alloca(&hwparams);	// Allocate params structure on stack
	// Open the pcm device for either blocking or non-blocking mode
	if (snd_pcm_open (&pcmHandle, pcmName, pbStream, block ? 0 : SND_PCM_NONBLOCK) < 0)
	{
		PLAYER_ERROR1 ("Error opening PCM device %s for playback", pcmName);
		return -1;
	}

	// Init parameters
	if (snd_pcm_hw_params_any (pcmHandle, hwparams) < 0)
	{
		PLAYER_ERROR ("Can not configure this PCM device");
		return -1;
	}

	// Set parameters
	unsigned int exactRate;	// Sample rate returned by snd_pcm_hw_params_set_rate_near
	int periods = 2;	// Number of periods
	periodSize = 8192;	// Periodsize (bytes)

	// Use interleaved access
	if (snd_pcm_hw_params_set_access (pcmHandle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
	{
		PLAYER_ERROR ("Error setting interleaved access");
		return -1;
	}
	// Set sound format
	if (snd_pcm_hw_params_set_format (pcmHandle, hwparams, SND_PCM_FORMAT_S16_LE) < 0)
	{
		PLAYER_ERROR ("Error setting format");
		return -1;
	}
	// Set sample rate
	exactRate = pbRate;
	if (snd_pcm_hw_params_set_rate_near (pcmHandle, hwparams, &exactRate, 0) < 0)
	{
		PLAYER_ERROR ("Error setting sample rate");
		return -1;
	}
	if (exactRate != pbRate)
		PLAYER_WARN2 ("Rate %dHz not supported by hardware, using %dHz instead", pbRate, exactRate);

	// Set number of channels
	if (snd_pcm_hw_params_set_channels(pcmHandle, hwparams, pbNumChannels) < 0)
	{
		PLAYER_ERROR ("Error setting channels");
		return -1;
	}

	// Set number of periods
	if (snd_pcm_hw_params_set_periods(pcmHandle, hwparams, periods, 0) < 0)
	{
		PLAYER_ERROR ("Error setting periods");
		return -1;
	}

	// Set the buffer size in frames TODO: update this to take into account pbNumChannels
	if (snd_pcm_hw_params_set_buffer_size (pcmHandle, hwparams, (periodSize * periods) >> 2) < 0)
	{
		PLAYER_ERROR ("Error setting buffersize");
		return -1;
	}

	// Apply hwparams to the pcm device
	if (snd_pcm_hw_params (pcmHandle, hwparams) < 0)
	{
		PLAYER_ERROR ("Error setting HW params");
		return -1;
	}

	// Don't need this anymore (I think) TODO: figure out why this causes glib errors
	//snd_pcm_hw_params_free (hwparams);

	StartThread ();
	printf ("Alsa driver initialised\n");
	return 0;
}


// Shutdown the device
int Alsa::Shutdown (void)
{
	printf ("Alsa driver shutting down\n");

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

int Alsa::HandleWavePlayCmd (player_audio_wav_t *waveData)
{
	// Check the format matches what the driver has been set up with

	if ((waveData->format & PLAYER_AUDIO_FORMAT_BITS) != PLAYER_AUDIO_FORMAT_RAW)
	{
		PLAYER_ERROR ("Alsa driver can only play RAW format audio");
		return -1;
	}
	if (((waveData->format & PLAYER_AUDIO_FREQ == PLAYER_AUDIO_FREQ_44k) &&
			!(pbRate != 44100)) ||
			 ((waveData->format & PLAYER_AUDIO_FREQ == PLAYER_AUDIO_FREQ_11k) &&
			 !(pbRate != 11025)) ||
			 ((waveData->format & PLAYER_AUDIO_FREQ == PLAYER_AUDIO_FREQ_22k) &&
			 !(pbRate != 22050)) ||
			 ((waveData->format & PLAYER_AUDIO_FREQ == PLAYER_AUDIO_FREQ_48k) &&
			 !(pbRate != 48000)))
	{
		PLAYER_ERROR ("Mismatched sample rates");
		return -1;
	}
	if (((waveData->format & PLAYER_AUDIO_STEREO) && pbNumChannels != 2) ||
			 (!(waveData->format & PLAYER_AUDIO_STEREO) && pbNumChannels == 2))
	{
		PLAYER_ERROR ("Mismatched number of channels");
		return -1;
	}
	if ((waveData->format & PLAYER_AUDIO_BITS) != PLAYER_AUDIO_16BIT)
	{
		PLAYER_ERROR ("Mismatched bitrate");
		return -1;
	}

	// Calculate the number of frames in the data
	// 16-bit stereo is 4 bytes per frame
	int pcmReturn = 0, frames;

	frames = waveData->data_count / 4;
	printf ("Data size: %d\tframe count: %d\n", waveData->data_count, frames);

	// Write the data until there is none left
	unsigned int framesToWrite = frames;
	do
	{
		pcmReturn = snd_pcm_writei (pcmHandle, waveData->data, framesToWrite);
		if (pcmReturn < 0)
		{
			snd_pcm_prepare (pcmHandle);
			printf ("BUFFER UNDERRUN!\t%s\n", snd_strerror (pcmReturn));
		}
		else
			framesToWrite = framesToWrite - pcmReturn;

		printf ("pcmReturn = %d\tframesToWrite = %d\n", pcmReturn, framesToWrite);
		fflush (stdout);
	}
	while (framesToWrite > 0);

	// Drain the remaining data once finished writing and stop playback
// 	snd_pcm_drain (pcmHandle);

	return 0;
}

int Alsa::ProcessMessage (MessageQueue *resp_queue, player_msghdr *hdr, void *data)
{
	// Check for capabilities requests first
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILTIES_REQ);
	// Position2d caps
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_AUDIO_WAV_PLAY_CMD);

	if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_AUDIO_WAV_PLAY_CMD, this->device_addr))
	{
		printf ("handling wave play cmd\n");
		HandleWavePlayCmd (reinterpret_cast<player_audio_wav_t*> (data));
		return 0;
	}

	return -1;
}
