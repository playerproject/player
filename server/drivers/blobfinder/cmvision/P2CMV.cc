/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *     Andrew Martignoni III
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
 * Uses CMVision to retrieve the blob data
 */

/*
 * TODO: remove the whole capture interface, and just call GetData directly
 * on the underlying camera device.
 */

#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <unistd.h> /* close(2),fcntl(2),getpid(2),usleep(3),execvp(3),fork(2)*/
#include <netinet/in.h>  /* for htons(3) */
#include <signal.h>  /* for kill(2) */
#include <fcntl.h>  /* for fcntl(2) */
#include <string.h>  /* for strncpy(3),memcpy(3) */
#include <stdlib.h>  /* for atexit(3),atoi(3) */
#include <pthread.h>  /* for pthread stuff */

#include <drivertable.h>
#include <player.h>

#include "cmvision.h"
#include "capture.h"

#include "captureCamera.h"

#define CMV_NUM_CHANNELS CMV_MAX_COLORS
#define CMV_HEADER_SIZE 4*CMV_NUM_CHANNELS  
#define CMV_BLOB_SIZE 16
#define CMV_MAX_BLOBS_PER_CHANNEL 10

#define DEFAULT_CMV_WIDTH CMV_DEFAULT_WIDTH
#define DEFAULT_CMV_HEIGHT CMV_DEFAULT_HEIGHT
/********************************************************************/

class CMVisionBF:public Driver 
{
  private:
    int debuglevel;             // debuglevel 0=none, 1=basic, 2=everything

    int width, height;  // the image dimensions
    const char* colorfile;
    int camera_index;

    CMVision *vision;
    capture *cap;

  public:

    // constructor 
    CMVisionBF( ConfigFile* cf, int section);

    virtual void Main();

    int Setup();
    int Shutdown();
};

// a factory creation function
Driver* CMVision_Init( ConfigFile* cf, int section)
{
  return((Driver*)(new CMVisionBF( cf, section)));
}

// a driver registration function
void 
CMVision_Register(DriverTable* table)
{
  table->AddDriver("cmvision", CMVision_Init);
}

CMVisionBF::CMVisionBF( ConfigFile* cf, int section)
  :Driver(cf, section, PLAYER_BLOBFINDER_CODE, PLAYER_READ_MODE,
          sizeof(player_blobfinder_data_t),0,0,0)
{
  vision=NULL;
  cap=NULL;
 
  // first, get the necessary args
  colorfile = cf->ReadString(section, "colorfile", "");
  camera_index = cf->ReadInt(section, "camera_index", 0);
}
    
int
CMVisionBF::Setup()
{
  printf("CMVision server initializing...");
  fflush(stdout);
  vision = new CMVision;

  assert(cap = new captureCamera(this->camera_index));

  // get image size from camera_data struct
  width = ((captureCamera*)cap)->Width();
  height = ((captureCamera*)cap)->Height();
  
  if(!cap->initialize(width,height) ||
     !vision->initialize(width,height))
  {
    PLAYER_ERROR("Vision init failed.");
    return -1;
  }

  if(colorfile[0])
  {
    if (!vision->loadOptions((char*)colorfile))
    {
      PLAYER_ERROR("Error loading color file");
      return(-1);
    }
  }
  else
  {
    PLAYER_ERROR("No color file given.  Use the \"colorfile\" option in the configuration file.");
    return(-1);
  }

  //vision->enable(CMV_DENSITY_MERGE);

  /* REMOVE
  player_blobfinder_data_t dummy;
  memset(&dummy,0,sizeof(dummy));
  // zero the data buffer
  PutData((unsigned char*)&dummy,
          sizeof(dummy.width)+sizeof(dummy.height)+sizeof(dummy.header),NULL);
  */

  puts("done.");	

  /* now spawn reading thread */
  StartThread();
  
  return(0);
}

int
CMVisionBF::Shutdown()
{
  /* if Setup() was never called, don't do anything */
  if (vision==NULL)
    return 0;

  StopThread();

  delete vision;
  delete cap;
  vision=NULL;
  cap=NULL;

  puts("CMVision server has been shutdown");
  return(0);
}

void
CMVisionBF::Main()
{
  int ch;
  CMVision::region *r=NULL;
  player_blobfinder_blob_t *blob;
  rgb c;

  // we'll transform the data into this structured buffer
  player_blobfinder_data_t local_data;

  /* loop and read */
  for(;;)
  {
    // clean our buffers
    memset(&local_data,0,sizeof(local_data));

    int nwidth = ((captureCamera*)cap)->Width();
    int nheight = ((captureCamera*)cap)->Height();
    if ((width!=nwidth)!=(height!=nheight))
    {
      width = nwidth;
      height = nheight;
      vision->initialize(width,height);
    }
      
    // put in some stuff that doesnt change (almost...)
    local_data.width = htons(this->width);
    local_data.height = htons(this->height);
      
    /* test if we are supposed to cancel */
    pthread_testcancel();

    //get the current frame and process it
    if (!vision->processFrame((image_pixel*)cap->captureFrame()))
    {
      fprintf(stderr,"Frame error.\n");
      continue; //no frame processed
    }
      
    // put image time in blob struct
    //local_data.time_sec = htonl((uint32_t)(cap->getFrameTime()/1e9));
    //local_data.time_usec = htonl((cap->getFrameTime()/1000)%1000000);

    local_data.blob_count = 0;
        
    for (ch = 0; ch < CMV_MAX_COLORS; ch++)
    {
      // Get the descriptive color
      c=vision->getColorVisual(ch);
	  
      // Grab the regions for this color
      for (r = vision->getRegions(ch); r != NULL; r = r->next)
      {
        if (local_data.blob_count >= PLAYER_BLOBFINDER_MAX_BLOBS)
          break;
            
        blob = local_data.blobs + local_data.blob_count++;

        blob->color = int(c.red)<<16 | int(c.green)<<8 | int(c.blue);
        blob->color = htonl(blob->color);
                
        // stage puts the range in here to simulate stereo vision. we
        // can't do that (yet?) so set the range to zero - rtv
        blob->range = 0;

        // get the area first
        blob->area = r->area;
        blob->area = htonl(blob->area);
	      
        // convert the other entries to byte-swapped shorts
        blob->x = uint16_t(r->cen_x+.5);
        blob->x = htons(blob->x);
	      
        blob->y = uint16_t(r->cen_y+.5);
        blob->y = htons(blob->y);
	      
        blob->left = r->x1;
        blob->left = htons(blob->left);
	      
        blob->right = r->x2;
        blob->right = htons(blob->right);
	      
        blob->top = r->y1;
        blob->top = htons(blob->top);
	      
        blob->bottom = r->y2;
        blob->bottom = htons(blob->bottom);
      }
    }
    local_data.blob_count = htons(local_data.blob_count);

    /* REMOVE
       fprintf(stderr,"Got a NULL region: #%d on channel %d, %d of %d\n",
       i,ch,i-ntohs(local_data.header[ch].index)+1,ntohs(local_data.header[ch].num));
    */
      
    /* got the data. now fill it in */
    PutData(&local_data, sizeof(local_data) - sizeof(local_data.blobs) +
            ntohs(local_data.blob_count) * sizeof(local_data.blobs[0]), NULL);
  }

  pthread_exit(NULL);
}
