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

#if HAVE_1394
  #include "capture1394.h"
#endif
#if HAVE_V4L2
  #include "captureV4L2.h"
#endif
#if HAVE_V4L
  #include "capturev4l.h"
#endif
#ifdef PLAYER_CAMERA_CODE
#include "captureCamera.h"
#endif

#define CMV_NUM_CHANNELS CMV_MAX_COLORS
#define CMV_HEADER_SIZE 4*CMV_NUM_CHANNELS  
#define CMV_BLOB_SIZE 16
#define CMV_MAX_BLOBS_PER_CHANNEL 10

#define DEFAULT_CMV_WIDTH CMV_DEFAULT_WIDTH
#define DEFAULT_CMV_HEIGHT CMV_DEFAULT_HEIGHT
/********************************************************************/

class CMVisionBF:public CDevice 
{
private:
  int debuglevel;             // debuglevel 0=none, 1=basic, 2=everything
  
  int width, height;  // the image dimensions
  const char* colorfile;
  const char* capturetype;

  
  int header_len; // length of incoming packet header
  int header_elt_len; // length of each header element
  int blob_size;  // size of each incoming blob
  
  CMVision *vision;

  capture *cap;
  // for getting Gazebo gz_camera to intialize if needed.
  // dirty hack ! :-(

#if INCLUDE_GAZEBO 
 char * gz_interface;
  ConfigFile * gz_cf;
  int gz_section;
#endif

     // ben -- adding bayer conversion
     bool DoBayerConversion;
     int BayerPattern;

  public:

    // constructor 
    //
    CMVisionBF(char* interface, ConfigFile* cf, int section);

    virtual void Main();

    int Setup();
    int Shutdown();
};

// a factory creation function
CDevice* CMVision_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_BLOBFINDER_STRING))
  {
    PLAYER_ERROR1("driver \"cmvision\" does not support interface \"%s\"\n",
                  interface);
    return(NULL);
  }
  else
    return((CDevice*)(new CMVisionBF(interface, cf, section)));
}

// a driver registration function
void 
CMVision_Register(DriverTable* table)
{
  table->AddDriver("cmvision", PLAYER_READ_MODE, CMVision_Init);
}

CMVisionBF::CMVisionBF(char* interface, ConfigFile* cf, int section)
  :CDevice(sizeof(player_blobfinder_data_t),0,0,0)
{
  vision=NULL;
  cap=NULL;
  
#ifdef PLAYER_CAMERA_CODE //INCLUDE_GAZEBO
  gz_interface=interface;
  gz_cf=cf;
  gz_section=section;
#endif
  
  // first, get the necessary args
  width = cf->ReadInt(section, "width", DEFAULT_CMV_WIDTH);
  height = cf->ReadInt(section, "height", DEFAULT_CMV_HEIGHT);
  colorfile = cf->ReadString(section, "colorfile", "");
  capturetype = cf->ReadString(section, "capture", "1394");
  // just here to supress a configfile warning. 
  // it's used by captureCamera
  int idx = cf->ReadInt(section, "index", 0);
  
#if HAVE_1394
  // this might belong in capture1394.cc
  // read config option to perform bayer color conversion
  DoBayerConversion = false;
  if (!strcmp(cf->ReadString(section, "do_bayer_conversion", ""),"BGGR")){
       DoBayerConversion = true;
       BayerPattern = BAYER_PATTERN_BGGR;
  } else if (!strcmp(cf->ReadString(section, "do_bayer_conversion", ""),"GRBG")){
       DoBayerConversion = true;
       BayerPattern = BAYER_PATTERN_GRBG;
  } else if (!strcmp(cf->ReadString(section, "do_bayer_conversion", ""),"RGGB")){
       DoBayerConversion = true;
       BayerPattern = BAYER_PATTERN_RGGB;
  } else if (!strcmp(cf->ReadString(section, "do_bayer_conversion", ""),"GBRG")){
       DoBayerConversion = true;
       BayerPattern = BAYER_PATTERN_GBRG;
  } else  
       DoBayerConversion = false;
#endif

  header_len = CMV_HEADER_SIZE;
  blob_size = CMV_BLOB_SIZE;
  header_elt_len = header_len / PLAYER_BLOBFINDER_MAX_CHANNELS;
    
}
    
int
CMVisionBF::Setup()
{
  printf("CMVision server initializing...");
  fflush(stdout);
  vision = new CMVision;

  if(!strcmp(capturetype, "1394"))
  {
#if HAVE_1394
    cap = new capture1394(DoBayerConversion,BayerPattern);
#else
    PLAYER_ERROR("Sorry, support for capture from a 1394 camera was not "
                 "included at compile-time");
    return(-1);
#endif
  }
  else if(!strcmp(capturetype, "V4L2"))
  {
#if HAVE_V4L2
    cap = new captureV4L2;
#else
    PLAYER_ERROR("Sorry, support for capture from a V4L2 camera was not "
                 "included at compile-time");
    return(-1);
#endif
  }
 else if(!strcmp(capturetype, "camera"))
  {
#ifdef PLAYER_CAMERA_CODE
       // does this interface always exist? 
       cap = new captureCamera(gz_interface,gz_cf,gz_section);
       // get image size from camera_data struct
       width = ((captureCamera*)cap)->Width();
       height = ((captureCamera*)cap)->Height();
#else
    PLAYER_ERROR("Sorry, support for the camera interface was not "
                 "included at compile-time");
    return(-1);
#endif
  }
  else if(!strcmp(capturetype, "V4L"))
  {
#if HAVE_V4L
    cap = new capturev4l;
#else
    PLAYER_ERROR("Sorry, support for capture from a V4L camera was not "
                 "included at compile-time");
    return(-1);
#endif
  }
  else
  {
    PLAYER_ERROR1("Unknown video capture type: \"%s\"", capturetype);
    return(-1);
  }
  
  if(!cap->initialize(width,height) ||
     !vision->initialize(width,height))
    {
      PLAYER_ERROR("Vision init failed.");
      return -1;
    }

  if (colorfile[0])
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

  player_blobfinder_data_t dummy;
  memset(&dummy,0,sizeof(dummy));
  // zero the data buffer
  PutData((unsigned char*)&dummy,
          sizeof(dummy.width)+sizeof(dummy.height)+sizeof(dummy.header),0,0);

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
  int num_blobs;
  int i,j;
  CMVision::region *r=NULL;
  rgb c;

  // we'll transform the data into this structured buffer
  player_blobfinder_data_t local_data;

  /* loop and read */
  for(;;)
    {
      // clean our buffers
      memset(&local_data,0,sizeof(local_data));
      
#ifdef PLAYER_CAMERA_CODE
      if(!strcmp(capturetype, "camera"))
	   {
		int nwidth = ((captureCamera*)cap)->Width();
		int nheight = ((captureCamera*)cap)->Height();
		if ((width!=nwidth)!=(height!=nheight))
		     {
			  width = nwidth;
			  height = nheight;
			  vision->initialize(width,height);
		     }
	   }
#endif
      
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

      for(i=0;i<PLAYER_BLOBFINDER_MAX_CHANNELS;i++)
	{
	  //determine the index of the first entry for this channel
	  if (i==0)
	    local_data.header[i].index = 0;
	  else
	    local_data.header[i].index = ntohs(local_data.header[i-1].index) +
	      ntohs(local_data.header[i-1].num);
	  local_data.header[i].index = 
	    htons(local_data.header[i].index);
	  
	  //number of entries in this channel
	  local_data.header[i].num = vision->numRegions(i);
	  if (local_data.header[i].num>CMV_MAX_BLOBS_PER_CHANNEL)
	    local_data.header[i].num=CMV_MAX_BLOBS_PER_CHANNEL;

	  local_data.header[i].num = 
	    htons(local_data.header[i].num);
	}
      
      /* sum up the data we expect */
      num_blobs=0;
      for(i=0;i<PLAYER_BLOBFINDER_MAX_CHANNELS;i++)
	num_blobs += ntohs(local_data.header[i].num);
      
     
      for(i=0;i<num_blobs;i++)
	{
	  // Figure out the blob channel number
	  int ch = 0;
	  for (j = 0; j < PLAYER_BLOBFINDER_MAX_CHANNELS; j++)
	    {
	      if (i >= ntohs(local_data.header[j].index) &&
		  i < ntohs(local_data.header[j].index) + 
		  ntohs(local_data.header[j].num))
		{
		  ch = j;
		  break;
		}
	    }

	  /*
	  if (i>=ntohs(local_data.header[j].index) + 
	      ntohs(local_data.header[j].num))
	    fprintf(stderr,"Blob index out of bounds.\n");
	  */
	  
	  // set the descriptive color
	  c=vision->getColorVisual(ch);
	  j=int(c.red)<<16 | int(c.green)<<8 | int(c.blue);
	  local_data.blobs[i].color = htonl(j);
	  
	  //grab the region for this blob
	  if (ntohs(local_data.header[ch].index)==i) //new color
	    r=vision->getRegions(ch);
	  else
	    r=(r==NULL)?r:r->next;
	  
	  // stage puts the range in here to simulate stereo vision. we
	  // can't do that (yet?) so set the range to zero - rtv
	  local_data.blobs[i].range = 0;
	  
	  if (r!=NULL)
	    {
	      // get the area first
	      local_data.blobs[i].area = r->area;
	      local_data.blobs[i].area = htonl(local_data.blobs[i].area);
	      
	      // convert the other entries to byte-swapped shorts
	      local_data.blobs[i].x = uint16_t(r->cen_x+.5);
	      local_data.blobs[i].x = htons(local_data.blobs[i].x);
	      
	      local_data.blobs[i].y = uint16_t(r->cen_y+.5);
	      local_data.blobs[i].y = htons(local_data.blobs[i].y);
	      
	      local_data.blobs[i].left = r->x1;
	      local_data.blobs[i].left = htons(local_data.blobs[i].left);
	      
	      local_data.blobs[i].right = r->x2;
	      local_data.blobs[i].right = htons(local_data.blobs[i].right);
	      
	      local_data.blobs[i].top = r->y1;
	      local_data.blobs[i].top = htons(local_data.blobs[i].top);
	      
	      local_data.blobs[i].bottom = r->y2;
	      local_data.blobs[i].bottom = htons(local_data.blobs[i].bottom);
	    }
	  else
	    fprintf(stderr,"Got a NULL region: #%d on channel %d, %d of %d\n",
		    i,ch,i-ntohs(local_data.header[ch].index)+1,ntohs(local_data.header[ch].num));
	}
      
      /* test if we are supposed to cancel */
      pthread_testcancel();
      
      /* got the data. now fill it in */
      PutData((unsigned char*)&local_data, 
	      (PLAYER_BLOBFINDER_HEADER_SIZE + 
	       num_blobs*PLAYER_BLOBFINDER_BLOB_SIZE),0,0);
    }

  pthread_exit(NULL);
}
