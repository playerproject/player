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

#include <alsa/asoundlib.h>

////////////////////////////////////////////////////////////////////////////////
// The class for the driver
class Alsa : public Driver
{
	public:
		Alsa (ConfigFile* cf, int section);
		~Alsa (void);

		virtual int Setup (void);
		virtual int Shutdown (void);

		virtual int ProcessMessage (MessageQueue *resp_queue, player_msghdr *hdr, void *data);

	private:
		virtual void Main (void);

		// Command handling
		int HandleWavePlayCmd (player_audio_wav_t *waveData);

		// Driver options
		bool block;						// If should block while playing or return immediatly
		unsigned int pbRate;			// Sample rate for playback
		int pbNumChannels;				// Number of sound channels for playback
		snd_pcm_uframes_t periodSize;	// Period size of the output buffer

		// ALSA variables
		snd_pcm_t *pcmHandle;			// Handle to the PCM device
		snd_pcm_stream_t pbStream;		// Stream for playback
		snd_pcm_hw_params_t *hwparams;	// Hardware parameters
		char *pcmName;					// Name of the sound interface
};
