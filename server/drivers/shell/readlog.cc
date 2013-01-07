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
 * Desc: Driver for reading log files.
 * Author: Andrew Howard
 * Date: 17 May 2003
 * CVS: $Id$
 *
 * The writelog driver will write data from another device to a log file.
 * The readlog driver will replay the data as if it can from the real sensors.
 */
/** @ingroup drivers */
/** @{ */
/** @defgroup driver_readlog readlog
 * @brief Playback of logged data

The readlog driver can be used to replay data stored in a log file.
This is particularly useful for debugging client programs, since users
may run their clients against the same data set over and over again.
Suitable log files can be generated using the @ref driver_writelog driver.
The format for the log file can be found in the
@ref tutorial_datalog "data logging tutorial".

See below for an example configuration file; note that the device
id's specified in the provides field must match those stored in the
log file (i.e., data logged as "position2d:0" must also be read back as
"position2d:0").

For help in controlling playback, try @ref util_playervcr.
Note that you must declare a @ref interface_log device to allow
playback control.

@par Compile-time dependencies

- none

@par Provides

The readlog driver can provide the following device interfaces.

- @ref interface_laser
- @ref interface_ranger
- @ref interface_position2d
- @ref interface_sonar
- @ref interface_wifi
- @ref interface_wsn
- @ref interface_imu
- @ref interface_pointcloud3d
- @ref interface_opaque
- @ref interface_ptz
- @ref interface_actarray
- @ref interface_fiducial
- @ref interface_blobfinder
- @ref interface_camera
- @ref interface_gps
- @ref interface_joystick
- @ref interface_position3d
- @ref interface_power
- @ref interface_dio
- @ref interface_aio
- @ref interface_coopobject

The driver also provides an interface for controlling the playback:

- @ref interface_log

@par Requires

- none

@par Configuration requests

- PLAYER_LOG_SET_READ_STATE_REQ
- PLAYER_LOG_GET_STATE_REQ
- PLAYER_LOG_SET_READ_REWIND_REQ

@par Configuration file options

- filename (filename)
  - Default: NULL
  - The log file to play back.
- speed (float)
  - Default: 1.0
  - Playback speed; 1.0 is real-time
- autoplay (integer)
  - Default: 1
  - Begin playing back log data when first client subscribes
    (as opposed to waiting for the client to tell the @ref
    interface_log device to play).
- autorewind (integer)
  - Default: 0
  - Automatically rewind and play the log file again when the end is
    reached (as opposed to not producing any more data).

@par Example

@verbatim

# Play back odometry and laser data at twice real-time from "mydata.log"
driver
(
  name "readlog"
  filename "mydata.log"
  provides ["position2d:0" "laser:0" "log:0"]
  speed 2.0
)
@endverbatim

@author Andrew Howard, Radu Bogdan Rusu, Rich Mattes

*/
/** @} */

#include <config.h>

#include <libplayercore/playercore.h>
#include <libplayerinterface/functiontable.h>
//#include <libplayerinterface/playerxdr.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#if !defined (WIN32) || defined (__MINGW32__)
  #include <sys/time.h>
  #include <unistd.h>
  #include <strings.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if HAVE_Z
  #include <zlib.h>
#endif

#include "encode.h"
#include "readlog_time.h"

#if defined (WIN32)
  #define strdup _strdup
#endif


#if 0
// we use this pointer to reset timestamps in the client objects when the
// log gets rewound
#include "clientmanager.h"
extern ClientManager* clientmanager;
#endif

// The logfile driver
class ReadLog: public ThreadedDriver
{
  // Constructor
  public: ReadLog(ConfigFile* cf, int section);

  // Destructor
  public: ~ReadLog();

  // Initialize the driver
  public: virtual int MainSetup();

  // Finalize the driver
  public: virtual void MainQuit();

  // Main loop
  public: virtual void Main();

  public: virtual int ProcessMessage(QueuePointer & resp_queue,
                                     player_msghdr_t * hdr,
                                     void * data);
  // Process log interface configuration requests
  private: int ProcessLogConfig(QueuePointer & resp_queue,
                                player_msghdr_t * hdr,
                                void * data);

  // Process position interface configuration requests
  private: int ProcessPositionConfig(QueuePointer & resp_queue,
                                     player_msghdr_t * hdr,
                                     void * data);
  
  // Process position3d interface configuration requests
  private: int ProcessPosition3dConfig(QueuePointer & resp_queue,
                                     player_msghdr_t * hdr,
                                     void * data);

  // Process fiducial interface configuration requests
  private: int ProcessFiducialConfig(QueuePointer & resp_queue,
                                  player_msghdr_t * hdr,
                                  void * data);

  // Process laser interface configuration requests
  private: int ProcessLaserConfig(QueuePointer & resp_queue,
                                  player_msghdr_t * hdr,
                                  void * data);

  // Process ranger interface configuration requests
  private: int ProcessRangerConfig(QueuePointer & resp_queue,
				   player_msghdr_t * hdr,
				   void * data);

  // Process sonar interface configuration requests
  private: int ProcessSonarConfig(QueuePointer & resp_queue,
                                  player_msghdr_t * hdr,
                                  void * data);

  // Process WSN interface configuration requests
  private: int ProcessWSNConfig(QueuePointer & resp_queue,
                                player_msghdr_t * hdr,
                                void * data);

  // Process IMU interface configuration requests
  private: int ProcessIMUConfig(QueuePointer &resp_queue,
                                player_msghdr_t * hdr,
                                void * data);

  // Parse the header info
  private: int ParseHeader(int linenum, int token_count, char **tokens,
                           player_devaddr_t *id, double *dtime,
                           unsigned short* type, unsigned short* subtype);
  // Parse some data
  private: int ParseData(player_devaddr_t id,
                         unsigned short type, unsigned short subtype,
                         int linenum, int token_count, char **tokens,
                         double time);

  // Parse fiducial data
  private: int ParseFiducial(player_devaddr_t id,
                             unsigned short type, unsigned short subtype,
                             int linenum, int token_count, char **tokens,
                             double time);

  // Parse blobfinder data
  private: int ParseBlobfinder(player_devaddr_t id,
                               unsigned short type, unsigned short subtype,
                               int linenum,
                               int token_count, char **tokens, double time);
  // Parse camera data
  private: int ParseCamera(player_devaddr_t id,
                           unsigned short type, unsigned short subtype,
                           int linenum,
                          int token_count, char **tokens, double time);
  // Parse gps data
  private: int ParseGps(player_devaddr_t id,
                        unsigned short type, unsigned short subtype,
                        int linenum,
                        int token_count, char **tokens, double time);
                        	
  // Parse joystick data
  private: int ParseJoystick(player_devaddr_t id,
                             unsigned short type, unsigned short subtype,
                             int linenum,
                        int token_count, char **tokens, double time);

  // Parse laser data
  private: int ParseLaser(player_devaddr_t id,
                          unsigned short type, unsigned short subtype,
                          int linenum,
                          int token_count, char **tokens, double time);

  // Parse ranger data
  private: int ParseRanger(player_devaddr_t id,
                          unsigned short type, unsigned short subtype,
                          int linenum,
                          int token_count, char **tokens, double time);

  // Parse localize data
  private: int ParseLocalize(player_devaddr_t id, unsigned short type,
			     unsigned short subtype,
			     int linenum, int token_count,
			     char **tokens, double time);

  // Parse sonar data
  private: int ParseSonar(player_devaddr_t id,
                          unsigned short type, unsigned short subtype,
                          int linenum,
                          int token_count, char **tokens, double time);
  // Parse position data
  private: int ParsePosition(player_devaddr_t id,
                             unsigned short type, unsigned short subtype,
                             int linenum,
                             int token_count, char **tokens, double time);

  // Parse opaque data
  private: int ParseOpaque(player_devaddr_t id,
                             unsigned short type, unsigned short subtype,
                             int linenum,
                             int token_count, char **tokens, double time);

  // Parse wifi data
  private: int ParseWifi(player_devaddr_t id,
                         unsigned short type, unsigned short subtype,
                         int linenum,
                         int token_count, char **tokens, double time);
  // Parse WSN data
  private: int ParseWSN(player_devaddr_t id,
                        unsigned short type, unsigned short subtype,
                        int linenum,
                        int token_count, char **tokens, double time);
  // Parse WSN data
  private: int ParseCoopObject(player_devaddr_t id,
                        unsigned short type, unsigned short subtype,
                        int linenum,
                        int token_count, char **tokens, double time);
  // Parse IMU data
  private: int ParseIMU (player_devaddr_t id,
                         unsigned short type, unsigned short subtype,
                         int linenum,
                         int token_count, char **tokens, double time);

  // Parse PointCloud3D data
  private: int ParsePointCloud3d (player_devaddr_t id,
                    		  unsigned short type, unsigned short subtype,
                    		  int linenum,
                    		  int token_count, char **tokens, double time);

  // Parse PTZ data
  private: int ParsePTZ (player_devaddr_t id,
                         unsigned short type, unsigned short subtype,
                         int linenum,
                         int token_count, char **tokens, double time);

  // Parse Actarray data
  private: int ParseActarray (player_devaddr_t id,
                              unsigned short type, unsigned short subtype,
                              int linenum,
                              int token_count, char **tokens, double time);

  // Parse AIO data
  private: int ParseAIO(player_devaddr_t id,
                        unsigned short type, unsigned short subtype,
                        int linenum, int token_count, char **tokens,
                        double time);

  // Parse DIO data
  private: int ParseDIO(player_devaddr_t id,
                        unsigned short type, unsigned short subtype,
                        int linenum, int token_count, char **tokens,
                        double time);

  // Parse RFID data
  private: int ParseRFID(player_devaddr_t id,
                         unsigned short type, unsigned short subtype,
                         int linenum, int token_count, char **tokens,
                         double time);

  // Parse position3d data
  private: int ParsePosition3d(player_devaddr_t id,
                               unsigned short type, unsigned short subtype,
                               int linenum,
                               int token_count, char **tokens, double time);
  // Parse power data
  private: int ParsePower(player_devaddr_t id,
                               unsigned short type, unsigned short subtype,
                               int linenum,
                               int token_count, char **tokens, double time);

  // List of provided devices
  private: int provide_count;
  private: player_devaddr_t provide_ids[1024];
  // spots to cache metadata for a device (e.g., sonar geometry)
  private: void* provide_metadata[1024];

  // The log interface (at most one of these)
  private: player_devaddr_t log_id;

  // File to read data from
  private: const char *filename;
  private: FILE *file;
#if HAVE_Z
  private: gzFile gzfile;
#endif

  // localize particles
  private: player_localize_get_particles_t particles;
  private: bool particles_set;
  private: player_devaddr_t localize_addr;


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

  private: typedef struct {
    player_ranger_geom_t* geom;
    player_ranger_config_t* config;
  } ranger_meta_t;

};


////////////////////////////////////////////////////////////////////////////
// Create a driver for reading log files
Driver* ReadReadLog_Init(ConfigFile* cf, int section)
{
  return ((Driver*) (new ReadLog(cf, section)));
}


////////////////////////////////////////////////////////////////////////////
// Device factory registration
void readlog_Register(DriverTable* table)
{
  table->AddDriver("readlog", ReadReadLog_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////
// Constructor
ReadLog::ReadLog(ConfigFile* cf, int section)
    : ThreadedDriver(cf, section)
{
  int i,j;
  player_devaddr_t id;

  this->filename = cf->ReadFilename(section, "filename", NULL);
  if(!this->filename)
  {
    PLAYER_ERROR("must specify a log file to read from");
    this->SetError(-1);
    return;
  }
  this->speed = cf->ReadFloat(section, "speed", 1.0);

  this->provide_count = 0;
  memset(&this->log_id, 0, sizeof(this->log_id));
  memset(this->provide_metadata,0,sizeof(this->provide_metadata));

  particles_set = false;

  // Get a list of devices to provide
  for (i = 0; i < 1024; i++)
  {
    // TODO: fix the indexing here
    if (cf->ReadDeviceAddr(&id, section, "provides", -1, i, NULL) != 0)
      break;
    if (id.interf == PLAYER_LOG_CODE)
      this->log_id = id;
    else
      this->provide_ids[this->provide_count++] = id;
  }

  // Register the log device
  if (this->log_id.interf == PLAYER_LOG_CODE)
  {
    if (this->AddInterface(this->log_id) != 0)
    {
      this->SetError(-1);
      return;
    }
  }

  // Register all the provides devices
  for (i = 0; i < this->provide_count; i++)
  {
    if (this->AddInterface(this->provide_ids[i]) != 0)
    {
      for(j=0;j<this->provide_count;j++)
      {
        // free any allocated metadata slots
        if(this->provide_metadata[j])
        {
          free(provide_metadata[j]);
          provide_metadata[j] = NULL;
        }
      }
      this->SetError(-1);
      return;
    }

    // if it's sonar, then make a spot to cache geometry info
    if(this->provide_ids[i].interf == PLAYER_SONAR_CODE)
    {
      this->provide_metadata[i] = calloc(sizeof(player_sonar_geom_t),1);
      assert(this->provide_metadata[i]);
    }

    // if it's localize, remember address
    if(this->provide_ids[i].interf == PLAYER_LOCALIZE_CODE){
      this->localize_addr = this->provide_ids[i];
    }
  }

  // Get replay options
  this->enable = cf->ReadInt(section, "autoplay", 1) != 0 ? true : false;
  this->autorewind = cf->ReadInt(section, "autorewind", 0) != 0 ? true : false;

  // Initialize other stuff
  this->format = strdup("unknown");
  this->file = NULL;
#if HAVE_Z
  this->gzfile = NULL;
#endif

  // Set up the global time object.  We're just shoving our own in over the
  // pre-existing WallclockTime object.  Not pretty but it works.
  if(GlobalTime)
    delete GlobalTime;
  GlobalTime = new ReadLogTime();

  return;
}


////////////////////////////////////////////////////////////////////////////
// Destructor
ReadLog::~ReadLog()
{
  // Free allocated metadata slots
  for(int i=0;i<this->provide_count;i++)
  {
    if(this->provide_metadata[i])
    {
      free(this->provide_metadata[i]);
      this->provide_metadata[i] = NULL;
    }
  }

  return;
}


////////////////////////////////////////////////////////////////////////////
// Initialize driver
int ReadLog::MainSetup()
{
  // Reset the time
  ReadLogTime_time.tv_sec = 0;
  ReadLogTime_time.tv_usec = 0;
  ReadLogTime_timeDouble = 0.0;

  // Open the file (possibly compressed)
  if (strlen(this->filename) >= 3 &&
#if defined (WIN32)
      _strnicmp(this->filename + strlen(this->filename) - 3, ".gz", 3) == 0)
#else
      strcasecmp(this->filename + strlen(this->filename) - 3, ".gz") == 0)
#endif
  {
#if HAVE_Z
    this->gzfile = gzopen(this->filename, "r");
#else
    PLAYER_ERROR("no support for reading compressed log files");
    return -1;
#endif
  }
  else
    this->file = fopen(this->filename, "r");

  /** @todo Fix support for reading gzipped files */
  if (this->file == NULL)
  {
    PLAYER_ERROR2("unable to open [%s]: %s\n", this->filename, strerror(errno));
    return -1;
  }

  // Rewind not requested by default
  this->rewind_requested = false;

  // Make some space for parsing data from the file.  This size is not
  // an exact upper bound; it's just my best guess.
  this->line_size = PLAYER_MAX_MESSAGE_SIZE;
  this->line = (char*) malloc(this->line_size);
  assert(this->line);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Finalize the driver
void ReadLog::MainQuit()
{
  // Free allocated mem
  free(this->line);

  // Close the file
#if HAVE_Z
  if (this->gzfile)
  {
    gzclose(this->gzfile);
    this->gzfile = NULL;
  }
#endif
  if(this->file)
  {
    fclose(this->file);
    this->file = NULL;
  }
}


////////////////////////////////////////////////////////////////////////////
// Driver thread
void ReadLog::Main()
{
  int ret;
  int i, len, linenum;
  bool use_stored_tokens;
  int token_count=0;
  char *tokens[4096];
  player_devaddr_t header_id, provide_id;
  struct timeval tv;
  double last_wall_time, curr_wall_time;
  double curr_log_time, last_log_time;
  unsigned short type, subtype;
  bool reading_configs;

  linenum = 0;

  last_wall_time = -1.0;
  last_log_time = -1.0;

  // First thing, we'll read all the configs from the front of the file
  reading_configs = true;
  use_stored_tokens = false;

  while (true)
  {
    pthread_testcancel();

    // Process requests
    if(!reading_configs)
      ProcessMessages();

    // If we're not supposed to playback data, sleep and loop
    if(!this->enable && !reading_configs)
    {
      usleep(10000);
      continue;
    }

    // If a client has requested that we rewind, then do so
    if(!reading_configs && this->rewind_requested)
    {
      // back up to the beginning of the file
#if HAVE_Z
      if (this->gzfile)
        ret = gzseek(this->gzfile,0,SEEK_SET);
      else
        ret = fseek(this->file,0,SEEK_SET);
#else
      ret = fseek(this->file,0,SEEK_SET);
#endif

      if(ret < 0)
      {
        // oh well, warn the user and keep going
        PLAYER_WARN1("while rewinding logfile, gzseek()/fseek() failed: %s",
                     strerror(errno));
      }
      else
      {
        linenum = 0;

        // reset the time
        ReadLogTime_time.tv_sec = 0;
        ReadLogTime_time.tv_usec = 0;
        ReadLogTime_timeDouble = 0.0;

#if 0
        // reset time-of-last-write in all clients
        //
        // FIXME: It's not really thread-safe to call this here, because it
        //        writes to a bunch of fields that are also being read and/or
        //        written in the server thread.  But I'll be damned if I'm
        //        going to add a mutex just for this.
        clientmanager->ResetClientTimestamps();
#endif

        // reset the flag
        this->rewind_requested = false;

        PLAYER_MSG0(2, "logfile rewound");
        continue;
      }
    }

    if(!use_stored_tokens)
    {
      // Read a line from the file; note that gzgets is really slow
      // compared to fgets (on uncompressed files), so use the latter.
#if HAVE_Z
      if (this->gzfile)
        ret = (gzgets(this->gzfile, this->line, this->line_size) == NULL);
      else
        ret = (fgets(this->line, this->line_size, (FILE*) this->file) == NULL);
#else
      ret = (fgets(this->line, this->line_size, (FILE*) this->file) == NULL);
#endif

      if (ret != 0)
      {
        PLAYER_MSG1(1, "reached end of log file %s", this->filename);
        // File is done, so just loop forever, unless we're on auto-rewind,
        // or until a client requests rewind.
        reading_configs = false;

        // deactivate driver so clients subscribing to the log interface will notice
        if(!this->autorewind && !this->rewind_requested)
          this->enable=false;

        while(!this->autorewind && !this->rewind_requested)
        {
          usleep(100000);
          pthread_testcancel();

          // Process requests
          this->ProcessMessages();

          ReadLogTime_timeDouble += 0.1;
          ReadLogTime_time.tv_sec = (time_t)floor(ReadLogTime_timeDouble);
          ReadLogTime_time.tv_sec = (time_t)fmod(ReadLogTime_timeDouble,1.0);
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
    }
    else
      use_stored_tokens = false;

    // Parse out the header info
    if (this->ParseHeader(linenum, token_count, tokens,
                          &header_id, &curr_log_time, &type, &subtype) != 0)
      continue;

    if(reading_configs)
    {
      if(type != PLAYER_MSGTYPE_RESP_ACK)
      {
        // not a config
        reading_configs = false;
        // we'll reuse this tokenized string next time through, instead of
        // reading a fresh line from the file
        use_stored_tokens = true;
        continue;
      }
    }

    // Set the global timestamp
    ::ReadLogTime_timeDouble = curr_log_time;
    ::ReadLogTime_time.tv_sec = (time_t)floor(curr_log_time);
    ::ReadLogTime_time.tv_usec = (time_t)fmod(curr_log_time,1.0);

    gettimeofday(&tv,NULL);
    curr_wall_time = tv.tv_sec + tv.tv_usec/1e6;
    if(!reading_configs)
    {
      // Have we published at least one message from this log?
      if(last_wall_time >= 0)
      {
        // Wait until it's time to publish this message
        while((curr_wall_time - last_wall_time) <
              ((curr_log_time - last_log_time) / this->speed))
        {
          gettimeofday(&tv,NULL);
          curr_wall_time = tv.tv_sec + tv.tv_usec/1e6;
          this->ProcessMessages();
          usleep(1000);
        }
      }

      last_wall_time = curr_wall_time;
      last_log_time = curr_log_time;
    }

    // Look for a matching read interface; data will be output on
    // the corresponding provides interface.
    for (i = 0; i < this->provide_count; i++)
    {
      provide_id = this->provide_ids[i];
      if(Device::MatchDeviceAddress(header_id, provide_id))
      {
        this->ParseData(provide_id, type, subtype,
                        linenum, token_count, tokens, curr_log_time);
        break;
      }
    }
    if(i >= this->provide_count)
    {
      PLAYER_MSG6(2, "unhandled message from %d:%d:%d:%d %d:%d\n",
                  header_id.host,
                  header_id.robot,
                  header_id.interf,
                  header_id.index,
                  type, subtype);

    }
  }

  return;
}



////////////////////////////////////////////////////////////////////////////
// Process configuration requests
int ReadLog::ProcessLogConfig(QueuePointer & resp_queue,
                              player_msghdr_t * hdr,
                              void * data)
{
  player_log_set_read_state_t* sreq;
  player_log_get_state_t greq;

  switch(hdr->subtype)
  {
    case PLAYER_LOG_REQ_SET_READ_STATE:
      if(hdr->size != sizeof(player_log_set_read_state_t))
      {
        PLAYER_WARN2("request wrong size (%d != %d)",
                     hdr->size, sizeof(player_log_set_read_state_t));
        return(-1);
      }
      sreq = (player_log_set_read_state_t*)data;
      if(sreq->state)
      {
        puts("ReadLog: start playback");
        this->enable = true;
      }
      else
      {
        puts("ReadLog: stop playback");
        this->enable = false;
      }
      this->Publish(this->log_id, resp_queue,
                    PLAYER_MSGTYPE_RESP_ACK,
                    PLAYER_LOG_REQ_SET_READ_STATE);
      return(0);

    case PLAYER_LOG_REQ_GET_STATE:
      greq.type = PLAYER_LOG_TYPE_READ;
      if(this->enable)
        greq.state = 1;
      else
        greq.state = 0;

      this->Publish(this->log_id, resp_queue,
                    PLAYER_MSGTYPE_RESP_ACK,
                    PLAYER_LOG_REQ_GET_STATE,
                    (void*)&greq, sizeof(greq), NULL);
      return(0);

    case PLAYER_LOG_REQ_SET_READ_REWIND:
      // set the appropriate flag in the manager
      this->rewind_requested = true;

      this->Publish(this->log_id, resp_queue,
                    PLAYER_MSGTYPE_RESP_ACK,
                    PLAYER_LOG_REQ_SET_READ_REWIND);
      return(0);

    default:
      return(-1);
  }
}

int
ReadLog::ProcessPositionConfig(QueuePointer & resp_queue,
                               player_msghdr_t * hdr,
                               void * data)
{
  switch(hdr->subtype)
  {
    case PLAYER_POSITION2D_REQ_GET_GEOM:
      {
        // Find the right place from which to retrieve it
        int j;
        for(j=0;j<this->provide_count;j++)
        {
          if(Device::MatchDeviceAddress(this->provide_ids[j], hdr->addr))
            break;
        }
        if(j>=this->provide_count)
          return(-1);

        if(!this->provide_metadata[j])
          return(-1);

        this->Publish(this->provide_ids[j], resp_queue,
                      PLAYER_MSGTYPE_RESP_ACK, hdr->subtype,
                      this->provide_metadata[j],
                      sizeof(player_position2d_geom_t),
                      NULL);
        return(0);
      }
    default:
      return(-1);
  }
}

int
ReadLog::ProcessPosition3dConfig(QueuePointer & resp_queue,
                               player_msghdr_t * hdr,
                               void * data)
{
  switch(hdr->subtype)
  {
    case PLAYER_POSITION3D_REQ_GET_GEOM:
      {
        // Find the right place from which to retrieve it
        int j;
        for(j=0;j<this->provide_count;j++)
        {
          if(Device::MatchDeviceAddress(this->provide_ids[j], hdr->addr))
            break;
        }
        if(j>=this->provide_count)
          return(-1);

        if(!this->provide_metadata[j])
          return(-1);

        this->Publish(this->provide_ids[j], resp_queue,
                      PLAYER_MSGTYPE_RESP_ACK, hdr->subtype,
                      this->provide_metadata[j],
                      sizeof(player_position3d_geom_t),
                      NULL);
        return(0);
      }
    default:
      return(-1);
  }
}

int
ReadLog::ProcessFiducialConfig(QueuePointer & resp_queue,
                            player_msghdr_t * hdr,
                            void * data)
{
  switch(hdr->subtype)
  {
    case PLAYER_FIDUCIAL_REQ_GET_GEOM:
      {
        // Find the right place from which to retrieve it
        int j;
        for(j=0;j<this->provide_count;j++)
        {
          if(Device::MatchDeviceAddress(this->provide_ids[j], hdr->addr))
            break;
        }
        if(j>=this->provide_count)
        {
          puts("no matching device");
          return(-1);
        }

        if(!this->provide_metadata[j])
        {
          puts("no metadata");
          return(-1);
        }

        this->Publish(this->provide_ids[j], resp_queue,
                      PLAYER_MSGTYPE_RESP_ACK, hdr->subtype,
                      this->provide_metadata[j],
                      sizeof(player_fiducial_geom_t),
                      NULL);
        return(0);
      }
    default:
      return(-1);
  }
}

int
ReadLog::ProcessLaserConfig(QueuePointer & resp_queue,
                            player_msghdr_t * hdr,
                            void * data)
{
  switch(hdr->subtype)
  {
    case PLAYER_LASER_REQ_GET_GEOM:
      {
        // Find the right place from which to retrieve it
        int j;
        for(j=0;j<this->provide_count;j++)
        {
          if(Device::MatchDeviceAddress(this->provide_ids[j], hdr->addr))
            break;
        }
        if(j>=this->provide_count)
        {
          puts("no matching device");
          return(-1);
        }

        if(!this->provide_metadata[j])
        {
          puts("no metadata");
          return(-1);
        }

        this->Publish(this->provide_ids[j], resp_queue,
                      PLAYER_MSGTYPE_RESP_ACK, hdr->subtype,
                      this->provide_metadata[j],
                      sizeof(player_laser_geom_t),
                      NULL);
        return(0);
      }
    default:
      return(-1);
  }
}

int
ReadLog::ProcessRangerConfig(QueuePointer & resp_queue,
                            player_msghdr_t * hdr,
                            void * data)
{
  switch(hdr->subtype)
  {
    case PLAYER_RANGER_REQ_GET_GEOM:
      {
        // Find the right place from which to retrieve it
        int j;
        for(j=0;j<this->provide_count;j++)
        {
          if(Device::MatchDeviceAddress(this->provide_ids[j], hdr->addr))
            break;
        }
        if(j>=this->provide_count)
        {
          puts("no matching device");
          return(-1);
        }

        if(!this->provide_metadata[j])
        {
          puts("no metadata");
          return(-1);
        }

        this->Publish(this->provide_ids[j], resp_queue,
                      PLAYER_MSGTYPE_RESP_ACK, hdr->subtype,
                      ((ranger_meta_t*)this->provide_metadata[j])->geom,
                      sizeof(player_ranger_geom_t),
                      NULL);
        return(0);
      }

    case PLAYER_RANGER_REQ_GET_CONFIG:
      {
        // Find the right place from which to retrieve it
        int j;
        for(j=0;j<this->provide_count;j++)
        {
          if(Device::MatchDeviceAddress(this->provide_ids[j], hdr->addr))
            break;
        }
        if(j>=this->provide_count)
        {
          puts("no matching device");
          return(-1);
        }

        if(!this->provide_metadata[j])
        {
          puts("no metadata");
          return(-1);
        }

        this->Publish(this->provide_ids[j], resp_queue,
                      PLAYER_MSGTYPE_RESP_ACK, hdr->subtype,
                      ((ranger_meta_t*)this->provide_metadata[j])->config,
                      sizeof(player_ranger_config_t),
                      NULL);
        return(0);
      }

    default:
      return(-1);
  }
}

int
ReadLog::ProcessSonarConfig(QueuePointer & resp_queue,
                            player_msghdr_t * hdr,
                            void * data)
{
  switch(hdr->subtype)
  {
    case PLAYER_SONAR_REQ_GET_GEOM:
      {
        // Find the right place from which to retrieve it
        int j;
        for(j=0;j<this->provide_count;j++)
        {
          if(Device::MatchDeviceAddress(this->provide_ids[j], hdr->addr))
            break;
        }
        if(j>=this->provide_count)
          return(-1);

        if(!this->provide_metadata[j])
          return(-1);

        this->Publish(this->provide_ids[j], resp_queue,
                      PLAYER_MSGTYPE_RESP_ACK, hdr->subtype,
                      this->provide_metadata[j],
                      sizeof(player_sonar_geom_t),
                      NULL);
        return(0);
      }
    default:
      return(-1);
  }
}

int
ReadLog::ProcessWSNConfig(QueuePointer & resp_queue,
                          player_msghdr_t * hdr,
                          void * data)
{
    switch(hdr->subtype)
    {
        case PLAYER_WSN_REQ_DATATYPE:
        {
        // Find the right place from which to retrieve it
            int j;
            for(j=0;j<this->provide_count;j++)
            {
                if(Device::MatchDeviceAddress(this->provide_ids[j], hdr->addr))
                    break;
            }
            if(j>=this->provide_count)
                return(-1);

            if(!this->provide_metadata[j])
                return(-1);

            this->Publish(this->provide_ids[j], resp_queue,
                          PLAYER_MSGTYPE_RESP_ACK, hdr->subtype,
                          this->provide_metadata[j],
                          sizeof(player_wsn_datatype_config_t),
                          NULL);
            return(0);
        }
        default:
            return(-1);
    }
}

int
ReadLog::ProcessIMUConfig(QueuePointer &resp_queue,
                          player_msghdr_t * hdr,
                          void * data)
{
    switch(hdr->subtype)
    {
        case PLAYER_IMU_REQ_SET_DATATYPE:
        {
        // Find the right place from which to retrieve it
            int j;
            for(j=0;j<this->provide_count;j++)
            {
                if(Device::MatchDeviceAddress(this->provide_ids[j], hdr->addr))
                    break;
            }
            if(j>=this->provide_count)
                return(-1);

            if(!this->provide_metadata[j])
                return(-1);

            this->Publish(this->provide_ids[j], resp_queue,
                          PLAYER_MSGTYPE_RESP_ACK, hdr->subtype,
                          this->provide_metadata[j],
                          sizeof(player_imu_datatype_config_t),
                          NULL);
            return(0);
        }
        default:
            return(-1);
    }
}

int
ReadLog::ProcessMessage(QueuePointer & resp_queue,
                        player_msghdr_t * hdr,
                        void * data)
{
  // Handle log config requests
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, -1,
                           this->log_id))
  {
    return(this->ProcessLogConfig(resp_queue, hdr, data));
  }
  else if((hdr->type == PLAYER_MSGTYPE_REQ) &&
          (hdr->addr.interf == PLAYER_FIDUCIAL_CODE))
  {
    return(this->ProcessFiducialConfig(resp_queue, hdr, data));
  }
  else if((hdr->type == PLAYER_MSGTYPE_REQ) &&
          (hdr->addr.interf == PLAYER_LASER_CODE))
  {
    return(this->ProcessLaserConfig(resp_queue, hdr, data));
  }
  else if((hdr->type == PLAYER_MSGTYPE_REQ) &&
          (hdr->addr.interf == PLAYER_RANGER_CODE))
  {
    return(this->ProcessRangerConfig(resp_queue, hdr, data));
  }
  else if((hdr->type == PLAYER_MSGTYPE_REQ) &&
          (hdr->addr.interf == PLAYER_SONAR_CODE))
  {
    return(this->ProcessSonarConfig(resp_queue, hdr, data));
  }
  else if((hdr->type == PLAYER_MSGTYPE_REQ) &&
           (hdr->addr.interf == PLAYER_WSN_CODE))
  {
      return(this->ProcessWSNConfig(resp_queue, hdr, data));
  }
  else if((hdr->type == PLAYER_MSGTYPE_REQ) &&
           (hdr->addr.interf == PLAYER_IMU_CODE))
  {
      return(this->ProcessIMUConfig(resp_queue, hdr, data));
  }
  else if((hdr->type == PLAYER_MSGTYPE_REQ) &&
          (hdr->addr.interf == PLAYER_POSITION2D_CODE))
  {
    return(this->ProcessPositionConfig(resp_queue, hdr, data));
  }
  else if((hdr->type == PLAYER_MSGTYPE_REQ) &&
          (hdr->addr.interf == PLAYER_POSITION3D_CODE))
  {
    return(this->ProcessPosition3dConfig(resp_queue, hdr, data));
  }
  else if(particles_set &&
          Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                                PLAYER_LOCALIZE_REQ_GET_PARTICLES,
                                this->localize_addr))
  {
    this->Publish(this->localize_addr, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_LOCALIZE_REQ_GET_PARTICLES,
                  (void*)&particles, sizeof(particles), NULL);
    return(0);
  }
  else
    return -1;
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
int
ReadLog::ParseHeader(int linenum, int token_count, char **tokens,
                     player_devaddr_t *id, double *dtime,
                     unsigned short* type, unsigned short* subtype)
{
  char *name;
  player_interface_t interf;

  if (token_count < 7)
  {
    PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
    return -1;
  }

  name = tokens[3];

  if (lookup_interface(name, &interf) == 0)
  {
    *dtime = atof(tokens[0]);
    id->host = atoi(tokens[1]);
    id->robot = atoi(tokens[2]);
    id->interf = interf.interf;
    id->index = atoi(tokens[4]);
    *type = atoi(tokens[5]);
    *subtype = atoi(tokens[6]);
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
int ReadLog::ParseData(player_devaddr_t id,
                       unsigned short type, unsigned short subtype,
                       int linenum, int token_count, char **tokens,
                       double time)
{
  if (id.interf == PLAYER_BLOBFINDER_CODE)
    return this->ParseBlobfinder(id, type, subtype, linenum,
                                 token_count, tokens, time); 
  else if (id.interf == PLAYER_CAMERA_CODE)
    return this->ParseCamera(id, type, subtype, linenum,
                             token_count, tokens, time);
  else if (id.interf == PLAYER_GPS_CODE)
    return this->ParseGps(id, type, subtype, linenum,
                          token_count, tokens, time);
  else if (id.interf == PLAYER_JOYSTICK_CODE)
    return this->ParseJoystick(id, type, subtype, linenum,
                               token_count, tokens, time);
  else if (id.interf == PLAYER_LASER_CODE)
    return this->ParseLaser(id, type, subtype, linenum,
                            token_count, tokens, time);
  else if (id.interf == PLAYER_RANGER_CODE)
    return this->ParseRanger(id, type, subtype, linenum,
                            token_count, tokens, time);
  else if (id.interf == PLAYER_FIDUCIAL_CODE)
    return this->ParseFiducial(id, type, subtype, linenum,
                               token_count, tokens, time);
  else if (id.interf == PLAYER_LOCALIZE_CODE)
    return this->ParseLocalize(id, type, subtype, linenum,
                               token_count, tokens, time);
  else if (id.interf == PLAYER_SONAR_CODE)
    return this->ParseSonar(id, type, subtype, linenum,
                            token_count, tokens, time);
  else if (id.interf == PLAYER_POSITION2D_CODE)
    return this->ParsePosition(id, type, subtype, linenum,
                               token_count, tokens, time);
  else if (id.interf == PLAYER_OPAQUE_CODE)
    return this->ParseOpaque(id, type, subtype, linenum,
                               token_count, tokens, time);
  else if (id.interf == PLAYER_WIFI_CODE)
    return this->ParseWifi(id, type, subtype, linenum,
                           token_count, tokens, time);
  else if (id.interf == PLAYER_WSN_CODE)
      return this->ParseWSN(id, type, subtype, linenum,
                            token_count, tokens, time);
  else if (id.interf == PLAYER_COOPOBJECT_CODE)
      return this->ParseCoopObject(id, type, subtype, linenum,
                            token_count, tokens, time);
  else if (id.interf == PLAYER_IMU_CODE)
      return this->ParseIMU (id, type, subtype, linenum,
                            token_count, tokens, time);
  else if (id.interf == PLAYER_POINTCLOUD3D_CODE)
      return this->ParsePointCloud3d (id, type, subtype, linenum,
                                      token_count, tokens, time);
  else if (id.interf == PLAYER_PTZ_CODE)
      return this->ParsePTZ (id, type, subtype, linenum,
                            token_count, tokens, time);

  else if (id.interf == PLAYER_ACTARRAY_CODE)
      return this->ParseActarray (id, type, subtype, linenum,
                                  token_count, tokens, time);
  else if (id.interf == PLAYER_AIO_CODE)
      return this->ParseAIO(id, type, subtype, linenum, token_count, tokens,
                            time);
  else if (id.interf == PLAYER_DIO_CODE)
      return this->ParseDIO(id, type, subtype, linenum, token_count, tokens,
                            time);
  else if (id.interf == PLAYER_RFID_CODE)
      return this->ParseRFID(id, type, subtype, linenum, token_count, tokens,
                            time);
  else if (id.interf == PLAYER_POSITION3D_CODE)
    return this->ParsePosition3d(id, type, subtype, linenum,
                                 token_count, tokens, time);
  else if (id.interf == PLAYER_POWER_CODE)
    return this->ParsePower(id, type, subtype, linenum,
                                 token_count, tokens, time);


  PLAYER_WARN1("unknown interface code [%s]",
               ::lookup_interface_name(0, id.interf));
  return -1;
}

////////////////////////////////////////////////////////////////////////////
// Parse blobfinder data
int ReadLog::ParseBlobfinder(player_devaddr_t id,
                           unsigned short type, unsigned short subtype,
                           int linenum, int token_count, char **tokens,
                           double time)
{
  player_blobfinder_data_t data;
  size_t size;
  int i, blob_count;

  if (token_count < 10)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }

  data.width = atoi(tokens[7]);
  data.height = atoi(tokens[8]);
  blob_count = atoi(tokens[9]);
  data.blobs_count = blob_count;

  if (token_count < 10 + blob_count * 10)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }
  player_blobfinder_blob_t *blob = new player_blobfinder_blob_t[blob_count];

  for (i = 0; i < blob_count; i++)
  {
    blob[i].id =  atoi(tokens[10 + i*10]);
    blob[i].color = atoi(tokens[11 + i*10]);
    blob[i].area = atoi(tokens[12 + i*10]);
    blob[i].x = atoi(tokens[13 + i*10]);
    blob[i].y = atoi(tokens[14 + i*10]);
    blob[i].left = atoi(tokens[15 + i*10]);
    blob[i].right = atoi(tokens[16 + i*10]);
    blob[i].top = atoi(tokens[17 + i*10]);
    blob[i].bottom = atoi(tokens[18 + i*10]);
    blob[i].range = atof(tokens[19 + i*10]);
  }
  data.blobs = blob;

  size = sizeof(data) - sizeof(data.blobs) + blob_count * sizeof(data.blobs[0]);
  this->Publish(id,type,subtype, (void*) &data, size, &time);
  delete[] blob;

  return 0;
}
////////////////////////////////////////////////////////////////////////////
// Parse camera data
int ReadLog::ParseCamera(player_devaddr_t id,
                           unsigned short type, unsigned short subtype,
                           int linenum, int token_count, char **tokens,
                           double time)
{
  player_camera_data_t data;
  size_t src_size, dst_size;

  if (token_count < 14)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }

  data.width = atoi(tokens[7]);
  data.height = atoi(tokens[8]);
  data.bpp = atoi(tokens[9]);
  data.format = atoi(tokens[10]);
  data.compression = atoi(tokens[11]);
  data.image_count = atoi(tokens[12]);

  // Check sizes
  src_size = strlen(tokens[13]);
  dst_size = ::DecodeHexSize(src_size);
  assert(dst_size == data.image_count);
  data.image = new uint8_t[dst_size];
  // TODO: This assertion doesn't seem to work
  //assert(dst_size < sizeof(data.image));

  // Decode string
  ::DecodeHex(data.image, dst_size, tokens[13], src_size);

  this->Publish(id,type,subtype,(void*) &data, sizeof(data) - sizeof(data.image) + dst_size, &time);
  
  delete [] data.image;
  
  return 0;
}
////////////////////////////////////////////////////////////////////////////
// Parse fiducial data
int ReadLog::ParseFiducial(player_devaddr_t id,
                           unsigned short type, unsigned short subtype,
                           int linenum, int token_count, char **tokens,
                           double time)
{
  if (token_count < 7)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }

  switch(type) {
      case PLAYER_MSGTYPE_DATA:
            switch (subtype) {
                case PLAYER_FIDUCIAL_DATA_SCAN:
                    player_fiducial_data_t data;
                    int fiducial_count;

                    fiducial_count = atoi(tokens[7]);

                    data.fiducials_count = fiducial_count;
                    //data.fiducials = (player_fiducial_item_t *) malloc(data.fiducials_count * sizeof(data.fiducials[0]));
                    data.fiducials = new player_fiducial_item_t[data.fiducials_count];

                    for (int i = 0; i < fiducial_count; i++) {
                        data.fiducials[i].id = atof(tokens[13 * i + 8]);
                        data.fiducials[i].pose.px = atof(tokens[13 * i + 9]);
                        data.fiducials[i].pose.py = atof(tokens[13 * i + 10]);
                        data.fiducials[i].pose.pz = atof(tokens[13 * i + 11]);
                        data.fiducials[i].pose.proll = atof(tokens[13 * i + 12]);
                        data.fiducials[i].pose.ppitch = atof(tokens[13 * i + 13]);
                        data.fiducials[i].pose.pyaw = atof(tokens[13 * i + 14]);
                        data.fiducials[i].upose.px = atof(tokens[13 * i + 15]);
                        data.fiducials[i].upose.py = atof(tokens[13 * i + 16]);
                        data.fiducials[i].upose.pz = atof(tokens[13 * i + 17]);
                        data.fiducials[i].upose.proll = atof(tokens[13 * i + 18]);
                        data.fiducials[i].upose.ppitch = atof(tokens[13 * i + 19]);
                        data.fiducials[i].upose.pyaw = atof(tokens[13 * i + 20]);
                    }
                    this->Publish(id, type, subtype,
                            (void*) & data, sizeof (data), &time);

                    delete [] data.fiducials;
                    return (0);
                default:
                    PLAYER_ERROR1("unimplemented fiducial data subtype %d\n", subtype);
                    return(-1);
            }
      case PLAYER_MSGTYPE_RESP_ACK:
            switch (subtype) {
                case PLAYER_FIDUCIAL_REQ_GET_GEOM:

                    if (token_count < 17) {
                        PLAYER_ERROR2("incomplete line at %s:%d",
                                this->filename, linenum);
                        return -1;
                    }
                    player_fiducial_geom_t* geom;
                    
                    geom = (player_fiducial_geom_t*) calloc(1, sizeof (player_fiducial_geom_t));
                    assert(geom);

                    geom->pose.px = atof(tokens[7]);
                    geom->pose.py = atof(tokens[8]);
                    geom->pose.pz = atof(tokens[9]);
                    geom->pose.proll = atof(tokens[10]);
                    geom->pose.ppitch = atof(tokens[11]);
                    geom->pose.pyaw = atof(tokens[12]);
                    geom->size.sl = atof(tokens[13]);
                    geom->size.sw = atof(tokens[14]);
                    geom->size.sh = atof(tokens[15]);
                    geom->fiducial_size.sl = atof(tokens[16]);
                    geom->fiducial_size.sw = atof(tokens[17]);

                    // Find the right place to put it
                    int j;
                    for (j = 0; j<this->provide_count; j++) {
                        if (Device::MatchDeviceAddress(this->provide_ids[j], id))
                            break;
                    }
                    assert(j<this->provide_count);

                    if (this->provide_metadata[j])
                        free(this->provide_metadata[j]);

                    this->provide_metadata[j] = (void*) geom;

                    // nothing to publish
                    return (0);
                    
                default:
                    PLAYER_ERROR1("unimplemented fiducial data subtype %d\n", subtype);
                    return(-1);
            }
        default:
            PLAYER_ERROR1("unimplemented fiducial data type %d\n", type);
            return (-1);

  }

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse GPS data
int ReadLog::ParseGps(player_devaddr_t id,
                           unsigned short type, unsigned short subtype,
                           int linenum, int token_count, char **tokens,
                           double time)
{
  player_gps_data_t data;

  if (token_count < 19)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }

  data.time_sec = (int) atof(tokens[7]);
  data.time_usec = (int) (atof(tokens[7]) - (float)(data.time_sec) * 1e6);

  data.latitude = (int) (atof(tokens[8]) * 1e7);
  data.longitude = (int) (atof(tokens[9]) * 1e7);
  data.altitude = (int) (atof(tokens[10]) * 1e3);

  data.utm_e = atof(tokens[11]);
  data.utm_n = atof(tokens[12]);

  data.hdop = (int) (10 * atof(tokens[13]));
  data.hdop = (int) (10 * atof(tokens[14]));
  data.err_horz = atof(tokens[15]);
  data.err_vert = atof(tokens[16]);

  data.quality = atoi(tokens[17]);
  data.num_sats = atoi(tokens[18]);

  this->Publish(id,type,subtype, (void*) &data, sizeof(data), &time);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse joystick data
int ReadLog::ParseJoystick(player_devaddr_t id,
                           unsigned short type, unsigned short subtype,
                           int linenum, int token_count, char **tokens,
                           double time)
{
  player_joystick_data_t data;

  if (token_count < 14)
  {
    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
    return -1;
  }

  data.pos[0] = atoi(tokens[7]);
  data.pos[1] = atoi(tokens[8]);
  data.pos[2] = atoi(tokens[9]);
  data.scale[0] = atoi(tokens[10]);
  data.scale[1] = atoi(tokens[11]);
  data.scale[2] = atoi(tokens[12]);
  data.buttons = (unsigned int)atoi(tokens[13]);

  this->Publish(id,type,subtype,(void*) &data, sizeof(data), &time);

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Parse laser data
int ReadLog::ParseLaser(player_devaddr_t id,
                        unsigned short type, unsigned short subtype,
                        int linenum,
                        int token_count, char **tokens, double time)
{
  int i, count, ret;
  ret = 0;
  switch(type)
  {
    case PLAYER_MSGTYPE_DATA:
      switch(subtype)
      {
        case PLAYER_LASER_DATA_SCAN:
          {
            player_laser_data_t data;

            if (token_count < 13)
            {
              PLAYER_ERROR2("incomplete line at %s:%d",
                            this->filename, linenum);
              return -1;
            }

            data.id = atoi(tokens[7]);
            data.min_angle = static_cast<float> (atof(tokens[8]));
            data.max_angle = static_cast<float> (atof(tokens[9]));
            data.resolution = static_cast<float> (atof(tokens[10]));
            data.max_range = static_cast<float> (atof(tokens[11]));
            data.ranges_count = atoi(tokens[12]);
            data.intensity_count = data.ranges_count;

            data.ranges = new float[ data.ranges_count ];
            data.intensity = new uint8_t[ data.ranges_count ];

            count = 0;
            for (i = 13; i < token_count; i += 2)
            {
              data.ranges[count] = static_cast<float> (atof(tokens[i + 0]));
              data.intensity[count] = atoi(tokens[i + 1]);
              count += 1;
            }

            if (count != (int)data.ranges_count)
            {
              PLAYER_ERROR2("range count mismatch at %s:%d",
                            this->filename, linenum);
              ret = -1;
            }
            else
            {
              this->Publish(id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                          (void*)&data, sizeof(data), &time);
            }
            delete [] data.ranges;
            delete [] data.intensity;

            return ret;
          }

        case PLAYER_LASER_DATA_SCANPOSE:
          {
            player_laser_data_scanpose_t data;

            if (token_count < 16)
            {
              PLAYER_ERROR2("incomplete line at %s:%d",
                            this->filename, linenum);
              return -1;
            }

            data.scan.id = atoi(tokens[7]);
            data.pose.px = atof(tokens[8]);
            data.pose.py = atof(tokens[9]);
            data.pose.pa = atof(tokens[10]);
            data.scan.min_angle = static_cast<float> (atof(tokens[11]));
            data.scan.max_angle = static_cast<float> (atof(tokens[12]));
            data.scan.resolution = static_cast<float> (atof(tokens[13]));
            data.scan.max_range = static_cast<float> (atof(tokens[14]));
            data.scan.ranges_count = atoi(tokens[15]);
            data.scan.intensity_count = data.scan.ranges_count;

            data.scan.ranges = new float[ data.scan.ranges_count ];
            data.scan.intensity = new uint8_t[ data.scan.ranges_count ];

            count = 0;
            for (i = 16; i < token_count; i += 2)
            {
              data.scan.ranges[count] = static_cast<float> (atof(tokens[i + 0]));
              data.scan.intensity[count] = atoi(tokens[i + 1]);
              count += 1;
            }

            if (count != (int)data.scan.ranges_count)
            {
              PLAYER_ERROR2("range count mismatch at %s:%d",
                            this->filename, linenum);
              ret = -1;
            }
            else
            {
              this->Publish(id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                          (void*)&data, sizeof(data), &time);
              delete [] data.scan.ranges;
              delete [] data.scan.intensity;
            }
            return ret;
          }


        default:
          PLAYER_ERROR1("unknown laser data subtype %d\n", subtype);
          return(-1);
      }
      break;

    case PLAYER_MSGTYPE_RESP_ACK:
      switch(subtype)
      {
        case PLAYER_LASER_REQ_GET_GEOM:
          {
            if(token_count < 12)
            {
              PLAYER_ERROR2("incomplete line at %s:%d",
                            this->filename, linenum);
              return -1;
            }

            // cache it
            player_laser_geom_t* geom =
                    (player_laser_geom_t*)calloc(1,sizeof(player_laser_geom_t));
            assert(geom);

            geom->pose.px = atof(tokens[7]);
            geom->pose.py = atof(tokens[8]);
            geom->pose.pyaw = atof(tokens[9]);
            geom->size.sl = atof(tokens[10]);
            geom->size.sw = atof(tokens[11]);

            // Find the right place to put it
            int j;
            for(j=0;j<this->provide_count;j++)
            {
              if(Device::MatchDeviceAddress(this->provide_ids[j], id))
                break;
            }
            assert(j<this->provide_count);

            if(this->provide_metadata[j])
              free(this->provide_metadata[j]);

            this->provide_metadata[j] = (void*)geom;

            // nothing to publish
            return(0);
          }

        default:
          PLAYER_ERROR1("unknown laser reply subtype %d\n", subtype);
          return(-1);
      }
      break;

    default:
      PLAYER_ERROR1("unknown laser msg type %d\n", type);
      return(-1);
  }
}


////////////////////////////////////////////////////////////////////////////
// Parse ranger data
int ReadLog::ParseRanger(player_devaddr_t id,
                        unsigned short type, unsigned short subtype,
                        int linenum,
                        int token_count, char **tokens, double time)
{
  int i, count, ret;
  ret = 0;
  switch(type)
  {
    case PLAYER_MSGTYPE_DATA:
      switch(subtype)
      {
        case PLAYER_RANGER_DATA_RANGE:
          {
            player_ranger_data_range_t data;

            if (token_count < 8)
            {
              PLAYER_ERROR2("incomplete line at %s:%d",
                            this->filename, linenum);
              return -1;
            }

            data.ranges_count = atoi(tokens[7]);

            data.ranges = new double[ data.ranges_count ];

            count = 0;
            for (i = 8; i < token_count; i++)
            {
              data.ranges[count] = static_cast<double> (atof(tokens[i + 0]));
              count++;
            }

            if (count != (int)data.ranges_count)
            {
              PLAYER_ERROR2("range count mismatch at %s:%d",
                            this->filename, linenum);
              ret = -1;
            }
            else
            {
              this->Publish(id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                          (void*)&data, sizeof(data), &time);
            }
            delete [] data.ranges;

            return ret;
          }

        case PLAYER_RANGER_DATA_RANGESTAMPED:
          {
            player_ranger_data_rangestamped_t data;

            if (token_count < 10)
            {
              PLAYER_ERROR2("incomplete line at %s:%d",
                            this->filename, linenum);
              return -1;
            }

	    int total_count=7;
            data.data.ranges_count = atoi(tokens[total_count]);
	    total_count++;

            data.data.ranges = new double[ data.data.ranges_count ];
	    
            count = 0;
	    int loop_size = token_count;
	    if (total_count + (int)data.data.ranges_count < loop_size)
	      loop_size = total_count + (int)data.data.ranges_count;	    
            for (i = total_count; i < loop_size; i += 2)
            {
              data.data.ranges[count] = static_cast<double> (atof(tokens[i]));
              count++;
	      total_count++;
            }

            if (count != (int)data.data.ranges_count)
            {
              PLAYER_ERROR2("range count mismatch at %s:%d",
                            this->filename, linenum);
              delete [] data.data.ranges;
	      return -1;
            }

            data.have_geom = atoi(tokens[total_count]);
	    total_count++;

	    if (data.have_geom)
	      {
		if (token_count < total_count+11)
		  {
		    PLAYER_ERROR2("incomplete line at %s:%d",
				  this->filename, linenum);
		    delete [] data.data.ranges;
		    return -1;
		  }

		data.geom.pose.px = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.geom.pose.py = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.geom.pose.pz = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.geom.pose.proll = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.geom.pose.ppitch = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.geom.pose.pyaw = static_cast<double> (atof(tokens[total_count]));
		
		total_count++;
		data.geom.size.sw = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.geom.size.sl = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.geom.size.sh = static_cast<double> (atof(tokens[total_count]));
		total_count++;

		data.geom.element_poses_count = atoi(tokens[total_count]);
		total_count++;
		
		data.geom.element_poses = new player_pose3d_t [ data.geom.element_poses_count ];

		count = 0;
		loop_size = token_count;
		if (total_count + (int)data.geom.element_poses_count*6 < loop_size)
		  loop_size = total_count + (int)data.geom.element_poses_count*6;
		for (i = total_count; i < loop_size; i += 6)
		  {
		    data.geom.element_poses[count].px = static_cast<double> (atof(tokens[i]));
		    total_count++;
		    data.geom.element_poses[count].py = static_cast<double> (atof(tokens[i+1]));
		    total_count++;
		    data.geom.element_poses[count].pz = static_cast<double> (atof(tokens[i+2]));
		    total_count++;
		    data.geom.element_poses[count].proll = static_cast<double> (atof(tokens[i+3]));
		    total_count++;
		    data.geom.element_poses[count].ppitch = static_cast<double> (atof(tokens[i+4]));
		    total_count++;
		    data.geom.element_poses[count].pyaw = static_cast<double> (atof(tokens[i+5]));
		    total_count++;
		    count++;
		  }

		if (count != (int)data.geom.element_poses_count || total_count > token_count)
		  {
		    PLAYER_ERROR2("poses count mismatch at %s:%d",
				  this->filename, linenum);
		    delete [] data.data.ranges;
		    delete [] data.geom.element_poses;
		    return -1;
		  }


		data.geom.element_sizes_count = atoi(tokens[total_count]);
		total_count++;
		
		data.geom.element_sizes = new player_bbox3d_t [ data.geom.element_sizes_count ];

		count = 0;
		loop_size = token_count;
		if (total_count + (int)data.geom.element_sizes_count*3 < loop_size)
		  loop_size = total_count + (int)data.geom.element_sizes_count*3;
		for (i = total_count; i < loop_size; i += 3)
		  {
		    data.geom.element_sizes[count].sw = static_cast<double> (atof(tokens[i]));
		    total_count++;
		    data.geom.element_sizes[count].sl = static_cast<double> (atof(tokens[i+1]));
		    total_count++;
		    data.geom.element_sizes[count].sh = static_cast<double> (atof(tokens[i+2]));
		    total_count++;
		    count++;
		  }

		if (count != (int)data.geom.element_sizes_count || total_count > token_count)
		  {
		    PLAYER_ERROR2("sizes count mismatch at %s:%d",
				  this->filename, linenum);
		    delete [] data.data.ranges;
		    delete [] data.geom.element_poses;
		    delete [] data.geom.element_sizes;
		    return -1;
		  }
	      }

            data.have_config = atoi(tokens[total_count]);

	    if (data.have_config)
	      {
		
		if (token_count < total_count+7)
		  {
		    PLAYER_ERROR2("incomplete line at %s:%d",
				  this->filename, linenum);
		    delete [] data.data.ranges;
		    if (data.have_geom)
		      {
			delete [] data.geom.element_poses;
			delete [] data.geom.element_sizes;
		      }
		    return -1;
		  }

		data.config.min_angle = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.config.max_angle = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.config.angular_res = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.config.min_range = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.config.max_range = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.config.range_res = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.config.frequency = static_cast<double> (atof(tokens[total_count]));
		total_count++;		
	      }

	    if (total_count != token_count)
	      {
		PLAYER_ERROR2("invalid line at %s:%d: number of tokens does not "
			      "match count", filename, linenum);
		delete [] data.data.ranges;
		if (data.have_geom)
		  {
		    delete [] data.geom.element_poses;
		    delete [] data.geom.element_sizes;
		  }		
		return -1;
	      }
	    
	    this->Publish(id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                          (void*)&data, sizeof(data), &time);
	    delete [] data.data.ranges;
	    if (data.have_geom)
	      {
		delete [] data.geom.element_poses;
		delete [] data.geom.element_sizes;
	      }
	    
            return ret;
          }

        case PLAYER_RANGER_DATA_INTNS:
          {
            player_ranger_data_intns_t data;

            if (token_count < 8)
            {
              PLAYER_ERROR2("incomplete line at %s:%d",
                            this->filename, linenum);
              return -1;
            }

            data.intensities_count = atoi(tokens[7]); 
            data.intensities = new double[ data.intensities_count ];

            count = 0;
            for (i = 8; i < token_count; i++)
            {
              data.intensities[count] = static_cast<double> (atof(tokens[i + 0]));
              count++;
            }

            if (count != (int)data.intensities_count)
            {
              PLAYER_ERROR2("range count mismatch at %s:%d",
                            this->filename, linenum);
              ret = -1;
            }
            else
            {
              this->Publish(id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                          (void*)&data, sizeof(data), &time);
            }
            delete [] data.intensities;

            return ret;
          }

        case PLAYER_RANGER_DATA_INTNSSTAMPED:
          {
            player_ranger_data_intnsstamped_t data;

            if (token_count < 10)
            {
              PLAYER_ERROR2("incomplete line at %s:%d",
                            this->filename, linenum);
              return -1;
            }

	    int total_count=7;
            data.data.intensities_count = atoi(tokens[total_count]);
	    total_count++;

            data.data.intensities = new double[ data.data.intensities_count ];
	    
            count = 0;
	    int loop_size = token_count;
	    if (total_count + (int)data.data.intensities_count < loop_size)
	      loop_size = total_count + (int)data.data.intensities_count;
            for (i = total_count; i < loop_size; i += 2)
            {
              data.data.intensities[count] = static_cast<double> (atof(tokens[i]));
              count++;
	      total_count++;
            }

            if (count != (int)data.data.intensities_count)
            {
              PLAYER_ERROR2("range count mismatch at %s:%d",
                            this->filename, linenum);
              delete [] data.data.intensities;
	      return -1;
            }

            data.have_geom = atoi(tokens[total_count]);
	    total_count++;

	    if (data.have_geom)
	      {
		if (token_count < total_count+11)
		  {
		    PLAYER_ERROR2("incomplete line at %s:%d",
				  this->filename, linenum);
		    delete [] data.data.intensities;
		    return -1;
		  }

		data.geom.pose.px = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.geom.pose.py = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.geom.pose.pz = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.geom.pose.proll = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.geom.pose.ppitch = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.geom.pose.pyaw = static_cast<double> (atof(tokens[total_count]));
		
		total_count++;
		data.geom.size.sw = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.geom.size.sl = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.geom.size.sh = static_cast<double> (atof(tokens[total_count]));
		total_count++;

		data.geom.element_poses_count = atoi(tokens[total_count]);
		total_count++;
		
		data.geom.element_poses = new player_pose3d_t [ data.geom.element_poses_count ];

		count = 0;
		loop_size=token_count;
		if (total_count + (int)data.geom.element_poses_count*6 < loop_size)
		  loop_size = total_count + (int)data.geom.element_poses_count*6;
		for (i = total_count; i < loop_size; i += 6)
		  {
		    data.geom.element_poses[count].px = static_cast<double> (atof(tokens[i]));
		    total_count++;
		    data.geom.element_poses[count].py = static_cast<double> (atof(tokens[i+1]));
		    total_count++;
		    data.geom.element_poses[count].pz = static_cast<double> (atof(tokens[i+2]));
		    total_count++;
		    data.geom.element_poses[count].proll = static_cast<double> (atof(tokens[i+3]));
		    total_count++;
		    data.geom.element_poses[count].ppitch = static_cast<double> (atof(tokens[i+4]));
		    total_count++;
		    data.geom.element_poses[count].pyaw = static_cast<double> (atof(tokens[i+5]));
		    total_count++;
		    count++;
		  }

		if (count != (int)data.geom.element_poses_count || total_count > token_count)
		  {
		    PLAYER_ERROR2("poses count mismatch at %s:%d",
				  this->filename, linenum);
		    delete [] data.data.intensities;
		    delete [] data.geom.element_poses;
		    return -1;
		  }


		data.geom.element_sizes_count = atoi(tokens[total_count]);
		total_count++;
		
		data.geom.element_sizes = new player_bbox3d_t [ data.geom.element_sizes_count ];

		count = 0;
		loop_size=token_count;
		if (total_count + (int)data.geom.element_sizes_count*3 < loop_size)
		  loop_size = total_count + (int)data.geom.element_sizes_count*3;
		for (i = total_count; i < loop_size; i += 3)
		  {
		    data.geom.element_sizes[count].sw = static_cast<double> (atof(tokens[i]));
		    total_count++;
		    data.geom.element_sizes[count].sl = static_cast<double> (atof(tokens[i+1]));
		    total_count++;
		    data.geom.element_sizes[count].sh = static_cast<double> (atof(tokens[i+2]));
		    total_count++;
		    count++;
		  }

		if (count != (int)data.geom.element_sizes_count || total_count > token_count)
		  {
		    PLAYER_ERROR2("sizes count mismatch at %s:%d",
				  this->filename, linenum);
		    delete [] data.data.intensities;
		    delete [] data.geom.element_poses;
		    delete [] data.geom.element_sizes;
		    return -1;
		  }
	      }

            data.have_config = atoi(tokens[total_count]);

	    if (data.have_config)
	      {
		
		if (token_count < total_count+7)
		  {
		    PLAYER_ERROR2("incomplete line at %s:%d",
				  this->filename, linenum);
		    delete [] data.data.intensities;
		    if (data.have_geom)
		      {
			delete [] data.geom.element_poses;
			delete [] data.geom.element_sizes;
		      }
		    return -1;
		  }

		data.config.min_angle = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.config.max_angle = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.config.angular_res = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.config.min_range = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.config.max_range = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.config.range_res = static_cast<double> (atof(tokens[total_count]));
		total_count++;
		data.config.frequency = static_cast<double> (atof(tokens[total_count]));
		total_count++;		
	      }
	    
	    if (total_count != token_count)
	      {
		PLAYER_ERROR2("invalid line at %s:%d: number of tokens does not "
			      "match count", filename, linenum);
		delete [] data.data.intensities;
		if (data.have_geom)
		  {
		    delete [] data.geom.element_poses;
		    delete [] data.geom.element_sizes;
		  }		
		return -1;
	      }

	    this->Publish(id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                          (void*)&data, sizeof(data), &time);
	    delete [] data.data.intensities;
	    if (data.have_geom)
	      {
		delete [] data.geom.element_poses;
		delete [] data.geom.element_sizes;
	      }	    
            return ret;
          }

        default:
          PLAYER_ERROR1("unknown ranger data subtype %d\n", subtype);
          return(-1);
      }
      break;

    case PLAYER_MSGTYPE_RESP_ACK:
      switch(subtype)
      {
        case PLAYER_RANGER_REQ_GET_GEOM:
          {
            if(token_count < 18)
            {
              PLAYER_ERROR2("incomplete line at %s:%d",
                            this->filename, linenum);
              return -1;
            }

	    int num_poses=atoi(tokens[16]);

            if(token_count < 18+num_poses*6)
            {
              PLAYER_ERROR2("incomplete line at %s:%d",
                            this->filename, linenum);
              return -1;
            }

	    int num_sizes=atoi(tokens[17+num_poses*6]);

            if(token_count < 18+num_poses*6+num_sizes*3)
            {
              PLAYER_ERROR2("incomplete line at %s:%d",
                            this->filename, linenum);
              return -1;
            }

	    
            // cache it
            player_ranger_geom_t* geom =
	      (player_ranger_geom_t*)calloc(1,sizeof(player_ranger_geom_t)+sizeof(player_pose3d_t)*num_poses+sizeof(player_bbox3d_t)*num_sizes);
            assert(geom);

            geom->pose.px = atof(tokens[7]);
            geom->pose.py = atof(tokens[8]);
	    geom->pose.pz = atof(tokens[9]);
            geom->pose.proll = atof(tokens[10]);
            geom->pose.ppitch = atof(tokens[11]);
            geom->pose.pyaw = atof(tokens[12]);
            geom->size.sw = atof(tokens[13]);
            geom->size.sl = atof(tokens[14]);
            geom->size.sh = atof(tokens[15]);
	    geom->element_poses_count = num_poses;
	    geom->element_poses = new player_pose3d_t [ num_poses ];
	    geom->element_sizes_count = num_sizes;
	    geom->element_sizes = new player_bbox3d_t [ num_sizes ];

	    for (int i=0; i < num_poses; i++)
	      {
		geom->element_poses[i].px=atof(tokens[17+i*6]);
		geom->element_poses[i].py=atof(tokens[17+i*6+1]);
		geom->element_poses[i].pz=atof(tokens[17+i*6+2]);
		geom->element_poses[i].proll=atof(tokens[17+i*6+3]);
		geom->element_poses[i].ppitch=atof(tokens[17+i*6+4]);
		geom->element_poses[i].pyaw=atof(tokens[17+i*6+5]);
	      }

	    for (int i=0; i < num_sizes; i++)
	      {
		geom->element_sizes[i].sw=atof(tokens[17+num_poses*6+1+i*3]);
		geom->element_sizes[i].sl=atof(tokens[17+num_poses*6+1+i*3+1]);
		geom->element_sizes[i].sh=atof(tokens[17+num_poses*6+1+i*3+2]);
	      }


            // Find the right place to put it
            int j;
            for(j=0;j<this->provide_count;j++)
            {
              if(Device::MatchDeviceAddress(this->provide_ids[j], id))
                break;
            }
            assert(j<this->provide_count);

	    // if something is already here, use it
            if(this->provide_metadata[j])
	      ((ranger_meta_t*)this->provide_metadata[j])->geom = geom;
	    else
	      {
		ranger_meta_t* meta =
		  (ranger_meta_t*)calloc(1,sizeof(ranger_meta_t));
		meta->geom=geom;
		this->provide_metadata[j] = (void*)meta;
	      }

            // nothing to publish
            return(0);
          }

        case PLAYER_RANGER_REQ_GET_CONFIG:
          {
            if(token_count < 14)
            {
              PLAYER_ERROR2("incomplete line at %s:%d",
                            this->filename, linenum);
              return -1;
            }

            // cache it
            player_ranger_config_t* config =
	      (player_ranger_config_t*)calloc(1,sizeof(player_ranger_config_t));
            assert(config);

            config->min_angle = atof(tokens[7]);
            config->max_angle = atof(tokens[8]);
	    config->angular_res = atof(tokens[9]);
            config->min_range = atof(tokens[10]);
            config->max_range = atof(tokens[11]);
            config->range_res = atof(tokens[12]);
            config->frequency = atof(tokens[13]);
	    
            // Find the right place to put it
            int j;
            for(j=0;j<this->provide_count;j++)
            {
              if(Device::MatchDeviceAddress(this->provide_ids[j], id))
                break;
            }
            assert(j<this->provide_count);

	    // if something is already here, use it
            if(this->provide_metadata[j]) {
	      ((ranger_meta_t*)this->provide_metadata[j])->config = config;
	    }
	    else
	      {
		ranger_meta_t* meta =
		  (ranger_meta_t*)calloc(1,sizeof(ranger_meta_t));
		meta->config=config;
		this->provide_metadata[j] = (void*)meta;
	      }
	    
            // nothing to publish
            return(0);
          }

        default:
          PLAYER_ERROR1("unknown ranger reply subtype %d\n", subtype);
          return(-1);
      }
      break;

    default:
      PLAYER_ERROR1("unknown ranger msg type %d\n", type);
      return(-1);
  }
}



////////////////////////////////////////////////////////////////////////////
// Parse localize data
int ReadLog::ParseLocalize(player_devaddr_t id,
                           unsigned short type, unsigned short subtype,
                           int linenum,
                           int token_count, char **tokens, double time)
{
  int i, count;

  switch(type)
  {
    case PLAYER_MSGTYPE_DATA:
      switch(subtype)
      {
        case PLAYER_LOCALIZE_DATA_HYPOTHS:
          {
	    player_localize_data_t hypoths;

            if (token_count < 10)
            {
              PLAYER_ERROR2("incomplete line at %s:%d",
                            this->filename, linenum);
              return -1;
            }


	    hypoths.pending_count = atoi(tokens[7]);
	    hypoths.pending_time = atof(tokens[8]);
	    hypoths.hypoths_count = atoi(tokens[9]);
	    hypoths.hypoths = new player_localize_hypoth_t[hypoths.hypoths_count];

            count = 0;
            for (i = 10; i < token_count; i += 10)
            {
              hypoths.hypoths[count].mean.px = atof(tokens[i + 0]);
              hypoths.hypoths[count].mean.py = atof(tokens[i + 1]);
              hypoths.hypoths[count].mean.pa = atof(tokens[i + 2]);
              hypoths.hypoths[count].cov[0] = atof(tokens[i + 3]);
              hypoths.hypoths[count].cov[1] = atof(tokens[i + 4]);
              hypoths.hypoths[count].cov[2] = atof(tokens[i + 5]);
              hypoths.hypoths[count].cov[3] = atof(tokens[i + 6]);
              hypoths.hypoths[count].cov[4] = atof(tokens[i + 7]);
              hypoths.hypoths[count].cov[5] = atof(tokens[i + 8]);
              hypoths.hypoths[count].alpha = atof(tokens[i + 9]);
              count += 1;
            }

            if (count != (int)hypoths.hypoths_count)
            {
              PLAYER_ERROR2("hypoths count mismatch at %s:%d",
                            this->filename, linenum);
              delete [] hypoths.hypoths;
              return -1;

            }

            this->Publish(id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                          (void*)&hypoths, sizeof(hypoths), &time);
            delete [] hypoths.hypoths;
            return(0);
          }


        default:
          PLAYER_ERROR1("unknown localize data subtype %d\n", subtype);
          return(-1);
      }
      break;

    case PLAYER_MSGTYPE_RESP_ACK:
      switch(subtype)
      {
        case PLAYER_LOCALIZE_REQ_GET_PARTICLES:
          {
            if(token_count < 12)
            {
              PLAYER_ERROR2("incomplete line at %s:%d",
                            this->filename, linenum);
              return -1;
            }


	    particles.mean.px = atof(tokens[7]);
	    particles.mean.py = atof(tokens[8]);
	    particles.mean.pa = atof(tokens[9]);
	    particles.variance = atof(tokens[10]);
	    particles.particles_count = atoi(tokens[11]);

            count = 0;
            for (i = 12; i < token_count; i += 4)
            {
              particles.particles[count].pose.px = atof(tokens[i + 0]);
	      particles.particles[count].pose.py = atof(tokens[i + 1]);
	      particles.particles[count].pose.pa = atof(tokens[i + 2]);
	      particles.particles[count].alpha = atof(tokens[i + 3]);
              count += 1;
            }

            if (count != (int)particles.particles_count)
            {
              PLAYER_ERROR2("particles count mismatch at %s:%d",
                            this->filename, linenum);
              return -1;
            }
	    particles_set = true;

            return(0);
          }

        default:
          PLAYER_ERROR1("unknown localize reply subtype %d\n", subtype);
          return(-1);
      }
      break;

    default:
      PLAYER_ERROR1("unknown localize msg type %d\n", type);
      return(-1);
  }
}


////////////////////////////////////////////////////////////////////////////
// Parse sonar data
int ReadLog::ParseSonar(player_devaddr_t id,
                        unsigned short type, unsigned short subtype,
                        int linenum,
                        int token_count, char **tokens, double time)
{
  switch(type)
  {
    case PLAYER_MSGTYPE_DATA:
      switch(subtype)
      {
        case PLAYER_SONAR_DATA_RANGES:
          {
            player_sonar_data_t data;
            if(token_count < 8)
            {
              PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
              return -1;
            }
            data.ranges_count = atoi(tokens[7]);
            int count = 0;
            data.ranges = new float[data.ranges_count];
            for(int i=8;i<token_count;i++)
            {
              data.ranges[count++] = static_cast<float> (atof(tokens[i]));
            }
            if(count != (int)data.ranges_count)
            {
              PLAYER_ERROR2("range count mismatch at %s:%d",
                            this->filename, linenum);
              delete [] data.ranges;
              return -1;
            }
            this->Publish(id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                          (void*)&data, sizeof(data), &time);
            delete [] data.ranges;
            return(0);
          }
        case PLAYER_SONAR_DATA_GEOM:
          {
            player_sonar_geom_t geom;
            if(token_count < 8)
            {
              PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
              return -1;
            }
            geom.poses_count = atoi(tokens[7]);
            geom.poses = new player_pose3d_t[geom.poses_count];
            int count = 0;
            for(int i=8;i<token_count;i+=3)
            {
              geom.poses[count].px = atof(tokens[i]);
              geom.poses[count].py = atof(tokens[i+1]);
              geom.poses[count].pyaw = atof(tokens[i+2]);
              count++;
            }
            if(count != (int)geom.poses_count)
            {
              PLAYER_ERROR2("range count mismatch at %s:%d",
                            this->filename, linenum);
              delete [] geom.poses;
              return -1;
            }
            this->Publish(id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                          (void*)&geom, sizeof(geom), &time);
            delete [] geom.poses;
            return(0);
          }
        default:
          PLAYER_ERROR1("unknown sonar data subtype %d\n", subtype);
          return(-1);
      }
    case PLAYER_MSGTYPE_RESP_ACK:
      switch(subtype)
      {
        case PLAYER_SONAR_REQ_GET_GEOM:
          {
            if(token_count < 8)
            {
              PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
              return -1;
            }

            // cache it
            player_sonar_geom_t* geom =
                    (player_sonar_geom_t*)calloc(1,sizeof(player_sonar_geom_t));
            assert(geom);

            geom->poses_count = atoi(tokens[7]);
            geom->poses = (player_pose3d_t*)calloc(geom->poses_count, sizeof(player_pose3d_t));

            int count = 0;
            for(int i=8;i<token_count;i+=3)
            {
              geom->poses[count].px = atof(tokens[i]);
              geom->poses[count].py = atof(tokens[i+1]);
              geom->poses[count].pyaw = atof(tokens[i+2]);
              count++;
            }
            if(count != (int)geom->poses_count)
            {
              PLAYER_ERROR2("range count mismatch at %s:%d",
                            this->filename, linenum);
              free(geom->poses);
              free(geom);
              return -1;
            }

            // Find the right place to put it
            int j;
            for(j=0;j<this->provide_count;j++)
            {
              if(Device::MatchDeviceAddress(this->provide_ids[j], id))
                break;
            }
            assert(j<this->provide_count);

            if(this->provide_metadata[j])
              free(this->provide_metadata[j]);

            this->provide_metadata[j] = (void*)geom;

            // nothing to publish
            return(0);
          }
        default:
          PLAYER_ERROR1("unknown sonar reply subtype %d\n", subtype);
          return(-1);
      }
    default:
      PLAYER_ERROR1("unknown sonar message type %d\n", type);
      return(-1);
  }
}


////////////////////////////////////////////////////////////////////////////
// Parse position data
int
ReadLog::ParsePosition(player_devaddr_t id,
                       unsigned short type, unsigned short subtype,
                       int linenum,
                       int token_count, char **tokens, double time)
{
  switch(type)
  {
    case PLAYER_MSGTYPE_DATA:
      switch(subtype)
      {
        case PLAYER_POSITION2D_DATA_STATE:
          {
            player_position2d_data_t data;
            if(token_count < 14)
            {
              PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
              return -1;
            }
            data.pos.px = atof(tokens[7]);
            data.pos.py = atof(tokens[8]);
            data.pos.pa = atof(tokens[9]);
            data.vel.px = atof(tokens[10]);
            data.vel.py = atof(tokens[11]);
            data.vel.pa = atof(tokens[12]);
            data.stall = atoi(tokens[13]);

            this->Publish(id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                          (void*)&data, sizeof(data), &time);
            return(0);
          }
        default:
          PLAYER_ERROR1("unknown position data subtype %d\n", subtype);
          return(-1);
      }
    case PLAYER_MSGTYPE_RESP_ACK:
      switch(subtype)
      {
        case PLAYER_POSITION2D_REQ_GET_GEOM:
          {
            if(token_count < 12)
            {
              PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
              return -1;
            }

            // cache it
            player_position2d_geom_t* geom =
                    (player_position2d_geom_t*)calloc(1,sizeof(player_position2d_geom_t));
            assert(geom);

            geom->pose.px = atof(tokens[7]);
            geom->pose.py = atof(tokens[8]);
            geom->pose.pyaw = atof(tokens[9]);
            geom->size.sl = atof(tokens[10]);
            geom->size.sw = atof(tokens[11]);

            // Find the right place to put it
            int j;
            for(j=0;j<this->provide_count;j++)
            {
              if(Device::MatchDeviceAddress(this->provide_ids[j], id))
                break;
            }
            assert(j<this->provide_count);

            if(this->provide_metadata[j])
              free(this->provide_metadata[j]);

            this->provide_metadata[j] = (void*)geom;

            // nothing to publish
            return(0);
          }
        default:
          PLAYER_ERROR1("unknown position reply subtype %d\n", subtype);
          return(-1);
      }
    default:
      PLAYER_ERROR1("unknown position message type %d\n", type);
      return(-1);
  }
}

////////////////////////////////////////////////////////////////////////////
// Parse opaque data
int ReadLog::ParseOpaque(player_devaddr_t id,
                        unsigned short type, unsigned short subtype,
                        int linenum,
                        int token_count, char **tokens, double time)
{
  int i, count;

  switch(type)
  {
    case PLAYER_MSGTYPE_DATA:
      switch(subtype)
      {
        case PLAYER_OPAQUE_DATA_STATE:
          {
            player_opaque_data_t data;

            if (token_count < 8)
            {
              PLAYER_ERROR2("incomplete line at %s:%d",
                            this->filename, linenum);
              return -1;
            }

            data.data_count = atoi(tokens[7]);
            data.data = new uint8_t[data.data_count];

            count = 0;
            for (i = 8; i < token_count; i++)
            {
              data.data[count] = atoi(tokens[i]);
              count++;
            }

            if (count != (int)data.data_count)
            {
              PLAYER_ERROR2("data count mismatch at %s:%d",
                            this->filename, linenum);
              delete [] data.data;
              return -1;
           }
            this->Publish(id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                          (void*)&data, sizeof(data), &time);
            delete [] data.data;
            return(0);
          }

        default:
          PLAYER_ERROR1("unknown opaque data subtype %d\n", subtype);
          return(-1);
      }
      break;

    case PLAYER_MSGTYPE_CMD:
      switch(subtype)
      {
        case PLAYER_OPAQUE_CMD:
          {
            player_opaque_data_t data;

            if (token_count < 8)
            {
              PLAYER_ERROR2("incomplete line at %s:%d",
                            this->filename, linenum);
              return -1;
            }

            data.data_count = atoi(tokens[7]);
            data.data = new uint8_t[data.data_count];
            count = 0;
            for (i = 8; i < token_count; i++)
            {
              data.data[count] = atoi(tokens[i]);
              count++;
            }

            if (count != (int)data.data_count)
            {
              PLAYER_ERROR2("data count mismatch at %s:%d",
                            this->filename, linenum);
              delete [] data.data;
              return -1;
           }
            this->Publish(id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                          (void*)&data, sizeof(data), &time);
            delete [] data.data;
            return(0);
          }

        default:
          PLAYER_ERROR1("unknown opaque data subtype %d\n", subtype);
          return(-1);
      }
      break;

    default:
      PLAYER_ERROR1("unknown opaque msg type %d\n", type);
      return(-1);
  }
}

////////////////////////////////////////////////////////////////////////////
// Parse wifi data
int ReadLog::ParseWifi(player_devaddr_t id,
                       unsigned short type, unsigned short subtype,
                       int linenum,
                       int token_count, char **tokens, double time)
{
  player_wifi_data_t data;
  player_wifi_link_t *link;
  int i;
  unsigned int reported_count;

  switch(type)
  {
    case PLAYER_MSGTYPE_DATA:
      switch(subtype)
      {
        case PLAYER_WIFI_DATA_STATE:
          {
            if (token_count < 8)
            {
              PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
              return -1;
            }

            reported_count = atoi(tokens[7]);
            data.links_count = 0;
            data.links = new player_wifi_link_t[data.links_count];
            for(i = 8; (i+8) < token_count; i += 9)
            {
              link = data.links + data.links_count;

              memcpy(link->mac, tokens[i + 0]+1, strlen(tokens[i+0])-2);
              link->mac_count = strlen(tokens[i+0])-2;
              memcpy(link->ip, tokens[i + 1]+1, strlen(tokens[i+1])-2);
              link->ip_count = strlen(tokens[i+1])-2;
              memcpy(link->essid, tokens[i + 2]+1, strlen(tokens[i+2])-2);
              link->essid_count = strlen(tokens[i+2])-2;
              link->mode = atoi(tokens[i + 3]);
              link->freq = atoi(tokens[i + 4]);
              link->encrypt = atoi(tokens[i + 5]);
              link->qual = atoi(tokens[i + 6]);
              link->level = atoi(tokens[i + 7]);
              link->noise = atoi(tokens[i + 8]);

              data.links_count++;
            }
            if(data.links_count != reported_count)
              PLAYER_WARN("read fewer wifi link entries than expected");

            this->Publish(id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                          (void*)&data, sizeof(data), &time);
            delete [] data.links;
            return(0);
          }
        default:
          PLAYER_ERROR1("unknown wifi data subtype %d\n", subtype);
          return(-1);
      }
    default:
      PLAYER_ERROR1("unknown wifi message type %d\n", type);
      return(-1);
  }
}

////////////////////////////////////////////////////////////////////////////
// Parse WSN data
int ReadLog::ParseWSN(player_devaddr_t id,
                      unsigned short type, unsigned short subtype,
                      int linenum,
                      int token_count, char **tokens, double time)
{
    player_wsn_data_t data;

    switch(type)
    {
        case PLAYER_MSGTYPE_DATA:
            switch(subtype)
            {
                case PLAYER_WSN_DATA_STATE:
                {
                    if(token_count < 20)
                    {
                        PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
                        return -1;
                    }
                    data.node_type      = atoi(tokens[7]);
                    data.node_id        = atoi(tokens[8]);
                    data.node_parent_id = atoi(tokens[9]);

                    data.data_packet.light       = static_cast<float> (atof(tokens[10]));
                    data.data_packet.mic         = static_cast<float> (atof(tokens[11]));
                    data.data_packet.accel_x     = static_cast<float> (atof(tokens[12]));
                    data.data_packet.accel_y     = static_cast<float> (atof(tokens[13]));
                    data.data_packet.accel_z     = static_cast<float> (atof(tokens[14]));
                    data.data_packet.magn_x      = static_cast<float> (atof(tokens[15]));
                    data.data_packet.magn_y      = static_cast<float> (atof(tokens[16]));
                    data.data_packet.magn_z      = static_cast<float> (atof(tokens[17]));
                    data.data_packet.temperature = static_cast<float> (atof(tokens[18]));
                    data.data_packet.battery     = static_cast<float> (atof(tokens[19]));

                    this->Publish(id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                                  (void*)&data, sizeof(data), &time);
                    return(0);
                }
                default:
                    PLAYER_ERROR1("unknown WSN data subtype %d\n", subtype);
                    return(-1);
            }
        default:
            PLAYER_ERROR1("unknown WSN message type %d\n", type);
            return(-1);
    }
}

////////////////////////////////////////////////////////////////////////////
// Parse WSN data
int ReadLog::ParseCoopObject(player_devaddr_t id,
                      unsigned short type, unsigned short subtype,
                      int linenum,
                      int token_count, char **tokens, double time)
{
	unsigned int i;

    switch(type)
    {
        case PLAYER_MSGTYPE_DATA:
            switch(subtype)
            {
                case PLAYER_COOPOBJECT_DATA_HEALTH:
                {
					player_coopobject_header_t data;
                    if(token_count < 11)
                    {
                        PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
                        return -1;
                    }
                    data.id        = atoi(tokens[7]);
                    data.parent_id = atoi(tokens[8]);
                    data.origin      = atoi(tokens[9]);

                    this->Publish(id, type, subtype,
                                  (void*)&data, sizeof(data), &time);
                    return(0);
                }
		case PLAYER_COOPOBJECT_DATA_RSSI:
                {
					player_coopobject_rssi_t data;
                    if(token_count < 19)
                    {
                        PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
                        return -1;
                    }
                    data.header.id        = atoi(tokens[7]);
                    data.header.parent_id	= atoi(tokens[8]);
		    data.header.origin      = atoi(tokens[9]);

                    data.sender_id     = atoi(tokens[10]);
                    data.rssi     = atoi(tokens[11]);
                    data.stamp      = atoi(tokens[12]);
                    data.nodeTimeHigh      = atoi(tokens[13]);
                    data.nodeTimeLow      = atoi(tokens[14]);
                    data.x = atof(tokens[15]);
                    data.y     = atof(tokens[16]);
		    data.z      = atof(tokens[17]);

                    this->Publish(id, type, subtype,
                                  (void*)&data, sizeof(data), &time);
                    return(0);
                }
		case PLAYER_COOPOBJECT_DATA_SENSOR:
                {
					player_coopobject_data_sensor_t data;
                    if(token_count < 13)
                    {
                        PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
                        return -1;
                    }
                    data.header.id        = atoi(tokens[7]);
                    data.header.parent_id	= atoi(tokens[8]);
		    data.header.origin      = atoi(tokens[9]);
				
		    data.data_count = atoi(tokens[10]);
		    data.data = new player_coopobject_sensor_t[ data.data_count ];	
		    for (i = 0; i < data.data_count; i++) {
			data.data[i].type = atoi(tokens[11+2*i]);
			data.data[i].value = atoi(tokens[11+2*i]);
		    }
                    this->Publish(id, type, subtype,
                                  (void*)&data, sizeof(data), &time);
                    return(0);
                }
		case PLAYER_COOPOBJECT_DATA_ALARM:
                {
					player_coopobject_data_sensor_t data;
                    if(token_count < 13)
                    {
                        PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
                        return -1;
			}
                    data.header.id        = atoi(tokens[7]);
                    data.header.parent_id	= atoi(tokens[8]);
		    data.header.origin      = atoi(tokens[9]);

		    data.data_count = atoi(tokens[10]);
		    data.data = new player_coopobject_sensor_t[ data.data_count ];		
		    for (i = 0; i < data.data_count; i++) {
			data.data[i].type = atoi(tokens[11+2*i]);
			data.data[i].value = atoi(tokens[11+2*i]);
		    }
                    this->Publish(id, type, subtype,
                                  (void*)&data, sizeof(data), &time);
                    return(0);
                }
		case PLAYER_COOPOBJECT_DATA_USERDEFINED:
                {
					player_coopobject_data_userdefined_t data;
                    if(token_count < 12)
                    {
                        PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
                        return -1;
                    }
                    data.header.id        = atoi(tokens[7]);
                    data.header.parent_id	= atoi(tokens[8]);
		    data.header.origin      = atoi(tokens[9]);

		    data.type = atoi (tokens[10]);
		    data.data_count = atoi(tokens[11]);
		    data.data = new uint8_t[ data.data_count ];	
		    for (i = 0; i < data.data_count; i++)
			data.data[i] = atoi(tokens[12+i]);

                    this->Publish(id, type, subtype,
                                  (void*)&data, sizeof(data), &time);
                    return(0);
                }
		case PLAYER_COOPOBJECT_DATA_REQUEST:
                {
					player_coopobject_req_t data;
                    if(token_count < 13)
                    {
                        PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
                        return -1;
                    }
                        data.header.id        = atoi(tokens[7]);
                    data.header.parent_id	= atoi(tokens[8]);
		    data.header.origin      = atoi(tokens[9]);

		    data.request = atoi (tokens[10]);
		    data.parameters_count = atoi(tokens[11]);
		    data.parameters = new uint8_t[ data.parameters_count ];	
		    for (i = 0; i < data.parameters_count; i++)
		      data.parameters[i] = atoi(tokens[12+i]);

                    this->Publish(id, type, subtype,
                                  (void*)&data, sizeof(data), &time);
                    return(0);
                }
				case PLAYER_COOPOBJECT_DATA_COMMAND:
                {
					player_coopobject_cmd_t data;
                    if(token_count < 13)
                    {
                        PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
                        return -1;
                    }
                    data.header.id        = atoi(tokens[7]);
                    data.header.parent_id	= atoi(tokens[8]);
		    data.header.origin      = atoi(tokens[9]);

		    data.command = atoi (tokens[10]);
		    data.parameters_count = atoi(tokens[11]);
		    data.parameters = new uint8_t[ data.parameters_count ];	
		    for (i = 0; i < data.parameters_count; i++)
		      data.parameters[i] = atoi(tokens[12+i]);

                    this->Publish(id, type, subtype,
                                  (void*)&data, sizeof(data), &time);
                    return(0);
                }
                default:
                    PLAYER_ERROR1("unknown WSN data subtype %d\n", subtype);
                    return(-1);
            }

/*		 case PLAYER_MSGTYPE_CMD:
            switch(subtype)
            {
                case PLAYER_COOPOBJECT_CMD_DATA:
                {
					player_coopobject_cmd_data_t data;
                    if(token_count < 16)
                    {
                        PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
                        return -1;
                    }
                    data.node_id		= atoi(tokens[7]);
                    data.source_id		= atoi(tokens[8]);
                    data.pos.px			= atof(tokens[9]);
                    data.pos.py			= atof(tokens[10]);
                    data.pos.pa			= atof(tokens[11]);
					data.status			= atoi(tokens[12]);

					data.data_type = atoi (tokens[12]);
					data.data_count = atoi(tokens[13]);
					data.data = new uint8_t[ data.data_count ];
					for (i = 0; i < data.data_count; i++)
						data.data[i] = atoi(tokens[14+i]);

                    this->Publish(id, type, subtype, (void*)&data, sizeof(data), &time);
                    return(0);
                }

				default:
                    PLAYER_ERROR1("unknown WSN data subtype %d\n", subtype);
                    return(-1);

			}
*/
        default:
            PLAYER_ERROR1("unknown WSN message type %d\n", type);
            return(-1);
    }
}


////////////////////////////////////////////////////////////////////////////
// Parse IMU data
int ReadLog::ParseIMU (player_devaddr_t id,
                      unsigned short type, unsigned short subtype,
                      int linenum,
                      int token_count, char **tokens, double time)
{
    switch(type)
    {
        case PLAYER_MSGTYPE_DATA:
            switch(subtype)
            {
                case PLAYER_IMU_DATA_STATE:
                {
                    if (token_count < 13)
                    {
                        PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
                        return -1;
                    }
		    player_imu_data_state_t data;

		    data.pose.px = atof (tokens[7]);
		    data.pose.py = atof (tokens[8]);
		    data.pose.pz = atof (tokens[9]);
		    data.pose.proll  = atof (tokens[10]);
		    data.pose.ppitch = atof (tokens[11]);
		    data.pose.pyaw   = atof (tokens[12]);

                    this->Publish (id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                                  (void*)&data, sizeof(data), &time);
                    return (0);
                }

		case PLAYER_IMU_DATA_CALIB:
		{
                    if (token_count < 16)
                    {
                        PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
                        return -1;
                    }
		    player_imu_data_calib_t data;

		    data.accel_x = static_cast<float> (atof (tokens[7]));
		    data.accel_y = static_cast<float> (atof (tokens[8]));
		    data.accel_z = static_cast<float> (atof (tokens[9]));
		    data.gyro_x  = static_cast<float> (atof (tokens[10]));
		    data.gyro_y  = static_cast<float> (atof (tokens[11]));
		    data.gyro_z  = static_cast<float> (atof (tokens[12]));
		    data.magn_x  = static_cast<float> (atof (tokens[13]));
		    data.magn_y  = static_cast<float> (atof (tokens[14]));
		    data.magn_z  = static_cast<float> (atof (tokens[15]));

                    this->Publish (id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                                  (void*)&data, sizeof(data), &time);
                    return (0);
		}

		case PLAYER_IMU_DATA_QUAT:
		{
                    if (token_count < 20)
                    {
                        PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
                        return -1;
                    }
		    player_imu_data_quat_t data;

		    data.calib_data.accel_x = static_cast<float> (atof (tokens[7]));
		    data.calib_data.accel_y = static_cast<float> (atof (tokens[8]));
		    data.calib_data.accel_z = static_cast<float> (atof (tokens[9]));
		    data.calib_data.gyro_x  = static_cast<float> (atof (tokens[10]));
		    data.calib_data.gyro_y  = static_cast<float> (atof (tokens[11]));
		    data.calib_data.gyro_z  = static_cast<float> (atof (tokens[12]));
		    data.calib_data.magn_x  = static_cast<float> (atof (tokens[13]));
		    data.calib_data.magn_y  = static_cast<float> (atof (tokens[14]));
		    data.calib_data.magn_z  = static_cast<float> (atof (tokens[15]));
		    data.q0      = static_cast<float> (atof (tokens[16]));
		    data.q1      = static_cast<float> (atof (tokens[17]));
		    data.q2      = static_cast<float> (atof (tokens[18]));
		    data.q3      = static_cast<float> (atof (tokens[19]));

                    this->Publish (id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                                  (void*)&data, sizeof(data), &time);
                    return (0);
		}

		case PLAYER_IMU_DATA_EULER:
		{
                    if (token_count < 19)
                    {
                        PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
                        return -1;
                    }
		    player_imu_data_euler_t data;

		    data.calib_data.accel_x = static_cast<float> (atof (tokens[7]));
		    data.calib_data.accel_y = static_cast<float> (atof (tokens[8]));
		    data.calib_data.accel_z = static_cast<float> (atof (tokens[9]));
		    data.calib_data.gyro_x  = static_cast<float> (atof (tokens[10]));
		    data.calib_data.gyro_y  = static_cast<float> (atof (tokens[11]));
		    data.calib_data.gyro_z  = static_cast<float> (atof (tokens[12]));
		    data.calib_data.magn_x  = static_cast<float> (atof (tokens[13]));
		    data.calib_data.magn_y  = static_cast<float> (atof (tokens[14]));
		    data.calib_data.magn_z  = static_cast<float> (atof (tokens[15]));
		    data.orientation.proll  = static_cast<float> (atof (tokens[16]));
		    data.orientation.ppitch = static_cast<float> (atof (tokens[17]));
		    data.orientation.pyaw   = static_cast<float> (atof (tokens[18]));

                    this->Publish (id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                                  (void*)&data, sizeof(data), &time);
                    return (0);
		}
		
		case PLAYER_IMU_DATA_FULLSTATE:
		{
			if (token_count < 22)
			{
				PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
				return -1;
			}
			player_imu_data_fullstate_t data;
			
			data.pose.px = static_cast<float> (atof (tokens[7]));
			data.pose.py = static_cast<float> (atof (tokens[8]));
			data.pose.pz = static_cast<float> (atof (tokens[9]));
			data.pose.proll  = static_cast<float> (atof (tokens[10]));
			data.pose.ppitch  = static_cast<float> (atof (tokens[11]));
			data.pose.pyaw  = static_cast<float> (atof (tokens[12]));
			data.vel.px = static_cast<float> (atof (tokens[13]));
			data.vel.py = static_cast<float> (atof (tokens[14]));
			data.vel.pz = static_cast<float> (atof (tokens[15]));
			data.vel.proll  = static_cast<float> (atof (tokens[16]));
			data.vel.ppitch  = static_cast<float> (atof (tokens[17]));
			data.vel.pyaw  = static_cast<float> (atof (tokens[18]));
			data.acc.px = static_cast<float> (atof (tokens[19]));
			data.acc.py = static_cast<float> (atof (tokens[20]));
			data.acc.pz = static_cast<float> (atof (tokens[21]));
			data.acc.proll = 0;
			data.acc.ppitch = 0;
			data.acc.pyaw = 0;
			
			this->Publish (id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
							(void*)&data, sizeof(data), &time);
							return (0);
		}
                default:
                    PLAYER_ERROR1 ("unknown IMU data subtype %d\n", subtype);
                    return (-1);
            }
        default:
            PLAYER_ERROR1 ("unknown IMU message type %d\n", type);
            return (-1);
    }
}

////////////////////////////////////////////////////////////////////////////
// Parse PointCloud3d data
int ReadLog::ParsePointCloud3d (player_devaddr_t id,
                                unsigned short type, unsigned short subtype,
                                int linenum,
                                int token_count, char **tokens, double time)
{
    unsigned int i;
    switch(type)
    {
        case PLAYER_MSGTYPE_DATA:
            switch(subtype)
            {
                case PLAYER_POINTCLOUD3D_DATA_STATE:
                {
		    player_pointcloud3d_data_t data;
		    data.points_count = atoi (tokens[7]);
                    if (token_count < (int)(7+data.points_count))
                    {
                        PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
                        return -1;
                    }
                    data.points = new player_pointcloud3d_element[data.points_count];
		    for (i = 0; i < data.points_count; i++)
		    {
			player_pointcloud3d_element_t element;
                        memset(&element,0,sizeof(player_pointcloud3d_element_t));
			player_point_3d_t point;
			point.px = atof (tokens[8+i*3]);
			point.py = atof (tokens[9+i*3]);
			point.pz = atof (tokens[10+i*3]);
			element.point = point;
			data.points[i] = element;
		    }
                    this->Publish (id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                                  (void*)&data, sizeof(data), &time);
                    delete [] data.points;
                    return (0);
                }

                default:
                    PLAYER_ERROR1 ("unknown PointCloud3d data subtype %d\n", subtype);
                    return (-1);
            }
        default:
            PLAYER_ERROR1 ("unknown PointCloud3d message type %d\n", type);
            return (-1);
    }
}

////////////////////////////////////////////////////////////////////////////
// Parse PTZ data
int ReadLog::ParsePTZ (player_devaddr_t id,
                      unsigned short type, unsigned short subtype,
                      int linenum,
                      int token_count, char **tokens, double time)
{
    switch(type)
    {
        case PLAYER_MSGTYPE_DATA:
            switch(subtype)
            {
                case PLAYER_PTZ_DATA_STATE:
                {
                    if (token_count < 12)
                    {
                        PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
                        return -1;
                    }
		    player_ptz_data_t data;

		    data.pan  = static_cast<float> (atof (tokens[7]));
		    data.tilt = static_cast<float> (atof (tokens[8]));
		    data.zoom = static_cast<float> (atof (tokens[9]));
		    data.panspeed  = static_cast<float> (atof (tokens[10]));
		    data.tiltspeed = static_cast<float> (atof (tokens[11]));
                    this->Publish (id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype),
                                  (void*)&data, sizeof(data), &time);
                    return (0);
                }

                default:
                    PLAYER_ERROR1 ("unknown PTZ data subtype %d\n", subtype);
                    return (-1);
            }
        default:
            PLAYER_ERROR1 ("unknown PTZ message type %d\n", type);
            return (-1);
    }
}

////////////////////////////////////////////////////////////////////////////
// Parse Actarray data
int ReadLog::ParseActarray (player_devaddr_t id,
                    	    unsigned short type, unsigned short subtype,
                            int linenum,
                            int token_count, char **tokens, double time)
{
    unsigned int i;
    switch(type)
    {
      case PLAYER_MSGTYPE_DATA:
        switch(subtype)
        {
          case PLAYER_ACTARRAY_DATA_STATE:
            player_actarray_data_t data;
            data.actuators_count = atoi (tokens[7]);
            data.actuators = new player_actarray_actuator[data.actuators_count];
            if (token_count < (int)(7+data.actuators_count))
            {
                PLAYER_ERROR2("invalid line at %s:%d", this->filename, linenum);
                return -1;
            }
            for (i = 0; i < data.actuators_count; i++)
            {
              player_actarray_actuator actuator;
              actuator.position     = static_cast<float> (atof (tokens[5*i+8]));
              actuator.speed        = static_cast<float> (atof (tokens[5*i+9]));
              actuator.acceleration = static_cast<float> (atof (tokens[5*i+10]));
              actuator.current      = static_cast<float> (atof (tokens[5*i+11]));
              actuator.state        = static_cast<uint8_t> (atoi (tokens[5*i+12]));
              data.actuators[i] = actuator;
            }
            data.motor_state = atoi (tokens[data.actuators_count*5 + 8]);

            this->Publish (id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype), (void*)&data, sizeof(data), &time);
            delete[] data.actuators;
            return (0);
          default:
            PLAYER_ERROR1 ("unknown Actarray data subtype %d\n", subtype);
            return (-1);
        }
        break;
      default:
        PLAYER_ERROR1 ("unknown Actarray message type %d\n", type);
        return (-1);
    }
}

////////////////////////////////////////////////////////////////////////////
// Parse AIO data
int ReadLog::ParseAIO(player_devaddr_t id, unsigned short type,
                      unsigned short subtype, int linenum, int token_count,
                      char **tokens, double time)
{
  switch (type) {
    case PLAYER_MSGTYPE_DATA:
      switch (subtype) {
        case PLAYER_AIO_DATA_STATE: {
            player_aio_data_t inputs;

            if (token_count < 8) {
              PLAYER_ERROR2("invalid line at %s:%d: count missing", filename,
                            linenum);
              return -1;
            }

            inputs.voltages_count = atoi(tokens[7]);

            if (token_count - 8 != (int)inputs.voltages_count) {
              PLAYER_ERROR2("invalid line at %s:%d: number of tokens does not "
                            "match count", filename, linenum);
              return -1;
            }

            char **t = tokens + 8;
            inputs.voltages = new float[inputs.voltages_count];
            for (float *v(inputs.voltages);
                 v != inputs.voltages + inputs.voltages_count; ++v, ++t)
              *v = static_cast<float> (atof(*t));

            Publish(id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype), (void *)&inputs, sizeof(inputs),
                    &time);
            delete [] inputs.voltages;
            return 0;
          }
        default:
          PLAYER_WARN3("cannot parse log of unknown aio data subtype '%d' at "
                       "%s:%d", subtype, filename, linenum);
          return -1;
      }
      default:
        PLAYER_WARN3("cannot parse log unknown of aio message type '%d' at "
                     "%s:%d", type, filename, linenum);
        return -1;
  }
}

////////////////////////////////////////////////////////////////////////////
// Parse DIO data
int ReadLog::ParseDIO(player_devaddr_t id, unsigned short type,
                      unsigned short subtype, int linenum, int token_count,
                      char **tokens, double time)
{
  switch (type) {
    case PLAYER_MSGTYPE_DATA:
      switch (subtype) {
        case PLAYER_DIO_DATA_VALUES: {
            player_dio_data_t inputs;

            if (token_count < 8) {
              PLAYER_ERROR2("invalid line at %s:%d: count missing", filename,
                            linenum);
              return -1;
            }

            inputs.count = atoi(tokens[7]);
            inputs.bits = 0;

            if (token_count - 8 != static_cast<int>(inputs.count)) {
              PLAYER_ERROR2("invalid line at %s:%d: number of tokens does not "
                            "match count", filename, linenum);
              return -1;
            }

            if (inputs.count > 32 /* MAX_INPUTS */) {
              PLAYER_ERROR2("invalid line at %s:%d: too much data for buffer",
                            filename, linenum);
              return -1;
            }

            char **t = tokens + 8;
            for (uint32_t mask(1); mask != (1ul << inputs.count);
                 mask <<=1, ++t) {
              if (strcmp(*t, "1") == 0) inputs.bits |= mask;
            }

            Publish(id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype), (void *)&inputs, sizeof(inputs),
                    &time);
            return 0;
          }
        default:
          PLAYER_WARN3("cannot parse log of unknown dio data subtype '%d' at "
                       "%s:%d", subtype, filename, linenum);
          return -1;
      }
      default:
        PLAYER_WARN3("cannot parse log of unknown dio message type '%d' at "
                     "%s:%d", type, filename, linenum);
        return -1;
  }
}

////////////////////////////////////////////////////////////////////////////
// Parse RFID data
/*
  The format changed so the rfid "type" will be saved.
  To convert the old log files to the new format use this awk filter:

  awk '/rfid/ {
           split($0, a);
           for (i=1; i<=length(a); i++)
               printf(i < 9 ? "%s " : "0001 %s ", a[i]);
           printf("\n")
       }
       !/rfid/ {
           print $0
       }'
*/
int ReadLog::ParseRFID(player_devaddr_t id, unsigned short type,
                      unsigned short subtype, int linenum, int token_count,
                      char **tokens, double time)
{
  switch (type) {
    case PLAYER_MSGTYPE_DATA:
      switch (subtype) {
        case PLAYER_RFID_DATA_TAGS: {
            player_rfid_data_t rdata;

            if (token_count < 8) {
              PLAYER_ERROR2("invalid line at %s:%d: count missing",
                            this->filename, linenum);
              return -1;
            }

            rdata.tags_count = strtoul(tokens[7], NULL, 10);

            if (token_count - 8 != 2 * static_cast<int>(rdata.tags_count)) {
              PLAYER_ERROR2("invalid line at %s:%d: number of tokens does not "
                            "match count", this->filename, linenum);
              return -1;
            }


            char **t = tokens + 8;
            rdata.tags = new player_rfid_tag_t[ rdata.tags_count];
            for (player_rfid_tag_t *r(rdata.tags);
                 r != rdata.tags + rdata.tags_count; ++r, ++t) {
              r->type = strtoul(*t, NULL, 10);
              ++t;
              r->guid_count = strlen(*t) / 2;
              r->guid = new char [r->guid_count];
              DecodeHex(r->guid, r->guid_count, *t, strlen(*t));
            }

            Publish(id, static_cast<uint8_t> (type), static_cast<uint8_t> (subtype), (void *)&rdata, sizeof(rdata),
                    &time);
            player_cleanup_fn_t cleanupfunc;
            if (!(cleanupfunc = playerxdr_get_cleanupfunc (PLAYER_RFID_CODE, PLAYER_MSGTYPE_DATA, PLAYER_RFID_DATA_TAGS)))
            {
              PLAYER_ERROR ("Couldn't fund clean up function to clean up RFID data");
              return -1;
            }
            (*cleanupfunc) (&rdata);
            return 0;
          }
        default:
          PLAYER_WARN3("cannot parse log of unknown rfid data subtype '%d' at "
                       "%s:%d", subtype, filename, linenum);
          return -1;
      }
      default:
        PLAYER_WARN3("cannot parse log of unknown rfid message type '%d' at "
                     "%s:%d", type, filename, linenum);
        return -1;
  }
}

////////////////////////////////////////////////////////////////////////////
// Parse position3d data
int ReadLog::ParsePosition3d(player_devaddr_t id, unsigned short type,
                      unsigned short subtype, int linenum, int token_count,
                      char **tokens, double time)
{
  switch (type){
    case PLAYER_MSGTYPE_DATA:
    
      switch (subtype) {
        case PLAYER_POSITION3D_DATA_STATE:
        
          player_position3d_data_t data;
		  
	  if (token_count < 20)
	  {
	    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
	    return -1;
	  }

	  data.pos.px = atof(tokens[7]);
	  data.pos.py = atof(tokens[8]);
	  data.pos.pz = atof(tokens[9]);

	  data.pos.proll = atof(tokens[10]);
	  data.pos.ppitch = atof(tokens[11]);
	  data.pos.pyaw = atof(tokens[12]);

	  data.vel.px = atof(tokens[13]);
	  data.vel.py = atof(tokens[14]);
	  data.vel.pz = atof(tokens[15]);

	  data.vel.proll = atof(tokens[16]);
	  data.vel.ppitch = atof(tokens[17]);
	  data.vel.pyaw = atof(tokens[18]);

	  data.stall = atoi(tokens[19]);

	  this->Publish(id,type,subtype,(void*) &data, sizeof(data), &time);

	  return 0;
	  
	case PLAYER_POSITION3D_DATA_GEOMETRY:
	  player_position3d_geom_t geom;
	  if (token_count < 16)
	  {
	    PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
	    return -1;
	  }
	  geom.pose.px = atof(tokens[7]);
	  geom.pose.py = atof(tokens[8]);
	  geom.pose.pz = atof(tokens[9]);
	  
	  geom.pose.proll = atof(tokens[10]);
	  geom.pose.ppitch = atof(tokens[11]);
	  geom.pose.pyaw = atof(tokens[12]);
	  
	  geom.size.sw = atof(tokens[13]);
	  geom.size.sl = atof(tokens[14]);
	  geom.size.sh = atof(tokens[15]);
	  
	  this->Publish(id,type,subtype,(void*) &geom, sizeof(geom), &time);
	  
	  return 0;
	default:
	  return (-1);
	}
      case PLAYER_MSGTYPE_RESP_ACK:
        player_position3d_geom_t geom;
        if (token_count < 16)
	{
	  PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
	  return -1;
	}
	geom.pose.px = atof(tokens[7]);
	  geom.pose.py = atof(tokens[8]);
	  geom.pose.pz = atof(tokens[9]);
	  
	  geom.pose.proll = atof(tokens[10]);
	  geom.pose.ppitch = atof(tokens[11]);
	  geom.pose.pyaw = atof(tokens[12]);
	  
	  geom.size.sw = atof(tokens[13]);
	  geom.size.sl = atof(tokens[14]);
	  geom.size.sh = atof(tokens[15]);
	
	this->Publish(id,type,subtype,(void*) &geom, sizeof(geom), &time);
	return 0;
      default:
        return (-1);
      }
}
////////////////////////////////////////////////////////////////////////////
// Parse power data
int ReadLog::ParsePower(player_devaddr_t id, unsigned short type,
                      unsigned short subtype, int linenum, int token_count,
                      char **tokens, double time)
{
  switch (type){
    case PLAYER_MSGTYPE_DATA:
      
      player_power_data_t data;
      if (token_count < 13)
      {
	PLAYER_ERROR2("incomplete line at %s:%d", this->filename, linenum);
	return -1;
      }
      data.volts = atof(tokens[7]);
      data.percent = atof(tokens[8]);
      data.joules = atof(tokens[9]);
      data.watts = atof(tokens[10]);
      data.charging = atoi(tokens[11]);
      data.valid = atoi(tokens[12]);
	
      this->Publish(id,type,subtype,(void*) &data, sizeof(data), &time);
      return 0;
    default:
      return (-1);
    }
}
