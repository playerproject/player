/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003  John Sweeney & Brian Gerkey
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

#include "canio.h"
#include "canio_kvaser.h"
#include "player.h"
#include "device.h"
#include "configfile.h"

// Both of these can be changed via the configuration file; please do NOT
// change them here!
#define RMP_DEFAULT_MAX_XSPEED 500   // mm/sec
#define RMP_DEFAULT_MAX_YAWSPEED 40  // deg/sec

// Number of RMP read cycles, without new speed commands from clients,
// after which we'll stop the robot (for safety).  The read loop
// seems to run at about 50Hz, or 20ms per cycle.
#define RMP_TIMEOUT_CYCLES 20 // about 400ms

// Format for data provided by the underlying segway rmp driver.  It's not
// for direct external consumption.  Rather, the SegwayRMPPosition and
// SegwayRMPPower drivers (and maybe others) will get the bits that they want
// and send them on to clients.  Essentially, this is the union of all data
// that the RMP can provide.  The underlying driver will byteswap everything
// before it goes in, so that the other drivers don't have to.
typedef struct
{
  player_position_data_t position_data;
  player_position3d_data_t position3d_data;
  player_power_data_t power_data;
} __attribute__ ((packed)) player_segwayrmp_data_t;

// Format for commands accepted by the underlying segway rmp driver. 
// The SegwayRMPPosition will put them in.  Essentially, this is the union of 
// all commands that the RMP can accept.  The SegwayRMPPosition driver can
// just forward the commands without byteswapping; the underlying driver will
// do that.
typedef struct
{
  // flag to tell the underlying driver which of the two commands to use.
  // should be either PLAYER_POSITION_CODE or PLAYER_POSITION3D_CODE.
  uint16_t code; 
  player_position_cmd_t position_cmd;
  player_position3d_cmd_t position3d_cmd;
} __attribute__ ((packed)) player_segwayrmp_cmd_t;

// Driver for robotic Segway
class SegwayRMP : public CDevice
{
  public: 
    // instantiation method.  use this instead of the constructor
    static CDevice* Instance(ConfigFile* cf, int section);

    // Constructor	  
    SegwayRMP();
    ~SegwayRMP();

    // Setup/shutdown routines.
    virtual int Setup();
    virtual int Shutdown();

  protected:
    player_segwayrmp_data_t data;

  private:

    void ProcessConfigFile(ConfigFile* cf, int section);

    static CDevice* instance;

    const char* portname;
    const char* caniotype;

    int timeout_counter;

    int max_xspeed, max_yawspeed;

    bool firstread;

    DualCANIO *canio;

    int16_t last_xspeed, last_yawspeed;

    bool motor_enabled;

    // For handling rollover
    uint32_t last_raw_yaw, last_raw_left, last_raw_right, last_raw_foreaft;

    // Odometry calculation
    double odom_x, odom_y, odom_yaw;

    // helper to handle config requests
    int HandleConfig(void* client, unsigned char* buffer, size_t len);

    // helper to read a cycle of data from the RMP
    int Read();
    
    // Calculate the difference between two raw counter values, taking care
    // of rollover.
    int Diff(uint32_t from, uint32_t to, bool first);

    // helper to write a packet
    int Write(CanPacket& pkt);

    // Main function for device thread.
    virtual void Main();

    // helper to create a status command packet from the given args
    void MakeStatusCommand(CanPacket* pkt, uint16_t cmd, uint16_t val);

    // helper to take a player command and turn it into a CAN command packet
    void MakeVelocityCommand(CanPacket* pkt, int32_t xspeed, int32_t yawspeed);
    
    void MakeShutdownCommand(CanPacket* pkt);
};


