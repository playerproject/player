/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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
 * Desc: Driver for writing log files.
 * Author: Andrew Howard
 * Date: 14 Jun 2003
 * CVS: $Id$
 *
 */

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_writelog writelog

The writelog driver will write data from another device to a log file.
The @ref player_driver_readlog driver can be used to replay the data
(to client programs, the replayed data will appear to come from the
real sensors).

For help in controlling logging, try @ref player_util_playervcr.
Note that you must declare a @ref player_interface_log device to allow
logging control.

@par Compile-time dependencies

- none

@par Provides

- @ref player_interface_log : can be used to turn logging on/off

@par Requires

The writelog driver takes as input a list of devices to log data from.
The driver with the <b>highest data rate</b> should be placed first in the list.
The writelog driver can will log data from the following interfaces:

- @ref player_interface_blobfinder
- @ref player_interface_camera
- @ref player_interface_fiducial
- @ref player_interface_gps
- @ref player_interface_joystick
- @ref player_interface_laser
- @ref player_interface_position
- @ref player_interface_position3d
- @ref player_interface_power
- @ref player_interface_truth
- @ref player_interface_wifi

@par Configuration requests

- PLAYER_LOG_SET_WRITE_STATE
- PLAYER_LOG_GET_STATE

@par Configuration file options

- filename (string)
  - Default: "writelog_YYYY_MM_DD_HH_MM.log", where YYYY is the year,
    MM is the month, etc.
  - Name of logfile.
- enable (integer)
  - Default: 0
  - Default log state; set to 1 for continous logging.
- camera_save_images (integer)
  - Default: 0
  - Save camera data to image files as well as to the log file.  The image
    files are named "writelog_YYYY_MM_DD_HH_MM_camera_II_NNNNNNN.pnm",
    where II is the device index and NNNNNNN is the frame number.
      
@par Example 

@verbatim
driver
(
  name "writelog"
  requires ["laser:0" "position:0"]
  provides ["log:0"]
  alwayson 1
  enable 1
)
@endverbatim
*/
/** @} */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "player.h"
#include "error.h"
#include "driver.h"
#include "devicetable.h"
#include "drivertable.h"
#include "deviceregistry.h"
#include "playertime.h"
#include "encode.h"


// Utility class for storing per-device info
class WriteLogDevice
{
  public: player_device_id_t id;
  public: Driver *device;
  public: struct timeval time;
  public: int cameraFrame;
};


// The logfile driver
class WriteLog: public Driver 
{
  // Constructor
  public: WriteLog(ConfigFile* cf, int section);

  // Destructor
  public: ~WriteLog();

  // Initialize the driver
  public: virtual int Setup();

  // Finalize the driver
  public: virtual int Shutdown();

  // Process configuration requests
  public: virtual int PutConfig(player_device_id_t id, void *client, 
                                void* src, size_t len,
                                struct timeval* timestamp);

  // Device thread
  private: virtual void Main(void);

  // Write data to file
  private: void Write(WriteLogDevice *device, void *data, size_t size, struct timeval time);

  // Write blobfinder data to file
  private: void WriteBlobfinder(player_blobfinder_data_t *data);

  // Write camera data to file
  private: void WriteCamera(player_camera_data_t *data);

  // Write camera data to image file as well 
  private: void WriteCameraImage(WriteLogDevice *device, player_camera_data_t *data, struct timeval *ts);

  // Write fiducial data to file
  private: void WriteFiducial(player_fiducial_data_t *data);
  
  // Write GPS data to file
  private: void WriteGps(player_gps_data_t *data);

  // Write joystick data to file
  private: void WriteJoystick(player_joystick_data_t *data);

  // Write laser data to file
  private: void WriteLaser(player_laser_data_t *data);

  // Write position data to file
  private: void WritePosition(player_position_data_t *data);

  // Write position3d data to file
  private: void WritePosition3d(player_position3d_data_t *data);

  // Write power data to file
  private: void WritePower(player_power_data_t *data);

  // Write truth data to file
  private: void WriteTruth(player_truth_data_t *data);

  // Write wifi data to file
  private: void WriteWiFi(player_wifi_data_t *data);

  // File to read data from
  private: char default_basename[1024];
  private: char default_filename[1024];
  private: const char *filename;
  private: FILE *file;

  // Subscribed device list
  private: int device_count;
  private: WriteLogDevice devices[1024];

  // Is writing enabled? (client can start/stop)
  private: bool enable;
  private: bool enable_default;

  // Save camera frames to image files as well?
  private: bool cameraSaveImages;
};


////////////////////////////////////////////////////////////////////////////
// Pointer to the one-and-only time
extern PlayerTime* GlobalTime;
extern int global_playerport;


////////////////////////////////////////////////////////////////////////////
// Create a driver for reading log files
Driver* WriteLog_Init(ConfigFile* cf, int section)
{
  return ((Driver*) (new WriteLog(cf, section)));
}


////////////////////////////////////////////////////////////////////////////
// Device factory registration
void WriteLog_Register(DriverTable* table)
{
  table->AddDriver("writelog", WriteLog_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////
// Constructor
WriteLog::WriteLog(ConfigFile* cf, int section)
    : Driver(cf, section, PLAYER_LOG_CODE, PLAYER_ALL_MODE,
             PLAYER_MAX_PAYLOAD_SIZE, PLAYER_MAX_PAYLOAD_SIZE, 1, 1)
{
  int i;
  player_device_id_t id;
  WriteLogDevice *device;
  time_t t;
  struct tm *ts;
  
  this->file = NULL;

  // Construct default filename from date and time.  Note that we use
  // the system time, *not* the Player time.  I think that this is the
  // correct semantics for working with simulators.
  time(&t);
  ts = localtime(&t);
  strftime(this->default_basename, sizeof(this->default_filename),
           "writelog_%Y_%m_%d_%H_%M", ts);
  snprintf(this->default_filename, sizeof(this->default_filename), "%s.log",
           this->default_basename);

  // Let user override default filename
  this->filename = cf->ReadString(section, "filename", this->default_filename);

  // Default enabled?
  if(cf->ReadInt(section, "enable", 1) > 0)
    this->enable_default = true;
  else
    this->enable_default = false;

  this->device_count = 0;
  
  // Get a list of input devices
  for (i = 0; i < cf->GetTupleCount(section, "requires"); i++)
  {
    if (cf->ReadDeviceId(&id, section, "requires", -1, i, NULL) != 0)
    {
      this->SetError(-1);
      return;
    }
    
    // Add to our device table
    assert(this->device_count < (int) (sizeof(this->devices) / sizeof(this->devices[0])));
    device = this->devices + this->device_count++;
    device->id = id;
    device->device = NULL;
    device->time.tv_sec = 0;
    device->time.tv_usec = 0;
    device->cameraFrame = 0;
  }

  // Camera specific settings
  this->cameraSaveImages = cf->ReadInt(section, "camera_save_images", 0);

  return;
}


////////////////////////////////////////////////////////////////////////////
// Destructor
WriteLog::~WriteLog()
{
  return;
}


////////////////////////////////////////////////////////////////////////////
// Initialize driver
int WriteLog::Setup()
{
  int i;
  WriteLogDevice *device;

  // Subscribe to the underlying devices
  for (i = 0; i < this->device_count; i++)
  {
    device = this->devices + i;

    device->device = deviceTable->GetDriver(device->id);
    if (!device->device)
    {
      PLAYER_ERROR3("unable to locate device [%d:%s:%d] for logging",
                    device->id.port,
                    ::lookup_interface_name(0, device->id.code),
                    device->id.index);
      return -1;
    }
    if (device->device->Subscribe(device->id) != 0)
    {
      PLAYER_ERROR("unable to subscribe to device for logging");
      return -1;
    }
  }

  // Open the file
  this->file = fopen(this->filename, "w+");
  if (this->file == NULL)
  {
    PLAYER_ERROR2("unable to open [%s]: %s\n", this->filename, strerror(errno));
    return -1;
  }

  // Write the file header
  fprintf(this->file, "## Player version %s \n", VERSION);
  fprintf(this->file, "## File version %s \n", "0.1.0");

  // Enable/disable logging, according to default set in config file
  this->enable = this->enable_default;

  // Start device thread
  this->StartThread();
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Finalize the driver
int WriteLog::Shutdown()
{
  int i;
  WriteLogDevice *device;

  // Stop the device thread
  this->StopThread();

  // Close the file
  if (this->file)
    fclose(this->file);
  this->file = NULL;

  // Unsubscribe to the underlying devices
  for (i = 0; i < this->device_count; i++)
  {
    device = this->devices + i;

    // Unsbscribe from the underlying device
    device->device->Unsubscribe(device->id);
    device->device = NULL;
  }
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Process configuration requests
int WriteLog::PutConfig(player_device_id_t id, void *client, 
                        void* src, size_t len,
                        struct timeval* timestamp)
{
  player_log_set_write_state_t sreq;
  player_log_get_state_t greq;
  uint8_t subtype;

  if(len < sizeof(sreq.subtype))
  {
    PLAYER_WARN2("request was too small (%d < %d)",
                  len, sizeof(sreq.subtype));
    if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
      PLAYER_ERROR("PutReply() failed");
  }

  subtype = ((player_log_set_write_state_t*)src)->subtype;
  switch(subtype)
  {
    case PLAYER_LOG_SET_WRITE_STATE_REQ:
      if(len != sizeof(sreq))
      {
        PLAYER_WARN2("request wrong size (%d != %d)", len, sizeof(sreq));
        if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
      }
      sreq = *((player_log_set_write_state_t*)src);
      if(sreq.state)
      {
        puts("WriteLog: start logging");
        this->enable = true;
      }
      else
      {
        puts("WriteLog: stop logging");
        this->enable = false;
      }
      if (this->PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    case PLAYER_LOG_GET_STATE_REQ:
      if(len != sizeof(greq.subtype))
      {
        PLAYER_WARN2("request wrong size (%d != %d)", 
                     len, sizeof(greq.subtype));
        if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
      }
      greq = *((player_log_get_state_t*)src);
      greq.type = PLAYER_LOG_TYPE_WRITE;
      if(this->enable)
        greq.state = 1;
      else
        greq.state = 0;

      if(this->PutReply(client, PLAYER_MSGTYPE_RESP_ACK,
                        &greq, sizeof(greq),NULL) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    default:
      PLAYER_WARN1("got request of unknown subtype %u", subtype);
      if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
  }

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void WriteLog::Main(void)
{
  int i;
  size_t size, maxsize;
  struct timeval time, sync_time, elapsed_time;
  void *data;
  WriteLogDevice *device;
  
  maxsize = PLAYER_MAX_MESSAGE_SIZE;
  data = malloc(maxsize);

  sync_time.tv_sec = 0;
  sync_time.tv_usec = 0;

  while (1)
  {
    pthread_testcancel();
    
    // Wait on the first device
    assert(this->device_count > 0);
    device = this->devices;
    device->device->Wait();

    // If logging is stopped, then don't log
    if(!this->enable)
      continue;

    // Walk the device list
    for (i = 0; i < this->device_count; i++)
    {
      device = this->devices + i;
      
      // Read data from underlying device
      size = device->device->GetData(device->id, (void*) data, maxsize, &time);
      assert(size < maxsize);

      // Check for new data
      if((device->time.tv_sec == time.tv_sec) && 
         (device->time.tv_usec == time.tv_usec))
        continue;
      device->time = time;
  
      // Write data to file
      this->Write(device, data, size, time);
    }

    /// Write the sync packet at 10Hz; it's just a heartbeat
    GlobalTime->GetTime(&time);
    TIMESUB(&time, &sync_time, &elapsed_time);
    if (elapsed_time.tv_usec > 100000)
    {
      sync_time = time;
      this->Write(NULL, NULL, 0, sync_time);
    }
  }

  free(data);
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Write data to file
void WriteLog::Write(WriteLogDevice *device, void *data, size_t size, struct timeval time)
{
  char host[256];
  int port, index;
  player_interface_t iface;
  struct timeval stime;

  // Get server time
  GlobalTime->GetTime(&stime);

  // Get interface name
  if (device)
  {
    lookup_interface_code(device->id.code, &iface);
    index = device->id.index;
  }
  else
  {
    iface.name = "sync";
    iface.code = PLAYER_PLAYER_CODE;
    index = 0;
    time = stime;
  }

  gethostname(host, sizeof(host));
  port = global_playerport;
  
  // Write header info
  fprintf(this->file, "%014.3f %s %d %s %02d %014.3f ",
           (double) stime.tv_sec + (double) stime.tv_usec * 1e-6,
           host, port, iface.name, index,
           (double) time.tv_sec + (double) time.tv_usec * 1e-6);

  // Write the data
  switch (iface.code)
  {
    case PLAYER_BLOBFINDER_CODE:
      this->WriteBlobfinder((player_blobfinder_data_t*) data);
      break;
    case PLAYER_CAMERA_CODE:
      this->WriteCamera((player_camera_data_t*) data);
      if (this->cameraSaveImages)
        this->WriteCameraImage(device, (player_camera_data_t*) data, &time);
      break;
    case PLAYER_FIDUCIAL_CODE:
      this->WriteFiducial((player_fiducial_data_t*) data);
      break;
    case PLAYER_GPS_CODE:
      this->WriteGps((player_gps_data_t*) data);
      break;
    case PLAYER_JOYSTICK_CODE:
      this->WriteJoystick((player_joystick_data_t*) data);
      break;
    case PLAYER_LASER_CODE:
      this->WriteLaser((player_laser_data_t*) data);
      break;
    case PLAYER_POSITION_CODE:
      this->WritePosition((player_position_data_t*) data);
      break;
    case PLAYER_POSITION3D_CODE:
      this->WritePosition3d((player_position3d_data_t*) data);
      break;
    case PLAYER_POWER_CODE:
      this->WritePower((player_power_data_t*) data);
      break;
    case PLAYER_TRUTH_CODE:
      this->WriteTruth((player_truth_data_t*) data);
      break;
    case PLAYER_WIFI_CODE:
      this->WriteWiFi((player_wifi_data_t*) data);
      break;
    case PLAYER_PLAYER_CODE:
      break;
    default:
      PLAYER_WARN1("unsupported interface type [%s]", ::lookup_interface_name(0, iface.code));
      break;
  }

  fprintf(this->file, "\n");

  // Flush the data (some drivers produce a lot of data; we dont want
  // it to back up and slow us down later).
  fflush(this->file);
  
  return;
}



////////////////////////////////////////////////////////////////////////////
// Signed int conversion macros
#define HINT16(x) ((int16_t) ntohs(x))
#define HUINT16(x) ((uint16_t) ntohs(x))
#define HINT32(x) ((int32_t) ntohl(x))
#define HUINT32(x) ((uint32_t) ntohl(x))


////////////////////////////////////////////////////////////////////////////
// Unit conversion macros
#define CM_M(x) ((x) / 100.0)
#define MM_M(x) ((x) / 1000.0)
#define DEG_RAD(x) ((x) * M_PI / 180.0)



////////////////////////////////////////////////////////////////////////////
// Write blobfinder data to file
void WriteLog::WriteBlobfinder(player_blobfinder_data_t *data)
{
  // format: width height count [id color area x y left right top bottom range] ...

  fprintf(this->file, " %d %d %d",
          HUINT16(data->width),
          HUINT16(data->height),
          HUINT16(data->blob_count));

  for(int i=0; i < HUINT16(data->blob_count); i++)
  {
    fprintf(this->file, " %d %d %d %d %d %d %d %d %d %f",
            HINT16(data->blobs[i].id),
            HUINT32(data->blobs[i].color),
            HUINT32(data->blobs[i].area),
            HUINT16(data->blobs[i].x),
            HUINT16(data->blobs[i].y),
            HUINT16(data->blobs[i].left),
            HUINT16(data->blobs[i].right),
            HUINT16(data->blobs[i].top),
            HUINT16(data->blobs[i].bottom),
            MM_M(HUINT16(data->blobs[i].range)));
  }
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Write camera data to file
void WriteLog::WriteCamera(player_camera_data_t *data)
{
  char *str;
  size_t src_size, dst_size;

  // Image format
  fprintf(this->file, "%d %d %d %d %d %d " ,
          HUINT16(data->width), HUINT16(data->height),
          data->bpp, data->format, data->compression,
          HUINT32(data->image_size));
  
  // Check image size
  src_size = HUINT32(data->image_size);
  dst_size = ::EncodeHexSize(src_size);
  str = (char*) malloc(dst_size + 1);

  // Encode image into string
  ::EncodeHex(str, dst_size, data->image, src_size);

  // Write image bytes
  fprintf(this->file, str);
  free(str);
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Write camera data to image file as well 
void WriteLog::WriteCameraImage(WriteLogDevice *device, player_camera_data_t *data, struct timeval *time)
{
  FILE *file;
  char filename[1024];

  if (data->compression != PLAYER_CAMERA_COMPRESS_RAW)
  {
    PLAYER_WARN("unsupported compression method");
    return;
  }
  
  snprintf(filename, sizeof(filename), "%s_camera_%02d_%06d.pnm",
           this->default_basename, device->id.index, device->cameraFrame++);
  
  file = fopen(filename, "w+");
  if (file == NULL)
    return;

  if (data->format == PLAYER_CAMERA_FORMAT_RGB888)
  {
    // Write ppm header
    fprintf(file, "P6\n%d %d\n%d\n", HUINT16(data->width), HUINT16(data->height), 255);
    fwrite(data->image, 1, HUINT32(data->image_size), file);
  }
  else if (data->format == PLAYER_CAMERA_FORMAT_MONO8)
  {
    // Write pgm header
    fprintf(file, "P5\n%d %d\n%d\n", HUINT16(data->width), HUINT16(data->height), 255);
    fwrite(data->image, 1, HUINT32(data->image_size), file);
  }
  else
  {
    PLAYER_WARN("unsupported image format");
  }
  
  fclose(file);
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Write fiducial data to file
void WriteLog::WriteFiducial(player_fiducial_data_t *data)
{
  // format: <count> [<id> <x> <y> <z> <roll> <pitch> <yaw> <ux> <uy> <uz> etc] ...
  fprintf(this->file, "%d", HUINT16(data->count));
  for(int i=0;i<HUINT16(data->count);i++)
  {
    fprintf(this->file, " %d"
            " %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f"
            " %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f",
            HINT16(data->fiducials[i].id),
            MM_M(HINT32(data->fiducials[i].pos[0])),
            MM_M(HINT32(data->fiducials[i].pos[1])),
            MM_M(HINT32(data->fiducials[i].pos[2])),
            MM_M(HINT32(data->fiducials[i].rot[0])),
            MM_M(HINT32(data->fiducials[i].rot[1])),
            MM_M(HINT32(data->fiducials[i].rot[2])),
            MM_M(HINT32(data->fiducials[i].upos[0])),
            MM_M(HINT32(data->fiducials[i].upos[1])),
            MM_M(HINT32(data->fiducials[i].upos[2])),
            MM_M(HINT32(data->fiducials[i].urot[0])),
            MM_M(HINT32(data->fiducials[i].urot[1])),
            MM_M(HINT32(data->fiducials[i].urot[2])));
  }
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Write GPS data to file
void WriteLog::WriteGps(player_gps_data_t *data)
{
  fprintf(this->file,
          "%.3f "
          "%.6f %.6f %.6f "
          "%.3f %.3f "
          "%.3f %.3f %.3f "
          "%d %d",
          (double) (uint32_t) HINT32(data->time_sec) +
          (double) (uint32_t) HINT32(data->time_sec) * 1e-6,
          (double) HINT32(data->latitude) / (60 * 60 * 60),
          (double) HINT32(data->longitude) / (60 * 60 * 60),
          MM_M(HINT32(data->altitude)),
          CM_M(HINT32(data->utm_e)),
          CM_M(HINT32(data->utm_n)), 
          (double) HINT16(data->hdop) / 10,
          MM_M(HINT32(data->err_horz)),
          MM_M(HINT32(data->err_vert)),
          (int) data->quality,
          (int) data->num_sats);
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Write joystick data to file
void WriteLog::WriteJoystick(player_joystick_data_t *data)
{
  fprintf(this->file, "%+d %+d %d %d %X",
          HINT16(data->xpos),
          HINT16(data->ypos),
          HINT16(data->xscale),
          HINT16(data->yscale),
          HUINT16(data->buttons));

  return;
}


////////////////////////////////////////////////////////////////////////////
// Write laser data to file
void WriteLog::WriteLaser(player_laser_data_t *data)
{
  int i;

  // Note that, in this format, we need a lot of precision in the
  // resolution field.
  
  fprintf(this->file, "%+07.4f %+07.4f %+.8f %04d ",
          DEG_RAD(HINT16(data->min_angle) * 0.01), DEG_RAD(HINT16(data->max_angle) * 0.01),
          DEG_RAD(HUINT16(data->resolution) * 0.01), HUINT16(data->range_count));

  for (i = 0; i < ntohs(data->range_count); i++)
    fprintf(this->file, "%.3f %2d ",
            MM_M(HUINT16(data->ranges[i]) * HUINT16(data->range_res)),
            data->intensity[i]);

  return;
}


////////////////////////////////////////////////////////////////////////////
// Write position data to file
void WriteLog::WritePosition(player_position_data_t *data)
{
  fprintf(this->file, "%+07.3f %+07.3f %+04.3f %+07.3f %+07.3f %+07.3f %d",
          MM_M(HINT32(data->xpos)),
          MM_M(HINT32(data->ypos)),
          DEG_RAD(HINT32(data->yaw)),
          MM_M(HINT32(data->xspeed)),
          MM_M(HINT32(data->yspeed)),
          DEG_RAD(HINT32(data->yawspeed)),
          data->stall);

  return;
}


////////////////////////////////////////////////////////////////////////////
// Write position3d data to file
void WriteLog::WritePosition3d(player_position3d_data_t *data)
{
  fprintf(this->file,
          "%+.4f %+.4f %+.4f "
          "%+.4f %+.4f %+.4f "
          "%+.4f %+.4f %+.4f "
          "%+.4f %+.4f %+.4f "
          "%d",
          MM_M(HINT32(data->xpos)),
          MM_M(HINT32(data->ypos)),
          MM_M(HINT32(data->zpos)),
          HINT32(data->roll) / 1000.0,
          HINT32(data->pitch) / 1000.0,
          HINT32(data->yaw) / 1000.0,
          MM_M(HINT32(data->xspeed)),
          MM_M(HINT32(data->yspeed)),
          MM_M(HINT32(data->zspeed)),
          HINT32(data->rollspeed) / 1000.0,
          HINT32(data->pitchspeed) / 1000.0,
          HINT32(data->yawspeed) / 1000.0,
          data->stall);

  return;
}


////////////////////////////////////////////////////////////////////////////
// Write power data to file
void WriteLog::WritePower(player_power_data_t *data)
{
  fprintf(this->file, "%.1f ", HUINT16(data->charge) / 10.0);
  return;
}


////////////////////////////////////////////////////////////////////////////
// Write wifi data to file
void WriteLog::WriteWiFi(player_wifi_data_t *data)
{
  int i;
  char *mac, *ip, *essid;
  
  fprintf(this->file, "%04d ", HUINT16(data->link_count));

  for (i = 0; i < ntohs(data->link_count); i++)
  {
    mac = data->links[i].mac;
    if (strlen(mac) == 0)
      mac = "''";

    ip = data->links[i].ip;
    if (strlen(ip) == 0)
      ip = "''";

    essid = data->links[i].essid;
    if (strlen(essid) == 0)
      essid = "''";

    fprintf(this->file, "'%s' '%s' '%s' %d %d %d %d %d %d ",
            mac, ip, essid,
            data->links[i].mode, HUINT16(data->links[i].freq),
            data->links[i].encrypt,
            HINT16(data->links[i].qual),
            HINT16(data->links[i].level),
            HINT16(data->links[i].noise));
  }

  return;
}


////////////////////////////////////////////////////////////////////////////
// Write truth data to file
void WriteLog::WriteTruth(player_truth_data_t *data)
{
  fprintf(this->file, "%+07.3f %+07.3f %+04.3f",
          MM_M(HINT32(data->px)),
          MM_M(HINT32(data->py)),
          DEG_RAD(HINT32(data->pa)));
  
  return;
}


