/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003  Brian Gerkey gerkey@usc.edu
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

#include "player.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"

#include "canio.h"
#include "canio_kvaser.h"

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
//#define RMP_COUNT_PER_DEG_PER_SS	72.437229
#define RMP_COUNT_PER_DEG_PER_SS	7.8
#define RMP_COUNT_PER_REV               112644

#define RMP_MAX_TRANS_VEL_MM_S		3576
#define RMP_MAX_ROT_VEL_DEG_S		18	// from rmi_demo: 1300*0.013805056
#define RMP_MAX_TRANS_VEL_COUNT		1176
#define RMP_MAX_ROT_VEL_COUNT		1024

#define RMP_READ_WRITE_PERIOD		500	// 10 ms period = 100 Hz

// this holds all the RMP data it gives us
class rmp_frame_t
{
  public:
    int16_t     	pitch;
    int16_t	pitch_dot;
    int16_t     	roll;
    int16_t     	roll_dot;
    int32_t      	yaw;
    int16_t	yaw_dot;
    int32_t      	left;
    int16_t      	left_dot;
    int32_t      	right;
    int16_t      	right_dot;
    int32_t      	foreaft;

    uint16_t     	frames;
    uint16_t	battery;
    uint8_t	ready;

  /*  rmp_frame_t() : pitch(0.0), pitch_dot(0.0),
      roll(0.0), roll_dot(0.0),
      yaw_dot(0.0), left(0),
      left_dot(0), right(0),
      right_dot(0), travel(0),
      turn(0), frames(0),
      battery(0), ready(0) {}
  */
    rmp_frame_t() : ready(0) {}

    void AddPacket(const CanPacket &pkt);
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
      yaw_dot = pkt.GetSlot(2);
      frames = pkt.GetSlot(3);
      break;

    case RMP_CAN_ID_MSG4:
      left = (int32_t)(((uint32_t)pkt.GetSlot(1) << 16) | 
                       (uint32_t)pkt.GetSlot(0));
      right = (int32_t)(((uint32_t)pkt.GetSlot(3) << 16) | 
                        (uint32_t)pkt.GetSlot(2));
      break;

    case RMP_CAN_ID_MSG5:
      foreaft = (int32_t) (((uint32_t)pkt.GetSlot(1) << 16) | 
                           (uint32_t)pkt.GetSlot(0));
      yaw = (int32_t) (((uint32_t)pkt.GetSlot(3) << 16) | 
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


// Driver for robotic Segway
class SegwayRMP : public CDevice
{
  public: 
    // Constructor	  
    SegwayRMP(char* interface, ConfigFile* cf, int section);
    ~SegwayRMP();

    // Setup/shutdown routines.
    virtual int Setup();
    virtual int Shutdown();

  protected:
    player_position_data_t position_data;
    player_power_data_t power_data;

  private: 
    DualCANIO *canio;

    int16_t last_xspeed, last_yawspeed;

    bool motor_enabled;

    const char* caniotype;

    // helper to handle config requests
    int HandleConfig(void* client, unsigned char* buffer, size_t len);

    // helper to read a cycle of data from the RMP
    int Read();

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

// Initialization function
CDevice* SegwayRMP_Init(char* interface, ConfigFile* cf, int section)
{
  if (strcmp(interface, PLAYER_POSITION_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"segwayrmp\" does not support interface \"%s\"\n",
                  interface);
    return (NULL);
  }

  return ((CDevice*) (new SegwayRMP(interface, cf, section)));
}


// a driver registration function
void SegwayRMP_Register(DriverTable* table)
{
  table->AddDriver("segwayrmp", PLAYER_ALL_MODE, SegwayRMP_Init);
}

SegwayRMP::SegwayRMP(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_position_data_t), 
              sizeof(player_position_cmd_t), 10, 10)
{
  last_xspeed = last_yawspeed = 0;
  canio = NULL;
  motor_enabled = false;

  caniotype = cf->ReadString(section, "canio", "kvaser");
}

SegwayRMP::~SegwayRMP()
{
  if(canio)
    delete canio;
}

int
SegwayRMP::Setup()
{
  printf("CAN bus initializing...");
  fflush(stdout);

  if(!strcmp(caniotype, "kvaser"))
    assert(canio = new CANIOKvaser);
  else
  {
    PLAYER_ERROR1("Unknown CAN I/O type: \"%s\"", caniotype);
    return(-1);
  }

  // start the CAN at 500 kpbs
  if(canio->Init(BAUD_500K) < 0) 
  {
    PLAYER_ERROR("error on CAN Init");
    return(-1);
  }

  StartThread();

  puts("done.");

  return(0);
}

int
SegwayRMP::Shutdown()
{
  // send zero velocities, for some semblance of safety
  CanPacket pkt;

  MakeVelocityCommand(&pkt,0,0);
  Write(pkt);

  // shutdown the CAN
  canio->Shutdown();
  StopThread();
  delete canio;
  motor_enabled = false;
  last_xspeed = last_yawspeed = 0;
  
  return(0);
}

// Main function for device thread.
void 
SegwayRMP::Main()
{
  unsigned char buffer[256];
  size_t buffer_len;
  player_position_cmd_t cmd;
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
    PutData((unsigned char *)&position_data, sizeof(position_data), 0, 0);
    
    // check for config requests
    if((buffer_len = GetConfig(&client, (void *)buffer, sizeof(buffer))) > 0)
    {
      // if we write to the CAN bus as a result of the config, don't write
      // a velocity command (may need to make this smarter if we get slow
      // velocity control).
      if(HandleConfig(client,buffer,buffer_len) > 0)
        continue;
    }

    // Get a new command
    GetCommand((unsigned char *)&cmd, sizeof(cmd));

    if(motor_enabled) 
    {
      //      printf("SEGWAYRMP: motors_enabled xs: %d ys: %d\n",
      //	     ntohl(cmd.xspeed), ntohl(cmd.yspeed));

      // convert to host order; let MakeVelocityCommand do the rest
      xspeed = ntohl(cmd.xspeed);
      yawspeed = ntohl(cmd.yawspeed);
    }
    else 
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
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
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
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
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
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
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
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
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
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
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
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
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
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      // return 1 to indicate that we wrote to the CAN bus this time
      return(1);

    case PLAYER_POSITION_RMP_SHUTDOWN:
      MakeShutdownCommand(&pkt);

      if(Write(pkt) < 0)
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK))
          PLAYER_ERROR("SEGWAY: Failed to PutReply\n");
      }
      else
      {
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK))
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
        // only bother doing the data conversions for one channel, picking
        // 0 arbitrarily
        if(channel == 0)
        {
          // xpos is fore/aft integrated position?
          // change from counts to mm
          position_data.xpos = 
                  htonl((uint32_t)rint((double)data_frame[channel].foreaft / 
                                       ((double)RMP_COUNT_PER_M/1000.0)));

          // ypos is going to be pitch for now...
          // change from counts to milli-degrees
          position_data.ypos = 
                  htonl((uint32_t)rint(((double)data_frame[channel].pitch / 
                                        (double)RMP_COUNT_PER_DEG) * 1000.0));

          // yaw is integrated yaw
          // not sure about this one..
          // from counts/rev to degrees.  one rev is 360 degree?
          position_data.yaw = 
                  htonl((uint32_t) rint(((double)data_frame[channel].yaw /
                                         (double)RMP_COUNT_PER_REV) * 360.0));

          // don't know the conversion yet...
          // change from counts/m/s into mm/s
          position_data.xspeed = 
                  htonl((uint32_t)rint(((double)data_frame[channel].left_dot / 
                                        (double)RMP_COUNT_PER_M_PER_S)*1000.0 ));

          position_data.yspeed = 
                  htonl((uint32_t)rint(((double)data_frame[channel].right_dot / 
                                        (double)RMP_COUNT_PER_M_PER_S)*1000.0));

          // from counts/deg/sec into mill-deg/sec
          position_data.yawspeed = 
                  htonl((uint32_t)(rint(((double)data_frame[channel].yaw_dot / 
                                         (double)RMP_COUNT_PER_DEG_PER_S))*1000.0) );

          position_data.stall = 0;

          // TODO: fill in power_data
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

int
SegwayRMP::Write(CanPacket& pkt)
{
  //    printf("SEGWAYIO: %d ms WRITE: %s\n", loopmillis, pkt.toString());

  // write it out!
  return(canio->WritePacket(pkt));
}

/* Creates a status CAN packet from the given arguments
 */  
void
SegwayRMP::MakeStatusCommand(CanPacket* pkt, uint16_t cmd, uint16_t val)
{
  pkt->id = RMP_CAN_ID_COMMAND;
  pkt->PutSlot(2, cmd);
  //  pkt.PutSlot(3, val);
  pkt->PutByte(6, val);
  pkt->PutByte(7, val);

  // put in the last speed commands as well
  pkt->PutSlot(0,(uint16_t)last_xspeed);
  pkt->PutSlot(1,(uint16_t)last_yawspeed);
  
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

  int16_t trans = (int16_t) rint((double)xspeed * RMP_COUNT_PER_MM_PER_S);

  if(trans > RMP_MAX_TRANS_VEL_COUNT)
    trans = RMP_MAX_TRANS_VEL_COUNT;
  else if(trans < -RMP_MAX_TRANS_VEL_COUNT)
    trans = -RMP_MAX_TRANS_VEL_COUNT;

  last_xspeed = trans;

  // rotational RMP command \in [-1024, 1024]
  // this is ripped from rmi_demo... to go from deg/s^2 to counts
  // deg/s^2 -> count = 1/0.013805056
  int16_t rot = (int16_t) rint((double)yawspeed * RMP_COUNT_PER_DEG_PER_SS);

  if(rot > RMP_MAX_ROT_VEL_COUNT)
    rot = RMP_MAX_ROT_VEL_COUNT;
  else if(rot < -RMP_MAX_ROT_VEL_COUNT)
    rot = -RMP_MAX_ROT_VEL_COUNT;

  last_yawspeed = rot;
  
  /*
  if (rot || trans) {
    printf("SEGWAYIO: trans: %d rot: %d\n", trans_command, rot_command);
  }
  */

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
