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

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_fixedtones fixedtones

The fixedtones driver provides access to sound hardware, via the
Linux OSS interface.  Incoming sound is put through a Discrete Fourier
Transform, and the frequencies and amplitudes of the five highest peaks
in the frequency domain are determined.  The fixedtones driver can also
produce fixed-tone sounds of given frequency, amplitude, and duration.

This driver is not widely used and may not function properly.  You might
want to try the @ref player_driver_audio driver instead.

@par Compile-time dependencies

- FFTW library

@par Provides

- @ref player_interface_audio

@par Requires

- none

@par Configuration requests

- none

@par Configuration file options

- none

@par Authors

- Esben Ostergaard

*/

/** } */

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
// replaced linux/soundcard for portability, but only tested on Linux - rtv
#include <sys/soundcard.h> 
// #include <linux/soundcard.h> 
#include <rfftw.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h> 
#include <unistd.h>

#include <pthread.h>

#include <driver.h>
#include <drivertable.h>

#define AUDIO_SLEEP_TIME_USEC 100000

//#define N 1024
#define N 784
#define nHighestPeaks 5
#define heardHistoryN 5
#define LENGTH 1 /* how many tenth of seconds of samplig max */
#define RATE 16000 /* the sampling rate */
#define CHANNELS 2 /* 1 = mono 2 = stereo */
#define SIZE 16 /* bits, 8 or 16 */
#define MIN_FREQUENCY 800
#define nIterations 10000

class FixedTones:public Driver 
{
 private:
  bool command_pending1;  // keep track of how many commands are pending;
  bool command_pending2;  // that way, we can cancel them if necessary
  bool audio_fd_blocking;
  
  
  // Esbens own additions
  int audio_fd; // audio device file descriptor
  int configureDSP();
  void openDSPforRead();
  void openDSPforWrite();
  void insertPeak(int f,int a);
  int sampleSound();
  void listenForTones();
  int playSound(int duration);
  virtual void PrintPacket(char* str, unsigned char* cmd, int len);

  rfftw_plan p;
  int fd;       /* sound device file descriptor */
  unsigned char sample[N*CHANNELS*SIZE/8];
  fftw_real in[N];
  fftw_real out[N];
  int frequency[N/2];
  int amplitude[N/2];
  int heard, heardFrequency;
  int heardHistory[heardHistoryN+1];
  int peakFrq[nHighestPeaks];
  int peakAmp[nHighestPeaks];

  unsigned char buf[(LENGTH*RATE*SIZE*CHANNELS/8)/10];

 public:

  FixedTones( ConfigFile* cf, int section);

  virtual void Main();

  virtual int Setup();
  virtual int Shutdown();
};

Driver* FixedTones_Init( ConfigFile* cf, int section)
{
  return((Driver*)(new FixedTones( cf, section)));
}

// a driver registration function
void 
FixedTones_Register(DriverTable* table)
{
  table->AddDriver("fixedtones",  FixedTones_Init);
}

FixedTones::FixedTones( ConfigFile* cf, int section) :
  Driver(cf, section, PLAYER_AUDIO_CODE, PLAYER_ALL_MODE, 
         AUDIO_DATA_BUFFER_SIZE,AUDIO_COMMAND_BUFFER_SIZE,0,0)
{
  fd=-1;
}

int 
FixedTones::configureDSP() 
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
FixedTones::openDSPforRead() 
{
  if (fd>0) close(fd);
  
  fd = open("/dev/dsp", O_RDONLY);
  if (fd < 0) {
    perror("open of /dev/dsp failed");
    exit(1);
  } 
}

void 
FixedTones::openDSPforWrite() 
{
  if (fd>0) close(fd);

  fd = open("/dev/dsp", O_WRONLY );
  if (fd < 0) {
    perror("open of /dev/dsp failed");
    exit(1);
  } 
}

int 
FixedTones::Setup()
{
  int r=0;

  p=rfftw_create_plan(N, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);
  r = configureDSP();
  
  printf("Audio: Ran setup() \n");

  // Start dsp-read/write thread
  StartThread();

  return r;
}

const int UNKNOWN=0;
const int LISTENING=1;
const int PLAYING=2;

/* main thread */
void 
FixedTones::Main()
{
  unsigned char data[AUDIO_DATA_BUFFER_SIZE];
  unsigned char command[AUDIO_COMMAND_BUFFER_SIZE];
  unsigned char zero[AUDIO_COMMAND_BUFFER_SIZE];
  unsigned int cnt, i;
  int state = UNKNOWN;
  short playFrq=0, playAmp=0, playDur;
  unsigned int playDuration=0;
  unsigned int current=0;
  unsigned int remaining;
  double omega = 0.0;
  double phase = 0.0;

  memset( command, 0, AUDIO_COMMAND_BUFFER_SIZE);
  memset( zero, 255, AUDIO_COMMAND_BUFFER_SIZE);
  memset(data,0,AUDIO_DATA_BUFFER_SIZE);
  PutData(data, sizeof(data),NULL);
  PutCommand(zero, sizeof(zero),NULL);

  while(1) 
  {
    pthread_testcancel();
    GetCommand(command, sizeof(command),NULL);
    PutCommand(zero, sizeof(zero),NULL);
    
    if(command[0]!=255) 
    {
      cnt = 0;
      playFrq = (short)ntohs((unsigned short)(*(short*)(&command[cnt])));
      cnt += sizeof(short);
      playAmp = (short)ntohs((unsigned short)(*(short*)(&command[cnt])));
      cnt += sizeof(short);
      playDur = (short)ntohs((unsigned short)(*(short*)(&command[cnt])));
      
      if (playFrq>0 && playFrq<RATE/2) 
      {
	if(state != PLAYING) 
        {
	  // clear data buffer while playing sound 
	  for(i=0; i < nHighestPeaks*sizeof(short)*2; i++) 
          {
	    data[i]=0;
	  }	
	  PutData(data, sizeof(data),NULL);
	  
	  openDSPforWrite();
	  pthread_testcancel();
	  state = PLAYING;
	}
	
	playDuration = (int)((playDur)*(RATE/10)*sizeof(char));
	current = 0;
      }
      else 
      {
	current=playDuration;
      }
    }
    
    if(current < playDuration) 
    {
      /* still playing an old sound */
      remaining = playDuration - current;
      omega=(double)playFrq*(double)2*M_PI/(double)RATE;
      if(remaining > sizeof(buf)) 
      {
	for(i=0; i < sizeof(buf); i++,current++) 
        {
	  phase+=omega;
	  if (phase>M_PI*2) phase-=M_PI*2;
	  buf[i]=(char)(127+(int)(playAmp*sin(phase)));
	}
      } 
      else 
      {
	for(i=0; i < remaining; i++,current++) 
        {
	  phase+=omega;
	  if (phase>M_PI*2) phase-=M_PI*2;
	  buf[i]=(char)(127+(int)(playAmp*sin(phase)));
	}
      }
      write(fd, buf, i);
      pthread_testcancel();
      usleep(20000);
    }
    else 
    {    
      if(state != LISTENING)
      {
        openDSPforRead();
	state = LISTENING;
      }
      
      listenForTones();
      cnt=0;
      for (i=0; i<nHighestPeaks; i++) 
      {
	*(short*)(&data[cnt]) = htons((unsigned short)((peakFrq[i]*RATE)/N));
	cnt += sizeof(short);
	*(short*)(&data[cnt]) = htons((unsigned short)peakAmp[i]);
	cnt += sizeof(short);
      }
      
      PutData(data, sizeof(data),NULL);
      usleep(100000);
    }
  }
}

void FixedTones::listenForTones() 
{
  int i,j,k;
  float peak;
  int tmpval;

  if(read(fd, sample, N*CHANNELS*SIZE/8) < N*CHANNELS*SIZE/8) 
  {
    PLAYER_WARN("not enough data read");
  } 
  
  for(i=0; i<N; i++)
  {
    tmpval=0;
    for(j=0; j<SIZE/8; j++)
      tmpval |= sample[(i*SIZE*CHANNELS/8)+j] << (j*8);
    in[i] = (fftw_real)tmpval;
  }
  
  rfftw_one(p, in, out);
  for (k = 1; k < (N+1)/2; ++k)  /* (k < N/2 rounded up) */
    frequency[k] = (int)((out[k]*out[k] + out[N-k]*out[N-k])/1000);
  if (N % 2 == 0) /* N is even */
    frequency[N/2] = (int)((out[N/2]*out[N/2])/1000) ;  /* Nyquist freq. */
  
  amplitude[0]=frequency[0]+frequency[1]/2;
  for (k = 1; k < (N-1)/2; ++k)  /* (k < N/2 rounded up) */ 
    amplitude[k] = (frequency[k-1]+frequency[k+1])/2+frequency[k];
  amplitude[(N-1)/2]=frequency[(N-3)/2]/2+frequency[(N-1)/2];
  
  for (i=0; i<nHighestPeaks; i++) 
  {
    peakFrq[i]=0;
    peakAmp[i]=0;
  }

  peak=0;
  for (i=MIN_FREQUENCY*N/RATE; i<(N-1)/2; i++) 
  {
    if(amplitude[i]>>6 > peakAmp[nHighestPeaks-1]) 
    {
      if((amplitude[i] >= amplitude[i-1]) && (amplitude[i]>amplitude[i+1])) 
      {
	insertPeak(i,amplitude[i] >> 7);
      }
    }
  }
}

int FixedTones::playSound(int duration) 
{
  int status;   /* return status of system calls */

  status = write(fd, buf, duration); /* play it back */
  if (status != duration)
    perror("Audiodevice: wrote wrong number of bytes played");
  /* wait for playback to complete before recording again */
  status = ioctl(fd, SOUND_PCM_SYNC, 0); 
  if (status == -1)
    perror("SOUND_PCM_SYNC ioctl failed");
  return status;
}


void FixedTones::insertPeak(int f,int a) 
{
  int i=nHighestPeaks-1;
  int j;
  while (peakAmp[i-1]<a && i>0) {
    i--;
  }
  for (j=nHighestPeaks-1; j>i; j--) {
    peakAmp[j]=peakAmp[j-1];
    peakFrq[j]=peakFrq[j-1];
  }
  peakAmp[i]=a;
  peakFrq[i]=f;
}

int 
FixedTones::Shutdown()
{
  StopThread();
  if (fd>0) close(fd);
  //printf("Audio-device has been shutdown\n");
  return 0;
}

void
FixedTones::PrintPacket(char* str, unsigned char* cmd, int len)
{
  printf("%s: ", str);
  for(int i=0; i<len; i++)
    printf(" %.2x", cmd[i]);
  puts("");
}

