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

#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <netinet/in.h>  /* for struct sockaddr_in, htons(3) */

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/soundcard.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h> 
#include <unistd.h>

#include <pthread.h>

#include <device.h>
#include <drivertable.h>

// If set, we generate tones instead of sampling from the device. This
// allows testing of this driver and a client on the same machine, as
// only one process can open the DSP at a time.
//#define TEST_TONE

const double DURATION = 0.1; /* seconds of sampling */
const int RATE = 16000; /* samples per second */
const int CHANNELS = 1; /* 1 = mono 2 = stereo */
const int DEPTH = 8; /* bits, 8 or 16 */

const int SAMPLES = (int)(DURATION*RATE*CHANNELS);
const int BYTES = SAMPLES*(DEPTH/8);

#define DEVICE "/dev/dsp"


class Waveaudio:public CDevice 
{
private:
  
  int configureDSP();
  void openDSPforRead();
  
  int fd;       /* sound device file descriptor */
  
public:
  
  Waveaudio(char* interface, ConfigFile* cf, int section);
  
  virtual void Main();
  virtual int Setup();
  virtual int Shutdown();
};




CDevice* Waveaudio_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_WAVEFORM_STRING))
  {
    PLAYER_ERROR1("driver \"wave_audio\" does not support interface \"%s\"\n",
                  interface);
    return(NULL);
  }
  else
    return((CDevice*)(new Waveaudio(interface, cf, section)));
}

// a driver registration function
void 
Waveaudio_Register(DriverTable* table)
{
  table->AddDriver("wave_audio", PLAYER_ALL_MODE, Waveaudio_Init);
}

Waveaudio::Waveaudio(char* interface, ConfigFile* cf, int section) :
  CDevice(sizeof(player_waveform_data_t),0,0,0)
{
}

int 
Waveaudio::configureDSP() 
{
#ifdef TEST_TONE
  return 0;
#else

  int arg;      /* argument for ioctl calls */
  int status;   /* return status of system calls */
  int r=0;
  
  openDSPforRead();

  /* set sampling parameters */
  arg = DEPTH;      /* sample size */
  status = ioctl(fd, SOUND_PCM_WRITE_BITS, &arg);
  if (status == -1) {
    perror("SOUND_PCM_WRITE_BITS ioctl failed"); 
    r=-1;
  }
  if (arg != DEPTH) {
    printf("SOUND_PCM_WRITE_BITS: asked for %d, got %d\n", DEPTH, arg);
    //perror("unable to set sample size");
    //r=-1;
  }
  arg = CHANNELS;  /* mono or stereo */
  status = ioctl(fd, SOUND_PCM_WRITE_CHANNELS, &arg);
  if (status == -1) {
    perror("SOUND_PCM_WRITE_CHANNELS ioctl failed");
    r=-1;
  }
  if (arg != CHANNELS) {
    perror("unable to set number of channels");
    r=-1;
  }
  arg = RATE;      /* sampling rate */
  status = ioctl(fd, SOUND_PCM_WRITE_RATE, &arg);
  if (status == -1) {
    perror("SOUND_PCM_WRITE_WRITE ioctl failed");return 0;
    r=-1;
  }

  /* set sampling parameters */
  arg = CHANNELS;  /* mono or stereo */
  status = ioctl(fd, SOUND_PCM_READ_CHANNELS, &arg);
  if (status == -1) {
    perror("SOUND_PCM_READ_CHANNELS ioctl failed");
    r=-1;
  }
  if (arg != CHANNELS) {
    perror("unable to set number of channels");
    r=-1;
  }
  arg = RATE;      /* sampling rate */
  status = ioctl(fd, SOUND_PCM_READ_RATE, &arg);
  if (status == -1) {
    perror("SOUND_PCM_READ_RATE ioctl failed");return 0;
    r=-1;
  }

  return r;

#endif
}

void 
Waveaudio::openDSPforRead() 
{
#ifndef TEST_TONE
  if (fd>0) close(fd);
  
  fd = open(DEVICE, O_RDONLY);
  if (fd < 0) {
    perror("failed to open sound device");
    exit(1);
  } 
#endif
}

int 
Waveaudio::Setup()
{
  int r=0;

  r = configureDSP();
  
  // Start dsp-read/write thread
  StartThread();

  return r;
}

/* main thread */
void 
Waveaudio::Main()
{
    player_waveform_data_t data;
    
    // clear the export buffer
    memset(&data,0,sizeof(data));
    PutData(&data, sizeof(data),0,0);
    
    openDSPforRead();
    
    unsigned int rate = RATE;
    unsigned short depth = DEPTH;
    unsigned int bytes = BYTES;
    
    data.rate = htonl(rate);
    data.depth = htons((unsigned short)depth);
    data.samples = htonl(bytes);
    
    //int freq = 0, minfreq = 1000, maxfreq = 5000;
    
    while(1) 
      {
	pthread_testcancel();
	
	//puts( "READING" );

#ifndef TEST_TONE
	// SAMPLE DATA FROM THE DSP
	if(read(fd, &(data.data), bytes) < (ssize_t)bytes ) 
	  {
	    PLAYER_WARN("not enough data read");
	  } 
#else
	// generate a series of test tones
	double omega= freq * 2.0 * M_PI / RATE;
	double amp = 32;
	double phase = 0;
	
	freq = (int)(freq*1.1); // raise note between bursts
	
	if( freq < minfreq || freq > maxfreq ) freq = minfreq;

	usleep( 100000 );
	
	for(unsigned int i=0; i < bytes; i++) 
	  {
	    phase+=omega;
	    if (phase>M_PI*2) phase-=M_PI*2;
	    data.data[i]=(unsigned char)(127+(int)(amp * sin(phase)));
	  }	
#endif
	/*
	  printf( "rate: %d depth: %d samples: %d\n", 
	  ntohl(data.rate),
	  ntohs(data.depth),
	  ntohl(data.samples) );
	*/
	
	PutData(&data, sizeof(data),0,0);
      }
}

int 
Waveaudio::Shutdown()
{
  StopThread();
  if (fd>0) close(fd);
  //printf("Audio-device has been shutdown\n");
  return 0;
}

