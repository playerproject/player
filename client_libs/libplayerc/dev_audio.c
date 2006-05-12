/*
 *  libplayerc : a Player client library
 *  Copyright (C) Andrew Howard 2002-2003
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>
#include <stdlib.h>

#include "playerc.h"
#include "error.h"

// Local declarations
void playerc_audio_putmsg(playerc_audio_t *device,
                             player_msghdr_t *header,
                             uint8_t *data, size_t len);

// Create an audio proxy
playerc_audio_t *playerc_audio_create(playerc_client_t *client, int index)
{
  playerc_audio_t *device;

  device = malloc(sizeof(playerc_audio_t));
  memset(device, 0, sizeof(playerc_audio_t));
  playerc_device_init(&device->info, client, PLAYER_AUDIO_CODE, index,
                       (playerc_putmsg_fn_t) playerc_audio_putmsg);

  return device;
}

// Destroy an audio proxy
void playerc_audio_destroy(playerc_audio_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}

// Subscribe to the audio device
int playerc_audio_subscribe(playerc_audio_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}

// Un-subscribe from the audio device
int playerc_audio_unsubscribe(playerc_audio_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}

void playerc_audio_putmsg(playerc_audio_t *device,
                             player_msghdr_t *header,
                             uint8_t *data, size_t len)
{
  if((header->type == PLAYER_MSGTYPE_DATA) && (header->subtype == PLAYER_AUDIO_WAV_REC_DATA))
  {
    assert(header->size > 0);
    player_audio_wav_t * wdata = (player_audio_wav_t *) data;
    device->wav_data.data_count = wdata->data_count;
    memcpy(device->wav_data.data, wdata->data, wdata->data_count * sizeof(device->wav_data.data[0]));
    device->wav_data.format = wdata->format;
  }
  else if((header->type == PLAYER_MSGTYPE_DATA) && (header->subtype == PLAYER_AUDIO_SEQ_DATA))
  {
    assert(header->size > 0);
    player_audio_seq_t * sdata = (player_audio_seq_t *) data;
    device->seq_data.tones_count = sdata->tones_count;
    memcpy(device->seq_data.tones, sdata->tones, sdata->tones_count * sizeof(device->seq_data.tones[0]));
  }
  else if((header->type == PLAYER_MSGTYPE_DATA) && (header->subtype == PLAYER_AUDIO_MIXER_CHANNEL_DATA))
  {
    assert(header->size > 0);
    player_audio_mixer_channel_list_t * wdata = (player_audio_mixer_channel_list_t *) data;
    device->mixer_data.channels_count = wdata->channels_count;
    memcpy(device->mixer_data.channels, wdata->channels, wdata->channels_count * sizeof(device->mixer_data.channels[0]));
  }
  else
    PLAYERC_WARN2("skipping audio message with unknown type/subtype: %d/%d\n",
                  header->type, header->subtype);
}

/** @brief Command to play an audio block */
int playerc_audio_wav_play_cmd(playerc_audio_t *device, player_audio_wav_t * data)
{
  return playerc_client_write(device->info.client, &device->info,
                              PLAYER_AUDIO_WAV_PLAY_CMD,
                              data, NULL);
}

/** @brief Command to set recording state */
int playerc_audio_wav_stream_rec_cmd(playerc_audio_t *device, uint8_t state)
{
  player_bool_t cmd;
  cmd.state = state;
  return playerc_client_write(device->info.client, &device->info,
                              PLAYER_AUDIO_WAV_STREAM_REC_CMD,
                              &cmd, NULL);
}


/** @brief Command to play prestored sample */
int playerc_audio_sample_play_cmd(playerc_audio_t *device, int index)
{
  player_audio_sample_item_t cmd;
  cmd.index = index;
  return playerc_client_write(device->info.client, &device->info,
                              PLAYER_AUDIO_SAMPLE_PLAY_CMD,
                              &cmd, NULL);
}

/** @brief Command to play sequence of tones */
int playerc_audio_seq_play_cmd(playerc_audio_t *device, player_audio_seq_t * tones)
{
  return playerc_client_write(device->info.client, &device->info,
                              PLAYER_AUDIO_SEQ_PLAY_CMD,
                              tones, NULL);
}

/** @brief Command to set mixer levels */
int playerc_audio_mixer_channel_cmd(playerc_audio_t *device, player_audio_mixer_channel_list_t * levels)
{
  return playerc_client_write(device->info.client, &device->info,
                              PLAYER_AUDIO_MIXER_CHANNEL_CMD,
                              levels, NULL);
}


/** @brief Request to record a single audio block
Value is returned into wav_data, block length is determined by device */
int playerc_audio_wav_rec(playerc_audio_t *device)
{
  int result = 0;

  if((result = playerc_client_request(device->info.client, &device->info,
                            PLAYER_AUDIO_WAV_REC_REQ,
                            NULL, (void*)&device->wav_data, sizeof(device->wav_data))) < 0)
    return result;

  return 0;
}


/** @brief Request to load an audio sample */
int playerc_audio_sample_load(playerc_audio_t *device, int index, player_audio_wav_t * data)
{
  int result = 0;
  player_audio_sample_t req;
  req.sample.data_count = data->data_count;
  memcpy(req.sample.data, data->data, req.sample.data_count * sizeof(req.sample.data[0]));
  req.sample.format = data->format;
  req.index = index;
  if((result = playerc_client_request(device->info.client, &device->info,
                            PLAYER_AUDIO_SAMPLE_LOAD_REQ,
                            &req, NULL, 0)) < 0)
    return result;

  return 0;
}

/** @brief Request to retrieve an audio sample 
Data is stored in wav_data */
int playerc_audio_sample_retrieve(playerc_audio_t *device, int index)
{
  int result = 0;
  player_audio_sample_t req;
  req.sample.data_count = 0;
  req.index = index;
  if((result = playerc_client_request(device->info.client, &device->info,
                            PLAYER_AUDIO_SAMPLE_RETRIEVE_REQ,
                            &req, &req, sizeof(req))) < 0)
    return result;

  device->wav_data.data_count = req.sample.data_count;
  memcpy(device->wav_data.data, req.sample.data, req.sample.data_count * sizeof(device->wav_data.data[0]));
  device->wav_data.format = req.sample.format;

  return 0;
}

/** @brief Request to record new sample */
int playerc_audio_sample_rec(playerc_audio_t *device, int index)
{
  int result = 0;
  player_audio_sample_item_t req;
  req.index = index;
  if((result = playerc_client_request(device->info.client, &device->info,
                            PLAYER_AUDIO_SAMPLE_REC_REQ,
                            &req, NULL, 0)) < 0)
    return result;

  return 0;
}

/** @brief Request mixer channel data 
result is stored in mixer_data*/
int playerc_audio_get_mixer_levels(playerc_audio_t *device)
{
  int result = 0;
  player_audio_mixer_channel_list_t req;
  if((result = playerc_client_request(device->info.client, &device->info,
                            PLAYER_AUDIO_MIXER_CHANNEL_LIST_REQ ,
                            NULL, &req, sizeof(req))) < 0)
    return result;


  device->mixer_data.channels_count = req.channels_count;
  memcpy(device->mixer_data.channels, req.channels, req.channels_count * sizeof(device->mixer_data.channels[0]));

  return 0;
}

/** @brief Request mixer channel details list 
result is stored in channel_details_list*/
int playerc_audio_get_mixer_details(playerc_audio_t *device)
{
  int result;
  player_audio_mixer_channel_list_detail_t req;
  if((result = playerc_client_request(device->info.client, &device->info,
                            PLAYER_AUDIO_MIXER_CHANNEL_LIST_REQ ,
                            NULL, &req, sizeof(req))) < 0)
    return result;


  device->channel_details_list.details_count = req.details_count;
  memcpy(device->channel_details_list.details, req.details, req.details_count * sizeof(device->channel_details_list.details[0]));
  device->channel_details_list.default_output = req.default_output;
  device->channel_details_list.default_input = req.default_input;

  return 0;
}


