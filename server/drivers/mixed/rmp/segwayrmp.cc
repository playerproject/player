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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>

#include "segwayrmp.h"
#include "player.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"

#define RMP_CAN_ID_SHUTDOWN	0x0412
#define RMP_CAN_ID_COMMAND	0x0413
#define RMP_CAN_ID_MSG1		0x0400
#define RMP_CAN_ID_MSG2		0x0401
#define RMP_CAN_ID_MSG3		0x0402
#define RMP_CAN_ID_MSG4		0x0403
#define RMP_CAN_ID_MSG5		0x0404

#define RMP_CAN_CMD_NONE		0
#define RMP_CAN_CMD_MAX_VEL		10
#define RMP_CAN_CMD_MAX_ACCL		11
#define RMP_CAN_CMD_MAX_TURN		12
#define RMP_CAN_CMD_GAIN_SCHED		13
#define RMP_CAN_CMD_CURR_LIMIT		14
#define RMP_CAN_CMD_RST_INT		50

#define RMP_CAN_RST_RIGHT		0x01
#define RMP_CAN_RST_LEFT		0x02
#define RMP_CAN_RST_YAW			0x04
#define RMP_CAN_RST_FOREAFT		0x08
#define RMP_CAN_RST_ALL			(RMP_CAN_RST_RIGHT | \
					 RMP_CAN_RST_LEFT | \
					 RMP_CAN_RST_YAW | \
					 RMP_CAN_RST_FOREAFT)

#define RMP_COUNT_PER_M			33215
#define RMP_COUNT_PER_DEG		7.8
#define RMP_COUNT_PER_M_PER_S		332
#define RMP_COUNT_PER_DEG_PER_S		7.8
#define RMP_COUNT_PER_MM_PER_S		0.32882963
#define RMP_COUNT_PER_DEG_PER_SS	7.8
#define RMP_COUNT_PER_REV               112644

#define RMP_MAX_TRANS_VEL_MM_S		3576
#define RMP_MAX_ROT_VEL_DEG_S		18	// from rmi_demo: 1300*0.013805056
#define RMP_MAX_TRANS_VEL_COUNT		1176
#define RMP_MAX_ROT_VEL_COUNT		1024

#define RMP_GEOM_WHEEL_SEP 0.54

// this holds all the RMP data it gives us
class rmp_frame_t
{
  public:
    int16_t pitch;
    int16_t pitch_dot;
    int16_t roll;
    int16_t roll_dot;
    uint32_t yaw;
    int16_t yaw_dot;
    uint32_t left;
    int16_t left_dot;
    uint32_t right;
    int16_t right_dot;
    uint32_t foreaft;

    uint16_t frames;
    uint16_t battery;
    uint8_t  ready;

    rmp_frame_t() : ready(0) {}

    // Adds a new packet to this frame
    void AddPacket(const CanPacket &pkt);

    // Is this frame ready (i.e., did we get all 5 messages)?
    bool IsReady() { return ready == 0x1F; }

};


/* Takes a CAN packet from the RMP and parses it into a
 * rmp_frame_t struct.  sets the ready bitfield 
 * depending on which CAN packet we have.  when
 * ready == 0x1F, then we have gotten 5 packets, so everything
 * is filled in.
 *
 * returns: 
 */
inline void
rmp_frame_t::AddPacket(const CanPacket &pkt)
{
  bool known = true;

  switch(pkt.id) 
  {
    case RMP_CAN_ID_MSG1:
      battery = pkt.GetSlot(2);
      break;

    case RMP_CAN_ID_MSG2:
      pitch = pkt.GetSlot(0);
      pitch_dot = pkt.GetSlot(1);
      roll =  pkt.GetSlot(2);
      roll_dot =  pkt.GetSlot(3);
      break;

    case RMP_CAN_ID_MSG3:
      left_dot = (int16_t) pkt.GetSlot(0);
      right_dot = (int16_t) pkt.GetSlot(1);
      yaw_dot = (int16_t) pkt.GetSlot(2);
      frames = pkt.GetSlot(3);
      break;

    case RMP_CAN_ID_MSG4:
      left = (uint32_t)(((uint32_t)pkt.GetSlot(1) << 16) | 
                        (uint32_t)pkt.GetSlot(0));
      right = (uint32_t)(((uint32_t)pkt.GetSlot(3) << 16) | 
                         (uint32_t)pkt.GetSlot(2));
      break;

    case RMP_CAN_ID_MSG5:
      foreaft = (uint32_t)(((uint32_t)pkt.GetSlot(1) << 16) | 
                           (uint32_t)pkt.GetSlot(0));
      yaw = (uint32_t)(((uint32_t)pkt.GetSlot(3) << 16) | 
                       (uint32_t)pkt.GetSlot(2));
      break;
    default:
      known = false;
      break;
  }

  // now set the ready flags
  if(known) 
    ready |= (1 << (pkt.id & 0xF));
}

CDevice* SegwayRMP::instance = NULL;

// instance method.  use it instead of the constructor, to ensure that we only
// get one instance per CAN interface.
CDevice* 
SegwayRMP::Instance(ConfigFile* cf, int section)
{
  if(!SegwayRMP::instance)
    SegwayRMP::instance = new SegwayRMP();

  ((SegwayRMP*)SegwayRMP::instance)->ProcessConfigFile(cf,section);

  return(SegwayRMP::instance);
}

SegwayRMP::SegwayRMP()
    : CDevice(sizeof(player_segwayrmp_data_t), 
              sizeof(player_segwayrmp_cmd_t), 10, 10)
{
  this->caniotype = "kvaser";
  this->max_xspeed = RMP_DEFAULT_MAX_XSPEED;
  this->max_yawspeed = RMP_DEFAULT_MAX_YAWSPEED;
}

void
SegwayRMP::ProcessConfigFile(ConfigFile* cf, int section)
{
  this->canio = NULL;
  this->caniotype = cf->ReadString(section, "canio", this->caniotype);
  this->max_xspeed = cf->ReadInt(section, "max_xspeed", this->max_xspeed);
  if(this->max_xspeed < 0)
    this->max_xspeed = -this->max_xspeed;
  this->max_yawspeed = cf->ReadInt(section, "max_yawspeed", this->max_yawspeed);
  if(this->max_yawspeed < 0)
    this->max_yawspeed = -this->max_yawspeed;
}

SegwayRMP::~SegwayRMP()
{
  //  this->instance = NULL;
  delete SegwayRMP::instance;
}

int
SegwayRMP::Setup()
{
  player_segwayrmp_cmd_t cmd;

  memset(&cmd,0,sizeof(cmd));

  PutCommand((void*)this,(unsigned char*)&cmd,sizeof(cmd));

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
  player_segwayrmp_cmd_t cmd;
  void *client;
  CanPacket pkt;
  int32_t xspeed,yawspeed;

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
    //
    // now give this data to clients
    PutData((unsigned char *)&data, sizeof(data), 0, 0);
    
    // check for config requests
    if((buffer_len = GetConfig(&client, (void *)buffer, sizeof(buffer))) > 0)
    {
      // if we write to the CAN bus as a result of the config, don't write
      // a velocity command (may need to make this smarter if we get slow
      // velocity control).
      if(HandleConfig(client,buffer,buffer_len) > 0)
        continue;
    }

    // start with the last commanded values
    xspeed = this->last_xspeed;
    yawspeed = this->last_yawspeed;
   
    if(GetCommand((unsigned char *)&cmd, sizeof(cmd)))
    {
      // zero the command buffer, so that we can timeout if a client doesn't
      // send commands for a while
      Lock();
      this->device_used_commandsize = 0; 
      Unlock();

      if(!cmd.code)
      {
        timeout_counter=0;
        xspeed = 0;
        yawspeed = 0;
      }
      else if(cmd.code == PLAYER_POSITION_CODE)
      {
        // convert to host order; let MakeVelocityCommand do the rest
        xspeed = ntohl(cmd.position_cmd.xspeed);
        yawspeed = ntohl(cmd.position_cmd.yawspeed);
        timeout_counter=0;
      }
      else if(cmd.code == PLAYER_POSITION3D_CODE)
      {
        // convert to host order; let MakeVelocityCommand do the rest
        // Position3d uses milliradians/sec, so convert here to
        // degrees/sec
        xspeed = ntohl(cmd.position3d_cmd.xspeed);
        yawspeed = (int32_t) (((double) (int32_t) ntohl(cmd.position3d_cmd.yawspeed)) / 1000 * 180 / M_PI);
        timeout_counter=0;
      }
      else
      {
        PLAYER_ERROR1("can't parse commands for interface %d", cmd.code);
        xspeed = 0;
        yawspeed = 0;
        timeout_counter=0;
      }
    }
    else
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
SegwayRMP::HandleConfig(void* client, unsigned char* buffer, size_t len)
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
        this->motor_enabled = true;
      else
        this->motor_enabled = false;

      printf("SEGWAYRMP: motors state: %d\n", this->motor_enabled);

      if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK))
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

      if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, 
                  sizeof(geom)))
        PLAYER_ERROR("Segway: failed to PutReply");
      break;

    case PLAYER_POSITION_RESET_ODOM_REQ:
      // we'll reset all the integrators

      MakeStatusCommand(&pkt, (uint16_t)RMP_CAN_CMD_RST_INT, 
                        (uint16_t)RMP_CAN_RST_ALL);
      if(Write(pkt) < 0)
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {

	if (Write(pkt) < 0) {
	  if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
	    PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
	} else {
	  if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK))
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
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK))
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
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK))
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
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK))
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
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK))
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
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK))
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
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
	if (Write(pkt) < 0) {
	  if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
	    PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
	} else {
	  if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK))
	    PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
	}
      }
      // return 1 to indicate that we wrote to the CAN bus this time
      return(1);

    case PLAYER_POSITION_RMP_SHUTDOWN:
      MakeShutdownCommand(&pkt);

      if(Write(pkt) < 0)
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      // return 1 to indicate that we wrote to the CAN bus this time
      return(1);

    default:
      printf("segwayrmp received unknown config request %d\n", 
             buffer[0]);
      if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
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
  data.position_data.xpos = htonl((int32_t)(this->odom_x * 1000.0));
  data.position_data.ypos = htonl((int32_t)(this->odom_y * 1000.0));
  data.position_data.yaw = htonl((int32_t)RTOD(this->odom_yaw));
  
  // combine left and right wheel velocity to get foreward velocity
  // change from counts/s into mm/s
  data.position_data.xspeed = 
    htonl((uint32_t)rint(((double)data_frame->left_dot +
                          (double)data_frame->right_dot) /
                         (double)RMP_COUNT_PER_M_PER_S 
                         * 1000.0 / 2.0));
  
  // no side speeds for this bot
  data.position_data.yspeed = 0;
  
  // from counts/sec into deg/sec.  also, take the additive
  // inverse, since the RMP reports clockwise angular velocity as
  // positive.
  data.position_data.yawspeed =
    htonl((int32_t)(-rint((double)data_frame->yaw_dot / 
                          (double)RMP_COUNT_PER_DEG_PER_S)));
  
  data.position_data.stall = 0;
  
  // now, do 3D info.
  data.position3d_data.xpos = htonl((int32_t)(this->odom_x * 1000.0));
  data.position3d_data.ypos = htonl((int32_t)(this->odom_y * 1000.0));
  // this robot doesn't fly
  data.position3d_data.zpos = 0;
  
  // normalize angles to [0,360]
  tmp = NORMALIZE(DTOR((double)data_frame->roll /
                       (double)RMP_COUNT_PER_DEG));
  if(tmp < 0)
    tmp += 2*M_PI;
  data.position3d_data.roll = htonl((int32_t)rint(RTOD(tmp) * 3600.0));
  
  // normalize angles to [0,360]
  tmp = NORMALIZE(DTOR((double)data_frame->pitch /
                       (double)RMP_COUNT_PER_DEG));
  if(tmp < 0)
    tmp += 2*M_PI;
  data.position3d_data.pitch = htonl((int32_t)rint(RTOD(tmp) * 3600.0));
  
  data.position3d_data.yaw = 
    htonl(((int32_t)(RTOD(this->odom_yaw) * 3600.0)));
  
  // combine left and right wheel velocity to get foreward velocity
  // change from counts/s into mm/s
  data.position3d_data.xspeed = 
    htonl((uint32_t)rint(((double)data_frame->left_dot +
                          (double)data_frame->right_dot) /
                         (double)RMP_COUNT_PER_M_PER_S 
                         * 1000.0 / 2.0));
  // no side or vertical speeds for this bot
  data.position3d_data.yspeed = 0;
  data.position3d_data.zspeed = 0;
  
  data.position3d_data.rollspeed = 
    htonl((int32_t)rint((double)data_frame->roll_dot /
                        (double)RMP_COUNT_PER_DEG_PER_S
                        * 3600.0));
  data.position3d_data.pitchspeed = 
    htonl((int32_t)rint((double)data_frame->pitch_dot /
                        (double)RMP_COUNT_PER_DEG_PER_S
                        * 3600.0));
  // from counts/sec into millirad/sec.  also, take the additive
  // inverse, since the RMP reports clockwise angular velocity as
  // positive.

  // This one uses left_dot and right_dot, which comes from odometry
  data.position3d_data.yawspeed = 
    htonl((int32_t)(rint((double)(data_frame->right_dot - data_frame->left_dot) /
                         (RMP_COUNT_PER_M_PER_S * RMP_GEOM_WHEEL_SEP * M_PI) * 1000)));
  // This one uses yaw_dot, which comes from the IMU
  //data.position3d_data.yawspeed = 
  //  htonl((int32_t)(-rint((double)data_frame->yaw_dot / 
  //                        (double)RMP_COUNT_PER_DEG_PER_S * M_PI / 180 * 1000)));
  
  data.position3d_data.stall = 0;
  
  // fill in power data.  the RMP returns a percentage of full,
  // and the specs for the HT say that it's a 72 volt system.  assuming
  // that the RMP is the same, we'll convert to decivolts for Player.
  data.power_data.charge = 
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
