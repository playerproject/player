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

//#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <netinet/in.h>

#include "playercommon.h"
#include "drivertable.h"
#include "player.h"

#include <playertime.h>
extern PlayerTime* GlobalTime;


// MicroStraing 3DM-G IMU driver
class MicroStrain3DMG : public CDevice
{
  // Constructor
  public: MicroStrain3DMG(char* interface, ConfigFile* cf, int section);

  // Initialise device
  public: virtual int Setup();

  // Shutdown the device
  public: virtual int Shutdown();

  // Main function for device thread.
  private: virtual void Main();

  // Open the serial port
  // Returns 0 on success
  private: int OpenPort();

  // Close the serial port
  // Returns 0 on success
  private: int ClosePort();

  // Read the firmware version
  private: int GetFirmware(char *firmware, int len);

  // Read the stabilized acceleration vectors
  private: int GetStabV(double *time, double v[3], double w[3]);

  // Read the stabilized orientation matrix
  private: int GetStabM(int M[3][3]);

  // Read the stabilized orientation quaternion
  private: int GetStabQ(double *time, double q[4]);

  // Send a packet and wait for a reply from the IMU.
  // Returns the number of bytes read.
  private: int Transact(void *cmd, int cmd_len, void *rep, int rep_len);
    
  // Name of port used to communicate with the laser;
  // e.g. /dev/ttyS1
  private: const char *port_name;

  // Port file descriptor
  private: int fd;
};


// Factory creation function
CDevice* MicroStrain3DMG_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_POSITION_STRING))
  {
    PLAYER_ERROR1("driver \"MicroStrain3DMG\" does not support interface \"%s\"\n",
                  interface);
    return(NULL);
  }
  else
    return ((CDevice*) (new MicroStrain3DMG(interface, cf, section)));
}

// Driver registration function
void MicroStrain3DMG_Register(DriverTable* table)
{
  table->AddDriver("microstrain3dmg", PLAYER_READ_MODE, MicroStrain3DMG_Init);
}


////////////////////////////////////////////////////////////////////////////////
// IMU codes

#define CMD_NULL      0x00
#define CMD_VERSION   0xF0
#define CMD_INSTANTV  0x03
#define CMD_STABV     0x02
#define CMD_STABM     0x0B
#define CMD_STABQ     0x05

#define TICK_TIME     6.5536e-3
#define G             9.81


////////////////////////////////////////////////////////////////////////////////
// Constructor
MicroStrain3DMG::MicroStrain3DMG(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_position_data_t), 0, 0, 0)
{
  // Default serial port
  this->port_name = cf->ReadString(section, "port", "/dev/ttyS1");

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


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void MicroStrain3DMG::Main() 
{
  int i;
  double ntime, dt;
  double al[3], ar[3];
  double vl[3], vr[3];
  double pl[3], pr[3];
  double q[4];
  double m;
  double e[3];

  struct timeval time;
  player_position_data_t data;
  
  for (i = 0; i < 3; i++)
  {
    vr[i] = 0.0;
    pr[i] = 0.0;
  }
  
  while (true)
  {
    // Test if we are supposed to cancel
    pthread_testcancel();

    GetStabQ(&ntime, q);
    
    m =  q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
        
    //printf("%+6.3f %+6.3f %+6.3f %+6.3f, %+6.3f\n", q[0], q[1], q[2], q[3], m);

    e[0] = 0.0;
    e[1] = 0.0;
    e[2] = atan2(2 * (q[1] * q[2] + q[0] * q[3]),
                 (q[0] * q[0] + q[1] * q[1] - q[2] * q[2] - q[3] * q[3]));

    // BROKEN
    //e[1] = asin(-2 * (q[0] * q[2] - q[3] * q[1]));
    //e[0] = atan(2 * (q[3] * q[0] + q[1] * q[2]) /
    //            (q[3] * q[3] - q[0] * q[0] - q[1] * q[1] + q[2] * q[2]));

    //printf("%5.3f %+6.1f %+6.1f %+6.1f\n",
    //       ntime, e[0] * 180 / M_PI, e[1] * 180 / M_PI, e[2] * 180 / M_PI);

    // Construct data packet
    data.xpos = 0;
    data.ypos = 0;
    data.yaw = htonl((int32_t) (-e[2] * 180 / M_PI));
    data.xspeed = 0;
    data.yspeed = 0;
    data.yawspeed = 0;
    data.stall = 0;

    // HACK get the time 
    GlobalTime->GetTime(&time);

    // Make data available
    PutData(&data, sizeof(data), time.tv_sec, time.tv_usec);

    /*
    // TESTING
    GetStabV(&ntime, al, vr);

    //printf("%+f %+f %+f\n", al[0], al[1], al[2]);

    if (time > 0)
    {
      // Integrate
      dt = ntime - time;

      pl[0] += vl[0] * dt + 0.5 * al[0] * dt * dt;
      pl[1] += vl[1] * dt + 0.5 * al[1] * dt * dt;

      vl[0] += al[0] * dt;
      vl[1] += al[1] * dt;
      
      pr[2] += vr[2] * dt;
    }

    printf("%5.3f %+6.3f %+6.3f %+6.3f\n",
           dt, pl[0], pl[1], pr[2] * 180 / M_PI);
    */
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
#if PLAYER_LINUX
  this->fd = open(this->port_name, O_RDWR | O_SYNC , S_IRUSR | S_IWUSR );
#else
  this->fd = open(this->port_name, O_RDWR, S_IRUSR | S_IWUSR );
#endif
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
