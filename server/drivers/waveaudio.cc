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
#include <linux/soundcard.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h> 
#include <unistd.h>

#include <pthread.h>

#include <device.h>
#include <drivertable.h>

#define AUDIO_SLEEP_TIME_USEC 100000

//#define N 1024
#define N 784
#define LENGTH 1 /* how many tenth of seconds of samplig max */
#define RATE 16000 /* the sampling rate */
#define CHANNELS 1 /* 1 = mono 2 = stereo */
#define SIZE 8 /* bits, 8 or 16 */

class Waveaudio:public CDevice 
{
 private:
  
  // Esbens own additions
  int audio_fd; // audio device file descriptor
  int configureDSP();
  void openDSPforRead();

  int fd;       /* sound device file descriptor */
  unsigned char sample[N*CHANNELS*SIZE/8];

  unsigned char buf[(LENGTH*RATE*SIZE*CHANNELS/8)/10];

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
  int arg;      /* argument for ioctl calls */
  int status;   /* return status of system calls */
  int r=0;
  
  openDSPforRead();

  /* set sampling parameters */
  arg = SIZE;      /* sample size */
  status = ioctl(fd, SOUND_PCM_WRITE_BITS, &arg);
  if (status == -1) {
    perror("SOUND_PCM_WRITE_BITS ioctl failed"); 
    r=-1;
  }
  if (arg != SIZE) {
    printf("SOUND_PCM_WRITE_BITS: asked for %d, got %d\n", SIZE, arg);
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
}

void 
Waveaudio::openDSPforRead() 
{
  if (fd>0) close(fd);
  
  fd = open("/dev/dsp", O_RDONLY);
  if (fd < 0) {
    perror("open of /dev/dsp failed");
    exit(1);
  } 
}

int 
Waveaudio::Setup()
{
  int r=0;

  r = configureDSP();
  
  printf("Waveaudio: Ran setup() \n");

  // Start dsp-read/write thread
  StartThread();

  return r;
}

const int UNKNOWN=0;
const int LISTENING=1;
const int PLAYING=2;

/* main thread */
void 
Waveaudio::Main()
{
    player_waveform_data_t data;
    
    unsigned int cnt, i;
    int state = UNKNOWN;
    short playFrq=0, playAmp=0, playDur;
    unsigned int playDuration=0;
    unsigned int current=0;
    unsigned int remaining;
    double omega = 0.0;
    double phase = 0.0;
    
    // clear the export buffer
    memset(&data,0,sizeof(data));
    PutData(&data, sizeof(data),0,0);
    
    openDSPforRead();
    
    
    unsigned int rate = 1600;
    unsigned short depth = SIZE;
    unsigned int samples = N*CHANNELS*SIZE/8;
    
    data.rate = htonl(rate);
    data.depth = htons((unsigned short)depth);
    data.samples = htonl(samples);
    
    while(1) 
      {
	pthread_testcancel();
	
	puts( "READING" );
	
	if(read(fd, &(data.data), samples) < samples ) 
	  {
	    PLAYER_WARN("not enough data read");
	  } 
	
	printf( "rate: %d depth: %d samples: %d\n", 
		ntohl(data.rate),
		ntohs(data.depth),
		ntohl(data.samples) );

	for( int s=0; s<samples; s++ )
	  printf( "%u ", data.data[s] );

	puts("");

	PutData(&data, sizeof(data),0,0);
	usleep(100000);
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

