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
 * $Id$
 *
 *   c++ Player client utils
 */

#ifndef ROBOT
#define ROBOT

//#include <offsets.h>

#define MAX_REQUEST_SIZE 128
#define MAX_REPLY_SIZE 1024

// the following are copied from include/offsets.h.
/* SICK laser stuff */
#define LASER_NUM_SAMPLES 361
#define LASER_DATA_BUFFER_SIZE (int)(LASER_NUM_SAMPLES*sizeof(unsigned short))
/* ACTS size stuff */
#define ACTS_NUM_CHANNELS 32
#define ACTS_HEADER_SIZE 2*ACTS_NUM_CHANNELS  
#define ACTS_BLOB_SIZE 10
#define ACTS_MAX_BLOBS_PER_CHANNEL 10
#define ACTS_MAX_BLOB_DATA_SIZE \
  ACTS_NUM_CHANNELS*ACTS_BLOB_SIZE*ACTS_MAX_BLOBS_PER_CHANNEL
#define ACTS_TOTAL_MAX_SIZE \
  ACTS_MAX_BLOB_DATA_SIZE+ACTS_HEADER_SIZE
/*
 * P2OS device stuff
 *
 *   this device's 'data' buffer is shared among many devices.  here
 *   is the layout (in this order):
 *     'position' data:
 *       3 ints: time X Y
 *       4 shorts: heading, forwardvel, turnrate, compass
 *       1 chars: stalls
 *     'sonar' data:
 *       16 shorts: 16 sonars
 *     'gripper' data:
 *       2 chars: gripstate,gripbeams
 *     'misc' data:
 *       2 chars: frontbumper,rearbumpers
 *       1 char:  voltage
 */
#define POSITION_DATA_BUFFER_SIZE 3*sizeof(int)+ \
                                  4*sizeof(unsigned short)+ \
                                  1*sizeof(char)
#define SONAR_DATA_BUFFER_SIZE 16*sizeof(unsigned short)
#define GRIPPER_DATA_BUFFER_SIZE 2*sizeof(char)
#define MISC_DATA_BUFFER_SIZE 3*sizeof(char)
#define PTZ_DATA_BUFFER_SIZE 3*sizeof(short)
#define P2OS_DATA_BUFFER_SIZE POSITION_DATA_BUFFER_SIZE + \
                              SONAR_DATA_BUFFER_SIZE + \
                              GRIPPER_DATA_BUFFER_SIZE + \
                              MISC_DATA_BUFFER_SIZE

/* gripper stuff */
#define GRIPopen   1
#define GRIPclose  2
#define GRIPstop   3
#define LIFTup     4
#define LIFTdown   5
#define LIFTstop   6
#define GRIPstore  7
#define GRIPdeploy 8
#define GRIPhalt   15
#define GRIPpress  16
#define LIFTcarry  17

struct CGripper {
  /* attributes */
  unsigned char	paddlesOpen;
  unsigned char paddlesClose;
  unsigned char paddlesMoving;
  unsigned char paddlesError;

  unsigned char leftPaddleOpen;
  unsigned char rightPaddleOpen;

  unsigned char gripLimitReached;
 
  unsigned char liftUp;
  unsigned char liftDown;
  unsigned char liftMoving;
  unsigned char liftError;

  unsigned char liftLimitReached;

  unsigned char outerBeamObstructed;
  unsigned char innerBeamObstructed;
};

struct CMisc {
  /* attributes */
  unsigned char frontbumpers;
  unsigned char rearbumpers;
  unsigned char voltage;
};

struct CPtz {
  /* attributes */
  short pan;
  short tilt;
  short zoom;
};

struct CPosition {
  /* attributes */
  unsigned int time;
  int xpos, ypos;

  unsigned short heading;
  short compass;
  short speed, turnrate;
  unsigned char stalls;
};

struct CBlob {
  unsigned int area;
  unsigned char x;
  unsigned char y;
  unsigned char left;
  unsigned char right;
  unsigned char top;
  unsigned char bottom;
};

struct CVision {
  /* attributes */
  char NumBlobs[ACTS_NUM_CHANNELS];
  CBlob* Blobs[ACTS_NUM_CHANNELS];
};

typedef enum
{
  REQUESTREPLY,
  CONTINUOUS
} data_mode_t;

typedef enum
{
  DIRECTWHEELVELOCITY,
  SEPARATETRANSROT
} velocity_mode_t;

typedef enum
{
  NONE,
  READ,
  WRITE,
  ALL
} access_t;


class CRobot {
  int sock;

  access_t laseraccess;
  access_t positionaccess;
  access_t sonaraccess;
  access_t visionaccess;
  access_t miscaccess;
  access_t ptzaccess;
  access_t gripperaccess;

  int ExactRead( unsigned char buf[], unsigned short size );
  int CountReads();
  void UpdateAccess(char* reply, unsigned short reply_size);
  
  void FillMiscData(unsigned char* buf, int size);
  void FillGripperData(unsigned char* buf, int size);
  void FillPositionData(unsigned char* buf, int size);
  void FillSonarData(unsigned char* buf, int size);
  void FillLaserData(unsigned char* buf, int size);
  void FillVisionData(unsigned char* buf, int size);
  void FillPtzData(unsigned char* buf, int size);
 
 public:
  /* our robot */
  char host[256];
  int port;

  /* from robot */
  unsigned short *sonar;
  unsigned short *laser;
  CPosition *position;
  CVision *vision;
  CGripper *gripper;
  CMisc *misc;
  CPtz *ptz;

  /* to robot */
  short newspeed;
  short newturnrate;
  short newpan;
  short newtilt;
  short newzoom;
  short newgrip;

  /* processed data */
  short minfrontsonar;
  short minbacksonar;
  unsigned short minlaser;
  int minlaser_index;

  ~CRobot();
  CRobot();

  /* User methods */
  void GripperCommand( unsigned char command );
  void GripperCommand( unsigned char command, unsigned char value );

  /* 
   * Connect to the server running on host 'desthost' at port 'port'
   *
   * returns 0 on success, non-zero otherwise
   */
  int Connect(const char* desthost, int port);

  /* 
   * Connect to the server running on host 'desthost' at default port
   *
   * returns 0 on success, non-zero otherwise
   */
  int Connect(const char* desthost);

  /* 
   * Connect to the server running on host 'this.host' at port 'this.port'
   *
   * returns 0 on success, non-zero otherwise
   */
  int Connect();

  /*
   * Request device access.
   *   'request' is of the form "lrsrpa" and MUST be NULL-terminated
   *
   * Returns 0 on success, non-zero otherwise
   */
  int Request( const char* request);

  /*
   * Request device access.
   *   'request' is of the form "lrsrpa"
   *   'size' is the length of 'request'
   *
   * Returns 0 on success, non-zero otherwise
   */
  int Request( const char* request, int size ); 

  /*
   * Write commands to all devices currently opened for writing.
   *
   * Returns 0 on success, non-zero otherwise
   */
  int Write();

  /*
   * Read data from all devices currently openend for reading
   *
   * Returns 0 on success; non-zero otherwise
   */
  int Read();

  /*
   * Print data from all devices currently opened for reading
   */
  void Print();

  /*
   * change velocity control mode
   *   'mode' should be either DIRECTWHEELVELOCITY or SEPARATETRANSROT
   *
   * Returns 0 on success; non-zero otherwise
   */
  int ChangeVelocityControl(velocity_mode_t mode);

  /*
   * Enable/disable the motors
   *   if 'state' is non-zero, then enable motors
   *   else disable motors
   *
   * Returns 0 on success; non-zero otherwise
   */
  int ChangeMotorState(unsigned char state);

  /*
   * Set continous data frequency
   *
   * Returns 0 on success; non-zero otherwise
   */
  int SetFrequency(unsigned short freq);

  /*
   * Set data delivery mode to 'mode'
   *   'mode' should be either REQUESTREPLY or CONTINUOUS
   *
   * Returns 0 on success; non-zero otherwise
   */
  int SetDataMode(data_mode_t mode);

  /*
   * Reset the robot's odometry to 0,0,0
   *
   * Returns 0 on success; non-zero otherwise
   */
  int ResetPosition();

  /*
   * When in REQUESTREPLY mode, request a round of sensor data
   *
   * Returns 0 on success; non-zero otherwise
   */
  int RequestData();
};


#endif
