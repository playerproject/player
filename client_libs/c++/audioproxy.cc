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

void
AudioProxy::FillData(player_msghdr_t hdr, const char *buffer)
{
  if(hdr.size != sizeof(player_audio_data_t)) 
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: AudioProxy expected %d bytes of "
              "audio data, but received %d. Unexpected results may "
              "ensue.\n",
              sizeof(player_audio_data_t),hdr.size);
  }

  frequency0 = ntohs(((player_audio_data_t*)buffer)->frequency0);
  amplitude0 = ntohs(((player_audio_data_t*)buffer)->amplitude0);
  frequency1 = ntohs(((player_audio_data_t*)buffer)->frequency1);
  amplitude1 = ntohs(((player_audio_data_t*)buffer)->amplitude1);
  frequency2 = ntohs(((player_audio_data_t*)buffer)->frequency2);
  amplitude2 = ntohs(((player_audio_data_t*)buffer)->amplitude2);
  frequency3 = ntohs(((player_audio_data_t*)buffer)->frequency3);
  amplitude3 = ntohs(((player_audio_data_t*)buffer)->amplitude3);
  frequency4 = ntohs(((player_audio_data_t*)buffer)->frequency4);
  amplitude4 = ntohs(((player_audio_data_t*)buffer)->amplitude4);
}

int
AudioProxy::PlayTone(unsigned short freq,
                     unsigned short amp,
                     unsigned short dur)
{
  player_audio_cmd_t cmd;

  cmd.frequency = htons(freq);
  cmd.amplitude = htons(amp);
  cmd.duration = htons(dur);

  return(client->Write(PLAYER_AUDIO_CODE,index,
                       (const char*)&cmd,sizeof(cmd)));
}

// interface that all proxies SHOULD provide
void
AudioProxy::Print()
{
  printf("#Audio(%d:%d) - %c\n", device, index, access);
  printf("(%u,%u) (%u,%u) (%u,%u) (%u,%u) (%u,%u)\n", 
         frequency0,amplitude0,
         frequency1,amplitude1,
         frequency2,amplitude2,
         frequency3,amplitude3,
         frequency4,amplitude4);
}

