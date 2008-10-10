/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2006 Radu Bogdan Rusu (rusu@cs.tum.edu)
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
 Desc: Driver for unicap/libunicap compatible capture devices.
 Author: Radu Bogdan Rusu
 Date: 10 Oct 2008
*/
/** @ingroup drivers */
/** @{ */
/** @defgroup driver_unicapimage unicapimage
 * @brief unicapimage

The unicapimage driver makes use of libunicap
(http://www.unicap-imaging.org) to get access to certain video capture
devices (such as the Imaging Source DFG/1394-1e converter) and provide the
data through the @ref interface_camera interface.

@par Compile-time dependencies

- libunicap (http://www.unicap-imaging.org) should be installed.

@par Provides

- @ref interface_camera

@par Requires

- none

@par Configuration requests

- none yet

@par Configuration file options

The examples below are given for the Imaging Source DFG/1394-1e
Video-to-Firewire converter.

- color_space (integer)
  - Default: 0 (UYVY)
  - Possible values:  0 (UYVY), 1 (YUY2), 2 (Y411), 3 (Grey).
  - Format of the image to provide.

- video_format (integer)
  - Default: 0 (320x240)
  - Possible values: 0 (320x240), 1 (640x480), 2 (768x576).
  - Resolution of the image to provide.

@par Example

@verbatim
driver
(
  name "unicapimage"
  provides ["camera:0"]
  color_space 3
  video_format 0
)
@endverbatim

@author Radu Bogdan Rusu

 */
/** @} */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>

#include <libplayercore/playercore.h>
#include <unicap.h>
#include <unicap_status.h>

////////////////////////////////////////////////////////////////////////////////
// The UniCap_Image device class.
class UniCap_Image : public Driver
{
  public:
    // Constructor
    UniCap_Image (ConfigFile* cf, int section);

    // Destructor
    ~UniCap_Image ();

    // Implementations of virtual functions
    virtual int Setup ();
    virtual int Shutdown ();

    // Camera interface (provides)
    player_devaddr_t         cam_id;
    player_camera_data_t     cam_data;
  private:

    // Main function for device thread.
    virtual void Main ();
    virtual void RefreshData  ();

    int color_space, video_format, device_id;
    
    unicap_handle_t handle;
    unicap_device_t device;
    unicap_format_t format_spec;
    unicap_format_t format;
    unicap_data_buffer_t buffer;
    unicap_data_buffer_t *returned_buffer;
};

////////////////////////////////////////////////////////////////////////////////
// Factory creation function. This functions is given as an argument when
// the driver is added to the driver table
Driver* UniCap_Image_Init (ConfigFile* cf, int section)
{
  // Create and return a new instance of this driver
  return ((Driver*)(new UniCap_Image (cf, section)));
}

////////////////////////////////////////////////////////////////////////////////
// Registers the driver in the driver table. Called from the
// player_driver_init function that the loader looks for
void unicapimage_Register (DriverTable* table)
{
  table->AddDriver ("unicapimage", UniCap_Image_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
UniCap_Image::UniCap_Image (ConfigFile* cf, int section)
    : Driver (cf, section)
{
  memset (&this->cam_id, 0, sizeof (player_devaddr_t));

  // Outgoing camera interface
  if (cf->ReadDeviceAddr(&(this->cam_id), section, "provides", PLAYER_CAMERA_CODE, -1, NULL) == 0)
  {
      if(this->AddInterface(this->cam_id) != 0)
      {
          this->SetError(-1);
          return;
      }
  }

  // Device ID number
  this->device_id = cf->ReadInt (section, "device", 0);
  if (!SUCCESS (unicap_enumerate_devices (NULL, &device, this->device_id)))
  {
    PLAYER_ERROR2 ("Could not get info for device %i: %s!", this->device_id, device.identifier);
    this->SetError (-1);
    return;
  }

  PLAYER_MSG2 (2, ">> UniCap_Image device at %i: %s", this->device_id, device.identifier);

  // Color space
  this->color_space = cf->ReadInt (section, "color_space", 0);

  // Video formats
  this->video_format = cf->ReadInt (section, "video_format", 0);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor.
UniCap_Image::~UniCap_Image()
{
  return;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int UniCap_Image::Setup ()
{
  PLAYER_MSG0 (1, "> UniCap_Image starting up... [done]");

  if ( !SUCCESS (unicap_open (&handle, &device)) )
  {
    PLAYER_ERROR1 ("Could not open device: %s!", device.identifier);
    return (-1);
  }
  
  // Set the desired format
  unicap_void_format (&format_spec);
//  format_spec.fourcc = FOURCC('U','Y','V','Y');

  // Get the list of video formats of the given colorformat
  for ( int i = 0; SUCCESS (unicap_enumerate_formats (handle, &format_spec, &format, i)); i++)
    PLAYER_MSG2 (2, "  Available color space %d: %s", i, format.identifier);
  
  if (!SUCCESS (unicap_enumerate_formats (handle, &format_spec, &format, this->color_space) ) )
  {
    PLAYER_ERROR1 ("Failed to set color space to %d!", this->color_space);
    return (-1);
  }
  else
    PLAYER_MSG2 (2, "Selected color space %d: %s", this->color_space, format.identifier);


  // If a video format has more than one size, ask for which size to use
   if (format.size_count )
   {
      for (int i = 0; i < format.size_count; i++)
         PLAYER_MSG3 (2, "  Available video format %d: %dx%d", i, format.sizes[i].width, format.sizes[i].height);
      format.size.width = format.sizes[this->video_format].width;
      format.size.height = format.sizes[this->video_format].height;
   }
   PLAYER_MSG3 (2, "Selected video format %d: [%dx%d]", this->video_format, format.size.width, format.size.height);

   if (!SUCCESS (unicap_set_format (handle, &format) ) )
   {
     PLAYER_ERROR1 ("Failed to set video format to %d!", this->video_format);
     return (-1);
   }
  
   // Start the capture process on the device
   if (!SUCCESS (unicap_start_capture (handle) ) )
   {
     PLAYER_ERROR1 ("Failed to start capture on device: %s", device.identifier);
     return (-1);
   }
  
  // Initialize the image buffer
  memset (&cam_data, 0, sizeof (cam_data));
  memset (&buffer,   0, sizeof (unicap_data_buffer_t));
  
  // Allocate buffer data
  buffer.data = (unsigned char*)(malloc (format.size.width * format.size.height * format.bpp / 8));
  buffer.buffer_size = format.size.width * format.size.height * format.bpp / 8;
  
  // Start the device thread
  StartThread ();

  return (0);
}

////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int UniCap_Image::Shutdown ()
{
  // Stop the driver thread
  StopThread ();

  // Stop the device
  if ( !SUCCESS (unicap_stop_capture (handle) ) )
    PLAYER_ERROR1 ("Failed to stop capture on device: %s\n", device.identifier);

  // Close the device
  if ( !SUCCESS (unicap_close (handle) ) )
    PLAYER_ERROR1 ("Failed to close the device: %s\n", device.identifier);

  free (buffer.data);
  PLAYER_MSG0 (1, "> UniCap_Image driver shutting down... [done]");
  return (0);
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void UniCap_Image::Main ()
{
  timespec sleepTime = {0, 1000};

  // The main loop; interact with the device here
  while (true)
  {
      nanosleep (&sleepTime, NULL);

      // test if we are supposed to cancel
      pthread_testcancel ();

      // Refresh data
      this->RefreshData ();
  }
}

void UniCap_Image::RefreshData ()
{
  // Queue the buffer
  // The buffer now gets filled with image data by the capture device
  if (!SUCCESS (unicap_queue_buffer (handle, &buffer) ) )
    return;

  // Wait until the image buffer is ready
  if (!SUCCESS (unicap_wait_buffer (handle, &returned_buffer) ) );
//    return;

  cam_data.width  = buffer.format.size.width;
  cam_data.height = buffer.format.size.height;
  
  // To do: implement the code for different formats later
  cam_data.format = PLAYER_CAMERA_FORMAT_MONO8;
  cam_data.image_count = buffer.buffer_size;
  cam_data.image = new unsigned char [cam_data.image_count];
  for (int i = 0; i < cam_data.image_count; i++)
    cam_data.image[i] = buffer.data[i];
  
  Publish (this->cam_id, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, &cam_data);
  delete [] cam_data.image;
}
