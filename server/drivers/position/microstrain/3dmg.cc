/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *                      
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
 * Desc: Driver for the MicroStrain 3DM-G IMU
 * Author: Andrew Howard
 * Date: 19 Nov 2002
 * CVS: $Id$
 */

#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>

#include "playercommon.h"
#include "drivertable.h"
#include "player.h"

#include <playertime.h>
extern PlayerTime* GlobalTime;


// MicroStraing 3DM-G IMU driver
class MicroStrain3DMG : public Driver
{
  ///////////////////////////////////////////////////////////////////////////
  // Top half methods; these methods run in the server thread
  ///////////////////////////////////////////////////////////////////////////
  
  // Constructor
  public: MicroStrain3DMG(int code, ConfigFile* cf, int section);

  // Destructor
  public: virtual ~MicroStrain3DMG();

  // Initialise device
  public: virtual int Setup();

  // Shutdown the device
  public: virtual int Shutdown();

  // Open the serial port
  // Returns 0 on success
  private: int OpenPort();

  // Close the serial port
  // Returns 0 on success
  private: int ClosePort();

  /* FIX
  // Get number of pending data packets
  private: virtual size_t GetNumData(void* client);

  // Get pending data packets
  private: virtual size_t GetData(void* client, unsigned char* dest, size_t len,
                                  uint32_t* timestamp_sec, uint32_t* timestamp_usec);
  */
                                  
  ///////////////////////////////////////////////////////////////////////////
  // Middle methods: these methods facilitate communication between the top
  // and bottom halfs.
  ///////////////////////////////////////////////////////////////////////////

  // Push data to the queue
  private: void Push(player_position3d_data_t *data,
                     uint32_t time_sec, uint32_t time_usec);

  // Pop data from the queue; returns the number of items popped
  private: int Pop(player_position3d_data_t *data,
                   uint32_t *time_sec, uint32_t *time_usec);
  
  ///////////////////////////////////////////////////////////////////////////
  // Bottom half methods; these methods run in the device thread
  ///////////////////////////////////////////////////////////////////////////

  // Main function for device thread.
  private: virtual void Main();
  
  // Read the firmware version
  private: int GetFirmware(char *firmware, int len);

  // Read the stabilized acceleration vectors
  private: int GetStabV(double *time, double v[3], double w[3]);

  // Read the stabilized orientation matrix
  private: int GetStabM(int M[3][3]);

  // Read the stabilized orientation quaternion
  private: int GetStabQ(double *time, double q[4]);

  // Read the stabilized Euler angles
  private: int GetStabEuler(double *time, double e[3]);
  
  // Send a packet and wait for a reply from the IMU.
  // Returns the number of bytes read.
  private: int Transact(void *cmd, int cmd_len, void *rep, int rep_len);
    
  // Interface to use
  private: int code;

  // Name of port used to communicate with the laser;
  // e.g. /dev/ttyS1
  private: const char *port_name;

  // Port file descriptor
  private: int fd;

  // Queue of pending data
  private: int q_count, q_max_count;
  private: struct QElem
  {
    player_position3d_data_t data;
    uint32_t time_sec, time_usec;
  } *q_elems;
};


// Factory creation function
Driver* MicroStrain3DMG_Init( ConfigFile* cf, int section)
{
  if (strcmp(interface, PLAYER_POSITION3D_STRING) == 0)
    return ((Driver*) (new MicroStrain3DMG(PLAYER_POSITION3D_CODE, cf, section)));

  PLAYER_ERROR1("driver \"MicroStrain3DMG\" does not support interface \"%s\"\n",
                interface);
  return NULL;
}

// Driver registration function
void MicroStrain3DMG_Register(DriverTable* table)
{
  table->AddDriver("microstrain3dmg", PLAYER_READ_MODE, MicroStrain3DMG_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// IMU codes

#define CMD_NULL      0x00
#define CMD_VERSION   0xF0
#define CMD_INSTANTV  0x03
#define CMD_STABV     0x02
#define CMD_STABM     0x0B
#define CMD_STABQ     0x05
#define CMD_STABEULER 0x0E

#define TICK_TIME     6.5536e-3
#define G             9.81


////////////////////////////////////////////////////////////////////////////////
// Constructor
MicroStrain3DMG::MicroStrain3DMG(int code, ConfigFile* cf, int section)
    : Driver(cf, section, sizeof(player_position3d_data_t), 0, 0, 0)
{
  // Interface to use
  this->code = code;
  
  // Default serial port
  this->port_name = cf->ReadString(section, "port", "/dev/ttyS1");

  // Initialize data queue
  this->q_max_count = 0;
  this->q_count = 0;
  this->q_elems = NULL;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
MicroStrain3DMG::~MicroStrain3DMG()
{
  free(this->q_elems);

  this->q_max_count = 0;
  this->q_count = 0;
  this->q_elems = NULL;
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device
int MicroStrain3DMG::Setup()
{   
  printf("IMU initialising (%s)\n", this->port_name);
    
  // Open the port
  if (OpenPort())
    return -1;
  
  // Start driver thread
  StartThread();

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int MicroStrain3DMG::Shutdown()
{
  // Stop driver thread
  StopThread();

  // Close the port
  ClosePort();

  return 0;
}

/* TODO: this is fundamentally broken for internal device
access; needs re-thinking.

////////////////////////////////////////////////////////////////////////////////
// Get number of pending data packets 
size_t MicroStrain3DMG::GetNumData(void* client)
{
  //printf("q_count %d\n", this->q_count);
  return this->q_count;
}


////////////////////////////////////////////////////////////////////////////////
// Get pending data packets
// TODO: queue doesnt work for internal clients.  Re-thing or remove
size_t MicroStrain3DMG::GetData(void* client, unsigned char* dest, size_t len,
                                uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  assert(len >= sizeof(player_position3d_data_t));
  if (this->Pop((player_position3d_data_t*) dest, timestamp_sec, timestamp_usec))
    return sizeof(player_position3d_data_t);
  return 0;
}
*/    

////////////////////////////////////////////////////////////////////////////////
// Push data to the queue
void MicroStrain3DMG::Push(player_position3d_data_t *data,
                           uint32_t time_sec, uint32_t time_usec)
{
  this->Lock();
  
  // Expand queue as necessary
  if (this->q_count >= this->q_max_count)
  {
    this->q_max_count += 1;
    this->q_max_count *= 2;
    this->q_elems = (QElem*) realloc(this->q_elems, this->q_max_count * sizeof(this->q_elems[0]));
    assert(this->q_elems);
  }

  // Push the data onto the end of the queue
  this->q_elems[this->q_count].data = *data;
  this->q_elems[this->q_count].time_sec = time_sec;
  this->q_elems[this->q_count].time_usec = time_usec;
  this->q_count += 1;

  this->Unlock();
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Pop data from the queue; returns the number of items popped
int MicroStrain3DMG::Pop(player_position3d_data_t *data,
                         uint32_t *time_sec, uint32_t *time_usec)
{
  if (this->q_count == 0)
    return 0;

  this->Lock();
  
  // Grab the data from the front of the queue
  *data = this->q_elems[0].data;
  if (time_sec)
    *time_sec = this->q_elems[0].time_sec;
  if (time_sec)
    *time_usec = this->q_elems[0].time_usec;
  
  // Shift the remaining elements
  this->q_count -= 1;
  memmove(this->q_elems, this->q_elems + 1, this->q_count * sizeof(this->q_elems[0]));

  this->Unlock();
    
  return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void MicroStrain3DMG::Main() 
{
  //int i;
  double ntime;
  double e[3];
  struct timeval time;
  player_position3d_data_t data;
  
  while (true)
  {
    // Test if we are supposed to cancel
    pthread_testcancel();

    // Get the time; this is probably a better estimate of when the
    // phenomena occured that getting the time after requesting data.
    GlobalTime->GetTime(&time);

    // Get the Euler angles from the sensor
    GetStabEuler(&ntime, e);

    if (this->code == PLAYER_POSITION3D_CODE)
    {
      // Construct data packet
      data.xpos = 0;
      data.ypos = 0;
      data.zpos = 0;

      data.xspeed = 0;
      data.yspeed = 0;
      data.zspeed = 0;

      data.roll = htonl((int32_t) (-e[0] * 180 / M_PI * 3600));
      data.pitch = htonl((int32_t) (-e[1] * 180 / M_PI * 3600));
      data.yaw = htonl((int32_t) (-e[2] * 180 / M_PI * 3600));

      data.pitchspeed = 0;
      data.rollspeed = 0;
      data.yawspeed = 0;
      
      data.stall = 0;

      this->PutData((uint8_t*) &data, sizeof(data), time.tv_sec, time.tv_usec);
      
      // Make data available
      // REMOVE this->Push(&data, time.tv_sec, time.tv_usec);
    }
    else
      assert(false);
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Open the terminal
// Returns 0 on success
int MicroStrain3DMG::OpenPort()
{
  char firmware[32];
  
  // Open the port
  this->fd = open(this->port_name, O_RDWR | O_SYNC , S_IRUSR | S_IWUSR );
  if (this->fd < 0)
  {
    PLAYER_ERROR2("unable to open serial port [%s]; [%s]",
                  (char*) this->port_name, strerror(errno));
    return -1;
  }

  // Change port settings
  struct termios term;
  if (tcgetattr(this->fd, &term) < 0)
  {
    PLAYER_ERROR("Unable to get serial port attributes");
    return -1;
  }

#if HAVE_CFMAKERAW
  cfmakeraw( &term );
#endif
  cfsetispeed(&term, B38400);
  cfsetospeed(&term, B38400);

  if (tcsetattr(this->fd, TCSAFLUSH, &term) < 0 )
  {
    PLAYER_ERROR("Unable to set serial port attributes");
    return -1;
  }

  // Make sure queues are empty before we begin
  tcflush(this->fd, TCIOFLUSH);

  printf("getting version...\n");
  
  // Check the firmware version
  if (GetFirmware(firmware, sizeof(firmware)) < 0)
    return -1;
  printf("opened %s\n", firmware);
    
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Close the serial port
// Returns 0 on success
int MicroStrain3DMG::ClosePort()
{
  close(this->fd);
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Read the firmware version
int MicroStrain3DMG::GetFirmware(char *firmware, int len)
{
  int version;
  uint8_t cmd[1];
  uint8_t rep[5];
  
  cmd[0] = CMD_VERSION;
  if (Transact(cmd, sizeof(cmd), rep, sizeof(rep)) < 0)
    return -1;

  version = MAKEUINT16(rep[2], rep[1]);
  
  snprintf(firmware, len, "3DM-G Firmware %d.%d.%02d",
           version / 1000, (version % 1000) / 100, version % 100);
  return 0;
}



////////////////////////////////////////////////////////////////////////////////
// Read the stabilized accelertion vectors
int MicroStrain3DMG::GetStabV(double *time, double v[3], double w[3])
{
  int i, k;
  uint8_t cmd[1];
  uint8_t rep[23];
  int ticks;
  
  cmd[0] = CMD_STABV;
  if (Transact(cmd, sizeof(cmd), rep, sizeof(rep)) < 0)
    return -1;

  for (i = 0; i < 3; i++)
  {
    k = 7 + 2 * i;
    v[i] = (double) ((int16_t) MAKEUINT16(rep[k + 1], rep[k])) / 8192 * G;
    k = 13 + 2 * i;
    w[i] = (double) ((int16_t) MAKEUINT16(rep[k + 1], rep[k])) / (64 * 8192 * TICK_TIME);
  }

  // TODO: handle rollover
  ticks = (uint16_t) MAKEUINT16(rep[20], rep[19]);
  *time = ticks * TICK_TIME;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Read the stabilized orientation matrix
// World coordinate system has X = north, Y = east, Z = down.
int MicroStrain3DMG::GetStabM(int M[3][3])
{
  int i, j, k;
  uint8_t cmd[1];
  uint8_t rep[23];
  
  cmd[0] = CMD_STABM;
  if (Transact(cmd, sizeof(cmd), rep, sizeof(rep)) < 0)
    return -1;

  // Read the orientation matrix
  k = 1;
  for (i = 0; i < 3; i++)
  {
    for (j = 0; j < 3; j++)
    {
      M[j][i] = (int) ((int16_t) MAKEUINT16(rep[k + 1], rep[k]));
      k += 2;

      printf("%+6d ", M[j][i]);
    }
    printf("\n");
  }
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Read the stabilized orientation quaternion
// World coordinate system has X = north, Y = east, Z = down.
int MicroStrain3DMG::GetStabQ(double *time, double q[4])
{
  int i, k;
  int ticks;
  uint8_t cmd[1];
  uint8_t rep[13];
  
  cmd[0] = CMD_STABQ;
  if (Transact(cmd, sizeof(cmd), rep, sizeof(rep)) < 0)
    return -1;

  // Read the quaternion
  k = 1;
  for (i = 0; i < 4; i++)
  {
    q[i] = (double) ((int16_t) MAKEUINT16(rep[k + 1], rep[k])) / 8192;
    k += 2;
  }

  // TODO: handle rollover
  ticks = (uint16_t) MAKEUINT16(rep[10], rep[9]);
  *time = ticks * TICK_TIME;  
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Read the stabilized Euler angles (pitch, roll, yaw) (radians)
// World coordinate system has X = north, Y = east, Z = down.
int MicroStrain3DMG::GetStabEuler(double *time, double e[3])
{
  int i, k;
  int ticks;
  uint8_t cmd[1];
  uint8_t rep[11];
  
  cmd[0] = CMD_STABEULER;
  if (Transact(cmd, sizeof(cmd), rep, sizeof(rep)) < 0)
    return -1;

  // Read the angles
  k = 1;
  for (i = 0; i < 3; i++)
  {
    e[i] = (double) ((int16_t) MAKEUINT16(rep[k + 1], rep[k])) * 2 * M_PI / 65536.0;
    k += 2;
  }

  // TODO: handle rollover
  ticks = (uint16_t) MAKEUINT16(rep[10], rep[9]);
  *time = ticks * TICK_TIME;  
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Send a packet and wait for a reply from the IMU.
// Returns the number of bytes read.
int MicroStrain3DMG::Transact(void *cmd, int cmd_len, void *rep, int rep_len)
{
  int nbytes, bytes;
  
  // Make sure both input and output queues are empty
  tcflush(this->fd, TCIOFLUSH);
    
  // Write the data to the port
  bytes = write(this->fd, cmd, cmd_len);
  if (bytes < 0)
    PLAYER_ERROR1("error writing to IMU [%s]", strerror(errno));
  assert(bytes == cmd_len);

  // Make sure the queue is drained
  // Synchronous IO doesnt always work
  tcdrain(this->fd);

  // Read data from the port
  bytes = 0;
  while (bytes < rep_len)
  {
    nbytes = read(this->fd, (char*) rep + bytes, rep_len - bytes);
    if (nbytes < 0)
      PLAYER_ERROR1("error reading from IMU [%s]", strerror(errno));
    bytes += nbytes;
  }
  
  return bytes;
}
