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
#ifndef PLAYERCCLIENT_H
#define PLAYERCCLIENT_H

#ifdef __cplusplus
extern "C" {
#endif


#include <player.h>
#include <netinet/in.h> /* for struct in_addr */

typedef struct
{
  /* the socket we will talk on */
  int sock;
  /* the banner given back by the server after connection */
  char banner[PLAYER_IDENT_STRLEN];
} player_connection_t;

#define PLAYER_CCLIENT_DEBUG_LEVEL_DEFAULT 5

/*
 * use this to turn off debug ouput.
 *
 * higher numbers are more output, 0 is none.
 *
 * incidentally, it returns the current level, and if you give 
 * -1 for <level>, then the current level is unchanged
 */
int player_debug_level(int level);

/*
 * connects to server listening at host:port . conn is filled in with
 * relevant information, and is used in subsequent player function
 * calls
 *
 * Returns:
 *    0 if everything is OK (connection opened)
 *   -1 if something went wrong (connection NOT opened)
 */
int player_connect(player_connection_t* conn, 
		   const char* host, 
		   const int port);

/*
 * connects to server listening at addr:port.  conn is filled in with
 * relevant information, and is used in subsequent player function
 * calls. (alternative to player_connect() using binary address)
 *
 * Returns:
 *    0 if everything is OK (connection opened)
 *   -1 if something went wrong (connection NOT opened)
 */
int player_connect_ip(player_connection_t* conn, 
		      const struct in_addr* addr, 
		      const int port);
/*
 * connects to server listening at the address specified in sockaddr.
 * conn is filled in with relevant information, and is used in
 * subsequent player function calls. player_connect() and player_connect_ip
 * use this function.
 * 
 *
 * Returns:
 *    0 if everything is OK (connection opened)
 *   -1 if something went wrong (connection NOT opened) */
int player_connect_sockaddr(player_connection_t* conn, 
			    const struct sockaddr_in* server);
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
 *
 * if grant_access is non-NULL, then the actual granted access will
 * be written there.
 *
 *   Returns:
 *      0 if everything went OK
 *     -1 if something went wrong (you should probably close the connection!)
 */
int player_request_device_access(player_connection_t* conn,
                                 uint16_t device,
                                 uint16_t device_index,
                                 uint8_t req_access,
                                 uint8_t* grant_access,
                                 char* driver_name,
                                 int driver_name_len);

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
 * read one message header from the indicated connection. 
 *
 * Returns:
 *    0 if everything went OK
 *   -1 if something went wrong (you should probably close the connection!)
 */
  int player_read_header(player_connection_t* conn, player_msghdr_t* hdr );

/*
 * read the data part of a message from the indicated connection.  put the 
 * data in buffer, up to bufferlen.
 *
 * Returns:
 *    0 if everything went OK
 *   -1 if something went wrong (you should probably close the connection!)
 */
int player_read_payload(player_connection_t* conn, char* payload, size_t payloadlen);


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
                 const char* command, size_t commandlen);
int _player_write(player_connection_t* conn, 
                 uint16_t device, uint16_t device_index,
                 const char* command, size_t commandlen,int reserved);


/********************************************************/
/* helper functions */

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

/* consumes the synch packet */
int player_read_synch(player_connection_t* conn);

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
int player_read_vision(player_connection_t* conn, player_blobfinder_data_t* data);

/*
 * to help with debug output
 */
void player_print_vision(player_blobfinder_data_t data);
void player_print_ptz(player_ptz_data_t data);
void player_print_laser(player_laser_data_t data);
void player_print_sonar(player_sonar_data_t data);
void player_print_position(player_position_data_t data);

int player_set_datamode(player_connection_t* conn, char mode);
int player_change_motor_state(player_connection_t* conn, char mode);

#ifdef __cplusplus
}
#endif

#endif

