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

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_stoc stoc
 * @brief STOC

The stoc driver controls Videre Design's STOC (Stereo on a Chip) cameras
through the SVS (Small Vision System) library. The driver provides a @ref
interface_stereo interface.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_stereo : left and right images, disparity image, and a 3d
                          point cloud generated from the disparity
 
@par Requires

- none

@par Supported configuration requests

  - none

@par Properties provided

  -none yet
  
@par Configuration file options

  - capture_type (integer)
    - Default: SVS defaults
    - CAP_DUAL or CAP_INTERLACE
    
  - format (integer)
    - Default: SVS defaults
    - MONOCHROME, YUV, or RGB24
    
  - channel (integer)
    - Default: SVS defaults
    - 0, 1, 2 etc, video channel on card
    
  - swap (boolean)
    - Default: SVS defaults
    - Swapping left/right on or off
    
  - color_mode (integer)
    - Default: SVS defaults
    - Color mode: 0 (both), 1 (left only), 2 (right only)
    
  - color_alg (integer)
    - Default: SVS defaults
    - Color algorithm (see SVS manual)
    
  - rectification (boolean)
    - Default: SVS defaults
    - Image rectification: enabled (1) / disabled (0)
    
  - proc_mode (integer)
    - Default: SVS defaults
    - Processing mode for the STOC
    
  - rate (integer)
    - Default: SVS defaults
    - Image rate
    
  - frame_div (integer)
    - Default: SVS defaults
    - Image sampling
    
  - image_size (integer typle)
    - Default: 640 480
    - Image width and height

  - z_max (float)
    - Default: 5 (meters)
    - Cutoff distance on Z-axis
    
  - ndisp (integer)
    - Default: SVS defaults
    - Set number of disparities, 8 to 128
    
  - tex_thresh (integer)
    - Default: SVS defaults
    - Set texture filter threshold
    
  - unique (integer)
    - Default: SVS defaults
    - Set uniqueness filter threshold
    
  - corrsize (integer)
    - Default: SVS defaults
    - Set correlation window size, 7 to 21
    
  - horopter (integer)
    - Default: SVS defaults
    - Set horopter (X offset)
    
  - speckle_size (integer)
    - Default: SVS defaults
    - Set minimum disparity region size
  
  - speckle_diff (integer)
    - Default: SVS defaults
    - Set disparity region neighbor difference
    
  - cut_di (integer) 
    - Default: 0
    - Number of lines to "cut" (disconsider) at the bottom of the disparity image
    
  - multiproc_en (boolean)
    - Default: 0
    - Use SVS multiprocessing capabilities (see SVS manual)

@par Example

@verbatim
driver
(
  name "stoc"
  provides ["stereo:0"]
  
  ## ---[ Misc camera parameters: all -1 values assume SVS defaults
  # Color mode: 0 (both), 1 (left only), 2 (right only)
  color_mode 0
  # Color algorithm: COLOR_ALG_BEST (2), or COLOR_ALG_FAST (0) - check SVS manual
  color_alg 2
  
  # Processing mode for STOC: PROC_MODE_OFF (0), PROC_MODE_NONE (1), PROC_MODE_TEST (2),
  #                           PROC_MODE_RECTIFIED (3), PROC_MODE_DISPARITY (4), PROC_MODE_DISPARITY_RAW (5)
  # Check your STOC camera manual for extra details.
  proc_mode 5
  
  # Image rate in fps
  rate 15
  
  # Image rectification: enabled (1) / disabled (0)
  rectification 1

  ## ---[ Stereo parameters: all -1 values assume SVS defaults
  # Points to disconsider at the bottom of the disparity image (due to errors in SVS?)
  cut_di 32
  
  # Number of disparities, 8 to 128
  ndisp 64
  
  # Texture filter threshold (confidence)
  tex_thresh 4

  # Uniqueness filter threshold
  unique 3
  
  # Correlation window size, 7 to 21
  corrsize 15
  
  # Minimum disparity region size
  speckle_size 400
)
@endverbatim

@author Radu Bogdan Rusu
 */
/** @} */

#include <stdlib.h>
#include <libplayercore/playercore.h>
#include <SVS/svsclass.h>

#define IMAGE_WIDTH  640
#define IMAGE_HEIGHT 480

#define CUTOFF_DIST 5

class STOC:public Driver
{
  public:
    // constructor
    STOC (ConfigFile* cf, int section);
    ~STOC ();

    int Setup ();
    int Shutdown ();

    // MessageHandler
    virtual int ProcessMessage (QueuePointer &resp_queue,
                                player_msghdr * hdr,
                                void * data);

  private:

    virtual void Main ();
    void RefreshData  ();

    // device bookkeeping
    player_devaddr_t     stereo_addr;
    player_stereo_data_t stereo_data;
    
    int capture_type, format, channel;
    bool swap_mode;
    int color_mode, color_alg;
    int proc_mode, rate, frame_div, size_w, size_h;
    bool rectification, multiproc_en;
    float z_max;

    int ndisp, tex_thresh, unique, corrsize, horopter, speckle_size, speckle_diff;
    int cut_di;

    const char *parameter_file;  // user given parameter file for the camera
    // SVS objects
    svsVideoImages *video;
    svsStereoProcess *process;
    svsMultiProcess *multiproc;

  protected:
    // Properties
    IntProperty exposure, balance, gamma, brightness, saturation;

};

////////////////////////////////////////////////////////////////////////////////
//Factory creation function. This functions is given as an argument when
// the driver is added to the driver table
Driver*
  STOC_Init (ConfigFile* cf, int section)
{
  return ((Driver*)(new STOC (cf, section)));
}

////////////////////////////////////////////////////////////////////////////////
//Registers the driver in the driver table. Called from the
// player_driver_init function that the loader looks for
void
  stoc_Register (DriverTable* table)
{
  table->AddDriver ("stoc", STOC_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
STOC::STOC (ConfigFile* cf, int section)
	: Driver (cf, section),
	exposure ("exposure", 0, 0),
	balance ("balance", 0, 0),
	gamma ("gamma", 0, 0),
	brightness ("brightness", 0, 0),
	saturation ("saturation", 0, 0)
{
  memset (&this->stereo_addr, 0, sizeof (player_devaddr_t));

  this->RegisterProperty ("exposure", &this->exposure, cf, section);
  this->RegisterProperty ("balance", &this->balance, cf, section);
  this->RegisterProperty ("gamma", &this->gamma, cf, section);
  this->RegisterProperty ("brightness", &this->brightness, cf, section);
  this->RegisterProperty ("saturation", &this->saturation, cf, section);
  
  // Check whether the user provided a parameter file for the STOC
  parameter_file = cf->ReadString (section, "param_file", NULL);
  
  // SVS camera parameters
  capture_type = cf->ReadInt (section, "capture_type", -1);  // CAP_DUAL or CAP_INTERLACE
  format       = cf->ReadInt (section, "format", -1);        // MONOCHROME, YUV, or RGB24
  channel      = cf->ReadInt (section, "channel", -1);       // 0, 1, 2 etc, video channel on card
  swap_mode    = cf->ReadBool (section, "swap", false);      // Swapping left/right on or off
  color_mode   = cf->ReadInt (section, "color_mode", -1);    // Color mode: 0 (both), 1 (left only), 2 (right only)
  color_alg    = cf->ReadInt (section, "color_alg", -1);     // Color algorithm (see SVS manual)
  
  rectification = cf->ReadBool (section, "rectification", -1); // Image rectification: enabled (1) / disabled (0)
  proc_mode     = cf->ReadInt (section, "proc_mode", -1);      // Processing mode for STOC
  rate          = cf->ReadInt (section, "rate", -1);           // Image rate
  frame_div     = cf->ReadInt (section, "frame_div", -1);      // Image sampling
  size_w        = cf->ReadTupleInt (section, "image_size", 0, IMAGE_WIDTH);   // Image width
  size_h        = cf->ReadTupleInt (section, "image_size", 1, IMAGE_HEIGHT);  // Image height

  // Stereo parameters
  z_max = cf->ReadFloat (section, "z_max", CUTOFF_DIST);  // Cutoff distance on Z-axis

  ndisp        = cf->ReadInt (section, "ndisp", -1);          // Set number of disparities, 8 to 128
  tex_thresh   = cf->ReadInt (section, "tex_thresh", -1);     // Set texture filter threshold
  unique       = cf->ReadInt (section, "unique", -1);         // Set uniqueness filter threshold
  corrsize     = cf->ReadInt (section, "corrsize", -1);       // Set correlation window size, 7 to 21
  horopter     = cf->ReadInt (section, "horopter", -1);       // Set horopter (X offset)
  speckle_size = cf->ReadInt (section, "speckle_size", -1);   // Set minimum disparity region size
  speckle_diff = cf->ReadInt (section, "speckle_dif", -1);    // Set disparity region neighbor difference

  cut_di       = cf->ReadInt (section, "cut_di", 0);          // Cut points at the bottom of the disparity image

  multiproc_en = cf->ReadBool (section, "multiproc", 0);      // SVS multiproc (check manual): enabled (1) / disabled (0)

  if (cf->ReadDeviceAddr (&(this->stereo_addr), section, "provides", PLAYER_STEREO_CODE, -1, NULL) == 0)
  {
    if (this->AddInterface (this->stereo_addr) != 0)
    {
      this->SetError (-1);
      return;
    }
  }

  process   = new svsStereoProcess ();  // Compute the disparity image, and 3D points
  multiproc = new svsMultiProcess ();   // Multiscale processing
  video     = getVideoObject ();        // Get access to the video stream
}

////////////////////////////////////////////////////////////////////////////////
// Destructor.
STOC::~STOC ()
{
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int
  STOC::Setup ()
{
  int res;
  
  int nr_cam = video->Enumerate ();  // Get the number of cameras available

  // We only support one camera for now
  if (nr_cam == 0)
  {
    PLAYER_ERROR ("No camera found!");
    return (-1);
  }
  
  for (int i = 0; i < nr_cam; i++)
    PLAYER_MSG2 (0, "> Found camera %d: %s", i, video->DeviceIDs ()[i]);

  res = video->Open (nr_cam - 1);  // Open camera
  
  if (!res)
  {
    PLAYER_ERROR ("Could not connect to camera!");
    return (-1);
  }

  /// ---[ Set camera options
  if (parameter_file != NULL)
  {
    video->ReadParams ((char*)parameter_file);
    PLAYER_MSG1 (1, ">> Using camera parameters from %s", parameter_file);
  }
  else
  {
    if (capture_type != -1)
    {
      if (video->SetCapture (capture_type))
        PLAYER_MSG1 (2, ">> Set capture type to %d", capture_type);
      else
        PLAYER_ERROR1 (">> Error while setting capture type to %d!", capture_type);
    }

    if (format != -1)
    {
      if (video->SetFormat (format))
        PLAYER_MSG1 (2, ">> Set format type to %d", format);
      else
        PLAYER_ERROR1 (">> Error setting format type to %d!", format);
    }

    if (channel != -1)
    {
      if (video->SetChannel (channel))
        PLAYER_MSG1 (2, ">> Set channel type to %d", channel);
      else
        PLAYER_ERROR1 (">> Error setting channel type to %d!", channel);
    }

    if (swap_mode)
    {
      if (video->SetSwap (swap_mode))
        PLAYER_MSG0 (2, ">> Swap mode enabled");
      else
        PLAYER_ERROR (">> Error enabling swap mode!");
    }

    if (color_mode != -1)
    {
      bool r;
      if (color_mode == 0) r = video->SetColor (true, true);
      if (color_mode == 1) r = video->SetColor (true, false);
      if (color_mode == 2) r = video->SetColor (false, true);
      if (r)
        PLAYER_MSG1 (2, ">> Color mode set to %d", color_mode);
      else
        PLAYER_ERROR (">> Error setting color mode to %d!");
    }

    if (color_alg != -1)
    {
      if (video->SetColorAlg (color_alg))
        PLAYER_MSG1 (2, ">> Color algorithm set to %d", color_alg);
      else
        PLAYER_ERROR1 (">> Error setting color algorithm to %d!", color_alg);
    }

    if (video->SetSize (size_w, size_h))
      PLAYER_MSG2 (2, ">> Image size set to %dx%d", size_w, size_h);
    else
      PLAYER_ERROR2 (">> Error setting image size to %dx%d!", size_w, size_h);

    if (frame_div != -1)
    {
      if (video->SetFrameDiv (frame_div))
        PLAYER_MSG1 (2, ">> Image sampling set to %d", frame_div);
      else
        PLAYER_ERROR1 (">> Error setting image sampling to %d!", frame_div);
    }

    if (rate != -1)
    {
      if (video->SetRate (rate))
        PLAYER_MSG1 (2, ">> Image rate set to %d", rate);
      else
        PLAYER_ERROR1 (">> Error setting image rate to %d!", rate);
    }

    if (proc_mode != -1)
    {
      if (video->SetProcMode (proc_mode_type(proc_mode)))
        PLAYER_MSG1 (2, ">> STOC processing mode set to %d", proc_mode);
      else
        PLAYER_ERROR1 (">> Error setting STOC processing mode to %d!", proc_mode);
    }

    if (rectification != -1)
    {
      if (video->SetRect (rectification))
        PLAYER_MSG1 (2, ">> Image rectification set to %d", rectification);
      else
        PLAYER_ERROR1 (">> Error setting image rectification to %d!", rectification);
    }
  }
  video->binning = 1;
  
  /// ---[ Stereo options
  if (cut_di != 0)
    PLAYER_MSG1 (2, ">> [stereo] Disconsidering the last %d lines from the bottom of the disparity image...", cut_di);
  if (ndisp != -1)
  {
    video->SetNDisp (ndisp);
    PLAYER_MSG1 (2, ">> [stereo] Number of disparities set to %d", ndisp);
  }
  if (tex_thresh != -1)
  {
    video->SetThresh (tex_thresh);
    PLAYER_MSG1 (2, ">> [stereo] Texture filter threshold set to %d", tex_thresh);
  }
  if (unique != -1)
  {
    video->SetUnique (unique);
    PLAYER_MSG1 (2, ">> [stereo] Uniqueness filter threshold set to %d", unique);
  }
  if (corrsize != -1)
  {
    video->SetCorrsize (corrsize);
    PLAYER_MSG1 (2, ">> [stereo] Correlation window size set to %d", corrsize);
  }
  if (horopter != -1)
  {
    video->SetHoropter (horopter);
    PLAYER_MSG1 (2, ">> [stereo] Horopter (X-Offset) value set to %d", horopter);
  }
  if (speckle_size != -1)
  {
    video->SetSpeckleSize (speckle_size);
    PLAYER_MSG1 (2, ">> [stereo] Minimum disparity region size set to %d", speckle_size);
  }
  if (speckle_diff != -1)
  {
    video->SetSpeckleDiff (speckle_diff);
    PLAYER_MSG1 (2, ">> [stereo] Disparity region neighbor diff to %d", speckle_diff);
  }

  PLAYER_MSG0 (0, "> Connected to camera");
  
  // Start video streaming
  res = video->Start ();
    
  StartThread ();
  return (0);
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int
  STOC::Shutdown ()
{
  StopThread ();

  int res = video->Stop ();   // Stop video streaming
  res = video->Close ();  // Close camera
  PLAYER_MSG0 (1, "> Closed camera connection.");
  return (0);
}

////////////////////////////////////////////////////////////////////////////////
// ProcessMessage
int
  STOC::ProcessMessage (QueuePointer &resp_queue,
                        player_msghdr * hdr,
                        void * data)
{
  assert (hdr);
  assert (data);

  // To do: offset, exposure, autoexposure parameters, balance, gamma, brightness, saturation
  //        level and horopter
  
  return (0);
}



////////////////////////////////////////////////////////////////////////////////
void
  STOC::Main ()
{
  for (;;)
  {
    // handle commands and requests/replies --------------------------------
    pthread_testcancel ();
    ProcessMessages ();
    RefreshData ();
  }
}

////////////////////////////////////////////////////////////////////////////////
// RefreshData function
void
  STOC::RefreshData ()
{
  int res;
  
  svsStereoImage *si = video->GetImage (10); // 10 ms timeout
  if (si == NULL)
    PLAYER_WARN ("No image, timed out...");
  
  // Compute the disparity image
  if (multiproc_en)
    multiproc->CalcStereo (si);
  else
    process->CalcStereo (si);
  
  // Trim down those nasty points at the bottom of the disparity image (errors?)
  for (int i = si->ip.height - cut_di; i < si->ip.height; i++)
    for (int j = 0; j < si->ip.width; j++)
      si->disparity[i * si->ip.width + j] = -2;

  // Compute the 3D point cloud information
  if (multiproc_en)
    multiproc->Calc3D (si, 0, 0, 0, 0, NULL, NULL, z_max);
  else
    process->Calc3D (si, 0, 0, 0, 0, NULL, NULL, z_max);

  // Save the 3D point cloud data in the structure, if present
  int nr_points = 0;
  stereo_data.points = NULL;
  if (si->have3D)
  {
    stereo_data.points = new player_pointcloud3d_stereo_element_t[si->ip.height * si->ip.width];

    svs3Dpoint *pts = si->pts3D;
    for (int i = 0; i < si->ip.height; i++)
    {
      for (int j = 0; j < si->ip.width; j++, pts++)
      {
        // Check if the point is valid
        if (pts->A <= 0)
          continue;

        player_pointcloud3d_stereo_element_t point;
        point.px = pts->X;
        point.py = pts->Y;
        point.pz = pts->Z;

        if (si->haveColor)
        {
          svsColorPixel *mpc = (svsColorPixel*)(si->color + (i*si->ip.width + j) * 4);
          point.red   = mpc->r;  // red
          point.green = mpc->g;  // green
          point.blue  = mpc->b;  // blue
        }
        else
        {
          point.red = point.green = point.blue = (unsigned char)si->Left ()[i*si->ip.width + j];
        }
        stereo_data.points[nr_points] = point;
        nr_points++;
      } // width
    } // height

  } // have3D
  stereo_data.points_count = nr_points;
  
  // Save the stereo left/right image
  stereo_data.left_channel.height = stereo_data.right_channel.height = si->ip.height;
  stereo_data.left_channel.width  = stereo_data.right_channel.width  = si->ip.width;
  stereo_data.left_channel.fdiv   = stereo_data.right_channel.fdiv   = frame_div;
  stereo_data.left_channel.compression = stereo_data.right_channel.compression = PLAYER_CAMERA_COMPRESS_RAW;
  stereo_data.left_channel.image_count = stereo_data.left_channel.width * stereo_data.left_channel.height;
  stereo_data.right_channel.image_count = stereo_data.left_channel.width * stereo_data.left_channel.height;

  // Set MONO by default on both left and right
  stereo_data.left_channel.format = PLAYER_CAMERA_FORMAT_MONO8;
  stereo_data.left_channel.bpp    = 8;
  stereo_data.left_channel.image  = si->Left ();
  stereo_data.right_channel.format = PLAYER_CAMERA_FORMAT_MONO8;
  stereo_data.right_channel.bpp    = 8;
  stereo_data.right_channel.image  = si->Right ();

  uint8_t *in_left, *in_right, *out_left, *out_right;
  // Check if <left> has color or monochrome
  if (si->haveColor)
  {
    stereo_data.left_channel.format = PLAYER_CAMERA_FORMAT_RGB888;
    stereo_data.left_channel.bpp    = 24;
    stereo_data.left_channel.image_count *= 3;
    in_left = (uint8_t*)si->Color ();
    stereo_data.left_channel.image = new unsigned char [stereo_data.left_channel.image_count];
    out_left  = (uint8_t*)stereo_data.left_channel.image;
  }

  // Check if <right> has color or monochrome
  if (si->haveColorRight)
  {
    stereo_data.right_channel.format = PLAYER_CAMERA_FORMAT_RGB888;
    stereo_data.right_channel.bpp    = 24;
    stereo_data.right_channel.image_count *= 3;
    in_right = (uint8_t*)si->ColorRight ();
    stereo_data.right_channel.image = new unsigned char [stereo_data.right_channel.image_count];
    out_right = (uint8_t*)stereo_data.right_channel.image;
  }
  
  for (int i = 0; i < (si->ip.height * si->ip.width); i++)
  {
    if (si->haveColor)
    {
      // Left
      out_left[0] = in_left[0];
      out_left[1] = in_left[1];
      out_left[2] = in_left[2];
      out_left += 3;
      in_left  += 4;
    }
    if (si->haveColorRight)
    {
      // Right
      out_right[0] = in_right[0];
      out_right[1] = in_right[1];
      out_right[2] = in_right[2];
      out_right += 3;
      in_right  += 4;
    }
  }

  // Save the disparity image
  stereo_data.disparity.width       = stereo_data.disparity.height = 0;
  stereo_data.disparity.bpp         = 16;
  stereo_data.disparity.fdiv        = 1;
  stereo_data.disparity.compression = PLAYER_CAMERA_COMPRESS_RAW;
  stereo_data.disparity.format      = PLAYER_CAMERA_FORMAT_MONO16;
  
  if (si->haveDisparity)
  {
    stereo_data.disparity.width  = si->dp.dwidth;
    stereo_data.disparity.height = si->dp.dheight;
    stereo_data.disparity.image = (uint8_t*)si->disparity;
  }
  stereo_data.disparity.image_count = stereo_data.disparity.width * stereo_data.disparity.height * 2;
  
  // Stereo data mode
  stereo_data.mode = 1 + 2 + 4;      // left + right + disparity

  // Publish the stereo data
  Publish (stereo_addr, PLAYER_MSGTYPE_DATA, PLAYER_STEREO_DATA_STATE, (void*)&stereo_data);
  
  // Cleanup
  if (stereo_data.points != NULL)
    delete [] stereo_data.points;
  if (si->haveColor)
    delete [] stereo_data.left_channel.image;
  if (si->haveColorRight)
    delete [] stereo_data.right_channel.image;
  
  return;
}
