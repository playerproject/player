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
 * the C++ client
 */

#ifndef PLAYERCLIENT_H
#define PLAYERCLIENT_H

#include <playercclient.h>
#include <messages.h>
#include <devicedatatable.h>

typedef enum
{
  DIRECTWHEELVELOCITY,
  SEPARATETRANSROT
} velocity_mode_t;

struct blob_data
{
  unsigned int area;
  unsigned char x;
  unsigned char y;
  unsigned char left;
  unsigned char right;
  unsigned char top;
  unsigned char bottom;
};

struct vision_data
{
  char NumBlobs[ACTS_NUM_CHANNELS];
  blob_data* Blobs[ACTS_NUM_CHANNELS];
};

class PlayerClient
{
   private:
     player_connection_t conn;
     
     // this table is used to keep track of which devices we have open
     DeviceDataTable* devicedatatable;

     // copy commands before writing them, to allow for transformation
     void FillCommand(void* dest, void* src, uint16_t device);
     // this method is used to possible transform incoming data
     void FillData(void* dest, void* src, player_msghdr_t hdr);
     // byte-swap incoming data as necessary
     void ByteSwapData(void* data, player_msghdr_t hdr);
     // byte-swap outgoing commands as necessary
     void ByteSwapCommands(void* cmd, uint16_t device);

   public:
     
     // the host and port to which we'll connect (by default)
     char host[MAX_FILENAME_SIZE];
     int port;

     // data structures to hold commands going TO robot
     //  they actually point into the command buffer areas of the appropriate
     //  devices in the devicedatatable
     short* newspeed;
     short* newturnrate;
     short* newpan;
     short* newtilt;
     unsigned short* newzoom;
     
     // data structures to hold sensor readings FROM robot
     unsigned short *sonar;
     unsigned short *laser;
     player_position_data_t* position;
     player_ptz_data_t* ptz;
     player_misc_data_t* misc;
     vision_data* vision;
     player_laserbeacon_data_t* laserbeacon_data;
     player_broadcast_data_t* broadcast_data;
     player_broadcast_cmd_t* broadcast_cmd;

     // processed data
     unsigned short minfrontsonar;
     unsigned short minbacksonar;
     unsigned short minlaser;
     int minlaser_index;

     // simple descructor and constructor
     ~PlayerClient();
     PlayerClient();

     /* Connect to Player server */
     int Connect(const char* host, int port);
     /* Connect using 'this.port' */
     int Connect(const char* host);
     /* Connect using 'this.host' */
     int Connect();

     /* Disconnect from Player server */
     int Disconnect();

     /* request access to a single device */
     int RequestDeviceAccess(uint16_t device, uint16_t index, uint8_t access);
     /* request access to a single (zeroth) device */
     int RequestDeviceAccess(uint16_t device,uint8_t access)
        { return(RequestDeviceAccess(device,0,access)); }

     /* query the current device access */
     uint8_t QueryDeviceAccess(uint16_t device, uint16_t index);
     
     /* write ALL commands to server */
     int Write();

     /* write one device's commands to server */
     int Write(uint16_t device, uint16_t index);

     /* write one device's (the zeroth) commands to server */
     int Write(uint16_t device)
         { return(Write(device,0)); }

     /* read one set of data from server */
     int Read();

     /* print current data (for debug) */
     void Print();

     /* change velocity control mode.
      *   'mode' should be either DIRECTWHEELVELOCITY or SEPARATETRANSROT
      *
      *   default is DIRECTWHEELVELOCITY, which should be faster, if a bit
      *      jerky
      */
     int ChangeVelocityControl(velocity_mode_t mode);

     /*
      * Set the laser configuration
      * Use min_ and max_segment to specify a restricted scan.
      * Set intensity to true to get intensity data in the top three bits
      * of the range scan data.
      *
      * Returns 0 on success; non-zero otherwise
      */
     int SetLaserConfig(int min_segment, int max_segment, bool intensity);

     /*
      * Enable/disable the motors
      *   if 'state' is non-zero, then enable motors
      *   else disable motors
      *
      * Returns 0 on success; non-zero otherwise
      */
     int ChangeMotorState(unsigned char state);
};

#endif
