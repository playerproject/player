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


class Cmucam2:public CDevice 
{
  private:

    // Descriptive colors for each channel.
    uint32_t colors[PLAYER_BLOBFINDER_MAX_CHANNELS];

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

  /* now spawn reading thread */
  StartThread();
  
  return(0);
}

int
Cmucam2::Shutdown()
{
  // TODO shutdown the camera nicely and close the file descriptor here

  StopThread();
  return(0);
}


void
Cmucam2::Main()
{
  int numread;
  int num_blobs = 1;

  // we'll transform the data into this structured buffer
  player_blobfinder_data_t local_data;
  memset( &local_data, 0, sizeof(local_data) );

  local_data.width = htons((uint16_t)150);
  local_data.height = htons((uint16_t)120);
  
  local_data.header[0].index = 0;
  local_data.header[0].num = htons((uint16_t)1);
 

  /* loop and read */
  for(;;)
  {
    /* test if we are supposed to cancel */
    pthread_testcancel();


    // TODO read data from the CMUCam here

    
    player_blobfinder_blob_elt_t blob;
    blob.color = 0xFF0000;
    blob.area = 40;
    blob.x = 75;
    blob.y = 60;
    blob.left = blob.x - 30;
    blob.right = blob.x + 30;
    blob.top = blob.y - 40;
    blob.bottom = blob.y + 40;
    blob.range = 0;

    blob.color = htonl(blob.color);
    blob.area = htonl(blob.area);
    blob.x = htons(blob.x);
    blob.y = htons(blob.y);
    blob.left = htons(blob.left);
    blob.right = htons(blob.right);
    blob.top = htons(blob.top);
    blob.bottom = htons(blob.bottom);
    blob.range = htons(blob.range);

    memcpy( &local_data.blobs[0], &blob, sizeof(blob) );

    //puts( "hello pouya" );
    //sleep(1);

    // TODO - if things go badly wrong, 'break' here.

    // TODO package the data into the blobfinder data format

    /* got the data. now fill it in */
    PutData((unsigned char*)&local_data, 
              (PLAYER_BLOBFINDER_HEADER_SIZE + 
    
           num_blobs*PLAYER_BLOBFINDER_BLOB_SIZE),0,0);

  }

  pthread_exit(NULL);
}

