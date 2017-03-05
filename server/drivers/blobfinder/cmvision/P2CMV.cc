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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

/*
 * $Id$
 *
 * Uses CMVision to retrieve the blob data
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_cmvision cmvision
@brief CMVision

CMVision (Color Machine Vision) is a fast
color-segmentation (aka blob-finding) software library.
CMVision was written by Jim Bruce at CMU and is Freely <a
href=http://www-2.cs.cmu.edu/~jbruce/cmvision/>available</a> under
the GNU GPL.  But you don't have to download CMVision yourself, because
Player's cmvision driver includes the CMVision code.  The cmvision driver
provides a stream of camera images to the CMVision code and assembles
the resulting blob information into Player's data format.

Consult the CMVision documentation for details on writing a CMVision
configuration file.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_blobfinder

@par Requires

- @ref interface_camera : camera device to get image data from

@par Configuration requests

- PLAYER_BLOBFINDER_REQ_SET_COLOR
- PLAYER_BLOBFINDER_REQ_GET_COLOR

@par Configuration file options

- debuglevel (int)
  - Default: 0
  - If set to 1, the blobfinder will output a testpattern of three blobs.

- colorfile (string)
  - Default: ""
  - CMVision configuration file.  In the colors section, the tuple is the RGB
  value of the intended color.  In the thresholds section, the values are the
  min:max of the respective YUV channels.

- minblobarea (int)
  - Default: CMV_MIN_AREA (20)
  - minimum number of pixels required to qualify as a blob

- maxblobarea (int)
  - Default: 0 (off)
  - maximum number of pixels allowed to qualify as a blob

@verbatim
[Colors]
(255,  0,  0) 0.000000 10 Red
(  0,255,  0) 0.000000 10 Green
(  0,  0,255) 0.000000 10 Blue

[Thresholds]
( 25:164, 80:120,150:240)
( 20:220, 50:120, 40:115)
( 15:190,145:255, 40:120)
@endverbatim

@par Example

@verbatim
driver
(
  name "cmvision"
  provides ["blobfinder:0"]
  requires ["camera:0"]
  colorfile "/path/to/colorfile"
  minblobarea 1
  maxblobarea 100
)
@endverbatim

@author Andy Martignoni III, Brian Gerkey, Brendan Burns,
Ben Grocholsky, Brad Kratochvil

*/

/** @} */

#include <config.h>

#include <assert.h>
#include <stdio.h>
#if !defined (WIN32)
  #include <unistd.h>  /* close(2),fcntl(2),getpid(2),usleep(3),execvp(3),fork(2)*/
#endif
#include <signal.h>  /* for kill(2) */
#include <fcntl.h>   /* for fcntl(2) */
#include <string.h>  /* for strncpy(3),memcpy(3) */
#include <stdlib.h>  /* for atexit(3),atoi(3) */
#include <pthread.h> /* for pthread stuff */
#include <math.h>    /* for rint */
#include <stddef.h>  /* for size_t and NULL */

#include <libplayercore/playercore.h>

#if HAVE_JPEG
  #include <libplayerjpeg/playerjpeg.h>
#endif

#include "conversions.h"
#include "cmvision.h"

#define CMV_NUM_CHANNELS CMV_MAX_COLORS
#define CMV_HEADER_SIZE 4*CMV_NUM_CHANNELS
#define CMV_BLOB_SIZE 16
#define CMV_MAX_BLOBS_PER_CHANNEL 10

#define DEFAULT_CMV_WIDTH CMV_DEFAULT_WIDTH
#define DEFAULT_CMV_HEIGHT CMV_DEFAULT_HEIGHT

#define RGB2YUV(r, g, b, y, u, v)\
  y = (306*r + 601*g + 117*b)  >> 10;\
  u = ((-172*r - 340*g + 512*b) >> 10)  + 128;\
  v = ((512*r - 429*g - 83*b) >> 10) + 128;\
  y = y < 0 ? 0 : y;\
  u = u < 0 ? 0 : u;\
  v = v < 0 ? 0 : v;\
  y = y > 255 ? 255 : y;\
  u = u > 255 ? 255 : u;\
  v = v > 255 ? 255 : v

/********************************************************************/

class CMVisionBF: public ThreadedDriver
{
  private:
    int              mDebugLevel; // debuglevel 0=none,
                                  //            1=basic,
                                  //            2=everything
    uint16_t         mWidth;
    uint16_t         mHeight;     // the image dimensions
    uint8_t*         mImg;
    uint8_t*         mTmp;
    const char*      mColorFile;
    uint16_t         mMinArea;
    uint16_t         mMaxArea;

    player_blobfinder_data_t   mData;
    unsigned int     allocated_blobs;

    player_devaddr_t mCameraAddr;
    Device*          mCameraDev;
    CMVision*        mVision;

    // this will output a testpattern for debugging
    void TestPattern();
    // print the blobs to the console
    void Print();

  public:

    // constructor
    CMVisionBF(ConfigFile* cf, int section);
    virtual ~CMVisionBF();
    // This method will be invoked on each incoming message
    virtual int ProcessMessage(QueuePointer & resp_queue,
                               player_msghdr * hdr,
                               void * data);
    virtual void Main();
    int MainSetup();
    void MainQuit();
    
    void ProcessImageData();
    int ProcessBlobfinderReqSetColor(player_msghdr_t *hdr, 
                                     player_blobfinder_color_config_t *data);
    int ProcessBlobfinderReqGetColor(player_msghdr_t *hdr, 
                                     player_blobfinder_color_config_t *data);
};

// a factory creation function
Driver*
CMVision_Init( ConfigFile* cf, int section)
{
  return((Driver*)(new CMVisionBF( cf, section)));
}

// a driver registration function
void
cmvision_Register(DriverTable* table)
{
  table->AddDriver("cmvision", CMVision_Init);
}

CMVisionBF::CMVisionBF( ConfigFile* cf, int section)
  : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN,
           PLAYER_BLOBFINDER_CODE),
           mWidth(0),
           mHeight(0),
           mImg(NULL),
           mTmp(NULL),
           mColorFile(NULL),
           mCameraDev(NULL),
           mVision(NULL)
{
  mColorFile  = cf->ReadString(section, "colorfile", "");
  mDebugLevel = cf->ReadInt(section, "debuglevel", 0);
  mMinArea    = cf->ReadInt(section, "minblobarea", CMV_MIN_AREA);
  mMaxArea    = cf->ReadInt(section, "maxblobarea", 0);
  // Must have an input camera
  if (cf->ReadDeviceAddr(&mCameraAddr, section, "requires",
                         PLAYER_CAMERA_CODE, -1, NULL) != 0)
  {
    PLAYER_ERROR("this driver requires a camera in the .cfg file");
    return;
  }
}

CMVisionBF::~CMVisionBF()
{
  if (mVision) delete mVision;
  if (mImg) delete []mImg;
  if (mTmp) delete []mTmp;
}

int
CMVisionBF::MainSetup()
{
  if (mVision)
  {
    PLAYER_ERROR("CMVision server already initialized");
    return -1;
  }
  printf("CMVision server initializing...");
  fflush(stdout);
  // Subscribe to the camera device
  if (!(this->mCameraDev = deviceTable->GetDevice(this->mCameraAddr)))
  {
    PLAYER_ERROR("unable to locate suitable camera device");
    return(-1);
  }
  if(0 != this->mCameraDev->Subscribe(this->InQueue))
  {
    PLAYER_ERROR("unable to subscribe to camera device");
    return(-1);
  }

  mVision = new CMVision();
  mVision->set_cmv_min_area(mMinArea);
  mVision->set_cmv_max_area(mMaxArea);
  // clean our data
  memset(&mData,0,sizeof(mData));
  allocated_blobs = 0;
  puts("done.");

  return(0);
}

void 
CMVisionBF::MainQuit()
{
  // Unsubscribe from the camera
  this->mCameraDev->Unsubscribe(this->InQueue);

  delete mVision; mVision = NULL;

  puts("CMVision server has been shutdown");
}

void
CMVisionBF::Main()
{
  // The main loop; interact with the device here
  for(;;)
  {
    // wait to receive a new message (blocking)
    Wait();

    // test if we are supposed to cancel
    pthread_testcancel();

    // Process incoming messages, and update outgoing data
    ProcessMessages();
  }
  return;
}

void
CMVisionBF::ProcessImageData()
{
    assert(mVision);
    // this shouldn't change often
    if ((mData.width != mWidth) || (mData.height != mHeight))
    {
      if(!(mVision->initialize(mWidth, mHeight)))
      {
        PLAYER_ERROR("Vision init failed.");
        exit(-1);
      }

      if(mColorFile[0])
      {
        if (!mVision->loadOptions(const_cast<char*>(mColorFile)))
        {
          PLAYER_ERROR("Error loading color file");
          exit(-1);
        }
      }
      else
      {
        PLAYER_ERROR("No color file given.  Use the \"mColorFile\" "
                     "option in the configuration file.");
        exit(-1);
      }
      mData.width      = mWidth;
      mData.height     = mHeight;
      printf("cmvision using camera: [w %d h %d]\n", mWidth, mHeight);
    }

    if (!mVision->processFrame(reinterpret_cast<image_pixel*>(mImg)))
    {
      PLAYER_ERROR("Frame error.");
    }


    mData.blobs_count = 0;
    for (int ch = 0; ch < CMV_MAX_COLORS; ++ch)
    {
      // Get the descriptive color
      rgb c = mVision->getColorVisual(ch);;

      // Grab the regions for this color
      CMVision::region* r = NULL;

      for (r = mVision->getRegions(ch); r != NULL; r = r->next)
      {
        if (mData.blobs_count >= allocated_blobs)
        {
          allocated_blobs = mData.blobs_count+1;
          mData.blobs = (player_blobfinder_blob_t*)realloc(mData.blobs,sizeof(mData.blobs[0])*allocated_blobs);
        }

        player_blobfinder_blob_t *blob;
        blob = mData.blobs + mData.blobs_count;
        mData.blobs_count++;

        blob->color = uint32_t(c.red)   << 16 |
                      uint32_t(c.green) <<  8 |
                      uint32_t(c.blue);

        // stage puts the range in here to simulate stereo mVision. we
        // can't do that (yet?) so set the range to zero - rtv
        blob->range = 0;

        // get the area first
        blob->area   = static_cast<uint32_t>(r->area);

        blob->x      = static_cast<uint32_t>(rint(r->cen_x+.5));
        blob->y      = static_cast<uint32_t>(rint(r->cen_y+.5));
        blob->left   = static_cast<uint32_t>(r->x1);
        blob->right  = static_cast<uint32_t>(r->x2);
        blob->top    = static_cast<uint32_t>(r->y1);
        blob->bottom = static_cast<uint32_t>(r->y2);
      }
    }

    // if we're debugging, output the test blobs
    if (0 != mDebugLevel)
      TestPattern();

    Publish(device_addr,
          PLAYER_MSGTYPE_DATA, PLAYER_BLOBFINDER_DATA_BLOBS,
          reinterpret_cast<void*>(&mData));
}

void
CMVisionBF::TestPattern()
{
  mData.blobs_count = 3;

  for (uint32_t i=0; i<mData.blobs_count; ++i)
  {
    uint32_t x = mWidth/5*i + mWidth/5;
    uint32_t y = mHeight/2;

    mData.blobs[i].x = x;
    mData.blobs[i].y = y;

    mData.blobs[i].top    = y+10;
    mData.blobs[i].bottom = y-10;
    mData.blobs[i].left   = x-10;
    mData.blobs[i].right  = x+10;

    mData.blobs[i].color  = 0xff << i*8;
  }
}

void
CMVisionBF::Print()
{
  // this is mainly for debugging purposes
  for (uint32_t i=0; i<mData.blobs_count; ++i)
  {
    printf("%i: ", i);
    printf("%i, ", mData.blobs[i].x);
    printf("%i, ", mData.blobs[i].y);
    printf("%i, ", mData.blobs[i].top);
    printf("%i, ", mData.blobs[i].left);
    printf("%i, ", mData.blobs[i].bottom);
    printf("%i\n", mData.blobs[i].right);
  }
  printf("-----\n");
}

int
CMVisionBF::ProcessMessage(QueuePointer & resp_queue,
                           player_msghdr* hdr,
                           void* data)
{
  HANDLE_CAPABILITY_REQUEST(device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILITIES_REQ);
  HANDLE_CAPABILITY_REQUEST(device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_BLOBFINDER_REQ_SET_COLOR);
  HANDLE_CAPABILITY_REQUEST(device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_BLOBFINDER_REQ_GET_COLOR);
  // Handle new data from the camera
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE,
                           this->mCameraAddr))
  {
    // we can't quite do this so easily with camera data
    // because the images are different than the max size
    //assert(hdr->size == sizeof(player_camera_data_t));
    player_camera_data_t* camera_data = reinterpret_cast<player_camera_data_t *>(data);
    uint8_t* ptr;
    uint8_t* ptr1;
    size_t mSize;
    int i;

    assert(camera_data);

    if (camera_data->format != PLAYER_CAMERA_FORMAT_RGB888)
    {
      PLAYER_ERROR("No support for formats other than PLAYER_CAMERA_FORMAT_RGB888");
      return(-1);
    }

#if !HAVE_JPEG
    if (camera_data->compression == PLAYER_CAMERA_COMPRESS_JPEG)
    {
      PLAYER_ERROR("No support for jpeg decompression");
      return(-1);
    }
#endif

    if ((camera_data->width) && (camera_data->height))
    {
      if ((mWidth != camera_data->width) || (mHeight != camera_data->height) || (!mImg) || (!mTmp))
      {
        mWidth  = camera_data->width;
        mHeight = camera_data->height;
        if (mImg) delete []mImg; mImg = NULL;
        if (mTmp) delete []mTmp; mTmp = NULL;
        // we need to allocate some memory
        if (!mImg) mImg = new uint8_t[mWidth * mHeight * 2];
        if (!mTmp) mTmp = new uint8_t[mWidth * mHeight * 3];
      }
      ptr = NULL;
      if (camera_data->compression == PLAYER_CAMERA_COMPRESS_JPEG)
      {
        assert(camera_data->bpp == 24);
#if HAVE_JPEG
	jpeg_decompress(reinterpret_cast<unsigned char *>(mTmp),
			mWidth * mHeight * 3,
                        camera_data->image,
                        camera_data->image_count
                       );
#endif
        ptr = mTmp;
      } else switch (camera_data->bpp)
      {
      case 24:
        ptr = camera_data->image;
        break;
      case 32:
        ptr = camera_data->image;
        ptr1 = mTmp;
        mSize = camera_data->width * camera_data->height;
        for (i = 0; i < static_cast<int>(mSize); i++)
        {
          ptr1[0] = ptr[0];
          ptr1[1] = ptr[1];
          ptr1[2] = ptr[2];
          ptr += 4;
          ptr1 += 3;
        }
        ptr = mTmp;
        break;
      default:
        PLAYER_ERROR1("Unsupported depth %u", camera_data->bpp);
        return(-1);
      }
      assert(ptr);

      // now deal with the data
      rgb2uyvy(ptr, mImg, mWidth*mHeight);

      // we have a new image,
      ProcessImageData();
    }
    return(0);
  }
  else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_BLOBFINDER_REQ_SET_COLOR, device_addr))
  {
    assert(hdr->size == sizeof(player_blobfinder_color_config_t));
    player_blobfinder_color_config_t* color_data = reinterpret_cast<player_blobfinder_color_config_t *>(data);
    if (this->ProcessBlobfinderReqSetColor(hdr,color_data) < 0) 
    {
      Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, hdr->subtype);
      return(-1);
    }
    Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, hdr->subtype);
    return(0);
  }
  else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_BLOBFINDER_REQ_GET_COLOR, device_addr))
  {
    assert(hdr->size == sizeof(player_blobfinder_color_config_t));
    player_blobfinder_color_config_t* color_data = reinterpret_cast<player_blobfinder_color_config_t *>(data);
    player_blobfinder_color_config_t resp;
    resp.channel=color_data->channel;
    int tmp;
    if ((tmp = this->ProcessBlobfinderReqGetColor(hdr,&resp)) < 0)  
    {   
      printf("ERROR! %d\n",tmp);
      Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, hdr->subtype,&resp,sizeof(resp),NULL);
      return(-1);
    }   
    printf("test: %d %d %d \n",resp.channel,resp.rmin,resp.rmax);
    Publish(device_addr,
         resp_queue, PLAYER_MSGTYPE_RESP_ACK,
         hdr->subtype,
         &resp,
         //reinterpret_cast<player_blobfinder_color_config_t*>(&resp),
         sizeof(resp),NULL);
    return(0);
  }

  // Tell the caller that you don't know how to handle this message
  return(-1);
}

int
CMVisionBF::ProcessBlobfinderReqSetColor(player_msghdr_t *hdr,
                                         player_blobfinder_color_config_t *data)
{
  if(data->channel < 0 || data->channel >= CMV_MAX_COLORS)
  {
    PLAYER_ERROR("CMVisionBF::ProcessBlobfinderReqSetColor data->channel out of range");
    return(-1);
  }

  // Get and copy existing color_info for the channel
  // Should this function skip this and just change the existing color_info?
  CMVision::color_info *oldinfo;
  CMVision::color_info newinfo;
  oldinfo = mVision->getColorInfo(data->channel);
  memcpy(&newinfo,oldinfo,sizeof(CMVision::color_info));

  // Convert RGB data to YUV
  RGB2YUV( data->rmin, data->gmin, data->bmin, newinfo.y_low, newinfo.u_low, newinfo.v_low );
  RGB2YUV( data->rmax, data->gmax, data->bmax, newinfo.y_high, newinfo.u_high, newinfo.v_high );
  printf(" colorinfo (%d:%d,%d,%d,%d,%d,%d)\n",
    data->channel,
    newinfo.y_low,
    newinfo.y_high,
    newinfo.u_low,
    newinfo.u_high,
    newinfo.v_low,
    newinfo.v_high);
//  mVision->setColorInfo(data->channel,newinfo);
  mVision->setThreshold(data->channel,
    newinfo.y_low,
    newinfo.y_high,
    newinfo.u_low,
    newinfo.u_high,
    newinfo.v_low,
    newinfo.v_high);
  return(0);
}

int
CMVisionBF::ProcessBlobfinderReqGetColor(player_msghdr_t *hdr,
                                         player_blobfinder_color_config_t *data)
{
  assert(hdr);
  assert(data);
  if(data->channel < 0 || data->channel >= CMV_MAX_COLORS)
  {
    PLAYER_ERROR("CMVisionBF::ProcessBlobfinderReqSetColor data->channel out of range");
    return(-1);
  }

  int v1,v2,v3,v4,v5,v6;

  if (mVision->getThreshold((int)data->channel,v1,v2,v3,v4,v5,v6)) {
    data->rmin = v1;
    data->rmax = v2;
    data->gmin = v3; 
    data->gmax = v4;
    data->bmin = v5; 
    data->bmax = v6; 
    return(0);
  }
  else {
    PLAYER_ERROR("CMVisionBF::ProcessBlobfinderReqSetColor getThreshold failed");
    return(-1);
  }
}

