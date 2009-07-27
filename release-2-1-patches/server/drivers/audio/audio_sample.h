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

// Sample type
typedef uint8_t SampleType;
// No type - no wave data loaded
const SampleType SAMPLE_TYPE_NONE	= 0;
// File samples must be opened, and the data comes from/goes to the file on demand
const SampleType SAMPLE_TYPE_FILE	= 1;
// Memory samples are stored as data in memory, eg a sample received via the player server
const SampleType SAMPLE_TYPE_MEM	= 2;

class AudioSample
{
	public:
		AudioSample (void);
		AudioSample (const player_audio_wav_t *source);
		AudioSample (const uint8_t *source, uint32_t length, uint16_t channels, uint32_t sr, uint16_t bps);
		~AudioSample (void);

		// Data management functions
		void SetDataPosition (uint32_t newPosition);	// Set current position in the data (in frames, not bytes)
		uint32_t GetDataPosition (void) const;		// Get current position in the data (in frames, not bytes)
		uint32_t GetDataLength (void) const;		// Get length of data (in frames, not bytes)
		int GetData (int frameCount, uint8_t *buffer);	// Get a block of data from current position
		void ClearSample (void);					// Clear the entire sample (including format), making this a SAMPLE_TYPE_NONE
		bool FillSilence (uint32_t time);			// Fill the sample with silence

		// Data conversion functions
		bool ToPlayer (player_audio_wav_t *dest);			// Convert to player wave structure
		bool FromPlayer (const player_audio_wav_t *source);	// Convert from player wave structure

		// File management functions
		bool LoadFile (const char *fileName);	// Load wave data from a file
		void CloseFile (void);					// Close the opened file
		const char* GetFilePath (void) const	{ return filePath; }

		// Wave format functions
		SampleType GetType (void) const			{ return type; }
		void SetType (SampleType val)			{ type = val; }
		uint16_t GetNumChannels (void) const	{ return numChannels; }
		void SetNumChannels (uint16_t val)		{ numChannels = val; }
		uint32_t GetSampleRate (void) const		{ return sampleRate; }
		void SetSampleRate (uint32_t val)		{ sampleRate = val; byteRate = blockAlign * sampleRate; }
		uint32_t GetByteRate (void) const		{ return byteRate; }
		uint16_t GetBlockAlign (void) const		{ return blockAlign; }
		void SetBlockAlign (uint16_t val)		{ blockAlign = val; byteRate = blockAlign * sampleRate; }
		uint16_t GetBitsPerSample (void) const	{ return bitsPerSample; }
		void SetBitsPerSample (uint16_t val)	{ bitsPerSample = val; }
		uint32_t GetNumFrames (void) const		{ return numFrames; }
		bool SameFormat (const AudioSample *rhs);
		void CopyFormat (const AudioSample *rhs);

		// Other useful functions
		void PrintWaveInfo (void);	// Print out the wave information

	private:
		SampleType type;		// Sample type

		// Information about the wave this sample stores
		uint16_t numChannels;	// Number of channels in the data
		uint32_t sampleRate;	// Number of samples per second
		uint32_t byteRate;		// Number of bytes per second
		uint16_t blockAlign;	// Number of bytes for one sample over all channels (i.e. frame size)
		uint16_t bitsPerSample;	// Number of bits per sample (eg 8, 16, 24, ...)
		uint32_t numFrames;		// Number of frames (divide by sampleRate to get time)

		// Current position in the wave data (in bytes)
		uint32_t position;

		// If this is a file sample, this is the file info
		FILE *waveFile;			// File pointer
		char *filePath;			// Path to the file on disc
		uint32_t headerSize;	// Size of the wave format header in the file (i.e. start of actual data in the file)

		// If this is a memory sample, the data is stored here
		uint32_t dataLength;	// Length of data in bytes
		uint8_t *data;			// Array on the heap containing the data
};
