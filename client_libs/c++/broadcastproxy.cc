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
 * $Id$
 *
 * client-side position device 
 */

#include <positionproxy.h>
#include <netinet/in.h>
#include <string.h>
#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif


///////////////////////////////////////////////////////////////////////////
// Constructor
BroadcastProxy::BroadcastProxy(PlayerClient* pc, unsigned short index, unsigned char access)
        : ClientProxy(pc, PLAYER_POSITION_CODE, index, access)
{
    memset(this->cmd, 0, sizeof(this->cmd));
    memset(this->data, 0, sizeof(this->data));
}


///////////////////////////////////////////////////////////////////////////
// Destructor
BroadcastProxy::~BroadcastProxy()
{
}


///////////////////////////////////////////////////////////////////////////
// Read a message from the incoming queue
// Returns -1 if there are no available messages
int BroadcastProxy::Read(uint8_t *msg, uint16_t *len)
{
    if (this->data.len <= 0)
        return -1;

    // Get the next message in the queue
    uint16_t _len = *((uint16_t*) this->data.buffer);
    uint8_t *_msg = this->data.buffer;

    // Make copy of message
    assert(len >= _len);
    memcpy(msg, _msg, _len);

    // Now move everything in the queue down
    memmove(this->data.buffer, this->data.buffer + _len,
            this->data.len - _len - sizeof(_len));
    this->data.len -= _len;

    return 0;
}


///////////////////////////////////////////////////////////////////////////
// Write a message to the outgoing queue
// Returns -1 if the queue is full
int BroadcastProxy::Write(uint8_t *msg, uint16_t len)
{
    // Check for overflow
    // Need to leave 2 bytes for end-of-list marker
    if (this->cmd.len + len + sizeof(len) + sizeof(len) > sizeof(this->cmd.buffer))
        return -1;

    memcpy(this->cmd.buffer + this->cmd.len, &len, sizeof(len));
    memcpy(this->cmd.buffer + this->cmd.len + sizeof(len), msg, len);
    this->cmd.len += sizeof(len) + len;
}


///////////////////////////////////////////////////////////////////////////
// Flush the outgoing message queue
int BroadcastProxy::Flush()
{
    // Write our command
    if (this->client->Write(PLAYER_BROADCAST_COST, this->index,
                            &this->cmd, this->cmd.len + sizeof(this->cmd.len)) < 0)
        return -1;

    // Reset the command buffer
    this->cmd.len = 0;
}


///////////////////////////////////////////////////////////////////////////
// Update the incoming queue
void PositionProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
    this->data.len = ((player_broadcast_data_t*) buffer)->len;
    memcpy(&this->data.buffer, buffer, this->data.len);
}




// send a motor command
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int PositionProxy::SetSpeed(short speed, short turnrate)
{
  if(!client)
    return(-1);

  player_position_cmd_t cmd;

  cmd.speed = htons(speed);
  cmd.turnrate = htons(turnrate);

  return(client->Write(PLAYER_POSITION_CODE,index,
                       (const char*)&cmd,sizeof(cmd)));
}

// enable/disable the motors
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int PositionProxy::SetMotorState(unsigned char state)
{
  if(!client)
    return(-1);

  char buffer[2];

  buffer[0] = PLAYER_POSITION_MOTOR_POWER_REQ;
  buffer[1] = state;


  return(client->Request(PLAYER_POSITION_CODE,index,(const char*)buffer,
                         sizeof(buffer)));
}

// select velocity control mode for the Pioneer 2
//   0 = direct wheel velocity control (default)
//   1 = separate translational and rotational control
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int PositionProxy::SelectVelocityControl(unsigned char mode)
{
  if(!client)
    return(-1);

  char buffer[2];

  buffer[0] = PLAYER_POSITION_VELOCITY_CONTROL_REQ;
  buffer[1] = mode;

  return(client->Request(PLAYER_POSITION_CODE,index,(const char*)buffer,
                         sizeof(buffer)));
}

// reset odometry to (0,0,0)
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int PositionProxy::ResetOdometry()
{
  if(!client)
    return(-1);

  char buffer[1];

  buffer[0] = PLAYER_POSITION_RESET_ODOM_REQ;

  return(client->Request(PLAYER_POSITION_CODE,index,(const char*)buffer,
                         sizeof(buffer)));
}

void PositionProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  if(hdr.size != sizeof(player_position_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of position data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_position_data_t),hdr.size);
  }

  xpos = (int)ntohl(((player_position_data_t*)buffer)->xpos);
  ypos = (int)ntohl(((player_position_data_t*)buffer)->ypos);
  theta = ntohs(((player_position_data_t*)buffer)->theta);
  speed = (short)ntohs(((player_position_data_t*)buffer)->speed);
  turnrate = (short)ntohs(((player_position_data_t*)buffer)->turnrate);
  compass = ntohs(((player_position_data_t*)buffer)->compass);
  stalls = ((player_position_data_t*)buffer)->stalls;
}

// interface that all proxies SHOULD provide
void PositionProxy::Print()
{
  printf("#Position(%d:%d) - %c\n", device, index, access);
  puts("#xpos\typos\ttheta\tspeed\tturn\tcompass\tstalls");
  printf("%d\t%d\t%u\t%d\t%d\t%u\t%d\n", 
         xpos,ypos,theta,speed,turnrate,compass,stalls);
}

