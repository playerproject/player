/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) Andrew Howard 2003
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <stdlib.h>
#include <string.h>

#include "playerc.h"
#include "error.h"

// Local declarations
void playerc_speech_recognition_putmsg(playerc_speechrecognition_t *device, player_msghdr_t *header,player_speech_recognition_data_t *data, size_t len);

// Create a new speech_recognition proxy
playerc_speechrecognition_t *playerc_speechrecognition_create(playerc_client_t *client, int index)
{
  playerc_speechrecognition_t *device;

  device = malloc(sizeof(playerc_speechrecognition_t));
  memset(device, 0, sizeof(playerc_speechrecognition_t));
  playerc_device_init(&device->info, client, PLAYER_SPEECH_RECOGNITION_CODE, index,
                      (playerc_putmsg_fn_t) playerc_speech_recognition_putmsg);
  return device;
}

// Destroy a speech_recognition proxy
void playerc_speechrecognition_destroy(playerc_speechrecognition_t *device)
{
  playerc_device_term(&device->info);
  free(device->rawText);
  free(device->words);
  free(device);
  return;
}

// Subscribe to the speech_recognition device
int playerc_speechrecognition_subscribe(playerc_speechrecognition_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}

// Un-subscribe from the speech_recognition device
int playerc_speechrecognition_unsubscribe(playerc_speechrecognition_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}

void playerc_speech_recognition_putmsg(playerc_speechrecognition_t *device, player_msghdr_t *hdr, player_speech_recognition_data_t *buffer, size_t len)
{
  uint32_t ii,jj;
	
  if((hdr->type == PLAYER_MSGTYPE_DATA) && (hdr->subtype == PLAYER_SPEECH_RECOGNITION_DATA_STRING ))
  {
    player_speech_recognition_data_t *data = (player_speech_recognition_data_t*)buffer;

    device->rawText = realloc(device->rawText,data->text_count*sizeof(device->rawText[0]));
    memcpy(device->rawText, data->text, data->text_count*sizeof(device->rawText[0]));
    device->rawText[data->text_count-1] = '\0';

    device->wordCount = 1;
    fprintf(stderr,"data->text %s\n",data->text);

    for (ii = 0; ii < data->text_count; ++ii)
    {
      if (device->rawText[ii] == ' ')
        device->wordCount++;
    }
    device->words = realloc(device->words,device->wordCount*sizeof(device->words[0]));
    
    jj = 0;
    for (ii = 0; ii < data->text_count; ++ii)
    {
      if (device->rawText[ii] == ' ')
        device->words[jj++] = &device->rawText[ii+1];
    }
  }
}

