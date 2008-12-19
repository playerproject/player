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

#include "audio_sample.h"

////////////////////////////////////////////////////////////////////////////////
// Describes a prestored sample for playing with PLAYER_AUDIO_CMD_SAMPLE_PLAY
typedef struct StoredSample
{
	AudioSample *sample;
	int index;					// Index of sample
	struct StoredSample *next;	// Next sample in the linked list
} StoredSample;

////////////////////////////////////////////////////////////////////////////////
// An item on the queue of waves to play
typedef struct QueueItem
{
	AudioSample *sample;	// Audio sample at this position in the queue
	bool temp;				// If true, the AudioSample will be deleted after playback completes
	struct QueueItem *next;	// Next item in the queue
} QueueItem;

////////////////////////////////////////////////////////////////////////////////
// Capabilities of a mixer element
typedef uint32_t ElemCap;
const ElemCap ELEMCAP_CAN_PLAYBACK		= 0x0001;
const ElemCap ELEMCAP_CAN_CAPTURE		= 0x0002;
const ElemCap ELEMCAP_COMMON			= 0x0004;	// Has a single volume control for both playback and record
const ElemCap ELEMCAP_PLAYBACK_VOL		= 0x0008;
const ElemCap ELEMCAP_CAPTURE_VOL		= 0x0010;
const ElemCap ELEMCAP_COMMON_VOL		= 0x0020;
const ElemCap ELEMCAP_PLAYBACK_SWITCH	= 0x0040;
const ElemCap ELEMCAP_CAPTURE_SWITCH	= 0x0080;
const ElemCap ELEMCAP_COMMON_SWITCH		= 0x0100;
// const ElemCap ELEMCAP_PB_JOINED_SWITCH	= 0x0200;
// const ElemCap ELEMCAP_CAP_JOINED_SWITCH	= 0x0400;

// Describes an ALSA mixer element
typedef struct MixerElement
{
	snd_mixer_elem_t *elem;			// ALSA Mixer element structure
	long minPlayVol, curPlayVol, maxPlayVol;	// min, current and max volume levels for playback
	long minCapVol, curCapVol, maxCapVol;		// min, current and max volume levels for capture
	long minComVol, curComVol, maxComVol;		// min, current and max volume levels for common
	int playSwitch, capSwitch, comSwitch;		// Current switch status
	char *name;						// Name of the element
	ElemCap caps;					// Capabilities
} MixerElement;

////////////////////////////////////////////////////////////////////////////////
// State of the playback system
typedef uint8_t PBState;
const PBState PB_STATE_STOPPED		= 0;	// Not playing anything
const PBState PB_STATE_PLAYING		= 1;	// Playing
const PBState PB_STATE_DRAIN		= 2;	// Draining current wave
const PBState PB_STATE_RECORDING	= 3;	// Recording

////////////////////////////////////////////////////////////////////////////////
// The class for the driver
class Alsa : public ThreadedDriver
{
	public:
		Alsa (ConfigFile* cf, int section);
		~Alsa (void);

		virtual int ProcessMessage (QueuePointer &resp_queue, player_msghdr *hdr, void *data);

	private:
		// Driver options
		bool useQueue;					// If should use a queue for playback or just stop currently playing
		uint8_t debugLevel;				// Debug info level
		char **mixerFilters;			// Null-terminated array of strings to filter mixer elements by
		bool mixerFilterExact;			// If mixer filters need to match element names exactly
		char *pbDevice;					// Name of the playback device
		char *mixerDevice;				// Name of the mixer device
		char *recDevice;				// Name of the record device
		uint32_t cfgPBPeriodTime;		// Length of a playback period in milliseconds
		uint32_t cfgPBBufferTime;		// Length of the playback buffer in milliseconds
		uint32_t silenceTime;			// Length of silence to put between samples in the queue
		uint32_t cfgRecBufferTime;		// Length of the record buffer in milliseconds
		uint32_t cfgRecStoreTime;		// Length of time to store recorded data before sending to clients
		uint32_t cfgRecPeriodTime;		// Length of a record period in milliseconds
		uint8_t recNumChannels;			// Number of channels for recording
		uint32_t recSampleRate;			// Sample rate for recording
		uint8_t recBits;				// Sample bits per frame for recording

		// ALSA variables
		// Playback
		snd_pcm_t *pbHandle;			// Handle to the PCM device for playback
		int numPBFDs;					// Number of playback file descriptors
		struct pollfd *pbFDs;			// Playback file descriptors for polling
		uint32_t actPBBufferTime;		// Length of the playback buffer in microseconds (actual value)
		uint32_t actPBPeriodTime;		// Length of a playback period in microseconds (actual value)
		snd_pcm_uframes_t pbPeriodSize;	// Size of a playback period (used for filling the buffer)
		uint8_t *periodBuffer;			// A buffer used to copy data from samples to the playback buffer
		// Record
		snd_pcm_t *recHandle;			// Handle to the PCM device for recording
		int numRecFDs;					// Number of record file descriptors
		struct pollfd *recFDs;			// Record file descriptors for polling
		uint32_t actRecBufferTime;		// Length of the record buffer in microseconds (actual value)
		uint32_t actRecPeriodTime;		// Length of a record period in microseconds (actual value)
		snd_pcm_uframes_t recPeriodSize;	// Size of a record period (used for filling the buffer)
		// Mixer
		snd_mixer_t *mixerHandle;		// Mixer for controlling volume levels
		MixerElement *mixerElements;	// Elements of the mixer
		uint32_t numElements;			// Number of elements

		// Other driver data
		int nextSampleIdx;				// Next free index to store a sample at
		StoredSample *samplesHead, *samplesTail;	// Stored samples
		QueueItem *queueHead, *queueTail;	// Queue of audio data waiting to play
		PBState playState;				// Playback state
		PBState recState;				// Record state
		uint32_t recDataLength;			// Length of recorded data buffer (in bytes)
		uint32_t recDataOffset;			// Current end of recorded data in recData
		uint8_t *recData;				// Somewhere to store recorded data before it goes to the client
		int recDest;					// Destination of recorded data: -ve for to clients via data packets,
										// >=0 for to stored sample at that index

		// Internal functions
		virtual void Main (void);
		virtual int MainSetup (void);
		virtual void MainQuit (void);
		void SendStateMessage (void);

		// Stored sample functions
		bool AddStoredSample (StoredSample *newSample);
		bool AddStoredSample (player_audio_wav_t *waveData);
		bool AddStoredSample (const char *filePath);
		StoredSample* GetSampleAtIndex (int index);

		// Queue functions
		void ClearQueue (void);
		bool AddToQueue (QueueItem *newItem);
		bool AddToQueue (player_audio_wav_t *waveData);
		bool AddToQueue (AudioSample *sample);
		bool AddSilence (uint32_t time, AudioSample *format);
		void AdvanceQueue (void);

		// Playback functions
// 		bool SetGeneralParams (void);
// 		bool SetWaveHWParams (AudioSample *sample);
		bool SetupPlayBack (void);
		bool SetPBParams (AudioSample *sample);
		void PlaybackCallback (int numFrames);

		// Record functions
		bool SetupRecord (void);
		bool SetRecParams (void);
		bool SetupRecordBuffer (uint32_t length);
		void RecordCallback (int numFrames);
		void HandleRecordedData (void);
		void PublishRecordedData (void);

		// Playback/record control functions
		void StartPlayback (void);
		void StopPlayback (void);
		void StartRecording (void);
		void StopRecording (void);

		// Mixer functions
		bool SetupMixer (void);
		bool EnumMixerElements (void);
		bool EnumElementCaps (MixerElement *element);
		MixerElement* SplitElements (MixerElement *elements, uint32_t &count);
		MixerElement* FilterElements (MixerElement *elements, uint32_t &count);
		void CleanUpMixerElements (MixerElement *elements, uint32_t count);
		void MixerDetailsToPlayer (player_audio_mixer_channel_list_detail_t *dest);
		void MixerLevelsToPlayer (player_audio_mixer_channel_list_t *dest);
		void SetElementLevel (uint32_t index, float level);
		void SetElementSwitch (uint32_t index, player_bool_t active);
		void PublishMixerData (void);
		float LevelToPlayer (long min, long max, long level);
		long LevelFromPlayer (long min, long max, float level);
		void PrintMixerElements (MixerElement *elements, uint32_t count);

		// Command and request handling
		int HandleWavePlayCmd (player_audio_wav_t *waveData);
		int HandleSamplePlayCmd (player_audio_sample_item_t *data);
		int HandleRecordCmd (player_bool_t *data);
		int HandleMixerChannelCmd (player_audio_mixer_channel_list_t *data);
		int HandleSampleLoadReq (player_audio_sample_t *data, QueuePointer &resp_queue);
		int HandleSampleRetrieveReq (player_audio_sample_t *data, QueuePointer &resp_queue);
		int HandleSampleRecordReq (player_audio_sample_rec_req_t *data, QueuePointer &resp_queue);
		int HandleMixerChannelListReq (player_audio_mixer_channel_list_detail_t *data, QueuePointer &resp_queue);
		int HandleMixerChannelLevelReq (player_audio_mixer_channel_list_t *data, QueuePointer &resp_queue);
};
