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
 *   by: Pouya Bastani      2004/05/1
 *   the P2 vision device.  it takes pan tilt zoom commands for the
 *   sony PTZ camera (if equipped), and returns color blob data gathered
 *   from CMUCAM2, which this device spawns and then talks to.
 */

#if HAVE_CONFIG_H
  #include <config.h>
#endif

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
#include <drivertable.h>
#include <player.h>
#include <math.h>
#include "camera.c"

class Cmucam2:public Driver 
{
  private:

    // Descriptive colors for each channel.
    uint32_t colors[PLAYER_BLOBFINDER_MAX_CHANNELS];
    void get_blob(packet_t cam_packet, player_blobfinder_blob_elt *blob, color_config range);
    int fd;
    int num_of_blobs;
    char devicepath[MAX_FILENAME_SIZE];
    color_config color[PLAYER_BLOBFINDER_MAX_BLOBS];
    //Camera cam;

  public:  

    // constructor 
    //
    Cmucam2( ConfigFile* cf, int section);

    virtual void Main();
  
    int Setup();
    int Shutdown();
};

// a factory creation function
Driver* Cmucam2_Init( ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_CMUCAM2_STRING))
  {
    PLAYER_ERROR1("driver \"cmucam2\" does not support interface \"%s\"\n",
                  interface);
    return(NULL);
  }
  else
    return((Driver*)(new Cmucam2(interface, cf, section)));
}

// a driver registration function
void 
Cmucam2_Register(DriverTable* table)
{
  table->AddDriver("cmucam2", PLAYER_READ_MODE, Cmucam2_Init);
}

Cmucam2::Cmucam2( ConfigFile* cf, int section)
  : Driver(cf, section, sizeof(player_cmucam2_data_t),sizeof(player_cmucam2_cmd_t), 5, 5)
{
  num_of_blobs = cf->ReadInt(section, "num_blobs", 1);
  strncpy(devicepath, cf->ReadString(section, "devicepath", NULL), sizeof(devicepath)-1);

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
  player_cmucam2_data_t dummy;
  fflush(stdout);

  memset(&dummy,0,sizeof(dummy));
  // zero the data buffer
  //PutData((unsigned char*)&dummy, sizeof(dummy), 0, 0);

  fd = open_port(devicepath);        // opening the serial port
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


void
Cmucam2::Main()
{
  // we'll transform the data into this structured buffer
  player_blobfinder_data_t blobfinder_data;
  player_ptz_data_t ptz_data;
  player_cmucam2_data_t cmucam_data;

  memset( &blobfinder_data, 0, sizeof(blobfinder_data) );
  memset( &ptz_data, 0, sizeof(ptz_data) );
  memset( &cmucam_data, 0, sizeof(cmucam_data) );

  blobfinder_data.width = htons((uint16_t)IMAGE_WIDTH);
  blobfinder_data.height = htons((uint16_t)IMAGE_HEIGHT);
  blobfinder_data.header[0].index = 0;
  blobfinder_data.header[0].num = htons((uint16_t)num_of_blobs);

  ptz_data.zoom = 45;          // cmucam does not have these 
  ptz_data.panspeed = 0;   
  ptz_data.tiltspeed = 0;
  ptz_data.zoom = htons(ptz_data.zoom);
  ptz_data.panspeed = htons(ptz_data.panspeed);
  ptz_data.tiltspeed = htons(ptz_data.tiltspeed); 
 
  packet_t blob_info;
  player_blobfinder_blob_elt blob;

  int blobs_observed;
  int pan_position = 0;
  player_ptz_cmd_t command;
  unsigned char config[CMUCAM_CONFIG_SIZE];    

  for(;;)
  {      
    // handle commands --------------------------------------------------------
    pthread_testcancel();
    GetCommand((unsigned char*)&command, sizeof(player_ptz_cmd_t));
    pthread_testcancel();    
   
    if(pan_position != (short)ntohs((unsigned short)(command.pan)))
    {      
      pan_position = (short)ntohs((unsigned short)(command.pan));      
      if( abs(pan_position) <= 90 )      
	set_servo_position(fd, 0, -1*pan_position);   // Pan value must be negated.      
    }   

    // handle configs --------------------------------------------------------            
    void* client;
    player_device_id_t id;
    size_t config_size = 0; 
    if((config_size = GetConfig(&id, &client, (void*)config, sizeof(config))))
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
	    if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
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
	    if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0))
		  PLAYER_ERROR("failed to PutReply");
	  }
    	  break;
	}
	default:
	{
	  puts("Cmucam2_ptz got unknown config request");
	  if(PutReply(&id, client, PLAYER_MSGTYPE_RESP_NACK, NULL, NULL, 0))
	    PLAYER_ERROR("failed to PutReply");
	  break;
	}
      }      
    }

    // get data ------------------------------------------------------    

    blobs_observed = 0;                               // Do some coordinate transformatiopns.
    ptz_data.pan = -1*get_servo_position(fd, 0);      // Pan & Tilt values must be negated.
    ptz_data.tilt = -1*get_servo_position(fd, 1);         
    ptz_data.pan = htons(ptz_data.pan);
    ptz_data.tilt = htons(ptz_data.tilt); 
    memcpy( &cmucam_data.ptz_data, &ptz_data, sizeof(ptz_data) );       
    
    for(int i = 0; i < num_of_blobs; i++)
    {      
      track_blob(fd, color[0]);
      if( !get_t_packet(fd, &blob_info) )
	exit(1);
      stop_tracking(fd);
      get_blob(blob_info, &blob, color[i]);    
      if(blob.area > 0)
	blobs_observed++;

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
      memcpy( &cmucam_data.blob, &blobfinder_data, sizeof(blobfinder_data) );      
    }     

    /* got the data. now fill it in */
    PutData((unsigned char*)&cmucam_data, sizeof(cmucam_data), 0, 0);    
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
Cmucam2::get_blob(packet_t cam_packet, player_blobfinder_blob_elt *blob, color_config range)
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
