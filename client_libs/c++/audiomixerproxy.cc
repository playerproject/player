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

int AudioMixerProxy::GetConfigure()
{
  if(!client)
    return(-1);

  player_audiomixer_config_t config;
  player_msghdr_t hdr;

  if(client->Request(m_device_id, (const char*)&config, sizeof(config.subtype),
        &hdr, (char*)&config,sizeof(config))<0)
    return(-1);

  masterLeft = ntohs(config.masterLeft);
  masterRight = ntohs(config.masterRight);

  pcmLeft = ntohs(config.pcmLeft);
  pcmRight = ntohs(config.pcmRight);

  lineLeft = ntohs(config.lineLeft);
  lineRight = ntohs(config.lineRight);

  micLeft = ntohs(config.micLeft);
  micRight = ntohs(config.micRight);

  iGain = ntohs(config.iGain);
  oGain = ntohs(config.oGain);

  return(0);
}

void AudioMixerProxy::FillData(player_msghdr_t hdr, const char *buffer)
{
  if(hdr.size != sizeof(player_audiodsp_data_t)) 
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: AudioProxy expected %d bytes of "
              "audiodsp data, but received %d. Unexpected results may "
              "ensue.\n",
              sizeof(player_audiodsp_data_t),hdr.size);
  }

  // We don't do anything here.
}

int AudioMixerProxy::SetMaster(unsigned short left, unsigned short right)
{
  player_audiomixer_cmd_t cmd;

  cmd.subtype = PLAYER_AUDIOMIXER_SET_MASTER;
  cmd.left = htons(left);
  cmd.right = htons(right);

  return( client->Write(m_device_id, (const char*)&cmd, sizeof(cmd)) );
}

int AudioMixerProxy::SetPCM(unsigned short left, unsigned short right)
{
  player_audiomixer_cmd_t cmd;

  cmd.subtype = PLAYER_AUDIOMIXER_SET_PCM;
  cmd.left = htons(left);
  cmd.right = htons(right);

  return( client->Write(m_device_id, (const char*)&cmd, sizeof(cmd)) );
}

int AudioMixerProxy::SetLine(unsigned short left, unsigned short right)
{
  player_audiomixer_cmd_t cmd;

  cmd.subtype = PLAYER_AUDIOMIXER_SET_LINE;
  cmd.left = htons(left);
  cmd.right = htons(right);

  return( client->Write(m_device_id, (const char*)&cmd, sizeof(cmd)) );
}

int AudioMixerProxy::SetMic(unsigned short left, unsigned short right)
{
  player_audiomixer_cmd_t cmd;

  cmd.subtype = PLAYER_AUDIOMIXER_SET_MIC;
  cmd.left = htons(left);
  cmd.right = htons(right);

  return( client->Write(m_device_id, (const char*)&cmd, sizeof(cmd)) );
}

int AudioMixerProxy::SetIGain(unsigned short gain)
{
  player_audiomixer_cmd_t cmd;

  cmd.subtype = PLAYER_AUDIOMIXER_SET_IGAIN;
  cmd.left = htons(gain);
  cmd.right = htons(gain);

  return( client->Write(m_device_id, (const char*)&cmd, sizeof(cmd)) );
}

int AudioMixerProxy::SetOGain(unsigned short gain)
{
  player_audiomixer_cmd_t cmd;

  cmd.subtype = PLAYER_AUDIOMIXER_SET_OGAIN;
  cmd.left = htons(gain);
  cmd.right = htons(gain);

  return( client->Write(m_device_id, (const char*)&cmd, sizeof(cmd)) );
}

// interface that all proxies SHOULD provide
void AudioMixerProxy::Print()
{
  printf("#Mixer(%d:%d) - %c\n", m_device_id.code, m_device_id.index, 
      access);

  printf("Master\t PCM\t Line\tMic\tIGain\tOGain\n");
  printf("%d,%d\t%d,%d\t%d,%d\t%d,%d\t %d\t %d\n\n",masterLeft,masterRight,
      pcmLeft,pcmRight,lineLeft,lineRight,micLeft,micRight,iGain,oGain);
}

