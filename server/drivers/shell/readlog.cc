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
 * Desc: Driver for reading log files.
 * Author: Andrew Howard
 * Date: 17 May 2003
 * CVS: $Id$
 *
 * The writelog driver will write data from another device to a log file.
 * The readlog driver will replay the data as if it can from the real sensors.
 */
/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_readlog readlog

The readlog driver can be used to replay data stored in a log file.
This is particularly useful for debugging client programs, since users
may run their clients against the same data set over and over again.
Suitable log files can be generated using the @ref
player_driver_writelog driver.

To make use of log file data, Player must be started in a special
mode:

@verbatim
  $ player -r <logfile> -f speed <configfile>
@endverbatim

The '-r' switch instructs Player to load the given log file, and
replay the data according the configuration specified in the
configuration file.  The '-f' switch controls replay speed ('-f 2'
will replay at twice normal speed).

See the below for an example configuration file; note that the device
id's specified in the provides field must match those stored in the
log file (i.e., data logged as "position:0" must also be read back as
"position:0").

For help in controlling playback, try @ref player_util_playervcr.
Note that you must declare a @ref player_interface_log device to allow
playback control.

@par Compile-time dependencies

- none


@par Provides

The readlog driver can provide the following device interfaces.

- @ref player_interface_blobfinder
- @ref player_interface_camera
- @ref player_interface_gps
- @ref player_interface_joystick
- @ref player_interface_laser
- @ref player_interface_position
- @ref player_interface_position3d
- @ref player_interface_wifi

The driver also provides an interface for controlling the playback:

- @ref player_interface_log

@par Requires

- none

@par Configuration requests

- PLAYER_LOG_SET_READ_STATE_REQ
- PLAYER_LOG_GET_STATE_REQ
- PLAYER_LOG_SET_READ_REWIND_REQ

@par Configuration file options

- enable (integer)
  - Default: 1
  - Begin playing back log data when first client subscribes
    (as opposed to waiting for the client to tell the @ref 
    player_interface_log device to play).
- autorewind (integer)
  - Default: 0
  - Automatically rewind and play the log file again when the end is
    reached (as opposed to not producing any more data).
      
@par Example 

@verbatim

# Use this device to get laser data from the log
driver
(
  name "readlog"
  provides ["position:0" "laser:0" "log:0"]
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
#include <math.h>
#include <unistd.h>

#include <zlib.h>
  
#include "player.h"
#include "error.h"
#include "driver.h"
#include "drivertable.h"
#include "deviceregistry.h"

#include "encode.h"
#include "readlog_time.h"
  

// The logfile driver
class ReadLog: public Driver 
{
  // Constructor
  public: ReadLog(ConfigFile* cf, int section);

  // Destructor
  public: ~ReadLog();

  // Initialize the driver
  public: virtual int Setup();

  // Finalize the driver
  public: virtual int Shutdown();

  // Main loop
  public: virtual void Main();

  // Process log interface configuration requests
  private: int ProcessLogConfig();

  // Process generic configuration requests
  private: int ProcessOtherConfig();

  // Parse the header info
  private: int ParseHeader(int linenum, int token_count, char **tokens,
                           player_device_id_t *id, struct timeval *stime, struct timeval *dtime);

  // Parse some data
  private: int ParseData(player_device_id_t id, int linenum,
                         int token_count, char **tokens, struct timeval time);

  // Parse blobfinder data
  private: int ParseBlobfinder(player_device_id_t id, int linenum,
                               int token_count, char **tokens, struct timeval time);

  // Parse camera data
  private: int ParseCamera(player_device_id_t id, int linenum,
                          int token_count, char **tokens, struct timeval time);

  // Parse gps data
  private: int ParseGps(player_device_id_t id, int linenum,
                        int token_count, char **tokens, struct timeval time);

  // Parse joystick data
  private: int ParseJoystick(player_device_id_t id, int linenum,
                        int token_count, char **tokens, struct timeval time);

  // Parse laser data
  private: int ParseLaser(player_device_id_t id, int linenum,
                          int token_count, char **tokens, struct timeval time);

  // Parse position data
  private: int ParsePosition(player_device_id_t id, int linenum,
                             int token_count, char **tokens, struct timeval time);

  // Parse position3d data
  private: int ParsePosition3d(player_device_id_t id, int linenum,
                               int token_count, char **tokens, struct timeval time);

  // Parse wifi data
  private: int ParseWifi(player_device_id_t id, int linenum,
                         int token_count, char **tokens, struct timeval time);

  // List of provided devices
  private: int provide_count;
  private: player_device_id_t provide_ids[1024];

  // The log interface (at most one of these)
  private: player_device_id_t log_id;
  
  // File to read data from
  private: char *filename;
  private: FILE *file;
  private: gzFile gzfile;

  // Input buffer
  private: size_t line_size;
  private: char *line;

  // File format
  private: char *format;

  // Playback speed (1 = real time, 2 = twice real time)
  private: double speed;

  // Playback enabled?
  public: bool enable;

  // Has a client requested that we rewind?
  public: bool rewind_requested;

  // Should we auto-rewind?  This is set in the log devie in the .cfg
  // file, and defaults to false
  public: bool autorewind;
};


////////////////////////////////////////////////////////////////////////////
// Some global variables that must be set from command line options
char *ReadLog_filename = NULL;
double ReadLog_speed = 1.0;


////////////////////////////////////////////////////////////////////////////
// Create a driver for reading log files
Driver* ReadReadLog_Init(ConfigFile* cf, int section)
{
  // Check that the global readlog options have been set
  if (::ReadLog_filename == NULL)
  {
    PLAYER_ERROR("no log file specified; did you forget to use -r <filename>?");
    return NULL;
  }

  return ((Driver*) (new ReadLog(cf, section)));
}


////////////////////////////////////////////////////////////////////////////
// Device factory registration
void ReadLog_Register(DriverTable* table)
{
  table->AddDriver("readlog", ReadReadLog_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////
// Constructor
ReadLog::ReadLog(ConfigFile* cf, int section)
    : Driver(cf, section)
{  
  int i;
  player_device_id_t id;

  this->provide_count = 0;
  memset(&this->log_id, 0, sizeof(this->log_id));

  // Get a list of devices to provide
  for (i = 0; i < 1024; i++)
  {
    if (cf->ReadDeviceId(&id, section, "provides", -1, i, NULL) != 0)
      break;
    if (id.code == PLAYER_LOG_CODE)
      this->log_id = id;
    else
      this->provide_ids[this->provide_count++] = id;
  }

  // Register the log device
  if (this->log_id.code == PLAYER_LOG_CODE)
  {
    if (this->AddInterface(this->log_id, PLAYER_ALL_MODE, 0, 0, 1, 1) != 0)
    {
      this->SetError(-1);
      return;
    }
  }
  
  // Register all the provides devices
  for (i = 0; i < this->provide_count; i++)
  {
    if (this->AddInterface(this->provide_ids[i], PLAYER_ALL_MODE,
                           PLAYER_MAX_PAYLOAD_SIZE, PLAYER_MAX_PAYLOAD_SIZE, 1, 1) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Get replay options
  this->enable = cf->ReadInt(section, "enable", 1);
  this->autorewind = cf->ReadInt(section, "autorewind", 0);

  // Initialize other stuff
  this->filename = strdup(::ReadLog_filename);
  this->format = strdup("unknown");
  this->speed = ::ReadLog_speed;
  this->file = NULL;
  this->gzfile = NULL;
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Destructor
ReadLog::~ReadLog()
{
  return;
}


////////////////////////////////////////////////////////////////////////////
// Initialize driver
int ReadLog::Setup()
{
  // Reset the time
  ReadLogTime_time.tv_sec = 0;
  ReadLogTime_time.tv_usec = 0;

  // Open the file (possibly compressed)
  if (strlen(this->filename) >= 3 && \
      strcasecmp(this->filename + strlen(this->filename) - 3, ".gz") == 0)
    this->gzfile = gzopen(this->filename, "r");
  else
    this->file = fopen(this->filename, "r");
  
  if (this->file == NULL)
  {
    PLAYER_ERROR2("unable to open [%s]: %s\n", this->filename, strerror(errno));
    return -1;
  }
  
  // Rewind not requested by default
  this->rewind_requested = false;

  // Make some space for parsing data from the file.  This size is not
  // an exact upper bound; it's just my best guess.
  this->line_size = 4 * PLAYER_MAX_MESSAGE_SIZE;
  this->line = (char*) malloc(this->line_size);

  // Start device thread
  this->StartThread();
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Finalize the driver
int ReadLog::Shutdown()
{
  // Stop the device thread
  this->StopThread();

  // Free allocated mem
  free(this->line);
  
  // Close the file
  if (this->gzfile)
    gzclose(this->gzfile);
  else
    fclose(this->file);
  this->gzfile = NULL;
  this->file = NULL;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Driver thread
void ReadLog::Main()
{
  int ret;
  int i, len, linenum;
  int token_count;
  char *tokens[4096];
  player_device_id_t header_id, provide_id;
  struct timeval stime, dtime;

  linenum = 0;
  
  while (true)
  {
    pthread_testcancel();

    // Process requests
    this->ProcessLogConfig();
    this->ProcessOtherConfig();

    // If we're not supposed to playback data, sleep and loop
    if(!this->enable)
    {
      usleep(10000);
      continue;
    }

    // If a client has requested that we rewind, then do so
    if(this->rewind_requested)
    {
      if (this->gzfile)
        ret = gzseek(this->file,0,SEEK_SET);
      else
        ret = fseek(this->file,0,SEEK_SET);
        
      // back up to the beginning of the file
      if(gzseek(this->file,0,SEEK_SET) < 0)
      {
        // oh well, warn the user and keep going
        PLAYER_WARN1("while rewinding logfile, gzseek() failed: %s",
                     strerror(errno));
      }
      else
      {
        linenum = 0;

        // reset the time
        ReadLogTime_time.tv_sec = 0;
        ReadLogTime_time.tv_usec = 0;

        // reset time-of-last-write in all clients
        //
        // FIXME: It's not really thread-safe to call this here, because it
        //        writes to a bunch of fields that are also being read and/or
        //        written in the server thread.  But I'll be damned if I'm
        //        going to add a mutex just for this.
        // TODO: clientmanager->ResetClientTimestamps();

        // reset the flag
        this->rewind_requested = false;

        PLAYER_MSG0(2, "logfile rewound");
        continue;
      }
    }

    // Read a line from the file; note that gzgets is really slow
    // compared to fgets (on uncompressed files), so use the latter.
    if (this->gzfile)
      ret = (gzgets(this->file, this->line, this->line_size) == NULL);
    else
      ret = (fgets(this->line, this->line_size, (FILE*) this->file) == NULL);
        
    if (ret != 0)
    {
      // File is done, so just loop forever, unless we're on auto-rewind,
      // or until a client requests rewind.
      while(!this->autorewind && !this->rewind_requested)
      {
        usleep(100000);
        pthread_testcancel();

        // Process requests
        this->ProcessLogConfig();
        this->ProcessOtherConfig();

        ReadLogTime_time.tv_usec += 100000;
        if (ReadLogTime_time.tv_usec >= 1000000)
        {
          ReadLogTime_time.tv_sec += 1;
          ReadLogTime_time.tv_usec = (ReadLogTime_time.tv_usec % 1000000);
        }
      }

      // request a rewind and start again
      this->rewind_requested = true;
      continue;
    }

    // Possible buffer overflow, so bail
    assert(strlen(this->line) < this->line_size);

    linenum += 1;

    //printf("line %d\n", linenum);
    //continue;

    // Tokenize the line using whitespace separators
    token_count = 0;
    len = strlen(line);
    for (i = 0; i < len; i++)
    {
      if (isspace(line[i]))
        line[i] = 0;
      else if (i == 0)
      {
        assert(token_count < (int) (sizeof(tokens) / sizeof(tokens[i])));
        tokens[token_count++] = line + i;
      }
      else if (line[i - 1] == 0)
      {
        assert(token_count < (int) (sizeof(tokens) / sizeof(tokens[i])));
        tokens[token_count++] = line + i;
      }
    }

    if (token_count >= 1)
    {
      // Discard comments
      if (strcmp(tokens[0], "#") == 0)
        continue;

      // Parse meta-data
      if (strcmp(tokens[0], "##") == 0)
      {
        if (token_count == 4)
        {
          free(this->format);
          this->format = strdup(tokens[3]);
        }
        continue;
      }
    }

    // Parse out the header info
    if (this->ParseHeader(linenum, token_count, tokens, &header_id, &stime, &dtime) != 0)
      continue;

    // Set the global timestamp
    ::ReadLogTime_time = stime;
    
    // Use sync messages to regulate replay
    if (header_id.code == PLAYER_PLAYER_CODE)
    {
      //printf("reading %d %d\n", stime.tv_sec, stime.tv_usec);
          
      // TODO: this needs a radical re-design
      usleep((int) (100000 / this->speed));
      continue;
    }

    // Look for a matching read interface; data will be output on
    // the corresponding provides interface.
    for (i = 0; i < this->provide_count; i++)
    {
      provide_id = this->provide_ids[i];
      if (header_id.code == provide_id.code && header_id.index == provide_id.index)
        this->ParseData(provide_id, linenum, token_count, tokens, dtime);
    } 
  }

  return;
}



////////////////////////////////////////////////////////////////////////////
// Process configuration requests
int ReadLog::ProcessLogConfig()
{
  player_log_set_read_rewind_t rreq;
  player_log_set_read_state_t sreq;
  player_log_get_state_t greq;
  uint8_t subtype;
  char src[PLAYER_MAX_REQREP_SIZE];
  void *client;
  struct timeval time;
  size_t len;

  len = this->GetConfig(this->log_id, &client, src, sizeof(src), &time);
  if (len == 0)
    return 0;
  
  if(len < sizeof(sreq.subtype))
  {
    PLAYER_WARN2("request was too small (%d < %d)",
                  len, sizeof(sreq.subtype));
    if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
      PLAYER_ERROR("PutReply() failed");
  }

  subtype = ((player_log_set_read_state_t*)src)->subtype;
  switch(subtype)
  {
    case PLAYER_LOG_SET_READ_STATE_REQ:
      if(len != sizeof(sreq))
      {
        PLAYER_WARN2("request wrong size (%d != %d)", len, sizeof(sreq));
        if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
      }
      sreq = *((player_log_set_read_state_t*)src);
      if(sreq.state)
      {
        puts("ReadLog: start playback");
        this->enable = true;
      }
      else
      {
        puts("ReadLog: stop playback");
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
      greq.type = PLAYER_LOG_TYPE_READ;
      if(this->enable)
        greq.state = 1;
      else
        greq.state = 0;

      if(this->PutReply(client, PLAYER_MSGTYPE_RESP_ACK,
                        &greq, sizeof(greq),NULL) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    case PLAYER_LOG_SET_READ_REWIND_REQ:
      if(len != sizeof(rreq.subtype))
      {
        PLAYER_WARN2("request wrong size (%d != %d)", 
                     len, sizeof(rreq.subtype));
        if (this->PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
      }

      // set the appropriate flag in the manager
      this->rewind_requested = true;

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


////////////////////////////////////////////////////////////////////////////
// Process generic requests
int ReadLog::ProcessOtherConfig()
{
  int i;
  player_device_id_t id;
  char src[PLAYER_MAX_REQREP_SIZE];
  void *client;
  struct timeval time;
  size_t len;
  
  // Check for request on all interfaces
  for (i = 0; i < this->provide_count; i++)
  {
    id = this->provide_ids[i];
    if (id.code == PLAYER_LOG_CODE)
      continue;
      
    len = this->GetConfig(id, &client, src, sizeof(src), &time);
    if (len == 0)
      continue;

    if (this->PutReply(id, client, PLAYER_MSGTYPE_RESP_NACK, NULL) != 0)
      PLAYER_ERROR("PutReply() failed");
  }

  return 0;
}



////////////////////////////////////////////////////////////////////////////
// Signed int conversion macros
#define NINT16(x) (htons((int16_t)(x)))
#define NUINT16(x) (htons((uint16_t)(x)))
#define NINT32(x) (htonl((int32_t)(x)))
#define NUINT32(x) (htonl((uint32_t)(x)))


////////////////////////////////////////////////////////////////////////////
// Unit conversion macros
#define M_MM(x) ((x) * 1000.0)
#define CM_MM(x) ((x) * 100.0)
#define RAD_DEG(x) ((x) * 180.0 / M_PI)


////////////////////////////////////////////////////////////////////////////
// Parse the header info
int ReadLog::ParseHeader(int linenum, int token_count, char **tokens,
                         player_device_id_t *id, struct timeval *stime, struct timeval *dtime)
{
  char *name;
  player_interface_t interface;
  
  if (token_count < 4)
  {
    PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
    return -1;
  }
  
  name = tokens[3];

  if (strcmp(name, "sync") == 0)
  {
    id->code = PLAYER_PLAYER_CODE;
    id->index = 0;
    stime->tv_sec = (uint32_t) floor(atof(tokens[0]));
    stime->tv_usec = ((uint32_t) floor(atof(tokens[0]) * 1e6)) % 1000000;
    dtime->tv_sec = stime->tv_sec;
    dtime->tv_usec = stime->tv_usec;
  }
  else if (lookup_interface(name, &interface) == 0)
  {
    id->code = interface.code;
    id->index = atoi(tokens[4]);
    stime->tv_sec = (uint32_t) floor(atof(tokens[0]));
    stime->tv_usec = ((uint32_t) (fmod(atof(tokens[0]), 1) * 1e6)) % 1000000;
    dtime->tv_sec = (uint32_t) floor(atof(tokens[5]));
    dtime->tv_usec = ((uint32_t) (fmod(atof(tokens[5]), 1) * 1e6)) % 1000000;
  }
  else
  {
    PLAYER_WARN1("unknown interface name [%s]", name);
    return -1;
  }
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse data
int ReadLog::ParseData(player_device_id_t id, int linenum,
                       int token_count, char **tokens, struct timeval time)
{
  if (id.code == PLAYER_BLOBFINDER_CODE)
    return this->ParseBlobfinder(id, linenum, token_count, tokens, time);
  else if (id.code == PLAYER_CAMERA_CODE)
    return this->ParseCamera(id, linenum, token_count, tokens, time);
  else if (id.code == PLAYER_GPS_CODE)
    return this->ParseGps(id, linenum, token_count, tokens, time);
  else if (id.code == PLAYER_JOYSTICK_CODE)
    return this->ParseJoystick(id, linenum, token_count, tokens, time);
  else if (id.code == PLAYER_LASER_CODE)
    return this->ParseLaser(id, linenum, token_count, tokens, time);
  else if (id.code == PLAYER_POSITION_CODE)
    return this->ParsePosition(id, linenum, token_count, tokens, time);
  else if (id.code == PLAYER_POSITION3D_CODE)
    return this->ParsePosition3d(id, linenum, token_count, tokens, time);
  else if (id.code == PLAYER_WIFI_CODE)
    return this->ParseWifi(id, linenum, token_count, tokens, time);

  PLAYER_WARN1("unknown interface code [%s]", ::lookup_interface_name(0, id.code));
  return -1;
}


////////////////////////////////////////////////////////////////////////////
// Parse blobfinder data
int ReadLog::ParseBlobfinder(player_device_id_t id, int linenum,
                             int token_count, char **tokens, struct timeval time)
{
  player_blobfinder_data_t data;
  player_blobfinder_blob_t *blob;
  size_t size;
  int i, blob_count;
  
  if (token_count < 9)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }
    
  data.width = NUINT16(atoi(tokens[6]));
  data.height = NUINT16(atoi(tokens[7]));
  blob_count = atoi(tokens[8]);
  data.blob_count = NUINT16(blob_count);

  if (token_count < 9 + blob_count * 10)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }

  for (i = 0; i < blob_count; i++)
  {
    blob = data.blobs + i;
    blob->id =  NINT16(atoi(tokens[9 + i]));
    blob->color = NUINT32(atoi(tokens[10 + i]));
    blob->area = NUINT32(atoi(tokens[11 + i]));
    blob->x = NUINT16(atoi(tokens[12 + i]));
    blob->y = NUINT16(atoi(tokens[13 + i]));
    blob->left = NUINT16(atoi(tokens[14 + i]));
    blob->right = NUINT16(atoi(tokens[15 + i]));
    blob->top = NUINT16(atoi(tokens[16 + i]));
    blob->bottom = NUINT16(atoi(tokens[17 + i]));
    blob->range = NUINT16(M_MM(atof(tokens[18 + i])));
  }
  
  size = sizeof(data) - sizeof(data.blobs) + blob_count * sizeof(data.blobs[0]);
  this->PutData(id, &data, size, &time);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse camera data
int ReadLog::ParseCamera(player_device_id_t id, int linenum,
                               int token_count, char **tokens, struct timeval time)
{
  player_camera_data_t *data;
  size_t src_size, dst_size;
  
  if (token_count < 13)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }

  data = (player_camera_data_t*) malloc(sizeof(player_camera_data_t));
  assert(data);
    
  data->width = NUINT16(atoi(tokens[6]));
  data->height = NUINT16(atoi(tokens[7]));
  data->bpp = atoi(tokens[8]);
  data->format = atoi(tokens[9]);
  data->compression = atoi(tokens[10]);
  data->image_size = NUINT32(atoi(tokens[11]));
  
  // Check sizes
  src_size = strlen(tokens[12]);
  dst_size = ::DecodeHexSize(src_size);
  assert(dst_size = NUINT32(data->image_size));
  assert(dst_size < sizeof(data->image));

  // Decode string
  ::DecodeHex(data->image, dst_size, tokens[12], src_size);
              
  this->PutData(id, data, sizeof(*data) - sizeof(data->image) + dst_size, &time);

  free(data);
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse GPS data
int ReadLog::ParseGps(player_device_id_t id, int linenum,
                      int token_count, char **tokens, struct timeval time)
{
  player_gps_data_t data;

  if (token_count < 17)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }

  data.time_sec = NUINT32((int) atof(tokens[6]));
  data.time_usec = NUINT32((int) fmod(atof(tokens[6]), 1.0));

  data.latitude = NINT32((int) (60 * 60 * 60 * atof(tokens[7])));
  data.longitude = NINT32((int) (60 * 60 * 60 * atof(tokens[8])));
  data.altitude = NINT32(M_MM(atof(tokens[9])));

  data.utm_e = NINT32(CM_MM(atof(tokens[10])));
  data.utm_n = NINT32(CM_MM(atof(tokens[11])));

  data.hdop = NINT16((int) (10 * atof(tokens[12])));
  data.err_horz = NUINT32(M_MM(atof(tokens[13])));
  data.err_vert = NUINT32(M_MM(atof(tokens[14])));

  data.quality = atoi(tokens[15]);
  data.num_sats = atoi(tokens[16]);

  this->PutData(id, &data, sizeof(data), &time);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse joystick data
int ReadLog::ParseJoystick(player_device_id_t id, int linenum,
                      int token_count, char **tokens, struct timeval time)
{
  player_joystick_data_t data;

  if (token_count < 11)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }

  data.xpos = NINT16((short) atoi(tokens[6]));
  data.ypos = NINT16((short) atoi(tokens[7]));
  data.xscale = NINT16((short) atoi(tokens[8]));
  data.yscale = NINT16((short) atoi(tokens[9]));
  data.buttons = NUINT16((unsigned short) (unsigned int) atoi(tokens[10]));

  this->PutData(id, &data, sizeof(data), &time);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse laser data
int ReadLog::ParseLaser(player_device_id_t id, int linenum,
                               int token_count, char **tokens, struct timeval time)
{
  int i, count;
  player_laser_data_t data;

  if (token_count < 12)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }

  data.min_angle = NINT16(RAD_DEG(atof(tokens[6])) * 100);
  data.max_angle = NINT16(RAD_DEG(atof(tokens[7])) * 100);
  data.resolution = NUINT16(RAD_DEG(atof(tokens[8])) * 100);
  data.range_res = NUINT16(1);
  data.range_count = NUINT16(atoi(tokens[9]));
    
  count = 0;
  for (i = 10; i < token_count; i += 2)
  {
    data.ranges[count] = NUINT16(M_MM(atof(tokens[i + 0])));
    data.intensity[count] = atoi(tokens[i + 1]);
    count += 1;
  }

  if (count != ntohs(data.range_count))
  {
    PLAYER_ERROR2("range count mismatch at %s:%d", this->filename, linenum);
    return -1;
  }

  /* DEPRECATED; REMOVE
     else
     {
     data.min_angle = +18000;
     data.max_angle = -18000;
    
     count = 0;
     for (i = 6; i < token_count; i += 3)
     {
     data.ranges[count] = NUINT16(M_MM(atof(tokens[i + 0])));      
     data.intensity[count] = atoi(tokens[i + 2]);

     angle = (int) (atof(tokens[i + 1]) * 180 / M_PI * 100);
     if (angle < data.min_angle)
     data.min_angle = angle;
     if (angle > data.max_angle)
     data.max_angle = angle;

     count += 1;
     }

     data.resolution = (data.max_angle - data.min_angle) / (count - 1);
     data.range_count = count;

     data.range_count = NUINT16(data.range_count);
     data.min_angle = NINT16(data.min_angle);
     data.max_angle = NINT16(data.max_angle);
     data.resolution = NUINT16(data.resolution);
     data.range_res = NUINT16(1);
     }
  */

  this->PutData(id, &data, sizeof(data), &time);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse position data
int ReadLog::ParsePosition(player_device_id_t id, int linenum,
                                  int token_count, char **tokens, struct timeval time)
{
  player_position_data_t data;

  if (token_count < 11) // 12
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }
  
  data.xpos = NINT32(M_MM(atof(tokens[6])));
  data.ypos = NINT32(M_MM(atof(tokens[7])));
  data.yaw = NINT32(RAD_DEG(atof(tokens[8])));
  data.xspeed = NINT32(M_MM(atof(tokens[9])));
  data.yspeed = NINT32(M_MM(atof(tokens[10])));
  data.yawspeed = NINT32(RAD_DEG(atof(tokens[11])));
  //data.stall = atoi(tokens[12]);

  this->PutData(id, &data, sizeof(data), &time);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse position3d data
int ReadLog::ParsePosition3d(player_device_id_t id, int linenum,
                             int token_count, char **tokens, struct timeval time)
{
 player_position3d_data_t data;

  if (token_count < 19)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }
  
  data.xpos = NINT32(M_MM(atof(tokens[6])));
  data.ypos = NINT32(M_MM(atof(tokens[7])));
  data.zpos = NINT32(M_MM(atof(tokens[8])));

  data.roll = NINT32(1000 * atof(tokens[9]));
  data.pitch = NINT32(1000 * atof(tokens[10]));
  data.yaw = NINT32(1000 * atof(tokens[11]));

  data.xspeed = NINT32(M_MM(atof(tokens[12])));
  data.yspeed = NINT32(M_MM(atof(tokens[13])));
  data.zspeed = NINT32(M_MM(atof(tokens[14])));

  data.rollspeed = NINT32(1000 * atof(tokens[15]));
  data.pitchspeed = NINT32(1000 * atof(tokens[16]));
  data.yawspeed = NINT32(1000 * atof(tokens[17]));
  
  data.stall = atoi(tokens[18]);

  this->PutData(id, &data, sizeof(data), &time);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse wifi data
int ReadLog::ParseWifi(player_device_id_t id, int linenum,
                              int token_count, char **tokens, struct timeval time)
{
  player_wifi_data_t data;
  player_wifi_link_t *link;
  int i;

  if (token_count < 6)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }

  data.link_count = 0;
  for (i = 6; i < token_count; i += 4)
  {
    link = data.links + data.link_count;
  
    strcpy(link->ip, tokens[i + 0]);
    link->qual = atoi(tokens[i + 1]);
    link->level = atoi(tokens[i + 2]);
    link->noise = atoi(tokens[i + 3]);

    link->qual = htons(link->qual);
    link->level = htons(link->level);
    link->noise = htons(link->noise);    
    
    data.link_count++;
  }
  data.link_count = htons(data.link_count);

  this->PutData(id, &data, sizeof(data), &time);

  return 0;
}


