/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2002
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *     Nik Melchior
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

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h>

int AudioDSPProxy::Configure( uint8_t _channels, uint16_t _sampleRate, 
    int16_t _sampleFormat)
{
  if(!client)
    return(-1);

  player_audiodsp_config_t config;

  config.subtype = PLAYER_AUDIODSP_SET_CONFIG;
  config.sampleFormat = htons(_sampleFormat);
  config.sampleRate = htons(_sampleRate);
  config.channels = _channels;

  sampleFormat = _sampleFormat;
  sampleRate = _sampleRate;
  channels = _channels;

  return( client->Request(m_device_id,(const char*)&config,sizeof(config)) );
}

int AudioDSPProxy::GetConfigure()
{
  if(!client)
    return(-1);

  player_audiodsp_config_t config;
  player_msghdr_t hdr;

  config.subtype = PLAYER_AUDIODSP_GET_CONFIG;

  if(client->Request(m_device_id, (const char*)&config, sizeof(config.subtype),
        &hdr, (char*)&config,sizeof(config))<0)
    return(-1);

  sampleRate = ntohs(config.sampleRate);
  sampleFormat = ntohs(config.sampleFormat);
  channels = config.channels;

  return(0);
}

void AudioDSPProxy::FillData(player_msghdr_t hdr, const char *buffer)
{
  if(hdr.size != sizeof(player_audiodsp_data_t)) 
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: AudioProxy expected %d bytes of "
              "audiodsp data, but received %d. Unexpected results may "
              "ensue.\n",
              sizeof(player_audiodsp_data_t),hdr.size);
  }

  // Get the most significant Frequencies and their amplitudes
  for( int i=0;i<5;i++ )
  {
    freq[i]=ntohs(((player_audiodsp_data_t*)buffer)->freq[i]);
    amp[i]=ntohs(((player_audiodsp_data_t*)buffer)->amp[i]);
  }
 
}

int AudioDSPProxy::PlayTone(unsigned short freq, unsigned short amp,
    unsigned int dur)
{
  player_audiodsp_cmd_t cmd;

  cmd.subtype = PLAYER_AUDIODSP_PLAY_TONE;
  cmd.frequency = htons(freq);
  cmd.amplitude = htons(amp);
  cmd.duration = htonl(dur);

  return( client->Write(m_device_id, (const char*)&cmd, sizeof(cmd)) );
}

int AudioDSPProxy::PlayChirp(unsigned short freq, unsigned short amp, 
    unsigned int dur, const unsigned char bitString[], 
    unsigned short bitStringLen)
{
  player_audiodsp_cmd_t cmd;

  cmd.subtype = PLAYER_AUDIODSP_PLAY_CHIRP;
  cmd.frequency = htons(freq);
  cmd.amplitude = htons(amp);
  cmd.duration = htonl(dur);
  strcpy((char*)(cmd.bitString),(char*)(bitString));
  cmd.bitStringLen = htons(bitStringLen);

  return( client->Write(m_device_id, (const char*)&cmd, sizeof(cmd)) );
}

int AudioDSPProxy::Replay()
{
  player_audiodsp_cmd_t cmd;

  cmd.subtype = PLAYER_AUDIODSP_REPLAY;

  return( client->Write(m_device_id, (const char*)&cmd, sizeof(cmd)) );
}

// interface that all proxies SHOULD provide
void AudioDSPProxy::Print()
{
  printf("#Acoustics(%d:%d) - %c\n", m_device_id.code, m_device_id.index, 
      access);

  printf("\tSample Rate:%d\n",sampleRate);
  printf("\tSample Format:%d\n",sampleFormat);
  printf("\tChannels:%d\n",channels);

  // Print the most significant Frequencies and their amplitudes
  for( int i=0;i<5;i++ )
  {
    printf("(%6u,%6u) ",freq[i],amp[i]);
  }
  printf("\n");
 
}

