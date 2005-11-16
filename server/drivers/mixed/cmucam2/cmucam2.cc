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
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_cmucam2 cmucam2
 * @brief CMUCam2 pan-tilt-zoom blob-tracking camera

The cmucam2 driver connects over a serial port to a CMUCam2. Presents a
@ref interface_blobfinder interface and a @ref interface_ptz
interface and can track multiple color blobs.  This driver is rudimentary
but working. Color tracking parameters are defined in Player's config file
(see below for an example).


@par Compile-time dependencies

- none

@par Provides

- @ref interface_blobfinder : the blobs detected by the CMUCam2
- @ref interface_ptz : control of the servos that pan and tilt
  the CMUCam2

@par Requires

- none

@par Supported configuration requests

- The @ref interface_ptz interface supports:
  - PLAYER_PTZ_AUTOSERVO

@par Configuration file options

- devicepath (string)
  - Default: NULL
  - Serial port where the CMUCam2 is connected
- num_blobs (integer)
  - Default: 1
  - Number of colors to track; you must also include this many color%d options
- color%d (float tuple)
  - Default: none
  - Each color%d is a tuple [rmin rmax gmin gmax bmin bmax] of min/max
    values for red, green, and blue, which defines a region in RGB space
    that the CMUCam2 will track.
  
@par Example 

@verbatim
driver
(
  name "cmucam2"
  provides ["blobfinder:0" "ptz:0"]
  devicepath "/dev/ttyS1"
  num_blobs 2
  # values must be between 40 and 240 (!)
  color0 [  red_min red_max blue_min blue_max green_min green_max] )
  # values must be between 40 and 240 (!)
  color1 [  red_min red_max blue_min blue_max green_min green_max] )  
)
@endverbatim

@author Pouya Bastani, Richard Vaughan
*/
/** @} */


#include <assert.h>
#include <stdio.h>
#include <unistd.h> /* close(2),fcntl(2),getpid(2),usleep(3),execvp(3),fork(2)*/
#include <netdb.h> /* for gethostbyname(3) */
#include <netinet/in.h>  /* for struct sockaddr_in, htons(3) */
#include <sys/types.h>  /* for socket(2) */
#include <sys/socket.h>  /* for socket(2) */
#include <signal.h>  /* for kill(2) */
#include <fcntl.h>  /* for fcntl(2) */
#include <string.h>  /* for strncpy(3),memcpy(3) */
#include <stdlib.h>  /* for atexit(3),atoi(3) */
#include <pthread.h>  /* for pthread stuff */
#include <socket_util.h>

#include <error.h>
#include <driver.h>
#include <drivertable.h>
#include <player.h>
#include <math.h>

#include "camera.h"

#define MAX_CHANNELS 32

class Cmucam2:public Driver 
{
  private:

    // Descriptive colors for each channel.
    uint32_t colors[MAX_CHANNELS];
    void get_blob(packet_t cam_packet, player_blobfinder_blob_t *blob, color_config range);
    int fd;
    int num_of_blobs;
    const char* devicepath;
    color_config color[PLAYER_BLOBFINDER_MAX_BLOBS];
    //Camera cam;

    // Blobfinder interface (provides)
    player_device_id_t blobfinder_id;
    player_blobfinder_data_t blobfinder_data;
  
    // PTZ interface (provides)
    player_device_id_t ptz_id;
    player_ptz_data_t ptz_data;

    int pan_position;
    int tilt_position;


  public:  

    // constructor 
    //
    Cmucam2( ConfigFile* cf, int section);

    // Process incoming messages from clients 
    int ProcessMessage(ClientData * client, player_msghdr * hdr, uint8_t * data, uint8_t * resp_data, size_t * resp_len);

    virtual void Main();
  
    int Setup();
    int Shutdown();
};

// a factory creation function
Driver* Cmucam2_Init( ConfigFile* cf, int section)
{
  return((Driver*)(new Cmucam2( cf, section)));
}

// a driver registration function
void 
Cmucam2_Register(DriverTable* table)
{
  table->AddDriver("cmucam2", Cmucam2_Init);
}

Cmucam2::Cmucam2( ConfigFile* cf, int section)
  : Driver(cf, section)
{

  // Outgoing blobfinder interface
  if (cf->ReadDeviceId(&(this->blobfinder_id), section, "provides",
                       PLAYER_BLOBFINDER_CODE, -1, NULL) == 0)
  {
    if (this->AddInterface(this->blobfinder_id, PLAYER_ALL_MODE) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }
  else
    memset(&this->blobfinder_id, 0, sizeof(this->blobfinder_id));

  // Outgoing ptz interface
  memset(&this->ptz_id, 0, sizeof(this->ptz_id));
  if (cf->ReadDeviceId(&(this->ptz_id), section, "provides",
                       PLAYER_PTZ_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->ptz_id, PLAYER_ALL_MODE) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  pan_position = 0;
  tilt_position = 0;
 
  num_of_blobs = cf->ReadInt(section, "num_blobs", 1);
  if(!(this->devicepath = (char*)cf->ReadString(section, "devicepath", NULL)))
  {
    PLAYER_ERROR("must specify devicepath");
    this->SetError(-1);
    return;
  }

  char variable[20];
  
  for(int i = 0; i < num_of_blobs; i++)
  {
    sprintf(variable, "color%d", i);   
    color[i].rmin = (int)cf->ReadTupleFloat(section, variable, 0, 16);
    color[i].rmax = (int)cf->ReadTupleFloat(section, variable, 1, 16);
    color[i].gmin = (int)cf->ReadTupleFloat(section, variable, 2, 16);
    color[i].gmax = (int)cf->ReadTupleFloat(section, variable, 3, 16);
    color[i].bmin = (int)cf->ReadTupleFloat(section, variable, 4, 16);
    color[i].bmax = (int)cf->ReadTupleFloat(section, variable, 5, 16);
  }
}
    
int
Cmucam2::Setup()
{
  fflush(stdout);

  fd = open_port((char*)devicepath);        // opening the serial port
  if(fd<0)                           // if not successful, stop
  {
    printf("Camera connection failed!\n");
    return 0; 
  }
  auto_servoing(fd, 0);

  /* now spawn reading thread */
  StartThread();

  return(0);
}


int
Cmucam2::Shutdown()
{
  StopThread();
  stop_tracking(fd);
  close_port(fd);                 // close the serial port
  return(0);
}

int Cmucam2::ProcessMessage(ClientData * client, player_msghdr * hdr, uint8_t * data, uint8_t * resp_data, size_t * resp_len)
{
  assert(hdr);
  assert(data);
  assert(resp_data);
  assert(resp_len);
  
  if (MatchMessage(hdr, PLAYER_MSGTYPE_CMD, 0, ptz_id))
  {
  	assert(hdr->size == sizeof(player_ptz_cmd_t));
  	player_ptz_cmd_t & command = *reinterpret_cast<player_ptz_cmd_t * > (data);

    if(pan_position != (short)ntohs((unsigned short)(command.pan)))
    {      
      pan_position = (short)ntohs((unsigned short)(command.pan));      
      if( abs(pan_position) <= 90 )      
        set_servo_position(fd, 0, -1*pan_position);   // Pan value must be negated.      
    }   
    if(tilt_position != (short)ntohs((unsigned short)(command.tilt)))
    {
      tilt_position = (short)ntohs((unsigned short)(command.tilt));
      if( abs(tilt_position) <= 90 )
        set_servo_position(fd, 1, -1*tilt_position);   // Tilt value must be negated.
    }
   	
   	*resp_len = 0;
   	return 0;
  }	
  
  if (MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_PTZ_AUTOSERVO, ptz_id))
  {
  	assert(hdr->size == sizeof(player_ptz_controlmode_config));
    player_ptz_controlmode_config *servo = (player_ptz_controlmode_config*)data;	    
    auto_servoing(fd, servo->mode);
    if(servo->mode)
      printf("Auto servoing is enabled.\n");
    else
      printf("Auto servoing is disabled.\n");
    
    *resp_len = 0;
    return PLAYER_MSGTYPE_RESP_ACK;
  }
  
  *resp_len = 0;
  return -1;
}



void
Cmucam2::Main()
{
  memset( &blobfinder_data, 0, sizeof(blobfinder_data) );
  memset( &ptz_data, 0, sizeof(ptz_data) );

  blobfinder_data.width = htons((uint16_t)IMAGE_WIDTH);
  blobfinder_data.height = htons((uint16_t)IMAGE_HEIGHT);
  blobfinder_data.blob_count = htons((uint16_t)num_of_blobs);

  ptz_data.zoom = 45;          // cmucam does not have these 
  ptz_data.panspeed = 0;   
  ptz_data.tiltspeed = 0;
  ptz_data.zoom = htons(ptz_data.zoom);
  ptz_data.panspeed = htons(ptz_data.panspeed);
  ptz_data.tiltspeed = htons(ptz_data.tiltspeed); 
 
  packet_t blob_info;
  player_blobfinder_blob_t blob;

  int blobs_observed;
  //player_ptz_cmd_t command;
  //unsigned char config[PLAYER_MAX_REQREP_SIZE];    

  for(;;)
  {      
    // handle commands --------------------------------------------------------
    pthread_testcancel();
    ProcessMessages();
    
//    GetCommand((unsigned char*)&command, sizeof(player_ptz_cmd_t),NULL);
//    pthread_testcancel();    
   


    // handle configs --------------------------------------------------------            
/*    void* client;
    size_t config_size = 0; 
    if((config_size = GetConfig(ptz_id, &client, (void*)config, sizeof(config),NULL)))
    {	      
      switch(config[0])
      {
        case PLAYER_PTZ_AUTOSERVO:
        {
          // check that the config is tghe right length
          if( config_size != sizeof(player_ptz_controlmode_config) )
          {
            // complain!
            puts("Cmucam2_ptz autoservo request is wrong size!");
            if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK, NULL))
              PLAYER_ERROR("failed to PutReply");
          }
          else
          {
            player_ptz_controlmode_config *servo = (player_ptz_controlmode_config*)config;	    
            auto_servoing(fd, servo->mode);
            if(servo->mode)
              printf("Auto servoing is enabled.\n");
            else
              printf("Auto servoing is disabled.\n");
            // reply OK
            if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL))
              PLAYER_ERROR("failed to PutReply");
          }
          break;
        }
        default:
        {
          puts("Cmucam2_ptz got unknown config request");
          if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK, NULL))
            PLAYER_ERROR("failed to PutReply");
          break;
        }
      }      
    }*/

    // get data ------------------------------------------------------    

    blobs_observed = 0;                               // Do some coordinate transformatiopns.
    ptz_data.pan = -1*get_servo_position(fd, 0);      // Pan & Tilt values must be negated.
    ptz_data.tilt = -1*get_servo_position(fd, 1);         
    ptz_data.pan = htons(ptz_data.pan);
    ptz_data.tilt = htons(ptz_data.tilt); 
    
    for(int i = 0; i < num_of_blobs; i++)
    {      
      track_blob(fd, color[0]);
      if( !get_t_packet(fd, &blob_info) )
        exit(1);
      stop_tracking(fd);
      get_blob(blob_info, &blob, color[i]);    
      if(blob.area > 0)
        blobs_observed++;

      blob.id = htons(0);
      blob.color = htonl(blob.color);
      blob.area = htonl(blob.area);
      blob.x = htons(blob.x);
      blob.y = htons(blob.y);
      blob.left = htons(blob.left);
      blob.right = htons(blob.right);
      blob.top = htons(blob.top);
      blob.bottom = htons(blob.bottom);
      blob.range = htons(blob.range);                
      
      memcpy( &blobfinder_data.blobs[i], &blob, sizeof(blob));
    }     

    /* got the data. now fill it in */
    PutMsg(blobfinder_id, NULL, PLAYER_MSGTYPE_DATA, 0, &blobfinder_data,
            sizeof(blobfinder_data) - sizeof(blobfinder_data.blobs) +
            ntohs(blobfinder_data.blob_count) * sizeof(blobfinder_data.blobs[0]), NULL);
    PutMsg(ptz_id, NULL, PLAYER_MSGTYPE_DATA, 0, &ptz_data, sizeof(ptz_data), NULL);
  }
  
  pthread_exit(NULL);
}


/**************************************************************************			
                           *** GET BLOB ***
**************************************************************************/
/* Description: This function uses CMUcam's T packet for tracking to get
   the blob information as described by Player
   Parameters:  cam_packet: camera's T packet generated during tracking
                color: the color range used in tracking
   Returns:     The Player format for blob information
*/

void
Cmucam2::get_blob(packet_t cam_packet, player_blobfinder_blob_t *blob, color_config range)
{  
  unsigned char red = (range.rmin + range.rmax)/2;                   // a decriptive color for the blob
  unsigned char green = (range.gmin + range.gmax)/2;
  unsigned char blue = (range.bmin + range.bmax)/2;
  (*blob).color = red << 16 + green << 8 + blue;   
  (*blob).area = cam_packet.blob_area;       // the number of pixels in the blob
  (*blob).x = 2*cam_packet.middle_x;           // setting the bounding box for the blob
  (*blob).y = cam_packet.middle_y;
  (*blob).left = 2*cam_packet.left_x;
  (*blob).right = 2*cam_packet.right_x;        // highest and lowest y-vlaue for top and bottom
  (*blob).top = (cam_packet.left_y > cam_packet.right_y) ? cam_packet.left_y : cam_packet.right_y;
  (*blob).bottom = (cam_packet.left_y <= cam_packet.right_y) ? cam_packet.left_y : cam_packet.right_y;
}
