/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
 *
 *  Audiodevice attempted written by Esben Ostergaard
 *
 * This program is free software; you can redistribute it and/or modify
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

#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include <sys/soundcard.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <netinet/in.h>
#include <gsl/gsl_fft_real.h>
#include <playertime.h>

#include "playercommon.h"
#include "drivertable.h"
#include "player.h"

extern PlayerTime* GlobalTime;

#define DEFAULT_DEVICE "/dev/dsp"
#define MIN_FREQUENCY 800

class Acoustics : public CDevice
{
  public:
    // Constructor
    Acoustics(char* interface, ConfigFile* cf, int section);

    int Setup();
    int Shutdown();

    size_t GetCommand(void* dest, size_t maxsize);

  private:

    // Open or close the device
    int OpenDevice( int flag );
    int CloseDevice();

    // The main loop
    virtual void Main();

    // Get and set the configuration of the driver
    int SetConfiguration(int len, void* client, unsigned char buffer[]);
    int GetConfiguration(int len, void* client, unsigned char buffer[]);
   
    // Set various attributes of the audio device
    int SetSampleFormat(int _format);
    int SetSampleRate(int _rate);
    int SetChannels(unsigned short _channels);
    int SetBufferSize(int _size);

    // Functions for finding primary frequencies
    int ListenForTones();
    void InsertPeak(int f, int a);

    // Record and playback
    int Record();
    int PlayBuffer(char* buffer,unsigned int size);


    // Create a sine wave
    void CreateSine(unsigned short freq, unsigned short amp, 
        unsigned int duration, char** buffer, unsigned int* bufferSize);

    // Create a BPSK chirp
    void CreateChirp(unsigned char mseq[],unsigned short mseqSize, 
        unsigned short freq, unsigned short amp, unsigned int pulseTime, 
        char** buffer, unsigned int* bufSize );


    int audioFD; // File descriptor for the device
    const char* deviceName; // Name of the device( ex: "/dev/dsp" )
    int openFlag; // Flag for WRITE or READ mode
    int channels; // Number of channels( 1 || 2 )
    int sampleFormat; // Defines format: sample size, endian convention
    int sampleRate; // In Hertz
    int audioBuffSize; // The size of the buffer, should be a power of two
    char* audioBuffer; // The buffer used to hold audio data
    float bytesPerSample; // The number of bytes per sample

    unsigned short* peakFreq; // List of peak frequencies
    unsigned short* peakAmp; // List of peak frequency amplitudes
    int N; // The length(in bytes) of the audio buffer to process
    short nHighestPeaks; // Number of peaks to find
    player_audiodsp_data_t data; // The data to return to the user
    struct timeval eventTime; // The time the data was captured

};

Acoustics::Acoustics(char* interface, ConfigFile* cf, int section)
  : CDevice(sizeof(player_audiodsp_data_t),sizeof(player_audiodsp_cmd_t),1,1)
{
  deviceName = cf->ReadString(section,"device",DEFAULT_DEVICE);
}

CDevice* Acoustics_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_AUDIODSP_STRING))
  {
    PLAYER_ERROR1("driver \"acoustics\" does not support interface \"%s\"\n", 
                  interface);
    return(NULL);
  }
  else
    return((CDevice*)(new Acoustics(interface, cf, section)));
}

void Acoustics_Register(DriverTable* table)
{
  table->AddDriver("acoustics", PLAYER_ALL_MODE, Acoustics_Init );
}

int Acoustics::Setup()
{

  N=1024;
  nHighestPeaks = 5;
  peakFreq = new unsigned short[nHighestPeaks];
  peakAmp = new unsigned short[nHighestPeaks];

  puts("audio ready");

  StartThread();
  return 0;
}

int Acoustics::Shutdown()
{
  StopThread();
  this->CloseDevice();

  puts("Acoustics has been shutdown");
}

size_t Acoustics::GetCommand(void* dest, size_t maxsize)
{
  int retval = device_used_commandsize;

  if(device_used_commandsize)
  {
    memcpy(dest,device_command,device_used_commandsize);
    device_used_commandsize = 0;
  }
  return(retval);
}

int Acoustics::OpenDevice( int flag )
{

  // We don't need to reopen the device if the flag is the same
  if( openFlag != flag )
  {
    openFlag = flag;

    // First close the device
    close(audioFD);

    // Then open it again with the new flag
    if( (audioFD = open(deviceName,openFlag)) == -1 )
    {
      PLAYER_ERROR1("failed to open audio device %s",deviceName);
      return -1;
    }

  }

  return 0;
}

int Acoustics::CloseDevice()
{
  openFlag = -1;
  return close(audioFD);
}

void Acoustics::Main()
{
  int len;
  void *client;
  unsigned char configBuffer[PLAYER_MAX_REQREP_SIZE];
  unsigned char cmdBuffer[sizeof(player_audiodsp_cmd)];
  char* playBuffer;
  unsigned int playBufferSize;

  player_audiodsp_cmd_t audioCmd;
  playBuffer = NULL;

  sleep(1);

  while(true)
  {
    // test if we are suppose to cancel
    pthread_testcancel();

    // Set/Get the configuration
    while((len = GetConfig(&client, &configBuffer, sizeof(configBuffer))) > 0)
    {

      switch(configBuffer[0])
      {
        case PLAYER_AUDIODSP_SET_CONFIG:
          this->SetConfiguration(len,client,configBuffer);
          break;

        case PLAYER_AUDIODSP_GET_CONFIG:
          this->GetConfiguration(len,client,configBuffer);
          break;

        default:
          if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
            PLAYER_ERROR("PutReply() failed");
          break;
      }

    }

    // Get the next command
    memset(&cmdBuffer,0,sizeof(cmdBuffer));
    len = this->GetCommand(&cmdBuffer,sizeof(cmdBuffer));

    // Process the command
    switch(cmdBuffer[0])
    {
      case PLAYER_AUDIODSP_PLAY_TONE:
        memcpy(&audioCmd, cmdBuffer, sizeof(audioCmd));

        // Create a tone
        this->CreateSine(ntohs(audioCmd.frequency), ntohs(audioCmd.amplitude), 
            ntohl(audioCmd.duration), &playBuffer, &playBufferSize);

        // Play the sound
        this->PlayBuffer(playBuffer,playBufferSize);
        break;

      case PLAYER_AUDIODSP_PLAY_CHIRP:
        memcpy(&audioCmd, cmdBuffer, sizeof(audioCmd));
        // Create a chirp
        this->CreateChirp(audioCmd.bitString,ntohs(audioCmd.bitStringLen),
            ntohs(audioCmd.frequency), ntohs(audioCmd.amplitude), 
            ntohl(audioCmd.duration), &playBuffer, &playBufferSize);

        // Play the chirp
        this->PlayBuffer(playBuffer,playBufferSize);
        break;

        // Replay the last buffer
      case PLAYER_AUDIODSP_REPLAY:
        memcpy(&audioCmd, cmdBuffer, sizeof(audioCmd));
        this->PlayBuffer(playBuffer,playBufferSize);
        break;

        // The default is to listen for tones and report the findings
      default:
        {
          // Get the time at which we started reading
          // This will be a pretty good estimate of when the phenomena occured
          GlobalTime->GetTime(&eventTime);
 
          // Get the most significant frequencies
          if( !ListenForTones() )
          {
            for (int i=0; i<nHighestPeaks; i++) 
            {
              data.freq[i]=htons((unsigned short)((peakFreq[i]*sampleRate)/N));
              data.amp[i]=htons((unsigned short)peakAmp[i]);
            }

            // Return the data to the user
            PutData((uint8_t*)&data, sizeof(data),0,0);
          }

          break;
        }
    }
  }
}

int Acoustics::SetConfiguration(int len, void* client, unsigned char buffer[])
{

  player_audiodsp_config_t config;

  if( len != sizeof(player_audiodsp_config_t))
  {
    PLAYER_ERROR2("config request len is invalid (%d != %d)", len, 
        sizeof(config));
    if( PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
      PLAYER_ERROR("PutReply() failed");
    return 1;
  }

  memcpy(&config, buffer, sizeof(config));
  this->channels = config.channels;

  // Must open the device for write in order to configure it
  this->OpenDevice(O_WRONLY);

  // Attempts to set the format and rate of each sample along with
  // the number of channels to use.
  if( this->SetSampleFormat( ntohs(config.sampleFormat)) == 0 &&
      this->SetChannels( ntohs(config.channels)) == 0 && 
      this->SetSampleRate( ntohs(config.sampleRate)) == 0 )
  {
    // Create the audio buffer
    this->SetBufferSize(0);

    if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &config, 
          sizeof(config)) != 0)
      PLAYER_ERROR("PutReply() failed");

    return -1;

  } else {
    if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
      PLAYER_ERROR("PutReply() failed");
  }

  return 0;
}

int Acoustics::GetConfiguration(int len, void* client, unsigned char buffer[])
{

  player_audiodsp_config_t config;

  if( len != 1 )
  {
    PLAYER_ERROR2("config request len is invalid (%d != %d)",len,1);
    if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
      PLAYER_ERROR("PutReply() failed");
    return 1;
  }

  config.sampleFormat = htons(this->sampleFormat);
  config.sampleRate = htons(this->sampleRate);
  config.channels = this->channels;

  if( PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &config,
        sizeof(config)) != 0)
    PLAYER_ERROR("PutReply() failed");

  return 0;
}

int Acoustics::SetSampleFormat( int _format )
{
  int result=0;

  sampleFormat = _format;

  // Try to set the sample format
  if (ioctl(audioFD,SNDCTL_DSP_SETFMT, &sampleFormat) == -1 )
  {
    PLAYER_ERROR1("error setting sample format: %d",_format);
    sampleFormat=255;

    if(ioctl(audioFD,SNDCTL_DSP_SETFMT, &sampleFormat) == -1 )
      result = -1;
  }

  // Check if we were able to set the specified format
  if( !result && sampleFormat != _format )
  {
    PLAYER_WARN2("specified format %d set to %d",_format,sampleFormat);
  }

  // Get the bytes per sample
  switch( sampleFormat )
  {
    // Formats with 4 bits per sample
    case AFMT_IMA_ADPCM:
      bytesPerSample = 0.5;
      break;

    // Formats with 8 bits per sample
    case AFMT_MU_LAW:
    case AFMT_A_LAW:
    case AFMT_U8:
    case AFMT_S8:
      bytesPerSample = 1.0;
      break;

    // Formats with 16 bits per sample
    case AFMT_S16_LE:
    case AFMT_S16_BE:
    case AFMT_U16_LE:
    case AFMT_U16_BE:
      bytesPerSample = 2.0;
      break;
    default:
      bytesPerSample = 2.0;
      break;
  }

  return result;
}

int Acoustics::SetSampleRate( int _sampleRate )
{
  int result = 0;

  sampleRate = _sampleRate;

  // Try to set the sample rate
  if( ioctl(audioFD, SNDCTL_DSP_SPEED, &sampleRate) == -1 )
  {
    PLAYER_ERROR1("error setting sample rate:%d",sampleRate);
    result = 1;
  }

  // Check if the sample rate was set properly
  if( sampleRate != _sampleRate ) 
  {
    PLAYER_WARN2("specified rate:%d set to: %d",_sampleRate, sampleRate);
  }

  return result;
}

int Acoustics::SetChannels( unsigned short _channels )
{
  int result = 0;

  channels = _channels;

  // Try to set the number of channels
  if( ioctl(audioFD, SNDCTL_DSP_CHANNELS, &channels) == -1)
  {
    PLAYER_ERROR("error setting the number of channels");
    result = 1;
  }

  return result;
}

int Acoustics::SetBufferSize(int _size)
{
  int result=0;

  // If size==0, then calc. the proper size.
  if( _size <= 0 )
  {
    ioctl( audioFD, SNDCTL_DSP_GETBLKSIZE, &audioBuffSize);
    if( audioBuffSize < 1)
    {
      PLAYER_ERROR("failed to calculate audio buffer size");
      result = -1;
    }

  } else {
    audioBuffSize = _size;
  }

  if( !result && audioBuffSize > 0 )
  {
    if( audioBuffer )
      delete audioBuffer;

    audioBuffer = new char[audioBuffSize];
  } else
    result = -1;

  return result;
}


int Acoustics::ListenForTones()
{
  int result=0;

  if( !Record() )
  {
    int i,k;

    int frequency[N/2];
    int amplitude[N/2];

    // We will just do a Fourer transform over the first 1024 samples.
    double* fft = new double[N];
    if( bytesPerSample == 2 )
    {
      char* index = audioBuffer;
      for(i=0;i<N;i++)
      {
        switch( sampleFormat )
        {
          case AFMT_S16_LE:
          case AFMT_U16_LE:
            fft[i] = (short)((*(index+1)&0xFF) << 8 | ( (*index)&0xFF ));
            break;

          case AFMT_S16_BE:
          case AFMT_U16_BE:
            fft[i] = (short)( ((*index)&0xFF) << 8 | *(index+1)&0xFF);
            break;
        }

        index+=2;
      }

    } else {
      char* index = audioBuffer;
      for(i=0;i<N;i++)     
      {
        fft[i] = (*index);
        index++;
      }
    }
    
    gsl_fft_real_radix2_transform(fft,1,N);

    // Convert to power spectrum
    for (k = 1; k < (N+1)/2; ++k)  /* (k < N/2 rounded up) */
      frequency[k] = (int)((fft[k]*fft[k] + fft[N-k]*fft[N-k])/1000);
    if (N % 2 == 0) /* N is even,  Nyquist freq. */
      frequency[N/2] = (int)((fft[N/2]*fft[N/2])/1000);  

    // I think this does a bit of smoothing
    amplitude[0]=frequency[0]+frequency[1]/2;
    for (k = 1; k < (N-1)/2; ++k)  /* (k < N/2 rounded up) */ 
      amplitude[k] = (frequency[k-1]+frequency[k+1])/2+frequency[k];
    amplitude[(N-1)/2]=frequency[(N-3)/2]/2+frequency[(N-1)/2];

    // Initialize the peak data
    for (i=0; i<nHighestPeaks; i++) 
    {
      peakFreq[i]=0;
      peakAmp[i]=0;
    }

    for (i=MIN_FREQUENCY*N/sampleRate; i<(N-1)/2; i++) 
    {
      if(amplitude[i] > peakAmp[nHighestPeaks-1]) 
      {
        if((amplitude[i] >= amplitude[i-1]) && (amplitude[i]>amplitude[i+1])) 
        {
          InsertPeak(i,amplitude[i]);
        }
      }
    }

  } else {
    result = -1;
  }

  return result;
}

void Acoustics::InsertPeak(int f,int a) 
{
  int i=nHighestPeaks-1;
  int j;

  while (peakAmp[i-1]<a && i>0) {
    i--;
  }

  for (j=nHighestPeaks-1; j>i; j--) {
    peakAmp[j]=peakAmp[j-1];
    peakFreq[j]=peakFreq[j-1];
  }

  peakAmp[i]=(unsigned short)(a);
  peakFreq[i]=(unsigned short)(f);
}

int Acoustics::PlayBuffer(char* buffer,unsigned int size)
{
  int result;

  // Open the device for writing
  result = this->OpenDevice(O_WRONLY);

  // Write the audio data to the device
  if( !result && write(audioFD,buffer,size) != (int)size )
  {
    PLAYER_ERROR("played wrong number of bytes");
    result = -1;
  }

  // Wait for the sync to occur
  if( !result && ioctl(audioFD,SNDCTL_DSP_SYNC,0) == -1 )
  {
    PLAYER_ERROR("ioctl SNDCTL_DSP_SYNC failed");
    result = -1;
  }

  return result;
}

int Acoustics::Record()
{
  int result = this->OpenDevice(O_RDONLY);

  if( !result && read(audioFD, audioBuffer, audioBuffSize) != audioBuffSize)
  {
    PLAYER_WARN("did not read specified amount from audio device");
    result = -1;
  }

  return result;
}


void Acoustics::CreateChirp(unsigned char mseq[], unsigned short mseqSize, 
    unsigned short freq, unsigned short amp, unsigned int pulseTime, 
    char** buffer, unsigned int* bufSize )
{
  unsigned int i;
  unsigned int pulseBufSize;
  char* oneBuffer = NULL;
  char* zeroBuffer = NULL;

  // Create one carrier pulse
  CreateSine(freq,amp,pulseTime,&oneBuffer,&pulseBufSize);

  // Create the zero buffer
  zeroBuffer=new char[pulseBufSize];
  memcpy(zeroBuffer,oneBuffer,pulseBufSize*sizeof(char));

  for(i=0;i<pulseBufSize;i++)
  {
    zeroBuffer[i]*=-1;
  }

  if( *buffer != NULL ) delete *buffer;

  *bufSize = mseqSize*pulseBufSize;
  *buffer = new char[*bufSize];

  for(i=0;i<mseqSize;i++)
  {
    if(mseq[i])
    {
      memcpy(*buffer+pulseBufSize*i,zeroBuffer,pulseBufSize);
    } else {
      memcpy(*buffer+pulseBufSize*i,oneBuffer,pulseBufSize);
    }

  }

  delete [] oneBuffer;
  delete [] zeroBuffer;

  oneBuffer=NULL;
  zeroBuffer=NULL;
}

// This function might behave badly when the durations is too small....
void Acoustics::CreateSine(unsigned short freq, unsigned short amp, 
    unsigned int duration, char** buffer, unsigned int* bufferSize)
{
  unsigned int i;

  double omega = (double)freq*2*M_PI/(double)this->sampleRate;
  double phase = 0;

  unsigned int numSamples = (unsigned int)((duration/1000.0)*this->sampleRate);
  *bufferSize =(unsigned int)(numSamples*this->bytesPerSample*this->channels); 
  short audio;

  if( *buffer != NULL ) delete *buffer;
  
  *buffer = new char[*bufferSize];

  // Calculate the first full wave
  for(phase=0,i=0;phase<2*M_PI&&i<numSamples;phase+=omega,i++)
  {
    audio=(short)(amp*sin(phase));

    if(channels==1)
    {
      (*buffer)[2*i+1]=audio&0xff;
      (*buffer)[2*i]=(audio>>8)&0xff;
    } else {
      (*buffer)[4*i]=audio&0xff;
      (*buffer)[4*i+1]=audio>>8;
      (*buffer)[4*i+2]=audio&0xff;
      (*buffer)[4*i+3]=audio>>8;
    }
  }

  // Get the rest of the data
  unsigned int bytesPerCopy = i*this->channels*2;
  for(unsigned int j=bytesPerCopy;j<*bufferSize;j+=bytesPerCopy)
  {
    memcpy(*buffer+j,*buffer,bytesPerCopy);
  }

}


