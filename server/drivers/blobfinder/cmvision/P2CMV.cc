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

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_cmvision cmvision

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

- @ref player_interface_blobfinder

@par Requires

- @ref player_interface_camera : camera device to get image data from

@par Configuration requests

- none

@par Configuration file options

- colorfile (string)
  - Default: "colors.txt"
  - CMVision configuration file

@par Example

@verbatim
driver
(
  name "cmvision"
  provides ["blobfinder:0"]
  requires ["camera:0"]
)
@endverbatim

@par Authors

Andy Martignoni III, Brian Gerkey, Brendan Burns, Ben Grocholsky

*/

/** @} */

#include <assert.h>
#include <stdio.h>
#include <unistd.h> /* close(2),fcntl(2),getpid(2),usleep(3),execvp(3),fork(2)*/
#include <signal.h>  /* for kill(2) */
#include <fcntl.h>  /* for fcntl(2) */
#include <string.h>  /* for strncpy(3),memcpy(3) */
#include <stdlib.h>  /* for atexit(3),atoi(3) */
#include <pthread.h>  /* for pthread stuff */

#include <libplayercore/playercore.h>
#include <libplayercore/error.h>

#include "cmvision.h"
#include "capture.h"

#define CMV_NUM_CHANNELS CMV_MAX_COLORS
#define CMV_HEADER_SIZE 4*CMV_NUM_CHANNELS
#define CMV_BLOB_SIZE 16
#define CMV_MAX_BLOBS_PER_CHANNEL 10

#define DEFAULT_CMV_WIDTH CMV_DEFAULT_WIDTH
#define DEFAULT_CMV_HEIGHT CMV_DEFAULT_HEIGHT
/********************************************************************/

class CMVisionBF: public Driver
{
  private:
    int              m_debuglevel;// debuglevel 0=none,
                                  //            1=basic,
                                  //            2=everything
    uint16_t         m_width;
    uint16_t         m_height;  // the image dimensions
    uint8_t*         m_img;
    const char*      m_colorfile;

    player_blobfinder_data_t   m_data;

    player_devaddr_t m_camera_addr;
    Device*          m_camera_dev;
    CMVision*        m_vision;

  public:
    int Setup();
    int Shutdown();
    // constructor
    CMVisionBF(ConfigFile* cf, int section);
    // This method will be invoked on each incoming message
    virtual int ProcessMessage(MessageQueue* resp_queue,
                               player_msghdr * hdr,
                               void * data);
    virtual void Main();
    void ProcessImageData();
};

// a factory creation function
Driver*
CMVision_Init( ConfigFile* cf, int section)
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
  : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN,
           PLAYER_BLOBFINDER_CODE),
           m_width(0),
           m_height(0),
           m_img(NULL),
           m_colorfile(NULL),
           m_camera_dev(NULL),
           m_vision(NULL)
{
  m_colorfile = cf->ReadString(section, "colorfile", "colors.txt");
  // Must have an input camera
  if (cf->ReadDeviceAddr(&m_camera_addr, section, "requires",
                         PLAYER_CAMERA_CODE, -1, NULL) != 0)
  {
    PLAYER_ERROR("this driver requires a camera in the .cfg file");
    return;
  }
}

int
CMVisionBF::Setup()
{
  printf("CMVision server initializing...");
  fflush(stdout);
  // Subscribe to the camera device
  if (!(this->m_camera_dev = deviceTable->GetDevice(this->m_camera_addr)))
  {
    PLAYER_ERROR("unable to locate suitable camera device");
    return(-1);
  }
  if(0 != this->m_camera_dev->Subscribe(this->InQueue))
  {
    PLAYER_ERROR("unable to subscribe to camera device");
    return(-1);
  }

  m_vision = new CMVision;
  // clean our data
  memset(&m_data,0,sizeof(m_data));
  puts("done.");

  StartThread();
  return(0);
}

int
CMVisionBF::Shutdown()
{
  /* if Setup() was never called, don't do anything */
  if (m_vision==NULL)
    return 0;

  StopThread();

    // Unsubscribe from the laser
  this->m_camera_dev->Unsubscribe(this->InQueue);

  delete m_vision;

  puts("CMVision server has been shutdown");
  return(0);
}

void
CMVisionBF::Main()
{
  // The main loop; interact with the device here
  for(;;)
  {
    // test if we are supposed to cancel
    pthread_testcancel();

    // Go to sleep for a while (this is a polling loop)
    //nanosleep(&NSLEEP_TIME, NULL);

    // Process incoming messages, and update outgoing data
    ProcessMessages();
  }
  return;
}

void
CMVisionBF::ProcessImageData()
{
    // this shouldn't change often
    if ((m_data.width != m_width) || (m_data.height != m_height))
    {
      if(!(m_vision->initialize(m_width, m_height)))
      {
        PLAYER_ERROR("Vision init failed.");
        exit(-1);
      }

      if(m_colorfile[0])
      {
        if (0 != m_vision->loadOptions(const_cast<char*>(m_colorfile)))
        {
          PLAYER_ERROR("Error loading color file");
          exit(-1);
        }
      }
      else
      {
        PLAYER_ERROR("No color file given.  Use the \"m_colorfile\" "
                    "option in the configuration file.");
        exit(-1);
      }
      m_data.width      = m_width;
      m_data.height     = m_height;
      printf("camera: [w %d h %d]\n", m_width, m_height);

    }

    if (!m_vision->processFrame(reinterpret_cast<image_pixel*>(m_img)))
    {
      PLAYER_ERROR("Frame error.");
      exit(-1);
    }

    m_data.blobs_count = 0;

    for (int ch = 0; ch < CMV_MAX_COLORS; ++ch)
    {
      rgb c;

      // Get the descriptive color
      c = m_vision->getColorVisual(ch);

      // Grab the regions for this color

      for (CMVision::region* r = m_vision->getRegions(ch); r != NULL; r = r->next)
      {
        if (m_data.blobs_count >= PLAYER_BLOBFINDER_MAX_BLOBS)
          break;

        player_blobfinder_blob_t *blob;

        blob = m_data.blobs + m_data.blobs_count++;

        blob->color = int(c.red)<<16 | int(c.green)<<8 | int(c.blue);
        blob->color = blob->color;

        // stage puts the range in here to simulate stereo m_vision. we
        // can't do that (yet?) so set the range to zero - rtv
        blob->range = 0;

        // get the area first
        blob->area = r->area;
        blob->area = blob->area;

        // convert the other entries to byte-swapped shorts
        blob->x      = uint16_t(r->cen_x+.5);
        blob->y      = uint16_t(r->cen_y+.5);
        blob->left   = r->x1;
        blob->right  = r->x2;
        blob->top    = r->y1;
        blob->bottom = r->y2;
      }

    }

    /* got the data. now fill it in */
    uint size = sizeof(m_data) - sizeof(m_data.blobs) +
                m_data.blobs_count * sizeof(m_data.blobs[0]);


    Publish(device_addr, NULL,
          PLAYER_MSGTYPE_DATA, PLAYER_BLOBFINDER_DATA_STATE,
          reinterpret_cast<void*>(&m_data), size, NULL);

/*
    Publish(device_addr, NULL,
          PLAYER_MSGTYPE_DATA, PLAYER_BLOBFINDER_DATA_STATE,
          reinterpret_cast<void*>(&m_data), sizeof(m_data), NULL);
*/
}

int
CMVisionBF::ProcessMessage(MessageQueue* resp_queue,
                           player_msghdr* hdr,
                           void* data)
{
  // Handle new data from the camera
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE,
                           this->m_camera_addr))
  {
    // we can't quite do this so easily with camera data
    // because the images are different than the max size
    //assert(hdr->size == sizeof(player_camera_data_t));
    player_camera_data_t* camera_data;
    camera_data = reinterpret_cast<player_camera_data_t *> (data);

    m_width  = camera_data->width;
    m_height = camera_data->height;
    m_img    = camera_data->image;

    // we have a new image,
    ProcessImageData();

    return(0);
  }

  // Tell the caller that you don't know how to handle this message
  return(-1);
}
