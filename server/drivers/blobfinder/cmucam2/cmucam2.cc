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
#include <camera.h>

class Cmucam2:public CDevice 
{
  private:

    // Descriptive colors for each channel.
    uint32_t colors[PLAYER_BLOBFINDER_MAX_CHANNELS];
    void get_blob(packet_t cam_packet, player_blobfinder_blob_elt *blob, color_config range);
    int fd;
    int num_of_blobs;
    color_config color[PLAYER_BLOBFINDER_MAX_BLOBS];
    Camera cam;

  public:  

    // constructor 
    //
    Cmucam2(char* interface, ConfigFile* cf, int section);

    virtual void Main();
  
    int Setup();
    int Shutdown();
};

// a factory creation function
CDevice* Cmucam2_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_BLOBFINDER_STRING))
  {
    PLAYER_ERROR1("driver \"cmucam2\" does not support interface \"%s\"\n",
                  interface);
    return(NULL);
  }
  else
    return((CDevice*)(new Cmucam2(interface, cf, section)));
}

// a driver registration function
void 
Cmucam2_Register(DriverTable* table)
{
  table->AddDriver("cmucam2", PLAYER_READ_MODE, Cmucam2_Init);
}

Cmucam2::Cmucam2(char* interface, ConfigFile* cf, int section)
  : CDevice(sizeof(player_blobfinder_data_t),0,0,0)
{

  // do any initial setup here, including reading parameters out of
  // Player's config file

  // you don't need to connect to the camera yet - do that in Setup();

  num_of_blobs = cf->ReadInt(section, "num_of_blobs", 16);
  char variable[10];
  for(int j = 0; j < 10; j++)
    variable[j] = '\0';
  char blob_num[2];  
  strcat(variable, "color");
  
  for(int i = 0; i < num_of_blobs; i++)
  {
    sprintf(blob_num, "%d", i);
    strcat(variable, blob_num);
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
  player_blobfinder_data_t dummy;
  memset(&dummy,0,sizeof(dummy));
  // zero the data buffer
  PutData((unsigned char*)&dummy,
          sizeof(dummy.width)+sizeof(dummy.height)+sizeof(dummy.header),0,0);

  // TODO set up the camera here
  // open a file descriptor, etc.
  fd = cam.open_port();        // opening the serial port
  if(fd<0)                     // if not successful, stop
    return 0; 
  cam.power(fd, 1);
  printf("Camera is ON.");

  /* now spawn reading thread */
  StartThread();
  
  return(0);
}

int
Cmucam2::Shutdown()
{
  StopThread();
  cam.stop_tracking(fd);
  cam.power(fd, 0);                   // shutdown the camera
  cam.close_port(fd);                 // close the serial port
  printf("Camera is OFF.\n");               
  return(0);
}


void
Cmucam2::Main()
{
  // we'll transform the data into this structured buffer
  player_blobfinder_data_t local_data;
  memset( &local_data, 0, sizeof(local_data) );

  local_data.width = htons((uint16_t)IMAGE_WIDTH);
  local_data.height = htons((uint16_t)IMAGE_HEIGHT);
  
  local_data.header[0].index = 0;
  local_data.header[0].num = htons((uint16_t)num_of_blobs);
 
  packet_t blob_info;
  player_blobfinder_blob_elt blob;

  /* loop and read */
  int i;
  for(;;)
  {
    /* test if we are supposed to cancel */
    pthread_testcancel();


    // TODO read data from the CMUCam here
    for(i = 0; i < num_of_blobs; i++)
    {
      cam.track_blob(fd, color[i]);
      cam.get_t_packet(fd, &blob_info);
      cam.stop_tracking(fd);
      get_blob(blob_info, &blob, color[i]);
      printf("confidence: %d  area: %d   x: %d   y: %d\n", blob_info.confidence, blob_info.blob_area,  blob_info.middle_x, blob_info.middle_y);
    
      blob.color = htonl(blob.color);
      blob.area = htonl(blob.area);
      blob.x = htons(blob.x);
      blob.y = htons(blob.y);
      blob.left = htons(blob.left);
      blob.right = htons(blob.right);
      blob.top = htons(blob.top);
      blob.bottom = htons(blob.bottom);
      blob.range = htons(blob.range);

      memcpy( &local_data.blobs[i], &blob, sizeof(blob) );
    }

    // TODO - if things go badly wrong, 'break' here.
    // TODO package the data into the blobfinder data format

    /* got the data. now fill it in */
    PutData((unsigned char*)&local_data, 
	    (PLAYER_BLOBFINDER_HEADER_SIZE + num_of_blobs*PLAYER_BLOBFINDER_BLOB_SIZE),0,0);

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
   (*blob).x = cam_packet.middle_x;           // setting the bounding box for the blob
   (*blob).y = cam_packet.middle_y;
   (*blob).left = cam_packet.left_x;
   (*blob).right = cam_packet.right_x;        // highest and lowest y-vlaue for top and bottom
   (*blob).top = (cam_packet.left_y > cam_packet.right_y) ? cam_packet.left_y : cam_packet.right_y;
   (*blob).bottom = (cam_packet.left_y <= cam_packet.right_y) ? cam_packet.left_y : cam_packet.right_y;
}
