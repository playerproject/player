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

/*
  Desc: Driver for the Segway RMP
  Author: John Sweeny and Brian Gerkey
  Date:
  CVS: $Id$
*/

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_segwayrmp SegwayRMP driver

The segwayrmp driver provides control of a Segway RMP (Robotic
Mobility Platform), which is an experimental robotic version of the
Segway HT (Human Transport), a kind of two-wheeled, self-balancing
electric scooter.

@par Notes

- Because of its power, weight, height, and dynamics, the Segway RMP is
a potentially dangerous machine.  Be @e very careful with it.

- Although the RMP does not actually support motor power control from 
software, for safety you must explicitly enable the motors using a
@p PLAYER_POSITION_MOTOR_POWER_REQ or @p PLAYER_POSITION3D_MOTOR_POWER_REQ
(depending on which interface you are using).  You must @e also enable
the motors in the command packet, by setting the @p state field to 1.

- For safety, this driver will stop the RMP (i.e., send zero velocities)
if no new command has been received from a client in the previous 400ms or so.
Thus, even if you want to continue moving at a constant velocity, you must
continuously send your desired velocities.

- Most of the configuration requests have not been tested.

- Currently, the only supported type of CAN I/O is "kvaser",
which uses Kvaser, Inc.'s CANLIB interface library.  This library provides
access to CAN cards made by Kvaser, such as the LAPcan II.  However, the CAN 
I/O subsystem within this driver is modular, so that it should be pretty
straightforward to add support for other CAN cards.


@par Interfaces

- @ref player_interface_position
  - This interface returns odometry data, and accepts velocity commands.

- @ref player_interface_position3d
  - This interface returns odometry data (x, y and yaw) from the wheel
  encoders, and attitude data (pitch and roll) from the IMU.  The
  driver accepts velocity commands (x vel and yaw vel).

- @ref player_interface_power
  - Returns the current battery voltage (72 V when fully charged).

@par Supported configuration requests

- position interface
  - PLAYER_POSITION_POWER_REQ

- position3d interface
  - PLAYER_POSITION_POWER_REQ

@par Configuration file options

- canio "kvaser"
  - Type of CANbus driver.

- max_xspeed 0.5
  - Maximum linear speed (m/sec)

- max_yawspeed 40
  - Maximum angular speed (deg/sec)
      
@par Example 

@verbatim
driver
(
  driver segwayrmp
  devices ["position:0" "position3d:0" "power:0"]
)
@endverbatim
*/
/** @} */

  
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>

#include "player.h"
#include "driver.h"
#include "devicetable.h"
#include "drivertable.h"

#include "rmp_frame.h"
#include "segwayrmp.h"


// Number of RMP read cycles, without new speed commands from clients,
// after which we'll stop the robot (for safety).  The read loop
// seems to run at about 50Hz, or 20ms per cycle.
#define RMP_TIMEOUT_CYCLES 20 // about 400ms


////////////////////////////////////////////////////////////////////////////////
// A factory creation function
Driver* SegwayRMP_Init(ConfigFile* cf, int section)
{
  // Create and return a new instance of this driver
  return ((Driver*) (new SegwayRMP(cf, section)));
}


////////////////////////////////////////////////////////////////////////////////
// A driver registration function.
void SegwayRMP_Register(DriverTable* table)
{
  table->AddDriver("segwayrmp", SegwayRMP_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
SegwayRMP::SegwayRMP(ConfigFile* cf, int section)
    : Driver(cf, section)
{
  player_device_id_t* ids;
  int num_ids;

  memset(&this->position_id.code, 0, sizeof(player_device_id_t));
  memset(&this->position3d_id.code, 0, sizeof(player_device_id_t));

  // Parse devices section
  if((num_ids = cf->ParseDeviceIds(section,&ids)) < 0)
  {
    this->SetError(-1);    
    return;
  }

  // Do we create a position interface?
  if(cf->ReadDeviceId(&(this->position_id), ids, num_ids, 
                      PLAYER_POSITION_CODE, 0) == 0)
  {
    if(this->AddInterface(this->position_id, PLAYER_ALL_MODE,
                          sizeof(player_position_data_t),
                          sizeof(player_position_cmd_t), 1, 1) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  // Do we create a position3d interface?
  if(cf->ReadDeviceId(&(this->position3d_id), ids, num_ids, 
                      PLAYER_POSITION3D_CODE, 0) == 0)
  {
    if(this->AddInterface(this->position3d_id, PLAYER_ALL_MODE,
                          sizeof(player_position3d_data_t),
                          sizeof(player_position3d_cmd_t), 1, 1) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  // Do we create a power interface?
  if(cf->ReadDeviceId(&(this->power_id), ids, num_ids, 
                      PLAYER_POWER_CODE, 0) == 0)
  {
    if(this->AddInterface(this->power_id, PLAYER_READ_MODE,
                          sizeof(player_power_data_t),
                          0, 1, 1) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  this->canio = NULL;
  this->caniotype = cf->ReadString(section, "canio", "kvaser");
  this->max_xspeed = (int) (1000 * cf->ReadLength(section, "max_xspeed", 0.5));
  if(this->max_xspeed < 0)
    this->max_xspeed = -this->max_xspeed;
  this->max_yawspeed = (int) (RTOD(cf->ReadAngle(section, "max_yawspeed", 40)));
  if(this->max_yawspeed < 0)
    this->max_yawspeed = -this->max_yawspeed;
  
  return;
}


SegwayRMP::~SegwayRMP()
{
  return;
}

int
SegwayRMP::Setup()
{
  // Clear the command buffers
  if (this->position_id.code)
    ClearCommand(this->position_id);
  if (this->position3d_id.code)
    ClearCommand(this->position3d_id);

  printf("segwayrmp: CAN bus initializing...");
  fflush(stdout);

  if(!strcmp(this->caniotype, "kvaser"))
    assert(this->canio = new CANIOKvaser);
  else
  {
    PLAYER_ERROR1("Unknown CAN I/O type: \"%s\"", this->caniotype);
    return(-1);
  }

  // start the CAN at 500 kpbs
  if(this->canio->Init(BAUD_500K) < 0) 
  {
    PLAYER_ERROR("error on CAN Init");
    return(-1);
  }

  // Initialize odometry
  this->odom_x = this->odom_y = this->odom_yaw = 0.0;

  this->last_xspeed = this->last_yawspeed = 0;
  this->motor_allow_enable = false;
  this->motor_enabled = false;
  this->firstread = true;
  this->timeout_counter = 0;

  StartThread();

  puts("done.");
  printf("segwayrmp: max_xspeed: %d\tmax_yawspeed: %d\n",
         this->max_xspeed, this->max_yawspeed);

  return(0);
}

int
SegwayRMP::Shutdown()
{
  printf("segwayrmp: Shutting down CAN bus...");
  fflush(stdout);

  
  // TODO: segfaulting in here somewhere on client disconnect, but only 
  // sometimes.  
  //
  // UPDATE: This might have been fixed by moving the call to StopThread()
  // to before the sending of zero velocities.   There could have been
  // a race condition, since Shutdown() is called from the server's thread 
  // context.

  StopThread();
  
  // send zero velocities, for some semblance of safety
  CanPacket pkt;

  MakeVelocityCommand(&pkt,0,0);
  Write(pkt);

  // shutdown the CAN
  canio->Shutdown();
  delete canio;
  canio = NULL;
  
  puts("done.");
  return(0);
}

// Main function for device thread.
void 
SegwayRMP::Main()
{
  unsigned char buffer[256];
  size_t buffer_len;
  player_position_cmd_t position_cmd;
  player_position3d_cmd_t position3d_cmd;
  void *client;
  CanPacket pkt;
  int32_t xspeed,yawspeed;
  bool got_command;

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

  for(;;)
  {
    pthread_testcancel();
    
    // Read from the RMP
    if(Read() < 0)
    {
      PLAYER_ERROR("Read() errored; bailing");
      pthread_exit(NULL);
    }

    // TODO: report better timestamps, possibly using time info from the RMP

    // Send data to clients
    PutData(this->position_id, &this->position_data, 
            sizeof(this->position_data), NULL);
    PutData(this->position3d_id, &this->position3d_data, 
            sizeof(this->position3d_data), NULL);
    PutData(this->power_id, &this->power_data, 
            sizeof(this->power_data), NULL);
    
    // check for config requests from the position interface
    if((buffer_len = GetConfig(this->position_id, &client, buffer, sizeof(buffer),NULL)) > 0)
    {
      // if we write to the CAN bus as a result of the config, don't write
      // a velocity command (may need to make this smarter if we get slow
      // velocity control).
      if(HandlePositionConfig(client,buffer,buffer_len) > 0)
        continue;
    }

    // check for config requests from the position3d interface
    if((buffer_len = GetConfig(this->position3d_id, &client, buffer, sizeof(buffer),NULL)) > 0)
    {
      // if we write to the CAN bus as a result of the config, don't write
      // a velocity command (may need to make this smarter if we get slow
      // velocity control).
      if(HandlePosition3DConfig(client,buffer,buffer_len) > 0)
        continue;
    }

    // start with the last commanded values
    xspeed = this->last_xspeed;
    yawspeed = this->last_yawspeed;
    got_command = false;

    // Check for commands from the position interface
    if (this->position_id.code)
    {
      if(GetCommand(this->position_id, (void*) &position_cmd, 
                    sizeof(position_cmd),NULL))
      {
        // zero the command buffer, so that we can timeout if a client doesn't
        // send commands for a while
        ClearCommand(this->position_id);

        // convert to host order; let MakeVelocityCommand do the rest
        xspeed = ntohl(position_cmd.xspeed);
        yawspeed = ntohl(position_cmd.yawspeed);
        motor_enabled = position_cmd.state && motor_allow_enable;
        timeout_counter=0;
        got_command = true;
      }
    }

    // Check for commands from the position3d interface
    if (this->position3d_id.code)
    {
      if(GetCommand(this->position3d_id, (void*) &position3d_cmd, 
                    sizeof(position3d_cmd),NULL))
      {
        // zero the command buffer, so that we can timeout if a client doesn't
        // send commands for a while
        ClearCommand(this->position3d_id);

        // convert to host order; let MakeVelocityCommand do the rest
        // Position3d uses milliradians/sec, so convert here to
        // degrees/sec
        xspeed = ntohl(position3d_cmd.xspeed);
        yawspeed = (int32_t) (((double) (int32_t) ntohl(position3d_cmd.yawspeed)) / 1000 * 180 / M_PI);
        motor_enabled = position3d_cmd.state && motor_allow_enable;
        timeout_counter=0;
        got_command = true;
      }
    }
    // No commands, so we may timeout soon
    if (!got_command)
      timeout_counter++;

    if(timeout_counter >= RMP_TIMEOUT_CYCLES)
    {
      if(xspeed || yawspeed)
      {
        PLAYER_WARN("timeout exceeded without new commands; stopping robot");
        xspeed = 0;
        yawspeed = 0;
      }
      // set it to the limit, to prevent overflow, but keep the robot
      // stopped until a new command comes in.
      timeout_counter = RMP_TIMEOUT_CYCLES;
    }

    if(!motor_enabled) 
    {
      xspeed = 0;
      yawspeed = 0;
    }

    // make a velocity command... could be zero
    MakeVelocityCommand(&pkt,xspeed,yawspeed);
    if(Write(pkt) < 0)
      PLAYER_ERROR("error on write");
  }
}
  
// helper to handle config requests
// returns 1 to indicate we wrote to the CAN bus
// returns 0 to indicate we did NOT write to CAN bus
int
SegwayRMP::HandlePositionConfig(void* client, unsigned char* buffer, size_t len)
{
  uint16_t rmp_cmd,rmp_val;
  player_rmp_config_t *rmp;
  CanPacket pkt;
  
  switch(buffer[0]) 
  {
    case PLAYER_POSITION_MOTOR_POWER_REQ:
      // just set a flag telling us whether we should
      // act on motor commands
      // set the commands to 0... think it will automatically
      // do this for us.  
      if(buffer[1]) 
        this->motor_allow_enable = true;
      else
        this->motor_allow_enable = false;

      printf("SEGWAYRMP: motors state: %d\n", this->motor_allow_enable);

      if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL))
        PLAYER_ERROR("Failed to PutReply in segwayrmp\n");
      break;

    case PLAYER_POSITION_GET_GEOM_REQ:
      player_position_geom_t geom;
      geom.subtype = PLAYER_POSITION_GET_GEOM_REQ;
      geom.pose[0] = htons((short)(0));
      geom.pose[1] = htons((short)(0));
      geom.pose[2] = htons((short)(0));
      geom.size[0] = htons((short)(508));
      geom.size[1] = htons((short)(610));

      if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, &geom, sizeof(geom),NULL))
        PLAYER_ERROR("Segway: failed to PutReply");
      break;

    case PLAYER_POSITION_RESET_ODOM_REQ:
      // we'll reset all the integrators

      MakeStatusCommand(&pkt, (uint16_t)RMP_CAN_CMD_RST_INT, 
                        (uint16_t)RMP_CAN_RST_ALL);
      if(Write(pkt) < 0)
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {

        if (Write(pkt) < 0) {
          if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
            PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
        } else {
          if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL))
            PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
        }
      }

      odom_x = odom_y = odom_yaw = 0.0;
      firstread = true;

      // return 1 to indicate that we wrote to the CAN bus this time
      return(1);

    case PLAYER_POSITION_RMP_VELOCITY_SCALE:
      rmp_cmd = RMP_CAN_CMD_MAX_VEL;
      rmp = (player_rmp_config_t *)buffer;
      rmp_val = ntohs(rmp->value);

      printf("SEGWAYRMP: velocity scale %d\n", rmp_val);

      MakeStatusCommand(&pkt, rmp_cmd, rmp_val);

      if(Write(pkt) < 0)
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      // return 1 to indicate that we wrote to the CAN bus this time
      return(1);

    case PLAYER_POSITION_RMP_ACCEL_SCALE:
      rmp_cmd = RMP_CAN_CMD_MAX_ACCL;
      rmp = (player_rmp_config_t *)buffer;
      rmp_val = ntohs(rmp->value);

      MakeStatusCommand(&pkt, rmp_cmd, rmp_val);

      if(Write(pkt) < 0)
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      // return 1 to indicate that we wrote to the CAN bus this time
      return(1);

    case PLAYER_POSITION_RMP_TURN_SCALE:
      rmp_cmd = RMP_CAN_CMD_MAX_TURN;
      rmp = (player_rmp_config_t *)buffer;
      rmp_val = ntohs(rmp->value);

      MakeStatusCommand(&pkt, rmp_cmd, rmp_val);

      if(Write(pkt) < 0)
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      // return 1 to indicate that we wrote to the CAN bus this time
      return(1);

    case PLAYER_POSITION_RMP_GAIN_SCHEDULE:
      rmp_cmd = RMP_CAN_CMD_GAIN_SCHED;
      rmp = (player_rmp_config_t *)buffer;
      rmp_val = ntohs(rmp->value);

      MakeStatusCommand(&pkt, rmp_cmd, rmp_val);

      if(Write(pkt) < 0)
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      // return 1 to indicate that we wrote to the CAN bus this time
      return(1);

    case PLAYER_POSITION_RMP_CURRENT_LIMIT:
      rmp_cmd = RMP_CAN_CMD_CURR_LIMIT;
      rmp = (player_rmp_config_t *)buffer;
      rmp_val = ntohs(rmp->value);

      MakeStatusCommand(&pkt, rmp_cmd, rmp_val);

      if(Write(pkt) < 0)
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      // return 1 to indicate that we wrote to the CAN bus this time
      return(1);

    case PLAYER_POSITION_RMP_RST_INTEGRATORS:
      rmp_cmd = RMP_CAN_CMD_RST_INT;
      rmp = (player_rmp_config_t *)buffer;
      rmp_val = ntohs(rmp->value);

      MakeStatusCommand(&pkt, rmp_cmd, rmp_val);

      if(Write(pkt) < 0)
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if (Write(pkt) < 0) {
          if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
            PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
        } else {
          if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL))
            PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
        }
      }
      // return 1 to indicate that we wrote to the CAN bus this time
      return(1);

    case PLAYER_POSITION_RMP_SHUTDOWN:
      MakeShutdownCommand(&pkt);

      if(Write(pkt) < 0)
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      // return 1 to indicate that we wrote to the CAN bus this time
      return(1);

    default:
      printf("segwayrmp received unknown config request %d\n", 
             buffer[0]);
      if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL))
        PLAYER_ERROR("Failed to PutReply in segwayrmp\n");
      break;
  }

  // return 0, to indicate that we did NOT write to the CAN bus this time
  return(0);
}

// helper to handle config requests
// returns 1 to indicate we wrote to the CAN bus
// returns 0 to indicate we did NOT write to CAN bus
int
SegwayRMP::HandlePosition3DConfig(void* client, unsigned char* buffer, size_t len)
{
  switch(buffer[0]) 
  {
    case PLAYER_POSITION3D_MOTOR_POWER_REQ:
      // just set a flag telling us whether we should
      // act on motor commands
      // set the commands to 0... think it will automatically
      // do this for us.  
      if(buffer[1]) 
        this->motor_allow_enable = true;
      else
        this->motor_allow_enable = false;

      printf("SEGWAYRMP: motors state: %d\n", this->motor_allow_enable);

      if(PutReply(this->position3d_id, client, PLAYER_MSGTYPE_RESP_ACK, NULL))
        PLAYER_ERROR("Failed to PutReply in segwayrmp\n");
      break;

    default:
      printf("segwayrmp received unknown config request %d\n", 
             buffer[0]);
      if(PutReply(this->position3d_id, client, PLAYER_MSGTYPE_RESP_NACK,NULL))
        PLAYER_ERROR("Failed to PutReply in segwayrmp\n");
      break;
  }

  // return 0, to indicate that we did NOT write to the CAN bus this time
  return(0);
}


int
SegwayRMP::Read()
{
  CanPacket pkt;
  int channel;
  int ret;
  rmp_frame_t data_frame[2];
  
  //static struct timeval last;
  //struct timeval curr;

  data_frame[0].ready = 0;
  data_frame[1].ready = 0;

  // read one cycle of data from each channel
  for(channel = 0; channel < DUALCAN_NR_CHANNELS; channel++) 
  {
    ret=0;
    // read until we've gotten all 5 packets for this cycle on this channel
    while((ret = canio->ReadPacket(&pkt, channel)) >= 0)
    {
      // then we got a new packet...

      //printf("SEGWAYIO: pkt: %s\n", pkt.toString());

      data_frame[channel].AddPacket(pkt);

      // if data has been filled in, then let's make it the latest data 
      // available to player...
      if(data_frame[channel].IsReady()) 
      {
        // Only bother doing the data conversions for one channel.
        // It appears that the data on channel 0 is garbarge, so we'll read
        // from channel 1.
        if(channel == 1)
        {
	  UpdateData(&data_frame[channel]);
        }

        data_frame[channel].ready = 0;
        break;
      }
    }

    if (ret < 0) 
    {
      PLAYER_ERROR2("error (%d) reading packet on channel %d",
                    ret, channel);
    }
  }

  return(0);
}

void
SegwayRMP::UpdateData(rmp_frame_t * data_frame)
{
  int delta_lin_raw, delta_ang_raw;
  double delta_lin, delta_ang;
  double tmp;

  // Get the new linear and angular encoder values and compute
  // odometry.  Note that we do the same thing here, regardless of 
  // whether we're presenting 2D or 3D position info.
  delta_lin_raw = Diff(this->last_raw_foreaft,
                       data_frame->foreaft,
                       this->firstread);
  this->last_raw_foreaft = data_frame->foreaft;
  
  delta_ang_raw = Diff(this->last_raw_yaw,
                       data_frame->yaw,
                       this->firstread);
  this->last_raw_yaw = data_frame->yaw;
  
  delta_lin = (double)delta_lin_raw / (double)RMP_COUNT_PER_M;
  delta_ang = DTOR((double)delta_ang_raw / 
                   (double)RMP_COUNT_PER_REV * 360.0);
  
  // First-order odometry integration
  this->odom_x += delta_lin * cos(this->odom_yaw);
  this->odom_y += delta_lin * sin(this->odom_yaw);
  this->odom_yaw += delta_ang;
  
  // Normalize yaw in [0, 360]
  this->odom_yaw = atan2(sin(this->odom_yaw), cos(this->odom_yaw));
  if (this->odom_yaw < 0)
    this->odom_yaw += 2 * M_PI;
  
  // first, do 2D info.
  this->position_data.xpos = htonl((int32_t)(this->odom_x * 1000.0));
  this->position_data.ypos = htonl((int32_t)(this->odom_y * 1000.0));
  this->position_data.yaw = htonl((int32_t)RTOD(this->odom_yaw));
  
  // combine left and right wheel velocity to get foreward velocity
  // change from counts/s into mm/s
  this->position_data.xspeed = 
    htonl((uint32_t)rint(((double)data_frame->left_dot +
                          (double)data_frame->right_dot) /
                         (double)RMP_COUNT_PER_M_PER_S 
                         * 1000.0 / 2.0));
  
  // no side speeds for this bot
  this->position_data.yspeed = 0;
  
  // from counts/sec into deg/sec.  also, take the additive
  // inverse, since the RMP reports clockwise angular velocity as
  // positive.
  this->position_data.yawspeed =
    htonl((int32_t)(-rint((double)data_frame->yaw_dot / 
                          (double)RMP_COUNT_PER_DEG_PER_S)));
  
  this->position_data.stall = 0;
  
  // now, do 3D info.
  this->position3d_data.xpos = htonl((int32_t)(this->odom_x * 1000.0));
  this->position3d_data.ypos = htonl((int32_t)(this->odom_y * 1000.0));
  // this robot doesn't fly
  this->position3d_data.zpos = 0;
  
  // normalize angles to [0,360]
  tmp = NORMALIZE(DTOR((double)data_frame->roll /
                       (double)RMP_COUNT_PER_DEG));
  if(tmp < 0)
    tmp += 2*M_PI;
  this->position3d_data.roll = htonl((int32_t)rint(tmp * 1000.0));
  
  // normalize angles to [0,360]
  tmp = NORMALIZE(DTOR((double)data_frame->pitch /
                       (double)RMP_COUNT_PER_DEG));
  if(tmp < 0)
    tmp += 2*M_PI;
  this->position3d_data.pitch = htonl((int32_t)rint(tmp * 1000.0));
  
  this->position3d_data.yaw = htonl(((int32_t)(this->odom_yaw * 1000.0)));
  
  // combine left and right wheel velocity to get foreward velocity
  // change from counts/s into mm/s
  this->position3d_data.xspeed = 
    htonl((uint32_t)rint(((double)data_frame->left_dot +
                          (double)data_frame->right_dot) /
                         (double)RMP_COUNT_PER_M_PER_S 
                         * 1000.0 / 2.0));
  // no side or vertical speeds for this bot
  this->position3d_data.yspeed = 0;
  this->position3d_data.zspeed = 0;
  
  this->position3d_data.rollspeed = 
    htonl((int32_t)rint((double)data_frame->roll_dot /
                        (double)RMP_COUNT_PER_DEG_PER_S * M_PI / 180 * 1000.0));
  this->position3d_data.pitchspeed = 
    htonl((int32_t)rint((double)data_frame->pitch_dot /
                        (double)RMP_COUNT_PER_DEG_PER_S * M_PI / 180 * 1000.00));
  // from counts/sec into millirad/sec.  also, take the additive
  // inverse, since the RMP reports clockwise angular velocity as
  // positive.

  // This one uses left_dot and right_dot, which comes from odometry
  this->position3d_data.yawspeed = 
    htonl((int32_t)(rint((double)(data_frame->right_dot - data_frame->left_dot) /
                         (RMP_COUNT_PER_M_PER_S * RMP_GEOM_WHEEL_SEP * M_PI) * 1000)));
  // This one uses yaw_dot, which comes from the IMU
  //data.position3d_data.yawspeed = 
  //  htonl((int32_t)(-rint((double)data_frame->yaw_dot / 
  //                        (double)RMP_COUNT_PER_DEG_PER_S * M_PI / 180 * 1000)));
  
  this->position3d_data.stall = 0;
  
  // fill in power data.  the RMP returns a percentage of full,
  // and the specs for the HT say that it's a 72 volt system.  assuming
  // that the RMP is the same, we'll convert to decivolts for Player.
  this->power_data.charge = 
    ntohs((uint16_t)rint(data_frame->battery * 7.2));
  
  firstread = false;  
}  


int
SegwayRMP::Write(CanPacket& pkt)
{
  return(canio->WritePacket(pkt));
}

/* Creates a status CAN packet from the given arguments
 */  
void
SegwayRMP::MakeStatusCommand(CanPacket* pkt, uint16_t cmd, uint16_t val)
{
  int16_t trans,rot;

  pkt->id = RMP_CAN_ID_COMMAND;
  pkt->PutSlot(2, cmd);
 
  // it was noted in the windows demo code that they
  // copied the 8-bit value into both bytes like this
  pkt->PutByte(6, val);
  pkt->PutByte(7, val);

  trans = (int16_t) rint((double)this->last_xspeed * 
                         (double)RMP_COUNT_PER_MM_PER_S);

  if(trans > RMP_MAX_TRANS_VEL_COUNT)
    trans = RMP_MAX_TRANS_VEL_COUNT;
  else if(trans < -RMP_MAX_TRANS_VEL_COUNT)
    trans = -RMP_MAX_TRANS_VEL_COUNT;

  rot = (int16_t) rint((double)this->last_yawspeed * 
                       (double)RMP_COUNT_PER_DEG_PER_SS);

  if(rot > RMP_MAX_ROT_VEL_COUNT)
    rot = RMP_MAX_ROT_VEL_COUNT;
  else if(rot < -RMP_MAX_ROT_VEL_COUNT)
    rot = -RMP_MAX_ROT_VEL_COUNT;

  // put in the last speed commands as well
  pkt->PutSlot(0,(uint16_t)trans);
  pkt->PutSlot(1,(uint16_t)rot);
  
  if(cmd) 
  {
    printf("SEGWAYIO: STATUS: cmd: %02x val: %02x pkt: %s\n", 
	   cmd, val, pkt->toString());
  }
}

/* takes a player command (in host byte order) and turns it into CAN packets 
 * for the RMP 
 */
void
SegwayRMP::MakeVelocityCommand(CanPacket* pkt, 
                               int32_t xspeed, 
                               int32_t yawspeed)
{
  pkt->id = RMP_CAN_ID_COMMAND;
  pkt->PutSlot(2, (uint16_t)RMP_CAN_CMD_NONE);
  
  // we only care about cmd.xspeed and cmd.yawspeed
  // translational velocity is given to RMP in counts 
  // [-1176,1176] ([-8mph,8mph])

  // player is mm/s
  // 8mph is 3576.32 mm/s
  // so then mm/s -> counts = (1176/3576.32) = 0.32882963

  if(xspeed > this->max_xspeed)
  {
    PLAYER_WARN2("xspeed thresholded! (%d > %d)", xspeed, this->max_xspeed);
    xspeed = this->max_xspeed;
  }
  else if(xspeed < -this->max_xspeed)
  {
    PLAYER_WARN2("xspeed thresholded! (%d < %d)", xspeed, -this->max_xspeed);
    xspeed = -this->max_xspeed;
  }

  this->last_xspeed = xspeed;

  int16_t trans = (int16_t) rint((double)xspeed * 
                                 (double)RMP_COUNT_PER_MM_PER_S);

  if(trans > RMP_MAX_TRANS_VEL_COUNT)
    trans = RMP_MAX_TRANS_VEL_COUNT;
  else if(trans < -RMP_MAX_TRANS_VEL_COUNT)
    trans = -RMP_MAX_TRANS_VEL_COUNT;

  if(yawspeed > this->max_yawspeed)
  {
    PLAYER_WARN2("yawspeed thresholded! (%d > %d)", 
                 yawspeed, this->max_yawspeed);
    yawspeed = this->max_yawspeed;
  }
  else if(yawspeed < -this->max_yawspeed)
  {
    PLAYER_WARN2("yawspeed thresholded! (%d < %d)", 
                 yawspeed, -this->max_yawspeed);
    yawspeed = -this->max_yawspeed;
  }
  this->last_yawspeed = yawspeed;

  // rotational RMP command \in [-1024, 1024]
  // this is ripped from rmi_demo... to go from deg/s to counts
  // deg/s -> count = 1/0.013805056
  int16_t rot = (int16_t) rint((double)yawspeed * 
                               (double)RMP_COUNT_PER_DEG_PER_S);

  if(rot > RMP_MAX_ROT_VEL_COUNT)
    rot = RMP_MAX_ROT_VEL_COUNT;
  else if(rot < -RMP_MAX_ROT_VEL_COUNT)
    rot = -RMP_MAX_ROT_VEL_COUNT;

  pkt->PutSlot(0, (uint16_t)trans);
  pkt->PutSlot(1, (uint16_t)rot);
}

void
SegwayRMP::MakeShutdownCommand(CanPacket* pkt)
{
  pkt->id = RMP_CAN_ID_SHUTDOWN;

  printf("SEGWAYIO: SHUTDOWN: pkt: %s\n",
	 pkt->toString());
}

// Calculate the difference between two raw counter values, taking care
// of rollover.
int
SegwayRMP::Diff(uint32_t from, uint32_t to, bool first)
{
  int diff1, diff2;
  static uint32_t max = (uint32_t)pow(2,32)-1;

  // if this is the first time, report no change
  if(first)
    return(0);

  diff1 = to - from;

  /* find difference in two directions and pick shortest */
  if(to > from)
    diff2 = -(from + max - to);
  else 
    diff2 = max - from + to;

  if(abs(diff1) < abs(diff2)) 
    return(diff1);
  else
    return(diff2);
}
