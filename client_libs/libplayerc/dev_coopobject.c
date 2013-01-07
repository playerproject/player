/*
 *  libplayerc : a Player client library
 *  Copyright (C) Andrew Howard and contributors 2002-2007
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.
 *
 */
/***************************************************************************
 * Desc: Cooperating Object proxy
 * Author: Adrian Jimenez Gonzalez
 * Date: 15 Aug 2011
 **************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "playerc.h"
#include "error.h"

#if defined (WIN32)
  #define snprintf _snprintf
#endif

// Process incoming data
void playerc_coopobject_putmsg (	playerc_coopobject_t *device,
                         	player_msghdr_t *header,
							void *data	);

// Create a new wsn proxy
playerc_coopobject_t *playerc_coopobject_create(playerc_client_t *client, int index)
{
    playerc_coopobject_t *device;

    device = malloc(sizeof(playerc_coopobject_t));
    memset(device, 0, sizeof(playerc_coopobject_t));
    playerc_device_init(&device->info, client, PLAYER_COOPOBJECT_CODE, index,
       (playerc_putmsg_fn_t) playerc_coopobject_putmsg);

    return device;
}


// Destroy a wsn proxy
void playerc_coopobject_destroy(playerc_coopobject_t *device)
{
    playerc_device_term(&device->info);
	free(device->sensor_data);
	free(device->alarm_data);
	free(device->user_data);
	free(device->parameters);
    free(device);
}


// Subscribe to the wsn device
int playerc_coopobject_subscribe(playerc_coopobject_t *device, int access)
{
    return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the wsn device
int playerc_coopobject_unsubscribe(playerc_coopobject_t *device)
{
    return playerc_device_unsubscribe(&device->info);
}

// Process incoming data
void playerc_coopobject_putmsg (playerc_coopobject_t *device,
			 player_msghdr_t *header,
			 void *data)
{
	device->messageType = PLAYER_COOPOBJECT_MSG_NONE;

    if((header->type == PLAYER_MSGTYPE_DATA) &&
       (header->subtype == PLAYER_COOPOBJECT_DATA_HEALTH)) {
			
		player_coopobject_header_t* wsn_data = (player_coopobject_header_t *)data;
	
		device->origin 				= wsn_data->origin;
		device->id 				= wsn_data->id;
		device->parent_id 			= wsn_data->parent_id;

		device->RSSIsender 			= 0;
		device->RSSIvalue 			= 0;
		device->RSSIstamp 			= 0;
		device->RSSInodeTimeHigh 		= 0;
		device->RSSInodeTimeLow 		= 0;
		device->x 				= 0;
		device->y 				= 0;
		device->z 				= 0;
		device->status				= 0;
		
		device->sensor_data_count 		= 0;
		device->sensor_data 			= NULL;
		device->alarm_data_count 		= 0;
		device->alarm_data 			= NULL;
		device->user_data_count 		= 0;
		device->user_data 			= NULL;

		device->command 			= 0;
		device->request 			= 0;
		device->parameters_count 		= 0;
		device->parameters 			= NULL;
		device->messageType 			= PLAYER_COOPOBJECT_MSG_HEALTH;

    } else if((header->type == PLAYER_MSGTYPE_DATA) &&
       (header->subtype == PLAYER_COOPOBJECT_DATA_RSSI)) {

		player_coopobject_rssi_t* wsn_data = (player_coopobject_rssi_t *)data;

		device->origin 		= wsn_data->header.origin;
		device->id 		= wsn_data->header.id;
		device->parent_id 		= 0xFFFF;

		device->RSSIsender 		= wsn_data->sender_id;
		device->RSSIvalue 		= wsn_data->rssi;
		device->RSSIstamp 		= wsn_data->stamp;
		device->RSSInodeTimeHigh 	= wsn_data->nodeTimeHigh;
		device->RSSInodeTimeLow 	= wsn_data->nodeTimeLow;

		device->x 			= 0;
		device->y 			= 0;
		device->z 			= 0;
		device->status			= 0;
		
		device->sensor_data_count 	= 0;
		device->sensor_data 		= NULL;
		device->alarm_data_count 	= 0;
		device->user_data_count 	= 0;
		device->user_data = NULL;

		device->command			= 0;
		device->request 		= 0;
		device->parameters_count 	= 0;
		device->parameters 		= NULL;

		device->messageType 		= PLAYER_COOPOBJECT_MSG_RSSI;

    } else if((header->type == PLAYER_MSGTYPE_DATA) &&
       (header->subtype == PLAYER_COOPOBJECT_DATA_POSITION)) {

		player_coopobject_position_t* wsn_data = (player_coopobject_position_t *)data;

		device->origin 				= wsn_data->header.origin;
		device->id 				= wsn_data->header.id;
		device->parent_id 			= wsn_data->header.parent_id;

		device->RSSIsender 			= 0;
		device->RSSIvalue 			= 0;
		device->RSSIstamp 			= 0;
		device->RSSInodeTimeHigh 		= 0;
		device->RSSInodeTimeLow 		= 0;

		device->x 				= wsn_data->x;
		device->y 				= wsn_data->y;
		device->z 				= wsn_data->z;
		device->status				= wsn_data->status;
		
		device->sensor_data_count 		= 0;
		device->sensor_data			= NULL;
		device->alarm_data_count 		= 0;
		device->alarm_data 			= NULL;
		device->user_data_count 		= 0;
		device->user_data 			= NULL;

		device->command 			= 0;
		device->request 			= 0;
		device->parameters_count 		= 0;
		device->parameters 			= NULL;

		device->messageType 			= PLAYER_COOPOBJECT_MSG_POSITION;


    } else if((header->type == PLAYER_MSGTYPE_DATA) &&
       (header->subtype == PLAYER_COOPOBJECT_DATA_SENSOR)) {

		player_coopobject_data_sensor_t* wsn_data = (player_coopobject_data_sensor_t *)data;
	
		device->origin 				= wsn_data->header.origin;
		device->id 				= wsn_data->header.id;
		device->parent_id 			= wsn_data->header.parent_id;

		device->RSSIsender 			= 0;
		device->RSSIvalue 			= 0;
		device->RSSIstamp 			= 0;
		device->RSSInodeTimeHigh 		= 0;
		device->RSSInodeTimeLow 		= 0;
		device->x 				= 0;
		device->y 				= 0;
		device->z 				= 0;
		device->status				= 0;
		
		device->sensor_data_count 		= wsn_data->data_count;

		if (wsn_data->data_count > 0)  {
			if ((device->sensor_data =
				(player_coopobject_sensor_t*) malloc (sizeof(player_coopobject_sensor_t)*wsn_data->data_count)) == NULL)
				PLAYERC_ERR ("Failed to allocate space to store request parameters locally\n");
			else memcpy (device->sensor_data, wsn_data->data,
						wsn_data->data_count * sizeof(player_coopobject_sensor_t));
			
		} else device->sensor_data 		= NULL;

		device->alarm_data_count 		= 0;
		device->alarm_data 			= NULL;
		device->user_data_count 		= 0;
		device->user_data 			= NULL;

		device->command 			= 0;
		device->request 			= 0;
		device->parameters_count 		= 0;
		device->parameters 			= NULL;

		device->messageType 			= PLAYER_COOPOBJECT_MSG_SENSOR;

    } else if((header->type == PLAYER_MSGTYPE_DATA) &&
       (header->subtype == PLAYER_COOPOBJECT_DATA_ALARM)) {

		player_coopobject_data_sensor_t* wsn_data = (player_coopobject_data_sensor_t *)data;

		device->origin 				= wsn_data->header.origin;
		device->id 				= wsn_data->header.id;
		device->parent_id 			= wsn_data->header.parent_id;

		device->RSSIsender			= 0;
		device->RSSIvalue 			= 0;
		device->RSSIstamp 			= 0;
		device->RSSInodeTimeHigh 		= 0;
		device->RSSInodeTimeLow 		= 0;
		device->x 				= 0;
		device->y 				= 0;
		device->z 				= 0;
		device->status				= 0;
		
		device->alarm_data_count 		= wsn_data->data_count;

		if (wsn_data->data_count > 0) {
			if ((device->alarm_data = 
				(player_coopobject_sensor_t*) malloc (sizeof(player_coopobject_sensor_t)*wsn_data->data_count)) == NULL)
				PLAYERC_ERR ("Failed to allocate space to store request parameters locally\n");
			else memcpy (device->alarm_data, wsn_data->data, wsn_data->data_count * sizeof(player_coopobject_sensor_t));
		} else device->alarm_data 		= NULL;
		device->sensor_data_count 		= 0;
		device->sensor_data 			= NULL;

		device->user_data_count 		= 0;
		device->user_data 			= NULL;

		device->command 			= 0;
		device->request 			= 0;
		device->parameters_count 		= 0;
		device->parameters 			= NULL;

		device->messageType 			= PLAYER_COOPOBJECT_MSG_ALARM;

    } else if((header->type == PLAYER_MSGTYPE_DATA) &&
       (header->subtype == PLAYER_COOPOBJECT_DATA_USERDEFINED)) {

		player_coopobject_data_userdefined_t* wsn_data = (player_coopobject_data_userdefined_t *)data;
	
		device->origin 				= wsn_data->header.origin;
		device->id 				= wsn_data->header.id;
		device->parent_id 			= wsn_data->header.parent_id;

		device->RSSIsender 			= 0;
		device->RSSIvalue 			= 0;
		device->RSSIstamp 			= 0;
		device->RSSInodeTimeHigh 		= 0;
		device->RSSInodeTimeLow 		= 0;
		device->x 				= 0;
		device->y 				= 0;
		device->z 				= 0;
		device->status				= 0;
		
		device->sensor_data_count 		= 0;
		device->sensor_data 			= NULL;
		device->alarm_data_count 		= 0;
		device->alarm_data 			= NULL;

		device->user_data_count 		= wsn_data->data_count;

		if (wsn_data->data_count > 0) {
			if ((device->user_data =
				(uint8_t*) malloc (sizeof(uint8_t)*wsn_data->data_count)) == NULL)
				PLAYERC_ERR ("Failed to allocate space to store request parameters locally\n");
			else memcpy (device->user_data, wsn_data->data,
						wsn_data->data_count * sizeof(uint8_t));
		} else device->user_data 		= NULL;

		device->command 			= 0;
		device->request 			= 0;
		device->parameters_count 		= 0;
		device->parameters 			= NULL;

		device->messageType 			= wsn_data->type;

    } else if((header->type == PLAYER_MSGTYPE_DATA) &&
       (header->subtype == PLAYER_COOPOBJECT_DATA_REQUEST)) {

		player_coopobject_req_t* wsn_data = (player_coopobject_req_t *)data;
	
		device->origin 				= wsn_data->header.origin;
		device->id 				= wsn_data->header.id;
		device->parent_id 			= wsn_data->header.parent_id;
		
		device->RSSIsender 			= 0;
		device->RSSIvalue 			= 0;
		device->RSSIstamp 			= 0;
		device->RSSInodeTimeHigh 		= 0;
		device->RSSInodeTimeLow 		= 0;
		device->x 				= 0;
		device->y 				= 0;
		device->z 				= 0;
		device->status				= 0;
		
		device->sensor_data_count 		= 0;
		device->sensor_data 			= NULL;
		device->alarm_data_count 		= 0;
		device->alarm_data 			= NULL;

		device->user_data_count 		= 0;
		device->user_data 			= NULL;

		device->command 			= 0;

		device->parameters_count 		= 0;
		device->parameters 			= NULL;

		device->request				= wsn_data->request;
		device->parameters_count 		= wsn_data->parameters_count;

		if (wsn_data->parameters_count > 0) {
			if ((device->parameters =
				(uint8_t*) malloc (sizeof(uint8_t)*wsn_data->parameters_count)) == NULL)
				PLAYERC_ERR ("Failed to allocate space to store request parameters locally\n");
			else memcpy (device->parameters, wsn_data->parameters,
						wsn_data->parameters_count * sizeof(uint8_t));
		} else device->parameters 		= NULL;

		device->messageType 			= PLAYER_COOPOBJECT_MSG_REQUEST;

    } else if((header->type == PLAYER_MSGTYPE_DATA) &&
       (header->subtype == PLAYER_COOPOBJECT_DATA_COMMAND)) {

		player_coopobject_cmd_t* wsn_data = (player_coopobject_cmd_t *)data;
	
		device->origin 				= wsn_data->header.origin;
		device->id 				= wsn_data->header.id;
		device->parent_id 			= wsn_data->header.parent_id;

		device->RSSIsender 			= 0;
		device->RSSIvalue 			= 0;
		device->RSSIstamp 			= 0;
		device->RSSInodeTimeHigh 		= 0;
		device->RSSInodeTimeLow 		= 0;
		device->x 				= 0;
		device->y 				= 0;
		device->z 				= 0;
		device->status				= 0;
		
		device->sensor_data_count 		= 0;
		device->sensor_data 			= NULL;
		device->alarm_data_count 		= 0;
		device->alarm_data 			= NULL;

		device->user_data_count 		= 0;
		device->user_data 			= NULL;

		device->request = 0;

		device->parameters_count = 0;
		device->parameters = NULL;

		device->command				= wsn_data->command;
		device->parameters_count 		= wsn_data->parameters_count;

		if (wsn_data->parameters_count > 0) {
			if ((device->parameters =
				(uint8_t*) malloc (sizeof(uint8_t)*wsn_data->parameters_count)) == NULL)
				PLAYERC_ERR ("Failed to allocate space to store request parameters locally\n");
			else memcpy (device->parameters, wsn_data->parameters,
						wsn_data->parameters_count * sizeof(uint8_t));
		} else device->parameters = NULL;

		device->messageType = PLAYER_COOPOBJECT_MSG_COMMAND;
    } else
	PLAYERC_WARN2("skipping wsn message with unknown type/subtype: %s/%d\n",
	    msgtype_to_str(header->type), header->subtype);
}

// Send robot position
int
playerc_coopobject_send_position(playerc_coopobject_t *device, uint16_t id, uint16_t source_id, player_pose2d_t pos, uint8_t status)
{

  player_coopobject_position_t data;
  memset(&data, 0, sizeof(data));
  data.header.origin 	= 3; 
  data.header.id	= id;
  data.header.parent_id	= source_id;
  data.x		= pos.px;
  data.y		= pos.py;
  data.z		= pos.pa;
  data.status		= status;
//  printf("%d, %d, %d, %f, %f, %f, %d\n",   data.header.origin, data.header.id, data.header.parent_id, data.x, data.y, data.z, data.status);

// printf("user data message sent to the mote: nodeid: %d, robotid: %d, type:%d, data: [ ",data.id, data.robot_id, data.data_type);
// 	for(i=0;i<data.data_count;i++)
// 		printf("0x%x ",data.data[i]);
// 	printf("] \n"); 

  return playerc_client_write(device->info.client, &device->info, PLAYER_COOPOBJECT_CMD_POSITION, &data, NULL);

}

// Send data
int
playerc_coopobject_send_data(playerc_coopobject_t *device, int id, int source_id, int data_type, int data_size, unsigned char *extradata)
{

  player_coopobject_data_userdefined_t data;
  memset(&data, 0, sizeof(data));

  data.header.origin 	= 3; 
  data.header.id	= id;
  data.header.parent_id	= source_id;

  data.type		= data_type;
  data.data_count	= data_size;
  data.data		= extradata;

//  printf("%d, %d, %d, %d, %d\n",   data.header.origin, data.header.id, data.header.parent_id, data.type, data.data_count);

  return playerc_client_write(device->info.client, &device->info, PLAYER_COOPOBJECT_CMD_DATA, &data, NULL);

}

int
playerc_coopobject_send_cmd(playerc_coopobject_t *device, int id, int source_id, int cmd, int parameters_size, unsigned char *parameters)
{
  player_coopobject_cmd_t command;

  memset(&command, 0, sizeof(command));

  command.header.origin 	= 3; 
  command.header.id	= id;
  command.header.parent_id	= source_id;

  command.command		= cmd;
  command.parameters_count	= parameters_size;
  command.parameters		= parameters;

  return playerc_client_write(device->info.client, &device->info,
                              PLAYER_COOPOBJECT_CMD_STANDARD,
                              &command, NULL);
}

/** Send request to wsn. */
int
playerc_coopobject_send_req(playerc_coopobject_t *device, int id, int source_id, int req, int parameters_size, unsigned char *parameters)
{
  player_coopobject_req_t request;

  memset(&request, 0, sizeof(request));
  
  request.header.origin 	= 3; 
  request.header.id		= id;
  request.header.parent_id	= source_id;

  request.request		= req;
  request.parameters_count	= parameters_size;
  request.parameters		= parameters;

  return playerc_client_request(device->info.client, &device->info, PLAYER_COOPOBJECT_REQ_STANDARD, &request, NULL);
}

