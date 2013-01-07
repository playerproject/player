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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */
/*
 * Desc: Driver for writing log files.
 * Author: Andrew Howard
 * Date: 14 Jun 2003
 * CVS: $Id$
 *
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_writelog writelog
 * @brief Logging data

The writelog driver will write data from another device to a log file.
Each data message is written to a separate line.  The format for the
file is given in the
@ref tutorial_datalog "data logging tutorial".

The @ref driver_readlog driver can be used to replay the data
(to client programs, the replayed data will appear to come from the
real sensors).

The writelog driver logs data independently of any client connections to
the devices that it is logging.  As long as it's enabled and recording,
the writelog driver records data from the specified list of devices
at the rate that new data is produced by the first device in that list
(so put the fastest one first).  Commands are not logged.

For help in remote-controlling logging, try @ref util_playervcr.
Note that you must declare a @ref interface_log device to allow
logging control.

Note that unless you plan to remote-control this driver via the @ref
interface_log interface (e.g., using @ref util_playervcr),
you should specify the @p alwayson option in the configuration file so
that logging start when Player starts.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_log : can be used to turn logging on/off

@par Requires

The writelog driver takes as input a list of devices to log data from.
The driver with the <b>highest data rate</b> should be placed first in the list.
The writelog driver can will log data from the following interfaces:

- @ref interface_laser
- @ref interface_ranger
- @ref interface_sonar
- @ref interface_position2d
- @ref interface_ptz
- @ref interface_wifi
- @ref interface_wsn
- @ref interface_opaque
- @ref interface_imu
- @ref interface_pointcloud3d
- @ref interface_actarray
- @ref interface_camera
- @ref interface_fiducial
- @ref interface_blobfinder
- @ref interface_gps
- @ref interface_joystick
- @ref interface_position3d
- @ref interface_power
- @ref interface_dio
- @ref interface_aio
- @ref interface_coopobject

@par Configuration requests

- PLAYER_LOG_REQ_SET_WRITE_STATE
- PLAYER_LOG_REQ_SET_STATE
- PLAYER_LOG_REQ_SET_FILENAME

@par Configuration file options
- log_directory (string)
  - Default: Directory player is run from.
  - Name of the directory to store the log file in. Relative paths are
    taken from the directory player is run from. Absolute paths work
    as expected. The directory is created if it doesn't exist.
- timestamp_directory (integer)
  - Default: 0
  - Add a timestamp to log_directory, in the format "YYYY_MM_DD_HH_MM_SS",
    where YYYY is the year, MM is the month, etc.
- basename (string)
  - Default: "writelog_"
  - Base name of the log file.
- timestamp (integer)
  - Default: 1
  - Add a timestamp to each file, in the format "YYYY_MM_DD_HH_MM_SS",
    where YYYY is the year, MM is the month, etc.
- extension (string)
  - Default: ".log"
  - File extension for the log file.
- filename (string)
  - Default: basename+timestamp+extension
  - Use this option to override the default basename/timestamp/extension 
    combination and set your own arbitrary filename for the log.
- autorecord (integer)
  - Default: 0
  - Default log state; set to 1 for continous logging.
- camera_log_images (integer)
  - Default: 1
  - Save image data to the log file. If this is turned off, a log line is still
    generated for each frame, containing all information except the raw image data.
- camera_save_images (integer)
  - Default: 0
  - Save image data to external files within the log directory.
    The image files are named "(basename)(timestamp)_camera_II_NNNNNNN.pnm",
    where II is the device index and NNNNNNN is the frame number.
@par Example

@verbatim
# Log data from laser:0 position2d:0 to "/home/data/logs/mydata_YYYY_MM_DD_HH_MM_SS.log"
driver
(
  name "writelog"
  log_directory "/home/user/logs"
  basename "mydata"
  requires ["laser:0" "position2d:0"]
  provides ["log:0"]
  alwayson 1
  autorecord 1
)
@endverbatim

@author Andrew Howard, Radu Bogdan Rusu, Rich Mattes

*/
/** @} */

#include "config.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#if defined (WIN32)
  #include <direct.h> // For _mkdir()
#else
  #include <unistd.h>
#endif

#include <libplayercore/playercore.h>

#include "encode.h"

#if defined (WIN32)
  #define snprintf _snprintf
#endif

// Utility class for storing per-device info
struct WriteLogDevice
{
  public: player_devaddr_t addr;
  public: Device *device;
  public: int cameraFrame;
};


// The logfile driver
class WriteLog: public ThreadedDriver
{
  /// Constructor
  public: WriteLog(ConfigFile* cf, int section);

  /// Destructor
  public: ~WriteLog();

  // MessageHandler
  public: virtual int ProcessMessage(QueuePointer & resp_queue,
                                     player_msghdr * hdr,
                                     void * data);

  /// Initialize the driver
  public: virtual int MainSetup();

  /// Finalize the driver
  public: virtual void MainQuit();

  // Device thread
  private: virtual void Main(void);

  // Open this->filename, save the resulting FILE* to this->file, and
  // write in the logfile header.
  private: int OpenFile();

  // Request and write geometries
  private: void WriteGeometries();

  // Flush and close this->file
  private: void CloseFile();

  // Write localize particles to file
  private: void WriteLocalizeParticles();

  // Write data to file
  private: void Write(WriteLogDevice *device,
                      player_msghdr_t* hdr, void *data);

  // Write laser data to file
  private: int WriteLaser(player_msghdr_t* hdr, void *data);

  // Write ranger data to file
  private: int WriteRanger(player_msghdr_t* hdr, void *data);

  // Write localize data to file
  private: int WriteLocalize(player_msghdr_t* hdr, void *data);

  // Write opaque data to file
  private: int WriteOpaque(player_msghdr_t* hdr, void *data);

  // Write position data to file
  private: int WritePosition(player_msghdr_t* hdr, void *data);

  // Write PTZ data to file
  private: int WritePTZ(player_msghdr_t* hdr, void *data);

  // Write sonar data to file
  private: int WriteSonar(player_msghdr_t* hdr, void *data);

  // Write wifi data to file
  private: int WriteWiFi(player_msghdr_t* hdr, void *data);

  // Write WSN data to file
  private: int WriteWSN (player_msghdr_t* hdr, void *data);
  
    // Write CO data to file
  private: int WriteCoopObject (player_msghdr_t* hdr, void *data);

  /* HHAA 15-02-2007 */
  // Write bumper data to file
  private: int WriteBumper(player_msghdr_t* hdr, void *data);

  /* HHAA 15-02-2007 */
  // Write fixed range IRs data to file
  private: int WriteIR(player_msghdr_t* hdr, void *data);

  // Write IMU data to file
  private: int WriteIMU (player_msghdr_t* hdr, void *data);

  // Write PointCloud3D data to file
  private: int WritePointCloud3d (player_msghdr_t* hdr, void *data);

  // Write Actarray data to file
  private: int WriteActarray (player_msghdr_t* hdr, void *data);

  // Write AIO data to file
  private: int WriteAIO(player_msghdr_t* hdr, void *data);

  // Write AIO data to file
  private: int WriteDIO(player_msghdr_t* hdr, void *data);

  // Write RFID data to file
  private: int WriteRFID(player_msghdr_t* hdr, void *data);

  // Write camera data to file
  private: int WriteCamera(WriteLogDevice *device, player_msghdr_t* hdr, void *data);

  // Write fiducial data to file
  private: int WriteFiducial(player_msghdr_t* hdr, void *data);

  // Write blobfinder data to file
  private: int WriteBlobfinder(player_msghdr_t* hdr, void *data);

  // Write GPS data to file
  private: int WriteGps(player_msghdr_t* hdr, void *data);
  
  // Write joystick data to file
  private: int WriteJoystick(player_msghdr_t* hdr, void *data);

  // Write position3d data to file
  private: int WritePosition3d(player_msghdr_t* hdr, void *data);

  // Write power data to file
  private: int WritePower(player_msghdr_t* hdr, void *data);

  // Where to save files
  private: char log_directory[1024];

  // File to write data to
  private: char filestem[1024];
  private: char filename[1024];
  private: FILE *file;

  // Subscribed device list
  private: int device_count;
  private: WriteLogDevice devices[1024];

  // Log particles (in case a localize interface is provided)
  public: bool write_particles;

  private: bool write_particles_now;
  private: WriteLogDevice* localize_device;

  // Is writing enabled? (client can start/stop)
  private: bool enable;
  private: bool enable_default;

  // Save camera frames to log file?
  private: bool cameraLogImages;
  // Save camera frames to image files as well?
  private: bool cameraSaveImages;
};


////////////////////////////////////////////////////////////////////////////
// Pointer to the one-and-only time
extern int global_playerport;


////////////////////////////////////////////////////////////////////////////
// Create a driver for reading log files
Driver* WriteLog_Init(ConfigFile* cf, int section)
{
  return ((Driver*) (new WriteLog(cf, section)));
}


////////////////////////////////////////////////////////////////////////////
// Device factory registration
void writelog_Register(DriverTable* table)
{
  table->AddDriver("writelog", WriteLog_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////
// Constructor
WriteLog::WriteLog(ConfigFile* cf, int section)
    : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_LOG_CODE)
{
  int i;
  player_devaddr_t addr;
  WriteLogDevice *device;
  time_t t;
  struct tm *ts;

  char time_stamp[32];
  char basename[1024];
  char extension[32];
  char default_filename[1024];
  char complete_filename[1024];

  this->file = NULL;

  // Construct timestamp from date and time.  Note that we use
  // the system time, *not* the Player time.  I think that this is the
  // correct semantics for working with simulators.
  time(&t);
  ts = localtime(&t);
  strftime(time_stamp, sizeof(time_stamp),
           "%Y_%m_%d_%H_%M_%S", ts);

  // Let user override default basename
  strcpy(basename, cf->ReadString(section, "basename", "writelog_"));

  strcpy(extension, cf->ReadString(section, "extension", ".log"));

  // Attach the time stamp
  snprintf(this->filestem, sizeof(this->filestem), "%s%s",
           basename, cf->ReadInt(section, "timestamp", 1) ? time_stamp : "");

  snprintf(default_filename, sizeof(default_filename), "%s%s",
           this->filestem, extension);

  // Let user override default filename
  strcpy(complete_filename,
         cf->ReadString(section, "filename", default_filename));

  // Let user override log file directory
  snprintf(this->log_directory, sizeof(this->log_directory), "%s%s",
           cf->ReadString(section, "log_directory", ""),
           cf->ReadInt(section, "timestamp_directory", 0) ? time_stamp : "");

  // If neither directory nor timestamp is specified, just use .
  if(this->log_directory[0] == 0) strcpy(this->log_directory, ".");

  // Prepend the directory
  snprintf(this->filename, sizeof(this->filename), "%s/%s",
           this->log_directory, complete_filename);

  // Default enabled?
  if(cf->ReadInt(section, "autorecord", 1) > 0)
    this->enable_default = true;
  else
    this->enable_default = false;

  this->device_count = 0;

  //write particles in case the localize interface is provided
  this->write_particles = cf->ReadInt(section, "write_particles", 0) != 0 ? true : false;

  this->write_particles_now = false;


  localize_device = NULL;
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
    device->cameraFrame = 0;
  }

  // Camera specific settings
  this->cameraLogImages = cf->ReadInt(section, "camera_log_images", 1) != 0 ? true : false;
  this->cameraSaveImages = cf->ReadInt(section, "camera_save_images", 0) != 0 ? true : false;

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
int WriteLog::MainSetup()
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
  }

  // Enable/disable logging, according to default set in config file
  this->enable = this->enable_default;

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Finalize the driver
void WriteLog::MainQuit()
{
  int i;
  WriteLogDevice *device;

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
}

int
WriteLog::OpenFile()
{
  // Create directory
#if defined (WIN32)
  _mkdir(this->log_directory);
#else
  mkdir(this->log_directory, 0755);
#endif

  // Open the file
  this->file = fopen(this->filename, "w+");
  if(this->file == NULL)
  {
    PLAYER_ERROR2("unable to open [%s]: %s\n", this->filename, strerror(errno));
    return(-1);
  }

  // Write the file header
  fprintf(this->file, "## Player version %s \n", PLAYER_VERSION);
  fprintf(this->file, "## File version %s \n", "0.3.0");

  fprintf(this->file, "## Format: \n");
  fprintf(this->file, "## - Messages are newline-separated\n");
  fprintf(this->file, "## - Common header to each message is:\n");
  fprintf(this->file, "##   time     host   robot  interface index  type   subtype\n");
  fprintf(this->file, "##   (double) (uint) (uint) (string)  (uint) (uint) (uint)\n");
  fprintf(this->file, "## - Following the common header is the message payload \n");

  this->WriteGeometries();

  return(0);
}

////////////////////////////////////////////////////////////////////////////
// Request and write geometries
void WriteLog::WriteGeometries()
{
  // Subscribe to the underlying devices
  for (int i = 0; i < this->device_count; i++)
  {
    WriteLogDevice* device = this->devices + i;

    if (device->addr.interf == PLAYER_SONAR_CODE)
    {
      // Get the sonar geometry
      Message* msg;
      if(!(msg = device->device->Request(this->InQueue,
                                         PLAYER_MSGTYPE_REQ,
                                         PLAYER_SONAR_REQ_GET_GEOM,
                                         NULL, 0, NULL, true)))
      {
        // oh well.
        PLAYER_WARN("unable to get sonar geometry");
      }
      else
      {
        // log it
        this->Write(device, msg->GetHeader(), msg->GetPayload());
        delete msg;
      }
    }
    else if (device->addr.interf == PLAYER_LASER_CODE)
    {
      // Get the laser geometry
      Message* msg;
      if(!(msg = device->device->Request(this->InQueue,
                                         PLAYER_MSGTYPE_REQ,
                                         PLAYER_LASER_REQ_GET_GEOM,
                                         NULL, 0, NULL, true)))
      {
        // oh well.
        PLAYER_WARN("unable to get laser geometry");
      }
      else
      {
        // log it
        this->Write(device, msg->GetHeader(), msg->GetPayload());
        delete msg;
      }
    }
    else if (device->addr.interf == PLAYER_RANGER_CODE)
    {
      // Get the ranger geometry
      Message* msg;
      if(!(msg = device->device->Request(this->InQueue,
                                         PLAYER_MSGTYPE_REQ,
                                         PLAYER_RANGER_REQ_GET_GEOM,
                                         NULL, 0, NULL, true)))
      {
        // oh well.
        PLAYER_WARN("unable to get ranger geometry");
      }
      else
      {
        // log it
        this->Write(device, msg->GetHeader(), msg->GetPayload());
        delete msg;
      }
      if(!(msg = device->device->Request(this->InQueue,
                                         PLAYER_MSGTYPE_REQ,
                                         PLAYER_RANGER_REQ_GET_CONFIG,
                                         NULL, 0, NULL, true)))
      {
        // oh well.
        PLAYER_WARN("unable to get ranger config");
      }
      else
      {
        // log it
        this->Write(device, msg->GetHeader(), msg->GetPayload());
        delete msg;
      }
    }
    else if (device->addr.interf == PLAYER_POSITION2D_CODE)
    {
      // Get the position geometry
      Message* msg;
      if(!(msg = device->device->Request(this->InQueue,
                                         PLAYER_MSGTYPE_REQ,
                                         PLAYER_POSITION2D_REQ_GET_GEOM,
                                         NULL, 0, NULL, true)))
      {
        // oh well.
        PLAYER_WARN("unable to get position geometry");
      }
      
      else
      {
        // log it
        this->Write(device, msg->GetHeader(), msg->GetPayload());
        delete msg;
      }
    }
    else if (device->addr.interf == PLAYER_POSITION3D_CODE)
    {
      // Get the position geometry
      Message* msg;
      if(!(msg = device->device->Request(this->InQueue,
                                         PLAYER_MSGTYPE_REQ,
                                         PLAYER_POSITION3D_REQ_GET_GEOM,
                                         NULL, 0, NULL, true)))
      {
        // oh well.
        PLAYER_WARN("unable to get position3d geometry");
      }
      
      else
      {
        // log it
        this->Write(device, msg->GetHeader(), msg->GetPayload());
        delete msg;
      }
    }
    /* HHAA 15-02-2007 */
    else if (device->addr.interf == PLAYER_BUMPER_CODE)
    {
      // Get the bumper geometry
      Message* msg;
      if(!(msg = device->device->Request(this->InQueue,
                                         PLAYER_MSGTYPE_REQ,
                                         PLAYER_BUMPER_REQ_GET_GEOM,
                                         NULL, 0, NULL, true)))
      {
        // oh well.
        PLAYER_WARN("unable to get bumper geometry");
      }
      else
      {
        // log it
        this->Write(device, msg->GetHeader(), msg->GetPayload());
        delete msg;
      }
    }
    /* HHAA 15-02-2007 */
    else if (device->addr.interf == PLAYER_IR_CODE)
    {
      // Get the IR geometry
      Message* msg;
      if(!(msg = device->device->Request(this->InQueue,
                                         PLAYER_MSGTYPE_REQ,
                                         PLAYER_IR_REQ_POSE,
                                         NULL, 0, NULL, true)))
      {
        // oh well.
        PLAYER_WARN("unable to get ir geometry");
      }
      else
      {
        // log it
        this->Write(device, msg->GetHeader(), msg->GetPayload());
        delete msg;
      }
    }
     else if (device->addr.interf == PLAYER_LOCALIZE_CODE)
    {
      localize_device = device;
    }

  }
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

int
WriteLog::ProcessMessage(QueuePointer & resp_queue,
                         player_msghdr * hdr,
                         void * data)
{
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                           PLAYER_LOG_REQ_SET_WRITE_STATE,
                           this->device_addr))
  {
    player_log_set_write_state_t* sreq = (player_log_set_write_state_t*)data;

    if(sreq->state)
    {
      puts("WriteLog: start logging");
      this->enable = true;
    }
    else
    {
      puts("WriteLog: stop logging");
      this->enable = false;
    }

    // send an empty ACK
    this->Publish(this->device_addr, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_LOG_REQ_SET_WRITE_STATE);
    return(0);
  }
  else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                                PLAYER_LOG_REQ_GET_STATE, this->device_addr))
  {
    player_log_get_state_t greq;

    greq.type = PLAYER_LOG_TYPE_WRITE;
    if(this->enable)
      greq.state = 1;
    else
      greq.state = 0;

    this->Publish(this->device_addr,
                  resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_LOG_REQ_GET_STATE,
                  (void*)&greq, sizeof(greq), NULL);
    return(0);
  }
  else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                                PLAYER_LOG_REQ_SET_FILENAME, this->device_addr))
  {
    player_log_set_filename_t* freq = (player_log_set_filename_t*)data;

    if(this->enable)
    {
      PLAYER_WARN("tried to switch filenames while logging");
      return(-1);
    }

    PLAYER_MSG1(1,"Closing logfile %s", this->filename);
    this->CloseFile();
    memset(this->filename,0,sizeof(this->filename));
    strncpy(this->filename,
            (const char*)freq->filename,
            freq->filename_count);
    this->filename[sizeof(this->filename)-1] = '\0';
    PLAYER_MSG1(1,"Opening logfile %s", this->filename);
    if(this->OpenFile() < 0)
    {
      PLAYER_WARN1("Failed to open logfile %s", this->filename);
      return(-1);
    }

    this->Publish(this->device_addr, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, PLAYER_LOG_REQ_SET_FILENAME);
    return(0);
  }
  else if(hdr->type == PLAYER_MSGTYPE_DATA)
  {
    // If logging is stopped, then don't log
    if(!this->enable)
      return(0);

    // Walk the device list
    for (int i = 0; i < this->device_count; i++)
    {
      WriteLogDevice * device = this->devices + i;

      if((device->addr.host != hdr->addr.host) ||
         (device->addr.robot != hdr->addr.robot) ||
         (device->addr.interf != hdr->addr.interf) ||
         (device->addr.index != hdr->addr.index))
        continue;


      // Write data to file
      this->Write(device, hdr, data);
      return(0);
    }
    return(-1);
  }
  return(-1);
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void
WriteLog::Main(void)
{
  if(this->OpenFile() < 0)
  {
    PLAYER_ERROR2("unable to open [%s]: %s\n", this->filename, strerror(errno));
    return;
  }

  while (1)
  {
    pthread_testcancel();

    // Wait on my queue
    this->Wait();


    if (write_particles_now){
      WriteLocalizeParticles();
      write_particles_now = false;
    }

    // Process all new messages (calls ProcessMessage on each)
    this->ProcessMessages();
  }
}


////////////////////////////////////////////////////////////////////////////
// Write data to file
void WriteLog::Write(WriteLogDevice *device,
                     player_msghdr_t* hdr,
                     void *data)
{
  //char host[256];
  player_interface_t iface;

  // Get interface name
  assert(device);
  ::lookup_interface_code(device->addr.interf, &iface);
  //gethostname(host, sizeof(host));

  // Write header info
  fprintf(this->file, "%014.3f %u %u %s %02u %03u %03u ",
          hdr->timestamp,
          device->addr.host,
          device->addr.robot,
          iface.name,
          device->addr.index,
          hdr->type,
          hdr->subtype);


  int retval;
  // Write the data
  switch (iface.interf)
  {
    case PLAYER_LASER_CODE:
      retval = this->WriteLaser(hdr, data);
      break;
    case PLAYER_RANGER_CODE:
      retval = this->WriteRanger(hdr, data);
      break;
    case PLAYER_LOCALIZE_CODE:
      retval = this->WriteLocalize(hdr, data);
      break;
    case PLAYER_POSITION2D_CODE:
      retval = this->WritePosition(hdr, data);
      break;
    case PLAYER_PTZ_CODE:
      retval = this->WritePTZ(hdr, data);
      break;
    case PLAYER_OPAQUE_CODE:
      retval = this->WriteOpaque(hdr, data);
      break;
    case PLAYER_SONAR_CODE:
      retval = this->WriteSonar(hdr, data);
      break;
    case PLAYER_WIFI_CODE:
      retval = this->WriteWiFi(hdr, data);
      break;
    case PLAYER_WSN_CODE:
      retval = this->WriteWSN(hdr, data);
      break;
    case PLAYER_COOPOBJECT_CODE:
      retval = this->WriteCoopObject(hdr, data);
      break;
    case PLAYER_IMU_CODE:
      retval = this->WriteIMU (hdr, data);
      break;
    case PLAYER_POINTCLOUD3D_CODE:
      retval = this->WritePointCloud3d (hdr, data);
      break;
    case PLAYER_ACTARRAY_CODE:
      retval = this->WriteActarray (hdr, data);
      break;
    case PLAYER_AIO_CODE:
      retval = this->WriteAIO(hdr, data);
      break;
    case PLAYER_DIO_CODE:
      retval = this->WriteDIO(hdr, data);
      break;
    case PLAYER_RFID_CODE:
      retval = this->WriteRFID(hdr, data);
      break;
    /* HHAA 15-02-2007 */
    case PLAYER_BUMPER_CODE:
      retval = this->WriteBumper(hdr, data);
      break;
    case PLAYER_IR_CODE:
      retval = this->WriteIR(hdr, data);
      break;
    case PLAYER_CAMERA_CODE:
      retval = this->WriteCamera(device, hdr, data);
      break;
    case PLAYER_FIDUCIAL_CODE:
      retval = this->WriteFiducial(hdr, data);
      break;
    case PLAYER_GPS_CODE:
      retval = this->WriteGps(hdr, data);
      break;
    case PLAYER_BLOBFINDER_CODE:
      retval = this->WriteBlobfinder(hdr, data);
      break;
    case PLAYER_JOYSTICK_CODE:
      retval = this->WriteJoystick(hdr, data);
      break;
    case PLAYER_POSITION3D_CODE:
      retval = this->WritePosition3d(hdr, data);
      break;
    case PLAYER_POWER_CODE:
      retval = this->WritePower(hdr, data);
      break;
#if 0
    case PLAYER_PLAYER_CODE:
      break;
#endif
    default:
      PLAYER_WARN1("unsupported interface type [%s]",
                   ::lookup_interface_name(0, iface.interf));
      retval = -1;
      break;
  }

  if(retval < 0)
    PLAYER_WARN2("not logging message to interface \"%s\" with subtype %d",
                 ::lookup_interface_name(0, iface.interf), hdr->subtype);

  fprintf(this->file, "\n");

  // Flush the data (some drivers produce a lot of data; we dont want
  // it to back up and slow us down later).
  fflush(this->file);

  return;
}


void
WriteLog::WriteLocalizeParticles()

{
  Message* msg;

  assert(localize_device != NULL);

  if(!(msg = localize_device->device->Request(this->InQueue,
                                              PLAYER_MSGTYPE_REQ,
                                              PLAYER_LOCALIZE_REQ_GET_PARTICLES,
                                              NULL, 0, NULL, true)))
  {
    // oh well.
    PLAYER_WARN("unable to get localize particles");
  }
  else
  {
    // log it
    this->Write(localize_device, msg->GetHeader(), msg->GetPayload());
    delete msg;
  }
}


/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_laser Laser format

@brief Laser log format

The following type:subtype laser messages can be logged:
- 1:1 (PLAYER_LASER_DATA_SCAN) - A scan.  The format is:
  - scan_id (int): unique, usually increasing index associated with the scan
  - min_angle (float): minimum scan angle, in radians
  - max_angle (float): maximum scan angle, in radians
  - resolution (float): angular resolution, in radians
  - max_range (float): maximum scan range, in meters
  - count (int): number of readings to follow
  - list of readings; for each reading:
    - range (float): in meters
    - intensity (int): intensity

- 1:2 (PLAYER_LASER_DATA_SCANPOSE) - A scan with an attached pose. The format is:
  - scan_id (int): unique, usually increasing index associated with the scan
  - px (float): X coordinate of the pose of the laser's parent object (e.g., the robot to which it is attached), in meters
  - py (float): Y coordinate of the pose of the laser's parent object (e.g., the robot to which it is attached), in meters
  - pa (float): yaw coordinate of the pose of the laser's parent object (e.g., the robot to which it is attached), in radians
  - min_angle (float): minimum scan angle, in radians
  - max_angle (float): maximum scan angle, in radians
  - resolution (float): angular resolution, in radians
  - max_range (float): maximum scan range, in meters
  - count (int): number of readings to follow
  - list of readings; for each reading:
    - range (float): in meters
    - intensity (int): intensity

- 4:1 (PLAYER_LASER_REQ_GET_GEOM) - Laser pose information. The format is:
  - lx (float): X coordinate of the laser's pose wrt its parent (e.g., the robot to which it is attached), in meters.
  - ly (float): Y coordinate of the laser's pose wrt its parent (e.g., the robot to which it is attached), in meters.
  - la (float): yaw coordinate of the laser's pose wrt its parent (e.g., the robot to which it is attached), in radians.
  - sx (float): length of the laser
  - sy (float): width of the laser
*/
int
WriteLog::WriteLaser(player_msghdr_t* hdr, void *data)
{
  size_t i;
  player_laser_data_t* scan;
  player_laser_data_scanpose_t* scanpose;
  player_laser_geom_t* geom;
  player_laser_data_scanangle_t* scanangle;

  // Check the type
  switch(hdr->type)
  {
    case PLAYER_MSGTYPE_DATA:
      // Check the subtype
      switch(hdr->subtype)
      {
        case PLAYER_LASER_DATA_SCAN:
          scan = (player_laser_data_t*)data;
          // Note that, in this format, we need a lot of precision in the
          // resolution field.

          fprintf(this->file, "%04d %+07.4f %+07.4f %+.8f %+07.4f %04d ",
                  scan->id, scan->min_angle, scan->max_angle,
                  scan->resolution, scan->max_range, scan->ranges_count);

          for (i = 0; i < scan->ranges_count; i++)
          {
            fprintf(this->file, "%.3f ", scan->ranges[i]);
            if(i < scan->intensity_count)
              fprintf(this->file, "%2d ", scan->intensity[i]);
            else
              fprintf(this->file, "%2d ", 0);
          }
          return(0);

        case PLAYER_LASER_DATA_SCANPOSE:
          scanpose = (player_laser_data_scanpose_t*)data;
          // Note that, in this format, we need a lot of precision in the
          // resolution field.

          fprintf(this->file, "%04d %+07.3f %+07.3f %+07.3f %+07.4f %+07.4f %+.8f %+07.4f %04d ",
                  scanpose->scan.id,
                  scanpose->pose.px, scanpose->pose.py, scanpose->pose.pa,
                  scanpose->scan.min_angle, scanpose->scan.max_angle,
                  scanpose->scan.resolution, scanpose->scan.max_range,
                  scanpose->scan.ranges_count);

          for (i = 0; i < scanpose->scan.ranges_count; i++)
          {
            fprintf(this->file, "%.3f ", scanpose->scan.ranges[i]);
            if(i < scanpose->scan.intensity_count)
              fprintf(this->file, "%2d ", scanpose->scan.intensity[i]);
            else
              fprintf(this->file, "%2d ", 0);
          }
          return(0);

        case PLAYER_LASER_DATA_SCANANGLE:
	  scanangle = (player_laser_data_scanangle_t*)data;
	  fprintf(this->file, "%04d %+07.4f %04d ",
		  scanangle->id, scanangle->max_range, scanangle->ranges_count);
	  
	  for (i = 0; i < scanangle->ranges_count; i++)
	    {
	      fprintf(this->file, "%.3f ", scanangle->ranges[i]);
	      fprintf(this->file, "%.3f ", scanangle->angles[i]);
	      if(i < scanangle->intensity_count)
		fprintf(this->file, "%2d ", scanangle->intensity[i]);
	      else
		fprintf(this->file, "%2d ", 0);
	    }
	  return(0);
	  
        default:
          return(-1);
      }
    case PLAYER_MSGTYPE_RESP_ACK:
      switch(hdr->subtype)
      {
        case PLAYER_LASER_REQ_GET_GEOM:
          geom = (player_laser_geom_t*)data;
          fprintf(this->file, "%+7.3f %+7.3f %7.3f %7.3f %7.3f",
                  geom->pose.px,
                  geom->pose.py,
                  geom->pose.pyaw,
                  geom->size.sl,
                  geom->size.sw);
          return(0);
        default:
          return(-1);
      }
    default:
      return(-1);
  }
}


/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_ranger Ranger format

@brief Ranger log format

The following type:subtype ranger messages can be logged:
- 1:1 (PLAYER_RANGER_DATA_RANGE) - A range scan.  The format is:
  - ranges_count (uint): number of ranges
  - list of ranges_count ranges:
    - range (double): distance 

- 1:2 (PLAYER_RANGER_DATA_RANGEPOSE) - A range scan optionally with
the (possibly estimated) geometry of the device when the scan was
acquired and optional sensor configuration. The format is:
  - ranges_count (uint): number of ranges
  - list of ranges_count ranges:
    - range (double): distance
  - have_geom (uint8): If non-zero, the geometry data has been filled
  - geometry of device at the time of range data:
    - pose of device:
      - px (float): X coordinate of the pose, in meters
      - py (float): Y coordinate of the pose, in meters
      - pz (float): Z coordinate of the pose, in meters
      - proll (float): roll coordinate of the pose, in radians
      - ppitch (float): pitch coordinate of the pose, in radians
      - pyaw (float): yaw coordinate of the pose, in radians
    - size of device:
      - sw (float): width of the device, in meters
      - sl (float): length of the device, in meters
      - sh (float): height of the device, in meters
    - element_poses_count (uint): pose of each individual range sensor that makes up the device
    - list of element_poses_count poses:
      - px (float): X coordinate of the pose, in meters
      - py (float): Y coordinate of the pose, in meters
      - pz (float): Z coordinate of the pose, in meters
      - proll (float): roll coordinate of the pose, in radians
      - ppitch (float): pitch coordinate of the pose, in radians
      - pyaw (float): yaw coordinate of the pose, in radians
    - element_sizes_count (uint): size of each individual range sensor that makes up the device
    - list of element_sizes_count sizes:
      - sw (float): width of the device, in meters
      - sl (float): length of the device, in meters
      - sh (float): height of the device, in meters
  - have_config(uint8): If non-zero, the config data has been filled
  - config of device:
    - min_angle (float): start angle of scans, in radians
    - max_angle (float): end angle of scans, in radians
    - angular_res (float): scan resolution, in radians
    - min_range (float): minimum range, in meters
    - max_range (float): maximum range, in meters
    - range_res (float): range resolution, in meters
    - frequency (float): scanning frequency, in Hz

- 1:3 (PLAYER_RANGER_DATA_INTNS) - An intensity scan.  The format is:
  - intensities_count (uint): number of intensities
  - list of intensities_count intensities:
    - intensity (double)

- 1:4 (PLAYER_RANGER_DATA_ITNSPOSE) - An intensity scan with an attached pose (estimated from the time of the scan). The format is:
  - intensities_count (uint): number of intensities
  - list of intensities_count intensities:
    - intensity (double)
  - have_geom (uint8): If non-zero, the geometry data has been filled
  - geometry of device at the time of intensity data:
    - pose of device:
      - px (float): X coordinate of the pose, in meters
      - py (float): Y coordinate of the pose, in meters
      - pz (float): Z coordinate of the pose, in meters
      - proll (float): roll coordinate of the pose, in radians
      - ppitch (float): pitch coordinate of the pose, in radians
      - pyaw (float): yaw coordinate of the pose, in radians
    - size of device:
      - sw (float): width of the device, in meters
      - sl (float): length of the device, in meters
      - sh (float): height of the device, in meters
    - element_poses_count (uint): pose of each individual range sensor that makes up the device
    - list of element_poses_count poses:
      - px (float): X coordinate of the pose, in meters
      - py (float): Y coordinate of the pose, in meters
      - pz (float): Z coordinate of the pose, in meters
      - proll (float): roll coordinate of the pose, in radians
      - ppitch (float): pitch coordinate of the pose, in radians
      - pyaw (float): yaw coordinate of the pose, in radians
    - element_sizes_count (uint): size of each individual range sensor that makes up the device
    - list of element_sizes_count sizes:
      - sw (float): width of the device, in meters
      - sl (float): length of the device, in meters
      - sh (float): height of the device, in meters
  - have_config(uint8): If non-zero, the config data has been filled
  - config of device:
    - min_angle (float): start angle of scans, in radians
    - max_angle (float): end angle of scans, in radians
    - angular_res (float): scan resolution, in radians
    - min_range (float): minimum range, in meters
    - max_range (float): maximum range, in meters
    - range_res (float): range resolution, in meters
    - frequency (float): scanning frequency, in Hz
    
- 4:1 (PLAYER_RANGER_REQ_GET_GEOM) - Ranger pose information. The format is:
  - pose of device:
    - px (float): X coordinate of the pose, in meters
    - py (float): Y coordinate of the pose, in meters
    - pz (float): Z coordinate of the pose, in meters
    - proll (float): roll coordinate of the pose, in radians
    - ppitch (float): pitch coordinate of the pose, in radians
    - pyaw (float): yaw coordinate of the pose, in radians
  - size of device:
    - sw (float): width of the device, in meters
    - sl (float): length of the device, in meters
    - sh (float): height of the device, in meters
  - element_poses_count (uint): pose of each individual range sensor that makes up the device
  - list of element_poses_count poses:
    - px (float): X coordinate of the pose, in meters
    - py (float): Y coordinate of the pose, in meters
    - pz (float): Z coordinate of the pose, in meters
    - proll (float): roll coordinate of the pose, in radians
    - ppitch (float): pitch coordinate of the pose, in radians
    - pyaw (float): yaw coordinate of the pose, in radians
  - element_sizes_count (uint): size of each individual range sensor that makes up the device
  - list of element_sizes_count sizes:
    - sw (float): width of the device, in meters
    - sl (float): length of the device, in meters
    - sh (float): height of the device, in meters
*/
int
WriteLog::WriteRanger(player_msghdr_t* hdr, void *data)
{
  size_t i;
  player_ranger_data_range_t* rscan;
  player_ranger_data_rangestamped_t* rscanpose;
  player_ranger_data_intns_t* iscan;
  player_ranger_data_intnsstamped_t* iscanpose;
  player_ranger_geom_t* geom;
  player_ranger_config_t* config;

  // Check the type
  switch(hdr->type)
  {
    case PLAYER_MSGTYPE_DATA:
      // Check the subtype
      switch(hdr->subtype)
      {
        case PLAYER_RANGER_DATA_RANGE:
          rscan = (player_ranger_data_range_t*)data;
          // Note that, in this format, we need a lot of precision in the
          // resolution field.

          fprintf(this->file, "%04d ", rscan->ranges_count);

          for (i = 0; i < rscan->ranges_count; i++)
	    {
	      fprintf(this->file, "%.3f ", rscan->ranges[i]);
	    }
          return(0);

        case PLAYER_RANGER_DATA_RANGESTAMPED:
          rscanpose = (player_ranger_data_rangestamped_t*)data;
          // Note that, in this format, we need a lot of precision in the
          // resolution field.

          fprintf(this->file, "%04d ", rscanpose->data.ranges_count);

          for (i = 0; i < rscanpose->data.ranges_count; i++)
	    {
	      fprintf(this->file, "%.3f ", rscanpose->data.ranges[i]);
	    }

          fprintf(this->file, "%d ", rscanpose->have_geom);

	  if (rscanpose->have_geom) 
	    {
	      fprintf(this->file, "%+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f ",
		      rscanpose->geom.pose.px, rscanpose->geom.pose.py, rscanpose->geom.pose.pz,
		      rscanpose->geom.pose.proll, rscanpose->geom.pose.ppitch, rscanpose->geom.pose.pyaw,
		      rscanpose->geom.size.sw, rscanpose->geom.size.sl, rscanpose->geom.size.sh);
	      
	      fprintf(this->file, "%04d ", rscanpose->geom.element_poses_count);
	  
	      for (i = 0; i < rscanpose->geom.element_poses_count; i++)
		{
		  fprintf(this->file, "%+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f ",
			  rscanpose->geom.element_poses[i].px, rscanpose->geom.element_poses[i].py, rscanpose->geom.element_poses[i].pz,
			  rscanpose->geom.element_poses[i].proll, rscanpose->geom.element_poses[i].ppitch, rscanpose->geom.element_poses[i].pyaw);
		}

	      fprintf(this->file, "%04d ", rscanpose->geom.element_sizes_count);
	  
	      for (i = 0; i < rscanpose->geom.element_sizes_count; i++)
		{
		  fprintf(this->file, "%+07.3f %+07.3f %+07.3f ",
			  rscanpose->geom.element_sizes[i].sw, rscanpose->geom.element_sizes[i].sl, rscanpose->geom.element_sizes[i].sh);
		}
	    }
	  
	  if (rscanpose->have_config) 
	    {
	      fprintf(this->file, "%.4f %.4f %.4f %.4f %.4f %.4f %.4f ",
		      rscanpose->config.min_angle, rscanpose->config.max_angle,
		      rscanpose->config.angular_res, rscanpose->config.min_range,
		      rscanpose->config.max_range, rscanpose->config.range_res,
		      rscanpose->config.frequency);
	    }
	  
          return(0);
	  
	  
        case PLAYER_RANGER_DATA_INTNS:
          iscan = (player_ranger_data_intns_t*)data;
          // Note that, in this format, we need a lot of precision in the
          // resolution field.

          fprintf(this->file, "%04d ", iscan->intensities_count);

          for (i = 0; i < iscan->intensities_count; i++)
	    {
	      fprintf(this->file, "%.3f ", iscan->intensities[i]);
	    }
          return(0);


        case PLAYER_RANGER_DATA_INTNSSTAMPED:
          iscanpose = (player_ranger_data_intnsstamped_t*)data;
          // Note that, in this format, we need a lot of precision in the
          // resolution field.

          fprintf(this->file, "%04d ", iscanpose->data.intensities_count);

          for (i = 0; i < iscanpose->data.intensities_count; i++)
	    {
	      fprintf(this->file, "%.3f ", iscanpose->data.intensities[i]);
	    }

          fprintf(this->file, "%d ", iscanpose->have_geom);

	  if (iscanpose->have_geom) 
	    {
	      fprintf(this->file, "%+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f ",
		      iscanpose->geom.pose.px, iscanpose->geom.pose.py, iscanpose->geom.pose.pz,
		      iscanpose->geom.pose.proll, iscanpose->geom.pose.ppitch, iscanpose->geom.pose.pyaw,
		      iscanpose->geom.size.sw, iscanpose->geom.size.sl, iscanpose->geom.size.sh);
	      
	      fprintf(this->file, "%04d ", iscanpose->geom.element_poses_count);
	  
	      for (i = 0; i < iscanpose->geom.element_poses_count; i++)
		{
		  fprintf(this->file, "%+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f ",
			  iscanpose->geom.element_poses[i].px, iscanpose->geom.element_poses[i].py, iscanpose->geom.element_poses[i].pz,
			  iscanpose->geom.element_poses[i].proll, iscanpose->geom.element_poses[i].ppitch, iscanpose->geom.element_poses[i].pyaw);
		}

	      fprintf(this->file, "%04d ", iscanpose->geom.element_sizes_count);
	  
	      for (i = 0; i < iscanpose->geom.element_sizes_count; i++)
		{
		  fprintf(this->file, "%+07.3f %+07.3f %+07.3f ",
			  iscanpose->geom.element_sizes[i].sw, iscanpose->geom.element_sizes[i].sl, iscanpose->geom.element_sizes[i].sh);
		}
	    }

	  if (iscanpose->have_config) 
	    {
	      fprintf(this->file, "%.4f %.4f %.4f %.4f %.4f %.4f %.4f ",
		      iscanpose->config.min_angle, iscanpose->config.max_angle,
		      iscanpose->config.angular_res, iscanpose->config.min_range,
		      iscanpose->config.max_range, iscanpose->config.range_res,
		      iscanpose->config.frequency);
	    }

          return(0);

        default:
          return(-1);
      }
    case PLAYER_MSGTYPE_RESP_ACK:
      switch(hdr->subtype)
      {
        case PLAYER_RANGER_REQ_GET_GEOM:
          geom = (player_ranger_geom_t*)data;
          fprintf(this->file, "%+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f ",
                  geom->pose.px,
                  geom->pose.py,
                  geom->pose.pz,
                  geom->pose.proll,
                  geom->pose.ppitch,
                  geom->pose.pyaw,
                  geom->size.sw,
		  geom->size.sl,
                  geom->size.sh);

	  fprintf(this->file, "%04d ", geom->element_poses_count);
	  
	  for (i = 0; i < geom->element_poses_count; i++)
	    {
	      fprintf(this->file, "%+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f ",
		      geom->element_poses[i].px, geom->element_poses[i].py, geom->element_poses[i].pz,
		      geom->element_poses[i].proll, geom->element_poses[i].ppitch, geom->element_poses[i].pyaw);
	    }
	  
	  fprintf(this->file, "%04d ", geom->element_sizes_count);
	  
	  for (i = 0; i < geom->element_sizes_count; i++)
	    {
	      fprintf(this->file, "%+07.3f %+07.3f %+07.3f ",
		      geom->element_sizes[i].sw, geom->element_sizes[i].sl, geom->element_sizes[i].sh);
	    }
	  
          return(0);

        case PLAYER_RANGER_REQ_GET_CONFIG:
          config = (player_ranger_config_t*)data;

	  fprintf(this->file, "%lf %lf %lf %lf %lf %lf %lf ",
		  config->min_angle, config->max_angle,
		  config->angular_res, config->min_range,
		  config->max_range, config->range_res,
		  config->frequency);
	  
	  return(0);
	  
        default:
          return(-1);
      }
    default:
      return(-1);
  }
}


/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_localize Localize format

@brief localize log format

The following type:subtype localize messages can be logged:
- 1:1 (PLAYER_LOCALIZE_DATA_HYPOTHS) - A set of pose hypotheses.  The format is:
  - pending_count (int): number of pending (unprocessed observations)
  - pending time (float): time stamp of the last observation processed
  - hypoths_coun (int): number of pose hypotheses
  - list of hypotheses; for each hypothesis
    - px (float): X coordinate of the mean value of the pose estimate (in m)
    - py (float): Y coordinate of the mean value of the pose estimate (in m)
    - pa (float): yaw coordinate of the mean value of the pose estimate (in rad)
    - cov[0] (float): X Variance of the covariance matrix to the pose estimate
    - cov[1] (float): Y Variance of the covariance matrix to the pose estimate
    - cov[2] (float): yaw Variance of the covariance matrix to the pose estimate
    - cov[3] (float): X,Y covariance of the pose estimate
    - cov[4] (float): Y,yaw covariance of the pose estimate
    - cov[5] (float): X,yaw covariance of the pose estimate
    - alpha (float): weight coefficient for linear combination

- 4:2 (PLAYER_LOCALIZE_REQ_GET_PARTICLES) - Current particle set. The format is:
  - px (float): X coordinate of best (?) pose (in m?)
  - py (float): Y coordinate of best (?) pose (in m?)
  - pa (float): yaw coordinate of best (?) pose (in ??)
  - variance (float): variance of the best(?) pose
  - particles_count (int): number of particles; for each particle:
    - px (float: X coordinate of particle's pose (in m)
    - py (float: Y coordinate of particle's pose (in m)
    - pa (float: yaw coordinate of particle's pose (in rad)
    - alpha (float): weight coefficient for linear combination
*/


int
WriteLog::WriteLocalize(player_msghdr_t* hdr, void *data)
{
  size_t i;
  player_localize_data_t* hypoths;
  player_localize_get_particles_t* particles;


  // Check the type
  switch(hdr->type)
  {
    case PLAYER_MSGTYPE_DATA:
      // Check the subtype
      switch(hdr->subtype)
      {
        case PLAYER_LOCALIZE_DATA_HYPOTHS:
          hypoths = (player_localize_data_t*)data;
          // Note that, in this format, we need a lot of precision in the
          // resolution field.


          fprintf(this->file, "%10d %+07.3f %2d ",
                  hypoths->pending_count, hypoths->pending_time,
                  hypoths->hypoths_count);

          for (i = 0; i < hypoths->hypoths_count; i++)
            fprintf(this->file, "%+7.3f %+7.3f %7.3f %7.3f %7.3f %7.3f %7.3f %7.3f %7.3f %7.3f ",
                    hypoths->hypoths[i].mean.px,
										hypoths->hypoths[i].mean.py,
										hypoths->hypoths[i].mean.pa,
										hypoths->hypoths[i].cov[0],
										hypoths->hypoths[i].cov[1],
										hypoths->hypoths[i].cov[2],
										hypoths->hypoths[i].cov[3],
										hypoths->hypoths[i].cov[4],
										hypoths->hypoths[i].cov[5],
										hypoths->hypoths[i].alpha);
          if (write_particles)
	    // every time we receive localize data also write localize particles
	    write_particles_now = true;
	  return(0);

        default:
          return(-1);
      }
    case PLAYER_MSGTYPE_RESP_ACK:
      switch(hdr->subtype)
      {
        case PLAYER_LOCALIZE_REQ_GET_PARTICLES:
          particles = (player_localize_get_particles_t*)data;
          fprintf(this->file, "%+7.3f %+7.3f %7.3f %7.3f %10d ",
                  particles->mean.px,
                  particles->mean.py,
                  particles->mean.pa,
		  particles->variance,
		  particles->particles_count);

          for (i = 0; i < particles->particles_count; i++)
	    fprintf(this->file, "%+7.3f %+7.3f %7.3f %7.3f ",
                    particles->particles[i].pose.px,
		    particles->particles[i].pose.py,
		    particles->particles[i].pose.pa,
		    particles->particles[i].alpha);
          return(0);
        default:
          return(-1);
      }
    default:
      return(-1);
  }
}

/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_position2d Position2d format

@brief position2d log format

The following type:subtype position2d messages can be logged:
- 1:1 (PLAYER_POSITION2D_DATA_STATE) Odometry information.  The format is:
  - px (float): X coordinate of the device's pose, in meters
  - py (float): Y coordinate of the device's pose, in meters
  - pa (float): yaw coordinate of the device's pose, in radians
  - vx (float): X coordinate of the device's velocity, in meters/sec
  - vy (float): Y coordinate of the device's velocity, in meters/sec
  - va (float): yaw coordinate of the device's velocity, in radians/sec
  - stall (int): Motor stall flag

- 4:1 (PLAYER_POSITION2D_REQ_GET_GEOM) Geometry info.  The format is:
  - px (float): X coordinate of the offset of the device's center of rotation, in meters
  - py (float): Y coordinate of the offset of the device's center of rotation, in meters
  - pa (float): yaw coordinate of the offset of the device's center of rotation, in radians
  - sx (float): The device's length, in meters
  - sy (float): The device's width, in meters
*/
int
WriteLog::WritePosition(player_msghdr_t* hdr, void *data)
{
  // Check the type
  switch(hdr->type)
  {
    case PLAYER_MSGTYPE_DATA:
      // Check the subtype
      switch(hdr->subtype)
      {
        case PLAYER_POSITION2D_DATA_STATE:
          {
            player_position2d_data_t* pdata =
                    (player_position2d_data_t*)data;
            fprintf(this->file,
                    "%+07.3f %+07.3f %+04.3f %+07.3f %+07.3f %+07.3f %d",
                    pdata->pos.px,
                    pdata->pos.py,
                    pdata->pos.pa,
                    pdata->vel.px,
                    pdata->vel.py,
                    pdata->vel.pa,
                    pdata->stall);
            return(0);
          }
        default:
          return(-1);
      }

    case PLAYER_MSGTYPE_RESP_ACK:
      // Check the subtype
      switch(hdr->subtype)
      {
        case PLAYER_POSITION2D_REQ_GET_GEOM:
          {
            player_position2d_geom_t* gdata =
                    (player_position2d_geom_t*)data;
            fprintf(this->file,
                    "%+07.3f %+07.3f %+04.3f %+07.3f %+07.3f",
                    gdata->pose.px,
                    gdata->pose.py,
                    gdata->pose.pyaw,
                    gdata->size.sl,
                    gdata->size.sw);

            return(0);
          }
        default:
          return(-1);
      }

    default:
      return(-1);
  }
}


/** @ingroup tutorial_datalog
 @defgroup player_driver_writelog_ptz PTZ format

@brief PTZ log format

The format for each @ref interface_wsn message is:
  - pan       (float): The pan angle/value
  - tilt      (float): The tilt angle/value
  - zoom      (float): The zoom factor
  - panspeed  (float): The current panning speed
  - tiltspeed (float): The current tilting speed
 */
int
WriteLog::WritePTZ (player_msghdr_t* hdr, void *data)
{
  // Check the type
  switch(hdr->type)
  {
    case PLAYER_MSGTYPE_DATA:
      // Check the subtype
      switch(hdr->subtype)
      {
        case PLAYER_PTZ_DATA_STATE:
          {
            player_ptz_data_t* pdata =
                    (player_ptz_data_t*)data;
            fprintf(this->file,
                    "%+07.3f %+07.3f %+04.3f %+07.3f %+07.3f",
                    pdata->pan,
                    pdata->tilt,
                    pdata->zoom,
                    pdata->panspeed,
                    pdata->tiltspeed);
            return(0);
          }
        default:
          return(-1);
      }
    default:
      return(-1);
  }
}

/** @ingroup tutorial_datalog
 @defgroup player_driver_writelog_opaque Opaque format

@brief opaque log format

The following type:subtype opaque messages can be logged:
- 1:1 (PLAYER_OPAQUE_DATA_STATE) Data information.  The format is:
  - data_count (uint32_t): Number of valid bytes to follow
  - list of bytes; for each byte:
    - data uint8_t:

- 2:2 (PLAYER_OPAQUE_CMD) Command information. The format is:
  - data_count (uint32_t): Number of valid bytes to follow
  - list of bytes; for each byte:
    - data uint8_t:
 */
int
WriteLog::WriteOpaque (player_msghdr_t* hdr, void *data)
{
  // Check the type
  switch(hdr->type)
  {
    case PLAYER_MSGTYPE_DATA:
      printf("Data State:\n");
      // Check the subtype
      switch(hdr->subtype)
      {
        case PLAYER_OPAQUE_DATA_STATE:
          {
            player_opaque_data_t* odata =
                    (player_opaque_data_t*)data;
            fprintf(this->file, "%04d ", odata->data_count);

            for (unsigned int i = 0; i < odata->data_count; i++)
            {
               fprintf(this->file, "%03d ", odata->data[i]);
            }

            return(0);
          }
        default:
          return(-1);
      }
    case PLAYER_MSGTYPE_CMD:
      printf("Data Command: \n");
      // Check the subtype
      switch(hdr->subtype)
      {
        case PLAYER_OPAQUE_CMD:
          {
            player_opaque_data_t* odata =
                    (player_opaque_data_t*)data;
            fprintf(this->file, "%04d ", odata->data_count);

            for (unsigned int i = 0; i < odata->data_count; i++)
            {
               fprintf(this->file, "%03d ", odata->data[i]);
            }

            return(0);
          }
        default:
          return(-1);
      }
    default:
      return(-1);
  }
}

/** @ingroup tutorial_datalog
 @defgroup player_driver_writelog_sonar Sonar format

@brief sonar log format

The following type:subtype sonar messages can be logged:
- 1:1 (PLAYER_SONAR_DATA_RANGES) Range data.  The format is:
  - range_count (int): number range values to follow
  - list of readings; for each reading:
    - range (float): in meters

- 1:2 (PLAYER_SONAR_DATA_GEOM) Geometry data. The format is:
  - pose_count (int): number of sonar poses to follow
  - list of tranducer poses; for each pose:
    - x (float): relative X position of transducer, in meters
    - y (float): relative Y position of transducer, in meters
    - a (float): relative yaw orientation of transducer, in radians

- 4:1 (PLAYER_SONAR_REQ_GET_GEOM) Geometry info. The format is:
  - pose_count (int): number of sonar poses to follow
  - list of tranducer poses; for each pose:
    - x (float): relative X position of transducer, in meters
    - y (float): relative Y position of transducer, in meters
    - a (float): relative yaw orientation of transducer, in radians
*/
int
WriteLog::WriteSonar(player_msghdr_t* hdr, void *data)
{
  unsigned int i;
  player_sonar_geom_t* geom;
  player_sonar_data_t* range_data;

  // Check the type
  switch(hdr->type)
  {
    case PLAYER_MSGTYPE_DATA:

      // Check the subtype
      switch(hdr->subtype)
      {
        case PLAYER_SONAR_DATA_GEOM:
          // Format:
          //   pose_count x0 y0 a0 x1 y1 a1 ...
          geom = (player_sonar_geom_t*)data;
          fprintf(this->file, "%u ", geom->poses_count);
          for(i=0;i<geom->poses_count;i++)
            fprintf(this->file, "%+07.3f %+07.3f %+07.4f ",
                    geom->poses[i].px,
                    geom->poses[i].py,
                    geom->poses[i].pyaw);
          return(0);

        case PLAYER_SONAR_DATA_RANGES:
          // Format:
          //   range_count r0 r1 ...
          range_data = (player_sonar_data_t*)data;
          fprintf(this->file, "%u ", range_data->ranges_count);
          for(i=0;i<range_data->ranges_count;i++)
            fprintf(this->file, "%.3f ", range_data->ranges[i]);

          return(0);
        default:
          return(-1);
      }

    case PLAYER_MSGTYPE_RESP_ACK:
      switch(hdr->subtype)
      {
        case PLAYER_SONAR_REQ_GET_GEOM:
          // Format:
          //   pose_count x0 y0 a0 x1 y1 a1 ...
          geom = (player_sonar_geom_t*)data;
          fprintf(this->file, "%u ", geom->poses_count);
          for(i=0;i<geom->poses_count;i++)
            fprintf(this->file, "%+07.3f %+07.3f %+07.4f ",
                    geom->poses[i].px,
                    geom->poses[i].py,
                    geom->poses[i].pyaw);

          return(0);
        default:
          return(-1);
      }
    default:
      return(-1);
  }
}

/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_wifi WiFi format

@brief wifi log format

The format for each @ref interface_wifi message is:
  - links_count (int): number of nodes to follow
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
int
WriteLog::WriteWiFi(player_msghdr_t* hdr, void *data)
{
  unsigned int i;
  char mac[32], ip[32], essid[32];
  player_wifi_data_t* wdata;

  // Check the type
  switch(hdr->type)
  {
    case PLAYER_MSGTYPE_DATA:
      // Check the subtype
      switch(hdr->subtype)
      {
	case PLAYER_WIFI_DATA_STATE:
	  wdata = (player_wifi_data_t*)data;
          fprintf(this->file, "%04d ", wdata->links_count);

          for (i = 0; i < wdata->links_count; i++)
          {
	    memset(mac,0,sizeof(mac));
	    memset(ip,0,sizeof(ip));
	    memset(essid,0,sizeof(essid));

	    assert(wdata->links[i].mac_count <= sizeof(mac));
	    assert(wdata->links[i].ip_count <= sizeof(ip));
	    assert(wdata->links[i].essid_count <= sizeof(essid));

	    memcpy(mac, wdata->links[i].mac, wdata->links[i].mac_count);
	    memcpy(ip, wdata->links[i].ip, wdata->links[i].ip_count);
            memcpy(essid, wdata->links[i].essid, wdata->links[i].essid_count);

            fprintf(this->file, "'%s' '%s' '%s' %d %d %d %d %d %d ",
                    mac, ip, essid,
                    wdata->links[i].mode,
                    wdata->links[i].freq,
                    wdata->links[i].encrypt,
                    wdata->links[i].qual,
                    wdata->links[i].level,
                    wdata->links[i].noise);
          }
          return(0);

        default:
          return(-1);
      }

   default:
     return(-1);
  }
}

/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_wsn WSN format

@brief WSN log format

The format for each @ref interface_wsn message is:
  - node_type      (int): The type of WSN node
  - node_id        (int): The ID of the WSN node
  - node_parent_id (int): The ID of the WSN node's parent (if existing)
  - data_packet         : The WSN node's data packet
    - light       (float): light measurement from a light sensor
    - mic         (float): accoustic measurement from a microphone
    - accel_x     (float): acceleration on X-axis from an acceleration sensor
    - accel_y     (float): acceleration on Y-axis from an acceleration sensor
    - accel_z     (float): acceleration on Z-axis from an acceleration sensor
    - magn_x      (float): magnetic measurement on X-axis from a magnetometer
    - magn_y      (float): magnetic measurement on Y-axis from a magnetometer
    - magn_z      (float): magnetic measurement on Z-axis from a magnetometer
    - temperature (float): temperature measurement from a temperature sensor
    - battery     (float): remaining battery voltage
 */
int
WriteLog::WriteWSN(player_msghdr_t* hdr, void *data)
{
    player_wsn_data_t* wdata;

  // Check the type
    switch(hdr->type)
    {
        case PLAYER_MSGTYPE_DATA:
      // Check the subtype
            switch(hdr->subtype)
            {
                case PLAYER_WSN_DATA_STATE:
                    wdata = (player_wsn_data_t*)data;
                    fprintf(this->file,"%d %d %d %f %f %f %f %f %f %f %f %f %f",
                            wdata->node_type,
                            wdata->node_id,
                            wdata->node_parent_id,
                            wdata->data_packet.light,
                            wdata->data_packet.mic,
                            wdata->data_packet.accel_x,
                            wdata->data_packet.accel_y,
                            wdata->data_packet.accel_z,
                            wdata->data_packet.magn_x,
                            wdata->data_packet.magn_y,
                            wdata->data_packet.magn_z,
                            wdata->data_packet.temperature,
                            wdata->data_packet.battery);
                    return(0);

                default:
                    return(-1);
            }

        default:
            return(-1);
    }
}


/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_coopobject CoopObject format

@brief CoopObject log format

The format for each @ref interface_coopobject message is:
  - node_id        	(int): The ID of the WSN node
  - node_parent_id	(int): The ID of the WSN node's parent (if existing)
  - node_type		(int): The type of WSN node
  - data_packet			: The WSN node's data packet
	- rssi_data				: The WSN node's RSSI data packet
		- sender_id 			(int): sender node ID
		- rssi     			(int): RSSI value
		- stamp	   			(int): Node stamp
		- nodeTimeHigh		(int): Node time most significant word
		- nodeTimeLow		(int): Node time less significant word
		- x					(int): Node position on X-axis (if known)
		- y					(int): Node position on Y-axis (if known)
		- z					(int): Node position on Z-axis (if known)
    - sensor_data_count	(int): Number of measurements from sensors
	- list of sensor measurements (int,int) (player_coopobject_sensor_t)
    - alarm_data_count	(int): Number of measurements from alarms
	- list of alarm measurements (int,int) (player_coopobject_alarm_t)
	- type_undefined 	(int): Undefined data type
    - undefined_data_count 	(int): Number of undefined data bytes
	- list of undefined data bytes (int)

 */
int
WriteLog::WriteCoopObject(player_msghdr_t* hdr, void *data)
{
	unsigned int i;

  // Check the type
    switch(hdr->type)
    {
        case PLAYER_MSGTYPE_DATA:
      // Check the subtype
            switch(hdr->subtype)
            {
                case PLAYER_COOPOBJECT_DATA_HEALTH:
		{
                    player_coopobject_header_t *wdata = (player_coopobject_header_t*)data;

                    fprintf(this->file,"%d %d %d ",
                            wdata->id,
                            wdata->parent_id,
			    wdata->origin);

                    return(0);
		    break;
		}
                case PLAYER_COOPOBJECT_DATA_RSSI:
		{
                    player_coopobject_rssi_t *wdata = (player_coopobject_rssi_t*)data;

                    fprintf(this->file,"%d %d %d %d %d %d %d %d %f %f %f ",
                            wdata->header.id,
                            wdata->header.parent_id,
			    wdata->header.origin,
                            wdata->sender_id,
                            wdata->rssi,
                            wdata->stamp,
                            wdata->nodeTimeHigh,
                            wdata->nodeTimeLow,
                            wdata->x,
			    wdata->y,
                            wdata->z);
                    return(0);
		    break;
		}
                case PLAYER_COOPOBJECT_DATA_SENSOR:
		{
                    player_coopobject_data_sensor_t *wdata = (player_coopobject_data_sensor_t*)data;
                    fprintf(this->file,"%d %d %d ",
                            wdata->header.id,
                            wdata->header.parent_id,
			    wdata->header.origin);

		    fprintf (this->file, "%d ", wdata->data_count);
		    for (i = 0; i < wdata->data_count; i++)
			fprintf (this->file,"%d %d ", wdata->data[i].type, wdata->data[i].value);

                    return(0);
		    break;
		}
                case PLAYER_COOPOBJECT_DATA_ALARM:
		{
                    player_coopobject_data_sensor_t *wdata = (player_coopobject_data_sensor_t*)data;
                    fprintf(this->file,"%d %d %d ",
                            wdata->header.id,
                            wdata->header.parent_id,
			    wdata->header.origin);

		    fprintf (this->file, "%d ", wdata->data_count);
		    for (i = 0; i < wdata->data_count; i++)
			fprintf (this->file,"%d %d ", wdata->data[i].type, wdata->data[i].value);

                    return(0);
		    break;
		}
                case PLAYER_COOPOBJECT_DATA_USERDEFINED:
		{
                    player_coopobject_data_userdefined_t *wdata = (player_coopobject_data_userdefined_t*)data;
                    fprintf(this->file,"%d %d %d ",
                            wdata->header.id,
                            wdata->header.parent_id,
			    wdata->header.origin);

		    fprintf (this->file, "%d %d ", wdata->type, wdata->data_count);
		    for (i = 0; i < wdata->data_count; i++)
			fprintf (this->file,"%d ", wdata->data[i]);

		    return(0);
		    break;
		}
                case PLAYER_COOPOBJECT_DATA_REQUEST:
		{
                    player_coopobject_req_t *wdata = (player_coopobject_req_t*)data;
                    fprintf(this->file,"%d %d %d %d ",
                            wdata->header.id,
                            wdata->header.parent_id,
			    wdata->header.origin);

		    fprintf (this->file, "%d %d ", wdata->request, wdata->parameters_count);
		    for (i = 0; i < wdata->parameters_count; i++)
			fprintf (this->file,"%d ", wdata->parameters[i]);

                    return(0);
		    break;
		}
		case PLAYER_COOPOBJECT_DATA_COMMAND:
		{
                    player_coopobject_cmd_t *wdata = (player_coopobject_cmd_t*)data;
                    fprintf(this->file,"%d %d %d %d ",
                            wdata->header.id,
                            wdata->header.parent_id,
			    wdata->header.origin);

		    fprintf (this->file, "%d %d ", wdata->command, wdata->parameters_count);
		    for (i = 0; i < wdata->parameters_count; i++)
			fprintf (this->file,"%d ", wdata->parameters[i]);

                    return(0);
				break;
				}

                default:
                    return(-1);
            }

/*		case PLAYER_MSGTYPE_CMD:
		fprintf(this->file,"cmd\n");
      // Check the subtype
            switch(hdr->subtype)
            {
                case PLAYER_COOPOBJECT_CMD_DATA:
				{
                    player_coopobject_cmd_data_t *wdata = (player_coopobject_cmd_data_t*)data;
                    fprintf(this->file,"%d %d %f %f %f %d ",
                            wdata->node_id,
                            wdata->source_id,
			    wdata->pos.px,
			    wdata->pos.py,
			    wdata->pos.pa,
		wdata->status );

			    fprintf (this->file, "%d %d ", wdata->data_type, wdata->data_count);
			    for (i = 0; i < wdata->data_count; i++)
			      fprintf (this->file,"%d ", wdata->data[i]);

                    return(0);
				break;
				}

                default:
                    return(-1);
			}
*/
        default:
            return(-1);
    }
}

/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_imu IMU format

@brief IMU log format

The format for each @ref interface_imu message is:
    - for PLAYER_IMU_DATA_STATE (player_imu_data_state_t):
	-> px     (float): X pose
	-> py     (float): Y pose
	-> pz     (float): Z pose
	-> proll  (float): roll angle
	-> ppitch (float): pitch angle
	-> pyaw   (float): yaw angle
    - for PLAYER_IMU_DATA_CALIB (player_imu_data_calib_t):
	-> accel_x (float): acceleration value for X axis
	-> accel_y (float): acceleration value for Y axis
	-> accel_z (float): acceleration value for Z axis
	-> gyro_x  (float): gyroscope value for X axis
	-> gyro_y  (float): gyroscope value for Y axis
	-> gyro_z  (float): gyroscope value for Z axis
	-> magn_x  (float): magnetometer value for X axis
	-> magn_y  (float): magnetometer value for Y axis
	-> magn_z  (float): magnetometer value for Z axis
    - for PLAYER_IMU_DATA_QUAT (player_imu_data_quat_t):
	-> accel_x (float): acceleration value for X axis
	-> accel_y (float): acceleration value for Y axis
	-> accel_z (float): acceleration value for Z axis
	-> gyro_x  (float): gyroscope value for X axis
	-> gyro_y  (float): gyroscope value for Y axis
	-> gyro_z  (float): gyroscope value for Z axis
	-> magn_x  (float): magnetometer value for X axis
	-> magn_y  (float): magnetometer value for Y axis
	-> magn_z  (float): magnetometer value for Z axis
	-> q0, q1, q2, q3 (floats): quaternion values
    - for PLAYER_IMU_DATA_EULER (player_imu_data_euler_t):
	-> accel_x (float): acceleration value for X axis
	-> accel_y (float): acceleration value for Y axis
	-> accel_z (float): acceleration value for Z axis
	-> gyro_x  (float): gyroscope value for X axis
	-> gyro_y  (float): gyroscope value for Y axis
	-> gyro_z  (float): gyroscope value for Z axis
	-> magn_x  (float): magnetometer value for X axis
	-> magn_y  (float): magnetometer value for Y axis
	-> magn_z  (float): magnetometer value for Z axis
	-> proll   (float): roll angle
	-> ppitch  (float): pitch angle
	-> pyaw    (float): yaw angle
	- for PLAYER_IMU_DATA_FULLSTATE (player_imu_data_fullstate_t):
	-> pose_px     (float): X pose
	-> pose_py     (float): Y pose
	-> pose_pz     (float): Z pose
	-> pose_proll  (float): roll angle
	-> pose_ppitch (float): pitch angle
	-> pose_pyaw   (float): yaw angle
	-> vel_px     (float): X velocity
	-> vel_py     (float): Y velocity
	-> vel_pz     (float): Z velocity
	-> vel_proll  (float): roll anglular velocity
	-> vel_ppitch (float): pitch anglular velocity
	-> vel_pyaw   (float): yaw anglular velocity
	-> acc_px     (float): X acceleration
	-> acc_py     (float): Y acceleration
	-> acc_pz     (float): Z acceleration
 */
int
WriteLog::WriteIMU (player_msghdr_t* hdr, void *data)
{
  // Check the type
    switch(hdr->type)
    {
        case PLAYER_MSGTYPE_DATA:
      // Check the subtype
            switch(hdr->subtype)
            {
                case PLAYER_IMU_DATA_STATE:
		{
		    player_imu_data_state_t* idata;
                    idata = (player_imu_data_state_t*)data;
                    fprintf (this->file,"%f %f %f %f %f %f",
                            idata->pose.px,
                            idata->pose.py,
                            idata->pose.pz,
                            idata->pose.proll,
                            idata->pose.ppitch,
                            idata->pose.pyaw);
                    return (0);
		}

                case PLAYER_IMU_DATA_CALIB:
		{
		    player_imu_data_calib_t* idata;
                    idata = (player_imu_data_calib_t*)data;
                    fprintf (this->file,"%f %f %f %f %f %f %f %f %f",
                            idata->accel_x,
                            idata->accel_y,
                            idata->accel_z,
                            idata->gyro_x,
                            idata->gyro_y,
			    idata->gyro_z,
			    idata->magn_x,
			    idata->magn_y,
                            idata->magn_z);
                    return (0);
		}

                case PLAYER_IMU_DATA_QUAT:
		{
		    player_imu_data_quat_t* idata;
                    idata = (player_imu_data_quat_t*)data;
                    fprintf (this->file,"%f %f %f %f %f %f %f %f %f %f %f %f %f",
                            idata->calib_data.accel_x,
                            idata->calib_data.accel_y,
                            idata->calib_data.accel_z,
                            idata->calib_data.gyro_x,
                            idata->calib_data.gyro_y,
			    idata->calib_data.gyro_z,
			    idata->calib_data.magn_x,
			    idata->calib_data.magn_y,
                            idata->calib_data.magn_z,
                            idata->q0,
                            idata->q1,
                            idata->q2,
                            idata->q3);
                    return (0);
		}

                case PLAYER_IMU_DATA_EULER:
		{
		    player_imu_data_euler_t* idata;
                    idata = (player_imu_data_euler_t*)data;
                    fprintf (this->file,"%f %f %f %f %f %f %f %f %f %f %f %f",
                            idata->calib_data.accel_x,
                            idata->calib_data.accel_y,
                            idata->calib_data.accel_z,
                            idata->calib_data.gyro_x,
                            idata->calib_data.gyro_y,
			    idata->calib_data.gyro_z,
			    idata->calib_data.magn_x,
			    idata->calib_data.magn_y,
                            idata->calib_data.magn_z,
                            idata->orientation.proll,
                            idata->orientation.ppitch,
                            idata->orientation.pyaw);
                    return (0);
		}
		
				case PLAYER_IMU_DATA_FULLSTATE:
				{
					player_imu_data_fullstate_t* idata;
					idata = (player_imu_data_fullstate_t*)data;
					fprintf (this->file,"%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
							  idata->pose.px,
							  idata->pose.py,
							  idata->pose.pz,
							  idata->pose.proll,
							  idata->pose.ppitch,
							  idata->pose.pyaw,
							  idata->vel.px,
							  idata->vel.py,
							  idata->vel.pz,
							  idata->vel.proll,
							  idata->vel.ppitch,
							  idata->vel.pyaw,
							  idata->acc.px,
							  idata->acc.py,
							  idata->acc.pz);
							  return (0);
				}

                default:
                    return (-1);
            }

        default:
            return (-1);
    }
}

/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_pointcloud3d Pointcloud3d format

@brief PointCloud3D log format

The format for each @ref interface_pointcloud3d message is:
    - points_count (int): the number of elements in the 3d point cloud
    - list of elements; for each element:
	- point.px (float): X [m]
	- point.py (float): Y [m]
	- point.pz (float): Z [m]
 */
int
WriteLog::WritePointCloud3d (player_msghdr_t* hdr, void *data)
{
  unsigned int i;
  // Check the type
    switch(hdr->type)
    {
        case PLAYER_MSGTYPE_DATA:
      // Check the subtype
            switch(hdr->subtype)
            {
                case PLAYER_POINTCLOUD3D_DATA_STATE:
		{
		    player_pointcloud3d_data_t* pdata;
                    pdata = (player_pointcloud3d_data_t*)data;
		    fprintf (this->file, "%d ", pdata->points_count);
		    for (i = 0; i < pdata->points_count; i++)
			fprintf (this->file,"%f %f %f ",
                            pdata->points[i].point.px,
                            pdata->points[i].point.py,
                            pdata->points[i].point.pz);
                    return (0);
		}

                default:
                    return (-1);
            }

        default:
            return (-1);
    }
}

/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_actarray Actarray format

@brief Actarray log format

The format for each @ref interface_actarray message is:
    - actuators_count (int): the number of actuators in the array
    - list of actuators; for each actuator:
	- position (float)     : the position of the actuator
	- speed (float)        : the speed of the actuator
	- acceleration (float) : the acceleration of the actuator
	- current (float)      : the current of the actuator
	- state (byte)         : the current state of the actuator
    - motor_state (byte): power state
 */
int
WriteLog::WriteActarray (player_msghdr_t* hdr, void *data)
{
  unsigned int i;
  // Check the type
    switch(hdr->type)
    {
      case PLAYER_MSGTYPE_DATA:
        // Check the subtype
        switch(hdr->subtype)
        {
          case PLAYER_ACTARRAY_DATA_STATE:
            player_actarray_data_t* pdata;
            pdata = (player_actarray_data_t*)data;
            fprintf (this->file, "%d ", pdata->actuators_count);
            for (i = 0; i < pdata->actuators_count; i++)
              fprintf (this->file,"%f %f %f %f %d ",
                        pdata->actuators[i].position,
                        pdata->actuators[i].speed,
                        pdata->actuators[i].acceleration,
                        pdata->actuators[i].current,
                        pdata->actuators[i].state);
              fprintf (this->file, "%d ", pdata->motor_state);
            delete[] pdata->actuators;
            return (0);
          default:
            return (-1);
        }
        break;
      default:
        return (-1);
    }
}

/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_aio AIO format

@brief AIO log format

The format for each @ref interface_aio message is:
  - voltages_count (int): the numer of analog input voltages
  - list of inputs; for each input:
    - voltage (float): the voltage on that input

 */
int
WriteLog::WriteAIO(player_msghdr_t* hdr, void* data)
{
  // Check the type
  switch (hdr->type) {
    case PLAYER_MSGTYPE_DATA:
      // Check the subtype
      switch (hdr->subtype) {
        case PLAYER_AIO_DATA_STATE: {
            player_aio_data_t* inputs(static_cast<player_aio_data_t*>(data));

            fprintf(this->file, "%04d ", inputs->voltages_count);

            for (float *v(inputs->voltages);
                 v != inputs->voltages + inputs->voltages_count; ++v)
              fprintf(this->file, "%.3f ", *v);

            return 0;
          }
        default:
          PLAYER_WARN1("cannot log unknown aio data subtype '%d'",
                       hdr->subtype);
          return -1;
      }
    default:
      PLAYER_WARN1("cannot log unknown aio message type '%d'", hdr->type);
      return -1;
  }
}

/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_dio DIO format

@brief DIO log format

The format for each @ref interface_dio message is:
  - count (int): the numer of digital inputs
  - list of input signals; for each input:
      state (string): '0' or '1'

 */
int
WriteLog::WriteDIO(player_msghdr_t* hdr, void* data)
{
  // Check the type
  switch (hdr->type) {
    case PLAYER_MSGTYPE_DATA:
      // Check the subtype
      switch (hdr->subtype) {
        case PLAYER_DIO_DATA_VALUES: {
            player_dio_data_t* inputs(static_cast<player_dio_data_t*>(data));

            // check for buffer overrun
            if (inputs->count > /* MAX_INPUTS */ 32) {
                // this shouldn't happen
                PLAYER_ERROR("count too big for bitfield");
                return -1;
            }

            fprintf(this->file, "%04d ", inputs->count);

            for (uint32_t mask(1); mask != (1ul << inputs->count); mask <<= 1)
              fprintf(this->file, "%d ", !!(mask & inputs->bits));

            return 0;
          }
        default:
          PLAYER_WARN1("cannot log unknown dio data subtype '%d'",
                       hdr->subtype);
          return -1;
      }
    default:
      PLAYER_WARN1("cannot log unknown dio message type '%d'", hdr->type);
      return -1;
  }
}

/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_rfid RFID format

@brief RFID log format

The format for each @ref interface_rfid message is:
  - tags_count (int): the numer of tags found
  - list of tags; for each tag:
    - type (int): type of the tag
    - guid (string): tag guid in hex

 */
int
WriteLog::WriteRFID(player_msghdr_t* hdr, void* data)
{
  // Check the type
  switch (hdr->type) {
    case PLAYER_MSGTYPE_DATA:
      // Check the subtype
      switch (hdr->subtype) {
        case PLAYER_RFID_DATA_TAGS: {
            player_rfid_data_t* rdata(static_cast<player_rfid_data_t*>(data));

            fprintf(file, "%04lu ", (long)rdata->tags_count);

            for (player_rfid_tag_t *t(rdata->tags);
                 t != rdata->tags + rdata->tags_count; ++t) {
              char *str;
              if ((str = new char[t->guid_count * 2 + 1]) == NULL)
              {
                PLAYER_ERROR("Failed to allocate space for str");
				return -1;
              }
              memset(str, '\0', sizeof(str));
              EncodeHex(str, sizeof(str), t->guid, t->guid_count);
              fprintf(file, "%04lu %s ", (long)t->type, str);
			  delete[] str;
            }

            return 0;
          }
        default:
          PLAYER_WARN1("cannot log unknown rfid data subtype '%d'",
                       hdr->subtype);
          return -1;
      }
    default:
      PLAYER_WARN1("cannot log unknown rfid message type '%d'", hdr->type);
      return -1;
  }
}
/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_ir IR format

@brief IR log format

The format for each @ref interface_ir message is:
  - ranges_count (uint): the numer of IR ranges present
  - list of inputs; for each input:
    - range (float): Detected range from sensor

 */

int
WriteLog::WriteIR(player_msghdr_t* hdr, void *data)
{
  unsigned int i;
  player_ir_pose_t* geom;
  player_ir_data_t* ir_data;

  // Check the type
  switch(hdr->type)
  {
    case PLAYER_MSGTYPE_DATA:

      // Check the subtype
      switch(hdr->subtype)
      {

        case PLAYER_IR_DATA_RANGES:
          // Format:
          //   bumpers_count bumper0 bumper1 ...
          ir_data = ( player_ir_data_t*)data;
          fprintf(this->file, "%u ", ir_data->ranges_count);
          for(i=0;i<ir_data->ranges_count;i++)
            // P2OS infrared lights are binary but I will use the %3.3f format
            fprintf(this->file, "%3.3f ", ir_data->ranges[i]);

          return(0);
        default:
          return(-1);
      }

    case PLAYER_MSGTYPE_RESP_ACK:
      switch(hdr->subtype)
      {
        case PLAYER_IR_REQ_POSE:
          // Format:
          //   bumper_def_count x0 y0 a0 l0 r0 x1 y1 a1 l1 r1...
          geom = (player_ir_pose_t*)data;
          fprintf(this->file, "%u ", geom->poses_count);
          for(i=0;i<geom->poses_count;i++)
            fprintf(this->file, "%+07.3f %+07.3f %+07.4f ",
                    geom->poses[i].px,
                    geom->poses[i].py,
                    geom->poses[i].pyaw);
          return(0);
        default:
          return(-1);
      }
    default:
      return(-1);
  }
}


/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_bumper Bumper format

@brief bumper log format

The format for each @ref interface_bumper message is:
  - bumper_count (uint): the numer of IR ranges present
  - list of inputs; for each input:
    - state (uint): Bumper bumped state

 */
int
WriteLog::WriteBumper(player_msghdr_t* hdr, void *data)
{
  unsigned int i;
  player_bumper_geom_t* geom;
  player_bumper_data_t* bumper_data;

  // Check the type
  switch(hdr->type)
  {
    case PLAYER_MSGTYPE_DATA:

      // Check the subtype
      switch(hdr->subtype)
      {
        case PLAYER_BUMPER_DATA_GEOM:
          // Format:
          //   bumper_def_count x0 y0 a0 l0 r0 x1 y1 a1 l1 r1...
          geom = (player_bumper_geom_t*)data;
          fprintf(this->file, "%u ", geom->bumper_def_count);
          for(i=0;i<geom->bumper_def_count;i++)
            fprintf(this->file, "%+07.3f %+07.3f %+07.4f %+07.4f %+07.4f ",
                    geom->bumper_def[i].pose.px,
                    geom->bumper_def[i].pose.py,
                    geom->bumper_def[i].pose.pyaw,
		    geom->bumper_def[i].length,
		    geom->bumper_def[i].radius);
          return(0);

        case PLAYER_BUMPER_DATA_STATE:
          // Format:
          //   bumpers_count bumper0 bumper1 ...
          bumper_data = ( player_bumper_data_t*)data;
          fprintf(this->file, "%u ", bumper_data->bumpers_count);
          for(i=0;i<bumper_data->bumpers_count;i++)
            fprintf(this->file, "%u ", bumper_data->bumpers[i]);

          return(0);
        default:
          return(-1);
      }

    case PLAYER_MSGTYPE_RESP_ACK:
      switch(hdr->subtype)
      {
        case PLAYER_BUMPER_REQ_GET_GEOM:
          // Format:
          //   bumper_def_count x0 y0 a0 l0 r0 x1 y1 a1 l1 r1...
          geom = (player_bumper_geom_t*)data;
          fprintf(this->file, "%u ", geom->bumper_def_count);
          for(i=0;i<geom->bumper_def_count;i++)
            fprintf(this->file, "%+07.3f %+07.3f %+07.4f %+07.4f %+07.4f ",
                    geom->bumper_def[i].pose.px,
                    geom->bumper_def[i].pose.py,
                    geom->bumper_def[i].pose.pyaw,
		    geom->bumper_def[i].length,
		    geom->bumper_def[i].radius);

          return(0);
        default:
          return(-1);
      }
    default:
      return(-1);
  }
}



/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_blobfinder Blobfinder format

@brief blobfinder log format

The format for each @ref interface_blobfinder message is:
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
int WriteLog::WriteBlobfinder(player_msghdr_t* hdr, void *data)
{
  player_blobfinder_data_t* bdata;
  bdata = (player_blobfinder_data_t*) data;
  
  switch(hdr->type){
    case PLAYER_MSGTYPE_DATA:

	  fprintf(this->file, "%d %d %d",
		bdata->width,
		bdata->height,
		bdata->blobs_count);

	  for(int i=0; i < (int)bdata->blobs_count; i++)
	  {
	  	fprintf(this->file, " %d %d %d %d %d %d %d %d %d %f",
		  	bdata->blobs[i].id,
		  	bdata->blobs[i].color,
		  	bdata->blobs[i].area,
			bdata->blobs[i].x,
			bdata->blobs[i].y,
			bdata->blobs[i].left,
			bdata->blobs[i].right,
			bdata->blobs[i].top,
			bdata->blobs[i].bottom,
			bdata->blobs[i].range);
	}

	return (0);
    default:
    	return (-1);
    }
}


/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_camera Camera format

@brief camera data format

The format for each @ref interface_camera message is:
  - width (int): in pixels
  - height (int): in pixels
  - depth (int): in bits per pixel
  - format (int): image format
  - compression (int): image compression
  - (optional) image data, encoded as a string of ASCII hex values
*/

int WriteLog::WriteCamera(WriteLogDevice *device, player_msghdr_t* hdr, void *data)
{
    player_camera_data_t *camera_data;
    switch(hdr->type)
    {
        case PLAYER_MSGTYPE_DATA:
            switch(hdr->subtype)
            {
                case PLAYER_CAMERA_DATA_STATE:
                    camera_data = (player_camera_data_t *) data;

                    // Image format
                    fprintf(this->file, "%d %d %d %d %d %d " ,
                            camera_data->width, camera_data->height,
                            camera_data->bpp, camera_data->format,
                            camera_data->compression, camera_data->image_count);

                    if(this->cameraLogImages)
                    {
                        char *str;
                        size_t src_size, dst_size;

                        // Check image size
                        src_size = camera_data->image_count;
                        dst_size = ::EncodeHexSize(src_size);
                        str = (char*) malloc(dst_size + 1);

                        // Encode image into string
                        ::EncodeHex(str, dst_size, camera_data->image, src_size);

                        // Write image bytes
                        fprintf(this->file, "%s", str);
                        free(str);
                    }
                    if(this->cameraSaveImages)
                    {
                        FILE *file;
                        char filename[1024];

			
			if (camera_data->compression == PLAYER_CAMERA_COMPRESS_RAW) {
			  snprintf(filename, sizeof(filename), "%s/%s_camera_%02d_%06d.pnm",
				   this->log_directory, this->filestem, device->addr.index, device->cameraFrame++);
			} else if (camera_data->compression == PLAYER_CAMERA_COMPRESS_JPEG) {
			  snprintf(filename, sizeof(filename), "%s/%s_camera_%02d_%06d.jpg",
				   this->log_directory, this->filestem, device->addr.index, device->cameraFrame++);
			} else {
                          PLAYER_WARN("unsupported compression method");
                          return -1;
			}

			file = fopen(filename, "w+");
			if (file == NULL)
			  return -1;
			
			if (camera_data->compression == PLAYER_CAMERA_COMPRESS_RAW) {
			  
			  if (camera_data->format == PLAYER_CAMERA_FORMAT_RGB888)
			    {
			      // Write ppm header
			      fprintf(file, "P6\n%d %d\n%d\n", camera_data->width, camera_data->height, 255);
			      fwrite(camera_data->image, 1, camera_data->image_count, file);
			    }
			  else if (camera_data->format == PLAYER_CAMERA_FORMAT_MONO8)
			    {
			      // Write pgm header
			      fprintf(file, "P5\n%d %d\n%d\n", camera_data->width, camera_data->height, 255);
			      fwrite(camera_data->image, 1, camera_data->image_count, file);
			    }
			  else
			    {
			      PLAYER_WARN("unsupported image format");
			    }
			  
			} else if (camera_data->compression == PLAYER_CAMERA_COMPRESS_JPEG) {
			  fwrite(camera_data->image, 1, camera_data->image_count, file);
			}

			fclose(file);
		    }
                    return 0;
                default:
                    return -1;
            }
        default:
            return -1;
    }
    return -1;
}

/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_fiducial Fiducial format

@brief fiducial log format

The following messages from @ref interface_fiducial interface are logged:

PLAYER_MSGTYPE_DATA:PLAYER_FIDUCIAL_DATA_SCAN (1:1) has the folowing format:
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

PLAYER_MSGTYPE_RESP_ACK:PLAYER_FIDUCIAL_REQ_GET_GEOM (4:1) has the following format:
    - x (float): relative X position, in meters
    - y (float): relative Y position, in meters
    - z (float): relative Z position, in meters
    - roll (float): relative roll orientation, in radians
    - pitch (float): relative pitch orientation, in radians
    - yaw (float): relative yaw orientation, in radians
    - length (float): fiducial finder length, in meters
    - width (float): fiducial finder width, in meters
    - height (float): fiducial finder height, in meters
    - fiducial_length (float): fiducial marker length, in meters
    - fiducial_width (float): fiducial marker width, in meters
*/
int WriteLog::WriteFiducial(player_msghdr_t* hdr, void *data)
{
  player_fiducial_data_t *fiducial_data;
  player_fiducial_geom_t *fiducial_geom;

  switch (hdr->type) {
        case PLAYER_MSGTYPE_DATA:
            switch (hdr->subtype) {
                case PLAYER_FIDUCIAL_DATA_SCAN:
                    fiducial_data = (player_fiducial_data_t*) data;
                    // format: <count> [<id> <x> <y> <z> <roll> <pitch> <yaw> <ux> <uy> <uz> <uroll> <upitch> <uyaw>] ...
                    fprintf(this->file, "%d", fiducial_data->fiducials_count);
                    for (unsigned i = 0; i < fiducial_data->fiducials_count; i++) {
                        fprintf(this->file, " %d"
                                " %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f"
                                " %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f %+07.3f",
                                fiducial_data->fiducials[i].id,
                                fiducial_data->fiducials[i].pose.px,
                                fiducial_data->fiducials[i].pose.py,
                                fiducial_data->fiducials[i].pose.pz,
                                fiducial_data->fiducials[i].pose.proll,
                                fiducial_data->fiducials[i].pose.ppitch,
                                fiducial_data->fiducials[i].pose.pyaw,
                                fiducial_data->fiducials[i].upose.px,
                                fiducial_data->fiducials[i].upose.py,
                                fiducial_data->fiducials[i].upose.pz,
                                fiducial_data->fiducials[i].upose.proll,
                                fiducial_data->fiducials[i].upose.ppitch,
                                fiducial_data->fiducials[i].upose.pyaw);
                    }
                    return(0);
                default:
                    return(-1);
            }
      case PLAYER_MSGTYPE_RESP_ACK:
          switch(hdr->subtype) {
              case PLAYER_FIDUCIAL_REQ_GET_GEOM:
                  fiducial_geom = (player_fiducial_geom_t*) data;
                  //format: <x> <y> <z> <roll> <pitch> <yaw> <length> ...
                  // <width> <height> <fiducial_length> <fiducial_width>
                  fprintf(this->file, "%+7.3f %+7.3f %+7.3f %+7.3f %+7.3f"
                          "%+7.3f %+7.3f %+7.3f %+7.3f %+7.3f %+7.3f",
                          fiducial_geom->pose.px,
                          fiducial_geom->pose.py,
                          fiducial_geom->pose.pz,
                          fiducial_geom->pose.proll,
                          fiducial_geom->pose.ppitch,
                          fiducial_geom->pose.pyaw,
                          fiducial_geom->size.sl,
                          fiducial_geom->size.sw,
                          fiducial_geom->size.sh,
                          fiducial_geom->fiducial_size.sl,
                          fiducial_geom->fiducial_size.sw);

                  return(0);
              default:
                  return(-1);
          }
      default:
          return(-1);
    }
}



/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_gps GPS format

@brief gps log format

The format for each @ref interface_gps message is:
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
int WriteLog::WriteGps(player_msghdr_t* hdr, void *data)
{
	player_gps_data_t *gdata;
	gdata = (player_gps_data_t*) data;
	switch(hdr->type){
		case PLAYER_MSGTYPE_DATA:
			fprintf(this->file,
				"%.3f "
				"%.7f %.7f %.7f "
				"%.3f %.3f "
				"%.3f %.3f %.3f %.3f "
				"%d %d",
				(double)gdata->time_sec +
				(double)gdata->time_sec * 1e-6,
				(double)gdata->latitude / (1e7),
				(double)gdata->longitude / (1e7),
				(double)gdata->altitude / (1e3),
				gdata->utm_e,
				gdata->utm_n,
				(double)gdata->hdop / 10.0,
				(double)gdata->vdop / 10.0,
				gdata->err_horz,
				gdata->err_vert,
				gdata->quality,
				gdata->num_sats);
			return (0);
		default:
			return (-1);
	}
}

/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_joystick Joystick format

@brief joystick log format

The format for each @ref interface_joystick message is:
  - xpos (int): unscaled X position of joystick
  - ypos (int): unscaled Y position of joystick
  - yawpos (int): unscaled Yaw position of the joystick
  - xscale (int): maximum X position
  - yscale (int): maximum Y position
  - yawscale (int): maximum Yaw position
  - buttons (hex string): bitmask of button states
*/
int WriteLog::WriteJoystick(player_msghdr_t* hdr, void *data)
{
	player_joystick_data_t *jdata;
	jdata = (player_joystick_data_t*) data;
	switch (hdr->type){
		case PLAYER_MSGTYPE_DATA:
			fprintf(this->file, "%+d %+d %+d %d %d %d %X",
				jdata->pos[0],
				jdata->pos[1],
				jdata->pos[2],
				jdata->scale[0],
				jdata->scale[1],
				jdata->scale[2],
				jdata->buttons);
			return (0);
		default:
			return (-1);
	}
}



/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_position3d Position3d format

@brief position3d format

The format for each @ref interface_position3d message is:
- 1:1 (PLAYER_POSITION3D_DATA_STATE) Odometry information.  The format is:
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
  
- 1:2 (PLAYER_POSITION3D_DATA_GEOM) Geometry information.  The format is:
  - xpos (float): in meters
  - ypos (float): in meters
  - zpos (float): in meters
  - roll (float): in radians
  - pitch (float): in radians
  - yaw (float): in radians
  - width (float): in meters
  - length (float): in meters
  - height (float): in meters
  
- 4:1 (PLAYER_POSITION3D_REQ_GET_GEOM) Geometry information.  The format is:
  - xpos (float): in meters
  - ypos (float): in meters
  - zpos (float): in meters
  - roll (float): in radians
  - pitch (float): in radians
  - yaw (float): in radians
  - width (float): in meters
  - length (float): in meters
  - height (float): in meters
*/
int WriteLog::WritePosition3d(player_msghdr_t* hdr, void *data)
{
	switch (hdr->type){
		case PLAYER_MSGTYPE_DATA:
			switch(hdr->subtype){
				case PLAYER_POSITION3D_DATA_STATE:
					player_position3d_data_t *pdata;
					pdata = (player_position3d_data_t*) data;
					fprintf(this->file,
						"%+.4f %+.4f %+.4f "
						"%+.4f %+.4f %+.4f "
						"%+.4f %+.4f %+.4f "
						"%+.4f %+.4f %+.4f "
						"%d",
						pdata->pos.px,
						pdata->pos.py,
						pdata->pos.pz,
						pdata->pos.proll,
						pdata->pos.ppitch,
						pdata->pos.pyaw,
						pdata->vel.px,
						pdata->vel.py,
						pdata->vel.pz,
						pdata->vel.proll,
						pdata->vel.ppitch,
						pdata->vel.pyaw,
						pdata->stall);

					return(0);
				case PLAYER_POSITION3D_DATA_GEOMETRY:
					player_position3d_geom_t *gdata;
					gdata = (player_position3d_geom_t*) data;
					fprintf(this->file,
						"%+.4f %+.4f %+.4f "
						"%+.4f %+.4f %+.4f "
						"%+.4f %+.4f %+.4f ",
						gdata->pose.px,
						gdata->pose.py,
						gdata->pose.pz,
						gdata->pose.proll,
						gdata->pose.ppitch,
						gdata->pose.pyaw,
						gdata->size.sw,
						gdata->size.sl,
						gdata->size.sh);
					return (0);
				default:
					return (-1);
			}
		case PLAYER_MSGTYPE_RESP_ACK:
			switch (hdr->subtype){
				case PLAYER_POSITION3D_REQ_GET_GEOM:
					printf("w00t\n");
					player_position3d_geom_t *gdata;
					gdata = (player_position3d_geom_t*) data;
					fprintf(this->file,
						"%+.4f %+.4f %+.4f "
						"%+.4f %+.4f %+.4f "
						"%+.4f %+.4f %+.4f ",
						gdata->pose.px,
						gdata->pose.py,
						gdata->pose.pz,
						gdata->pose.proll,
						gdata->pose.ppitch,
						gdata->pose.pyaw,
						gdata->size.sw,
						gdata->size.sl,
						gdata->size.sh);
					return (0);
				default:
					return (-1);
			}
		default: 
			return (-1);
	}
}


/** @ingroup tutorial_datalog
 * @defgroup player_driver_writelog_power Power format

@brief power log format

The format for each @ref interface_power message is:
  - battery voltage (float): in volts
  - charge percentage (float): in percent
  - energy stored (float): in joules
  - estimated energy consumption (float): in watts
  - charging status (int): 1 for charging
  - valid (int): bitfield for valid fields

This driver wil attempt to print all of the data fields,though in most cases the fields are not all used.
If the field is invalid (unused), a 0 will be printed.  The "valid" bitfield should be used to verify which fields are active.
*/
int WriteLog::WritePower(player_msghdr_t* hdr, void *data)
{
  player_power_data_t *pdata;
  pdata = (player_power_data_t*) data;
  switch (hdr->type){
  	case(PLAYER_MSGTYPE_DATA):
  		if (!(pdata->valid & PLAYER_POWER_MASK_VOLTS))
  			pdata->volts = 0;
  		if (!(pdata->valid & PLAYER_POWER_MASK_WATTS))
  			pdata->watts = 0;
  		if (!(pdata->valid & PLAYER_POWER_MASK_JOULES))
  			pdata->joules = 0;
  		if (!(pdata->valid & PLAYER_POWER_MASK_PERCENT))
  			pdata->percent = 0;
  		if (!(pdata->valid & PLAYER_POWER_MASK_CHARGING))
  			pdata->charging = 0;
  		
 		fprintf(this->file, "%.3f %.3f %.3f %.3f %d %d", 
 			pdata->volts,
 			pdata->percent,
 			pdata->joules,
 			pdata->watts,
 			pdata->charging,
 			pdata->valid);
  		return (0);
  	default:
  		return (-1);
  }
}

