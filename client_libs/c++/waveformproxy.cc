/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
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
 * $Id$
 *
 * reads raw sample data from a waveform interface. Supports writing
 * the wave to the DSP device - works for playing sounds captured with
 * waveaudio driver.
 *
 * TODO:
 * - configurable output device (dump to file, etc).  
 */

#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include <playerclient.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>

#if HAVE_SYS_SOUNDCARD_H
  #include <sys/soundcard.h>
#endif

WaveformProxy::~WaveformProxy()   
{ 
  if(this->fd > 0 ) close(this->fd);
}

void WaveformProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  if(hdr.size != sizeof(player_waveform_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of waveform data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_waveform_data_t),hdr.size);
  }

  player_waveform_data_t* data = (player_waveform_data_t*)buffer;

  printf( "rate: %d depth: %d samples: %d\n", 
	  (unsigned int)ntohl(data->rate),
	  (unsigned short)ntohs(data->depth),
	  (unsigned int)ntohl(data->samples) );
  

  this->bitrate = 
    (unsigned int)ntohl(data->rate);
  this->depth = 
    (unsigned short)ntohs(data->depth); 
  this->last_samples = 
    (unsigned int)ntohl(data->samples);
  
  memcpy( (void*)this->buffer, data->data, this->last_samples );
}

// interface that all proxies SHOULD provide
void WaveformProxy::Print()
{
  printf("#Waveform(%d:%d) - %c\n", m_device_id.code, 
         m_device_id.index, access);

  printf("Bitrate: %d bps Depth: %d bits Last samples: %d\n", 
	 this->bitrate, this->depth, this->last_samples );

  /*
    for( int s=0; s< this->last_samples; s++ )
    printf( "%d ", this->buffer[s] );
    puts("");
  */
  
}

// Play the waveform through the DSP
void WaveformProxy::Play()
{
  write(fd, this->buffer, this->last_samples );
}


void 
WaveformProxy::OpenDSPforWrite() 
{
  if (fd>0) close(fd);
  
  fd = open("/dev/dsp", O_WRONLY );
  if (fd < 0) {
    perror("open of /dev/dsp failed");
    exit(1);
  } 
}

int WaveformProxy::ConfigureDSP() 
{
  int arg;      /* argument for ioctl calls */
  int status;   /* return status of system calls */
  int r=0;
  
#if HAVE_SYS_SOUNDCARD_H
  OpenDSPforWrite();

  /* set sampling parameters */
  arg = this->depth;      /* sample size */
  status = ioctl(fd, SOUND_PCM_WRITE_BITS, &arg);
  if (status == -1) {
    perror("SOUND_PCM_WRITE_BITS ioctl failed"); 
    r=-1;
  }
  if (arg != this->depth) {
    printf("SOUND_PCM_WRITE_BITS: asked for %d, got %d\n", this->depth, arg);
    //perror("unable to set sample size");
    //r=-1;
  }
  arg = 1;  /* mono */
  status = ioctl(fd, SOUND_PCM_WRITE_CHANNELS, &arg);
  if (status == -1) {
    perror("SOUND_PCM_WRITE_CHANNELS ioctl failed");
    r=-1;
  }
  if (arg != 1) {
    perror("unable to set number of channels");
    r=-1;
  }
  arg = this->bitrate;      /* sampling rate */
  status = ioctl(fd, SOUND_PCM_WRITE_RATE, &arg);
  if (status == -1) {
    perror("SOUND_PCM_WRITE_WRITE ioctl failed");return 0;
    r=-1;
  }

  //close(fd);
#else
  fprintf(stderr, "no soundcard support compiled in\n");
  r=1;
#endif
 
  return r;
}

