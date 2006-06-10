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
// Detailed description of wave data
// This is more advanced than the player wave data structure: it can store
// actual sample rate values, etc, making it more flexible and so useful for
// locally loaded wave data
typedef struct WaveData
{
	uint16_t numChannels;	// Number of channels in the data
	uint32_t sampleRate;	// Number of samples per second
	uint32_t byteRate;		// Number of bytes per second
	uint16_t blockAlign;	// Number of bytes for one sample over all channels (i.e. frame size)
	uint16_t bitsPerSample;	// Number of bits per sample (eg 8, 16, 24, ...)
	uint32_t numFrames;		// Number of frames (divide by sampleRate to get time)
	uint32_t dataLength;	// Length of data in bytes
	uint8_t *data;
} WaveData;

////////////////////////////////////////////////////////////////////////////////
// Describes an audio sample
typedef int SampleType;
const SampleType SAMPLE_TYPE_LOCAL = 0;		// Local samples are stored in a file
const SampleType SAMPLE_TYPE_REMOTE = 1;	// Remote samples are stored in memory

typedef struct AudioSample
{
	SampleType type;			// Type of sample - local file or stored in memory
	char *localPath;			// Path to local file if local
	WaveData *memData;			// Stored audio data if sample is memory
	int index;					// Index of sample
	struct AudioSample *next;	// Next sample in the linked list
} AudioSample;

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
		// Driver options
		bool block;						// If should block while playing or return immediatly
//		uint32_t pbRate;				// Sample rate for playback
//		int pbNumChannels;				// Number of sound channels for playback
		AudioSample *samplesHead, *samplesTail;	// Stored samples

		// ALSA variables
		snd_pcm_t *pcmHandle;			// Handle to the PCM device
		snd_pcm_stream_t pbStream;		// Stream for playback
		char *pcmName;					// Name of the sound interface

		// Other driver data
		int nextSampleIdx;				// Next free index to store a sample at

		// Command and request handling
		int HandleWavePlayCmd (player_audio_wav_t *waveData);
		int HandleSamplePlayCmd (player_audio_sample_item_t *data);
		int HandleSampleLoadReq (player_audio_sample_t *data, MessageQueue *resp_queue);
		int HandleSampleRetrieveReq (player_audio_sample_t *data, MessageQueue *resp_queue);

		// Internal functions
		virtual void Main (void);
		bool AddSample (AudioSample *newSample);
		bool AddFileSample (const char *path);
		bool AddMemorySample (player_audio_wav_t *sampleData);
		AudioSample* GetSampleAtIndex (int index);
		WaveData* LoadWaveFromFile (char *fileName);
		bool WaveDataToPlayer (WaveData *source, player_audio_wav_t *dest);
		bool PlayerToWaveData (player_audio_wav_t *source, WaveData *dest);
		void PrintWaveData (WaveData *wave);

		// Sound functions
		bool SetHWParams (WaveData *wave);
		bool PlayWave (WaveData *wave);
};
