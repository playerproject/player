/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
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
 * a pure C client
 */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef PLAYERCCLIENT_H
#define PLAYERCCLIENT_H

#include <messages.h>

typedef struct
{
  /* the socket we will talk on */
  int sock;
  /* the banner given back by the server after connection */
  char banner[PLAYER_IDENT_STRLEN];
} player_connection_t;

/*
 * connects to server listening at host:port.  conn is filled in with
 * relevant information, and is used in subsequent player function
 * calls
 *
 * Returns:
 *    0 if everything is OK (connection opened)
 *   -1 if something went wrong (connection NOT opened)
 */
int player_connect(player_connection_t* conn, const char* host, int port);

/*
 * close a connection. conn should be a value that was previously returned
 * by a call to player_connect()
 *
 * Returns:
 *    0 if everything is OK (connection closed)
 *   -1 if something went wrong (connection not closed)
 */
int player_disconnect(player_connection_t* conn);

/*
 * issue some request to the server. requestlen is the length of the 
 * request.  reply, if non-NULL, will be used to hold the reply; replylen
 * is the size of the buffer (player_request() will not overrun your buffer)
 *
 *   Returns:
 *      0 if everything went OK
 *     -1 if something went wrong (you should probably close the connection!)
 */
int player_request(player_connection_t* conn, 
                   uint16_t device, uint16_t device_index, 
                   const char* payload, size_t payloadlen, 
                   player_msghdr_t* replyhdr, char* reply, size_t replylen);

/*
 * issue a single device request (special case of player_request())
 */
int player_request_device_access(player_connection_t* conn,
                                 uint16_t device,
                                 uint16_t device_index,
                                 uint8_t access);

/*
 * read from the indicated connection.  put the data in buffer, up to
 * bufferlen.
 *
 * Returns:
 *    0 if everything went OK
 *   -1 if something went wrong (you should probably close the connection!)
 */
int player_read(player_connection_t* conn, player_msghdr_t* hdr,
                char* payload, size_t payloadlen);

/*
 * write commands to the indicated connection. writes the data contained
 * in command, up to commandlen.
 *
 * Returns:
 *    0 if everything goes OK
 *   -1 if something went wrong (you should probably close the connection!)
 */
int player_write(player_connection_t* conn, 
                 uint16_t device, uint16_t device_index,
                 char* command, size_t commandlen);

/******* helper functions *********/
/*
 * write to position
 */
int player_write_position(player_connection_t* conn, player_position_cmd_t cmd);

/*
 * write to ptz
 */
int player_write_ptz(player_connection_t* conn, player_ptz_cmd_t cmd);

/*
 * read laser data into designated buffer.
 *
 * Returns:
 *   0 if OK
 *  -1 if something wrong (like got unexpected device code)
 */
int player_read_laser(player_connection_t* conn, player_laser_data_t* data);

/*
 * read sonar data into designated buffer.
 *
 * Returns:
 *   0 if OK
 *  -1 if something wrong (like got unexpected device code)
 */
int player_read_sonar(player_connection_t* conn, player_sonar_data_t* data);

/*
 * read position data into designated buffer.
 *
 * Returns:
 *   0 if OK
 *  -1 if something wrong (like got unexpected device code)
 */
int player_read_position(player_connection_t* conn,
                player_position_data_t* data);

/*
 * read misc data into designated buffer.
 *
 * Returns:
 *   0 if OK
 *  -1 if something wrong (like got unexpected device code)
 */
int player_read_misc(player_connection_t* conn, player_misc_data_t* data);

/*
 * read ptz data into designated buffer.
 *
 * Returns:
 *   0 if OK
 *  -1 if something wrong (like got unexpected device code)
 */
int player_read_ptz(player_connection_t* conn, player_ptz_data_t* data);

/*
 * read vision data into designated buffer.
 *
 * Returns:
 *   0 if OK
 *  -1 if something wrong (like got unexpected device code)
 */
int player_read_vision(player_connection_t* conn, player_vision_data_t* data);

void player_print_vision(player_vision_data_t data);
void player_print_misc(player_misc_data_t data);
void player_print_ptz(player_ptz_data_t data);
void player_print_laser(player_laser_data_t data);
void player_print_sonar(player_sonar_data_t data);
void player_print_position(player_position_data_t data);

#endif

#ifdef __cplusplus
}
#endif

