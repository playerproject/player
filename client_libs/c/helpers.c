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
 * helper functions to make the C client a bit easier to use.
 */
#include <playercclient.h>
#include <netdb.h>
#include <netinet/in.h>  /* for struct sockaddr_in, htons(3) */

/*
 * read laser data into designated buffer.
 *
 * Returns:
 *   0 if OK
 *  -1 if something wrong (like got unexpected device code)
 */
int player_read_laser(player_connection_t* conn, player_laser_data_t* data)
{
  player_msghdr_t hdr;
  int i;

  if(player_read(conn, &hdr, (char*)data, sizeof(player_laser_data_t)) == -1)
    return(-1);

  if(hdr.device != PLAYER_LASER_CODE)
  {
    fprintf(stderr, "player_read_laser(): received wrong device code\n");
    return(-1);
  }

  for(i=0;i<PLAYER_NUM_LASER_SAMPLES;i++)
    data->ranges[i] = ntohs(data->ranges[i]);

  return(0);
}

/*
 * read sonar data into designated buffer.
 *
 * Returns:
 *   0 if OK
 *  -1 if something wrong (like got unexpected device code)
 */
int player_read_sonar(player_connection_t* conn, player_sonar_data_t* data)
{
  player_msghdr_t hdr;
  int i;

  if(player_read(conn, &hdr, (char*)data, sizeof(player_sonar_data_t)) == -1)
    return(-1);

  if(hdr.device != PLAYER_SONAR_CODE)
  {
    fprintf(stderr, "player_read_sonar(): received wrong device code\n");
    return(-1);
  }

  for(i=0;i<PLAYER_NUM_SONAR_SAMPLES;i++)
    data->ranges[i] = ntohs(data->ranges[i]);

  return(0);
}

/*
 * read position data into designated buffer.
 *
 * Returns:
 *   0 if OK
 *  -1 if something wrong (like got unexpected device code)
 */
int player_read_position(player_connection_t* conn,player_position_data_t* data)
{
  player_msghdr_t hdr;

  if(player_read(conn, &hdr, (char*)data, sizeof(player_position_data_t)) == -1)
    return(-1);

  if(hdr.device != PLAYER_POSITION_CODE)
  {
    fprintf(stderr, "player_read_position(): received wrong device code\n");
    return(-1);
  }

  data->xpos = ntohl(data->xpos);
  data->ypos = ntohl(data->ypos);
  data->theta = ntohs(data->theta);
  data->speed = ntohs(data->speed);
  data->turnrate = ntohs(data->turnrate);
  data->compass = ntohs(data->compass);

  return(0);
}

/*
 * read misc data into designated buffer.
 *
 * Returns:
 *   0 if OK
 *  -1 if something wrong (like got unexpected device code)
 */
int player_read_misc(player_connection_t* conn, player_misc_data_t* data)
{
  player_msghdr_t hdr;

  if(player_read(conn, &hdr, (char*)data, sizeof(player_misc_data_t)) == -1)
    return(-1);

  if(hdr.device != PLAYER_MISC_CODE)
  {
    fprintf(stderr, "player_read_misc(): received wrong device code\n");
    return(-1);
  }

  /* no byte-swapping here... */

  return(0);
}

/*
 * read ptz data into designated buffer.
 *
 * Returns:
 *   0 if OK
 *  -1 if something wrong (like got unexpected device code)
 */
int player_read_ptz(player_connection_t* conn, player_ptz_data_t* data)
{
  player_msghdr_t hdr;

  if(player_read(conn, &hdr, (char*)data, sizeof(player_ptz_data_t)) == -1)
    return(-1);

  if(hdr.device != PLAYER_PTZ_CODE)
  {
    fprintf(stderr, "player_read_ptz(): received wrong device code\n");
    return(-1);
  }

  data->pan = ntohs(data->pan);
  data->tilt = ntohs(data->tilt);
  data->zoom = ntohs(data->zoom);

  return(0);
}

/*
 * read vision data into designated buffer.
 *
 * Returns:
 *   0 if OK
 *  -1 if something wrong (like got unexpected device code)
 */
int player_read_vision(player_connection_t* conn, player_vision_data_t* data)
{
  player_msghdr_t hdr;

  if(player_read(conn, &hdr, (char*)data, sizeof(player_vision_data_t)) == -1)
    return(-1);

  if(hdr.device != PLAYER_VISION_CODE)
  {
    fprintf(stderr, "player_read_vision(): received wrong device code\n");
    return(-1);
  }

  return(0);
}

/*
 * write to 0th position device
 */
int player_write_position(player_connection_t* conn, player_position_cmd_t cmd)
{
  player_position_cmd_t swapped_cmd;
  swapped_cmd.speed = htons(cmd.speed);
  swapped_cmd.turnrate = htons(cmd.turnrate);
  return(player_write(conn, PLAYER_POSITION_CODE, 0, 
                          (char*)&swapped_cmd, sizeof(player_position_cmd_t)));
}

/*
 * write to 0th ptz device
 */
int player_write_ptz(player_connection_t* conn, player_ptz_cmd_t cmd)
{
  player_ptz_cmd_t swapped_cmd;
  swapped_cmd.pan = htons(cmd.pan);
  swapped_cmd.tilt = htons(cmd.tilt);
  swapped_cmd.zoom = htons(cmd.zoom);
  return(player_write(conn, PLAYER_PTZ_CODE, 0, 
                          (char*)&swapped_cmd, sizeof(player_ptz_cmd_t)));
}

