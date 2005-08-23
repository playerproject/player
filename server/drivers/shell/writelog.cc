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
Each data message is written to a separate line.  The format for each
supported interface is given below.

The @ref player_driver_readlog driver can be used to replay the data
(to client programs, the replayed data will appear to come from the
real sensors).

The writelog driver logs data independently of any client connections to
the devices that it is logging.  As long as it's enabled and recording,
the writelog driver records data from the specified list of devices
at the rate that new data is produced by the first device in that list
(so put the fastest one first).  Commands are not logged.

For help in remote-controlling logging, try @ref player_util_playervcr.
Note that you must declare a @ref player_interface_log device to allow
logging control.

Note that unless you plan to remote-control this driver via the @ref
player_interface_log interface (e.g., using @ref player_util_playervcr),
you should specify the @p alwayson option in the configuration file so
that logging start when Player starts.

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
- @ref player_interface_sonar
- @ref player_interface_position
- @ref player_interface_position3d
- @ref player_interface_power
- @ref player_interface_truth
- @ref player_interface_wifi

@par Configuration requests

- PLAYER_LOG_SET_WRITE_STATE
- PLAYER_LOG_GET_STATE
- PLAYER_LOG_SET_FILENAME

@par Configuration file options

- filename (string)
  - Default: "writelog_YYYY_MM_DD_HH_MM.log", where YYYY is the year,
    MM is the month, etc.
  - Name of logfile.
- autorecord (integer)
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
  autorecord 1
)
@endverbatim

@par Authors

Andrew Howard

*/
/** @} */

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libplayercore/playercore.h>

#include "encode.h"
  
// Utility class for storing per-device info
struct WriteLogDevice
{
  public: player_devaddr_t addr;
  public: Device *device;
  public: double time;
  public: int cameraFrame;
  // only filled out for sonar devices; we cache the sonar geometry
  // right after subscribing, then prefix every line of sonar data
  // with this info.
  public: player_sonar_geom_t sonar_geom;
};


// The logfile driver
class WriteLog: public Driver 
{
  /// Constructor
  public: WriteLog(ConfigFile* cf, int section);

  /// Destructor
  public: ~WriteLog();

  // MessageHandler
  public: virtual int ProcessMessage(MessageQueue * resp_queue, 
                                     player_msghdr * hdr, 
                                     void * data, 
                                     void ** resp_data, 
                                     size_t * resp_len);

  /// Initialize the driver
  public: virtual int Setup();

  /// Finalize the driver
  public: virtual int Shutdown();

  // Device thread
  private: virtual void Main(void);

  // Open this->filename, save the resulting FILE* to this->file, and
  // write in the logfile header.
  private: int OpenFile();

  // Flush and close this->file
  private: void CloseFile();

  // Write data to file
  private: void Write(WriteLogDevice *device, void *data, 
                      size_t size, double time);

  // Write laser data to file
  private: void WriteLaser(player_laser_data_t *data);

#if 0
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

  // Write sonar data to file
  private: void WriteSonar(player_sonar_data_t *data, WriteLogDevice* device);

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
#endif

  // File to write data to
  private: char default_basename[1024];
  private: char default_filename[1024];
  private: char filename[1024];
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
    : Driver(cf, section, PLAYER_LOG_CODE)
{
  int i;
  player_devaddr_t addr;
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
  strcpy(this->filename, 
         cf->ReadString(section, "filename", this->default_filename));

  // Default enabled?
  if(cf->ReadInt(section, "autorecord", 1) > 0)
    this->enable_default = true;
  else
    this->enable_default = false;

  this->device_count = 0;
  
  // Get a list of input devices
  for (i = 0; i < cf->GetTupleCount(section, "requires"); i++)
  {
    if (cf->ReadDeviceAddr(&addr, section, "requires", -1, i, NULL) != 0)
    {
      this->SetError(-1);
      return;
    }
    
    // Add to our device table
    assert(this->device_count < (int) (sizeof(this->devices) / sizeof(this->devices[0])));
    device = this->devices + this->device_count++;
    device->addr = addr;
    device->device = NULL;
    device->time = 0.0;
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

    device->device = deviceTable->GetDevice(device->addr);
    if (!device->device)
    {
      PLAYER_ERROR3("unable to locate device [%d:%s:%d] for logging",
                    device->addr.robot,
                    ::lookup_interface_name(0, device->addr.interf),
                    device->addr.index);
      return -1;
    }
    if(device->device->Subscribe(this->InQueue) != 0)
    {
      PLAYER_ERROR("unable to subscribe to device for logging");
      return -1;
    }
#if 0
    if (device->addr.interf == PLAYER_SONAR_CODE)
    {
      // We need to cache the sonar geometry
      unsigned short reptype;
      size_t replen = sizeof(device->sonar_geom);
      reptype=device->device->ProcessMessage(PLAYER_MSGTYPE_REQ, PLAYER_SONAR_GET_GEOM, 
            device->id, 0, (uint8_t *)&(device->sonar_geom), (uint8_t *)&(device->sonar_geom), &replen);
/*      device->sonar_geom.subtype = PLAYER_SONAR_GET_GEOM_REQ;
      if((device->device->Request(device->id,(void*)this,
                                  (void*)&(device->sonar_geom),
                                  sizeof(device->sonar_geom.subtype),
                                  (struct timeval*)NULL,
                                  &reptype,
                                  (void*)&(device->sonar_geom),
                                  sizeof(device->sonar_geom),
                                  (struct timeval*)NULL) < 0) ||*/
      if (reptype != PLAYER_MSGTYPE_RESP_ACK)
      {
        // oh well.
        PLAYER_WARN("unable to get sonar geometry");
        device->sonar_geom.pose_count = 0;
      }
      else
      {
        // byteswap
        device->sonar_geom.pose_count = ntohs(device->sonar_geom.pose_count);
        for(int j=0; j<device->sonar_geom.pose_count; j++)
        {
          device->sonar_geom.poses[j][0] = 
                  (int16_t)ntohs(device->sonar_geom.poses[j][0]);
          device->sonar_geom.poses[j][1] = 
                  (int16_t)ntohs(device->sonar_geom.poses[j][1]);
          device->sonar_geom.poses[j][2] = 
                  (int16_t)ntohs(device->sonar_geom.poses[j][2]);
        }
      }
    }
#endif
  }

  if(this->OpenFile() < 0)
  {
    PLAYER_ERROR2("unable to open [%s]: %s\n", this->filename, strerror(errno));
    return(-1);
  }

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
  this->CloseFile();

  // Unsubscribe to the underlying devices
  for (i = 0; i < this->device_count; i++)
  {
    device = this->devices + i;

    // Unsbscribe from the underlying device
    device->device->Unsubscribe(this->InQueue);
    device->device = NULL;
  }
  
  return 0;
}

int
WriteLog::OpenFile()
{
  // Open the file
  this->file = fopen(this->filename, "w+");
  if(this->file == NULL)
  {
    PLAYER_ERROR2("unable to open [%s]: %s\n", this->filename, strerror(errno));
    return(-1);
  }

  // Write the file header
  fprintf(this->file, "## Player version %s \n", VERSION);
  fprintf(this->file, "## File version %s \n", "0.2.0");

  return(0);
}

void
WriteLog::CloseFile()
{
  if(this->file)
  {
    fflush(this->file);
    fclose(this->file);
    this->file = NULL;
  }
}

int WriteLog::ProcessMessage(MessageQueue * resp_queue, 
                             player_msghdr * hdr, 
                             void * data, 
                             void ** resp_data, 
                             size_t * resp_len)
{
  *resp_len = 0;

#if 0
  if (MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_LOG_SET_WRITE_STATE, device_id))
  {
    assert(hdr->size == sizeof(player_log_set_write_state_t));
    player_log_set_write_state_t & sreq = *reinterpret_cast<player_log_set_write_state_t*> (data);
		
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
    *resp_len = 0;
    return PLAYER_MSGTYPE_RESP_ACK;
  }
  
  if (MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_LOG_GET_STATE, device_id))
  {
    assert(*resp_len >= sizeof(player_log_get_state_t));
    player_log_get_state_t & greq = *reinterpret_cast<player_log_get_state_t*> (resp_data);
    *resp_len = sizeof(player_log_get_state_t);
    
    greq.type = PLAYER_LOG_TYPE_WRITE;
    if(this->enable)
      greq.state = 1;
    else
      greq.state = 0;

    return PLAYER_MSGTYPE_RESP_ACK;
  }  
 
  if (MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_LOG_SET_FILENAME, device_id))
  {
    assert(hdr->size <= sizeof(player_log_set_filename_t));
    assert(hdr->size > 0);
    player_log_set_filename_t & freq = *reinterpret_cast<player_log_set_filename_t*> (data);
    *resp_len = 0;

    if(this->enable)
    {
      PLAYER_WARN("tried to switch filenames while logging");
      return PLAYER_MSGTYPE_RESP_NACK;
    }

    PLAYER_MSG1(1,"Closing logfile %s", this->filename);
    this->CloseFile();
    strncpy(this->filename,
            (const char*)freq.filename,
            hdr->size);
    this->filename[sizeof(this->filename)-1] = '\0';
    PLAYER_MSG1(1,"Opening logfile %s", this->filename);
    if(this->OpenFile() < 0)
    {
      PLAYER_WARN1("Failed to open logfile %s", this->filename);
      return PLAYER_MSGTYPE_RESP_NACK;
    }
    return PLAYER_MSGTYPE_RESP_ACK;
  }  
#endif

  if(hdr->type == PLAYER_MSGTYPE_DATA)
  {
    *resp_len = 0;
    // If logging is stopped, then don't log
    if(!this->enable)
      return 0;

    // Walk the device list
    for (int i = 0; i < this->device_count; i++)
    {
      WriteLogDevice * device = this->devices + i;

      if((device->addr.host != hdr->addr.host) ||
         (device->addr.robot != hdr->addr.robot) ||
         (device->addr.interf != hdr->addr.interf) ||
         (device->addr.index != hdr->addr.index))
        continue;

      // record timestamp
      device->time = hdr->timestamp;

      // Write data to file
      this->Write(device, data, hdr->size, device->time);
      return 0;
    }
    return -1;
  }    	
  return -1;
}

////////////////////////////////////////////////////////////////////////////
// Process configuration requests
/*int WriteLog::PutConfig(player_device_id_t id, void *client, 
                        void* src, size_t len,
                        struct timeval* timestamp)
{
  player_log_set_write_state_t sreq;
  player_log_get_state_t greq;
  player_log_set_filename_t freq;
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
    case PLAYER_LOG_SET_FILENAME:
      if(len < (sizeof(freq.subtype)+1))
      {
        PLAYER_WARN("request to change filename too short");
        if(this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
      }
      if(len > sizeof(freq))
      {
        PLAYER_WARN("request to change filename too long");
        if(this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
      }
      // can't switch filenames while logging
      if(this->enable)
      {
        PLAYER_WARN("tried to switch filenames while logging");
        if(this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
      }
      freq = *((player_log_set_filename_t*)src);
      PLAYER_MSG1(1,"Closing logfile %s", this->filename);
      this->CloseFile();
      strncpy(this->filename,
              (const char*)freq.filename,
              len-sizeof(freq.subtype));
      this->filename[sizeof(this->filename)-1] = '\0';
      PLAYER_MSG1(1,"Opening logfile %s", this->filename);
      if(this->OpenFile() < 0)
      {
        PLAYER_WARN1("Failed to open logfile %s", this->filename);
        if(this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
      }
      if(this->PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL) != 0)
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
*/





////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void 
WriteLog::Main(void)
{
  while (1)
  {
    pthread_testcancel();
    
    // Wait on my queue
    this->Wait();

    // Process all new messages (calls ProcessMessage on each)
    this->ProcessMessages();
  }
}


////////////////////////////////////////////////////////////////////////////
// Write data to file
void WriteLog::Write(WriteLogDevice *device, void *data, 
                     size_t size, double time)
{
  char host[256];
  player_interface_t iface;
  double stime;

  // Get server time
  GlobalTime->GetTimeDouble(&stime);

  // Get interface name
  assert(device);
  ::lookup_interface_code(device->addr.interf, &iface);
  gethostname(host, sizeof(host));
  
  // Write header info
  fprintf(this->file, "%014.3f %s %d %s %02d %014.3f ",
          stime, host, device->addr.robot, iface.name, 
          device->addr.index, time);

  // Write the data
  switch (iface.interf)
  {
    case PLAYER_LASER_CODE:
      this->WriteLaser((player_laser_data_t*) data);
      break;
#if 0
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
    case PLAYER_SONAR_CODE:
      this->WriteSonar((player_sonar_data_t*) data, device);
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
#endif
    default:
      PLAYER_WARN1("unsupported interface type [%s]", 
                   ::lookup_interface_name(0, iface.interf));
      break;
  }

  fprintf(this->file, "\n");

  // Flush the data (some drivers produce a lot of data; we dont want
  // it to back up and slow us down later).
  fflush(this->file);
  
  return;
}

/** @addtogroup player_driver_writelog */
/** @{ */

/** @defgroup player_driver_writelog_laser Laser format
 
@brief @ref player_interface_laser format 

The format for each @ref player_interface_laser message is:
  - min_angle (float): minimum scan angle, in radians
  - max_angle (float): maximum scan angle, in radians
  - resolution (float): angular resolution, in radians
  - count (int): number of readings to follow
  - list of readings; for each reading:
    - range (float): in meters
    - intensity (int): intensity
*/
void WriteLog::WriteLaser(player_laser_data_t *data)
{
  int i;

  // Note that, in this format, we need a lot of precision in the
  // resolution field.
  
  fprintf(this->file, "%+07.4f %+07.4f %+.8f %04d ",
          data->min_angle, data->max_angle,
          data->resolution, data->ranges_count);

  for (i = 0; i < ntohs(data->ranges_count); i++)
    fprintf(this->file, "%.3f %2d ",
            data->ranges[i], data->intensity[i]);

  return;
}

#if 0
/** @defgroup player_driver_writelog_blobfinder Blobfinder format
 
@brief @ref player_interface_blobfinder format 

The format for each @ref player_interface_blobfinder message is:
  - width (int): in pixels, of image
  - height (int): in pixels, of image
  - count (int): number of blobs to follow
  - list of blobs; for each blob:
    - id (int): id of blob (if supported)
    - color (int): packed 24-bit RGB color of blob
    - area (int): in pixels, of blob
    - x y (ints): in pixels, of blob center
    - left right top bottom (ints): in pixels, of bounding box
    - range (int): in mm, of range to blob (if supported)
 
*/
void WriteLog::WriteBlobfinder(player_blobfinder_data_t *data)
{

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


/** @defgroup player_driver_writelog_camera Camera format
 
@brief @ref player_interface_camera format 

The format for each @ref player_interface_camera message is:
  - width (int): in pixels
  - height (int): in pixels
  - depth (int): in bits per pixel
  - format (int): image format
  - compression (int): image compression
  - image data, encoded as a string of ASCII hex values
*/
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


/** @defgroup player_driver_writelog_fiducial Fiducial format
 
@brief @ref player_interface_fiducial format 

The format for each @ref player_interface_fiducial message is:
  - count (int): number of fiducials to follow
  - list of fiducials; for each fiducial:
    - id (int): fiducial ID
    - x (float): relative X position, in meters
    - y (float): relative Y position, in meters
    - z (float): relative Z position, in meters
    - roll (float): relative roll orientation, in radians
    - pitch (float): relative pitch orientation, in radians
    - yaw (float): relative yaw orientation, in radians
    - ux (float): uncertainty in relative X position, in meters
    - uy (float): uncertainty in relative Y position, in meters
    - uz (float): uncertainty in relative Z position, in meters
    - uroll (float): uncertainty in relative roll orientation, in radians
    - upitch (float): uncertainty in relative pitch orientation, in radians
    - uyaw (float): uncertainty in relative yaw orientation, in radians

*/
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


/** @defgroup player_driver_writelog_gps GPS format
 
@brief @ref player_interface_gps format 

The format for each @ref player_interface_gps message is:
  - time (float): current GPS time, in seconds
  - latitude (float): in degrees
  - longitude (float): in degrees
  - altitude (float): in meters
  - utm_e (float): UTM WGS84 easting, in meters
  - utm_n (float): UTM WGS84 northing, in meters
  - hdop (float): horizontal dilution of position
  - vdop (float): vertical dilution of position
  - err_horz (float): horizontal error, in meters
  - err_vert (float): vertical error, in meters
  - quality (int): quality of fix (0 = invalid, 1 = GPS fix, 2 = DGPS fix)
  - num_sats (int): number of satellites in view
*/
void WriteLog::WriteGps(player_gps_data_t *data)
{
  fprintf(this->file,
          "%.3f "
          "%.6f %.6f %.6f "
          "%.3f %.3f "
          "%.3f %.3f %.3f %.3f "
          "%d %d",
          (double) (uint32_t) HINT32(data->time_sec) +
          (double) (uint32_t) HINT32(data->time_sec) * 1e-6,
          (double) HINT32(data->latitude) / (1e7),
          (double) HINT32(data->longitude) / (1e7),
          MM_M(HINT32(data->altitude)),
          CM_M(HINT32(data->utm_e)),
          CM_M(HINT32(data->utm_n)), 
          (double) HINT16(data->hdop) / 10,
          (double) HINT16(data->vdop) / 10,
          MM_M(HINT32(data->err_horz)),
          MM_M(HINT32(data->err_vert)),
          (int) data->quality,
          (int) data->num_sats);
  
  return;
}


/** @defgroup player_driver_writelog_joystick Joystick format
 
@brief @ref player_interface_joystick format 

The format for each @ref player_interface_joystick message is:
  - xpos (int): unscaled X position of joystick
  - ypos (int): unscaled Y position of joystick
  - xscale (int): maximum X position
  - yscale (int): maximum Y position
  - buttons (hex string): bitmask of button states
*/
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



/** @defgroup player_driver_writelog_sonar Sonar format
 
@brief @ref player_interface_sonar format 

The format for each @ref player_interface_sonar message is:
  - pose_count (int): number of sonar poses to follow
  - list of tranducer poses; for each pose:
    - x (float): relative X position of transducer, in meters
    - y (float): relative Y position of transducer, in meters
    - a (float): relative yaw orientation of transducer, in radians
  - range_count (int): number range values to follow
  - list of readings; for each reading:
    - range (float): in meters
*/
void WriteLog::WriteSonar(player_sonar_data_t *data, WriteLogDevice* device)
{
  unsigned int i;

  // Note that, in this format, we need a lot of precision in the
  // resolution field.

  // Format:
  //   pose_count x0 y0 a0 x1 y1 a1 ...  range_count r0 r1 ...
  fprintf(this->file, "%u ", device->sonar_geom.pose_count);
  for(i=0;i<device->sonar_geom.pose_count;i++)
    fprintf(this->file, "%+07.3f %+07.3f %+07.4f ", 
            MM_M(device->sonar_geom.poses[i][0]),
            MM_M(device->sonar_geom.poses[i][1]),
            DEG_RAD(device->sonar_geom.poses[i][2]));

  fprintf(this->file, "%u ", HUINT16(data->range_count));
  for(i=0;i<HUINT16(data->range_count);i++)
    fprintf(this->file, "%.3f ", MM_M(HUINT16(data->ranges[i])));

  return;
}


/** @defgroup player_driver_writelog_position Position format
 
@brief @ref player_interface_position format 

The format for each @ref player_interface_position message is:
  - xpos (float): in meters
  - ypos (float): in meters
  - yaw (float): in radians
  - xspeed (float): in meters / second
  - yspeed (float): in meters / second
  - yawspeed (float): in radians / second
  - stall (int): motor stall sensor
*/
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


/** @defgroup player_driver_writelog_position3d Position3d format
 
@brief @ref player_interface_position3d format 

The format for each @ref player_interface_position3d message is:
  - xpos (float): in meters
  - ypos (float): in meters
  - zpos (float): in meters
  - roll (float): in radians
  - pitch (float): in radians
  - yaw (float): in radians
  - xspeed (float): in meters / second
  - yspeed (float): in meters / second
  - zspeed (float): in meters / second
  - rollspeed(float): in radians / second
  - pitchspeed(float): in radians / second
  - yawspeed(float): in radians / second
  - stall (int): motor stall sensor
*/
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


/** @defgroup player_driver_writelog_power Power format
 
@brief @ref player_interface_power format 

The format for each @ref player_interface_power message is:
  - charge (float): in volts
*/
void WriteLog::WritePower(player_power_data_t *data)
{
  fprintf(this->file, "%.1f ", HUINT16(data->charge) / 10.0);
  return;
}


/** @defgroup player_driver_writelog_wifi WiFi format
 
@brief @ref player_interface_wifi format 

The format for each @ref player_interface_wifi message is:
  - link_count (int): number of nodes to follow
  - list of nodes; for each node:
    - mac (string): MAC address
    - ip (string): IP address
    - essid (string): ESSID
    - mode (int): operating mode (master, adhoc, etc.)
    - freq (int): in MHz
    - encrypt (int): encrypted?
    - qual (int): link quality
    - level (int): link level
    - noise (int): noise level
*/
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


/** @defgroup player_driver_writelog_truth Truth format
 
@brief @ref player_interface_truth format 

The format for each @ref player_interface_truth message is:
  - x (float): in meters
  - y (float): in meters
  - z (float): in meters
  - roll (float): in radians
  - pitch (float): in radians
  - yaw (float): in radians
*/
void WriteLog::WriteTruth(player_truth_data_t *data)
{
  fprintf(this->file, "%+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f",
          MM_M(HINT32(data->pos[0])),
          MM_M(HINT32(data->pos[1])),
          MM_M(HINT32(data->pos[2])),
          MM_M(HINT32(data->rot[0])),
          MM_M(HINT32(data->rot[1])),
          MM_M(HINT32(data->rot[2])));
  
  return;
}
#endif


/** @} */
