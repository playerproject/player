/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2006 -
 *     Brian Gerkey
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


#include <unistd.h>

#include <libplayercore/playercore.h>

#include "roomba_comms.h"

#define CYCLE_TIME_US 100000

class Roomba : public Driver
{
  public:
    Roomba(ConfigFile* cf, int section);

    int Setup();
    int Shutdown();

    // MessageHandler
    int ProcessMessage(MessageQueue * resp_queue, 
		       player_msghdr * hdr, 
		       void * data);

  private:
    // Main function for device thread.
    virtual void Main();

    // Serial port where the roomba is
    const char* serial_port;

    // full control or not
    bool safe;

    // The underlying roomba object
    roomba_comm_t* roomba_dev;
};

// a factory creation function
Driver* Roomba_Init(ConfigFile* cf, int section)
{
  return((Driver*)(new Roomba(cf, section)));
}

// a driver registration function
void Roomba_Register(DriverTable* table)
{
  table->AddDriver("roomba", Roomba_Init);
}

Roomba::Roomba(ConfigFile* cf, int section)
    : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_POSITION2D_CODE)
{
  this->serial_port = cf->ReadString(section, "port", "/dev/ttyS0");
  this->safe = cf->ReadInt(section, "safe", 1);
  this->roomba_dev = NULL;
}

int
Roomba::Setup()
{
  this->roomba_dev = roomba_create(this->serial_port);

  if(roomba_open(this->roomba_dev, !this->safe) < 0)
  {
    roomba_destroy(this->roomba_dev);
    this->roomba_dev = NULL;
    PLAYER_ERROR("failed to connect to roomba");
    return(-1);
  }

  this->StartThread();

  return(0);
}

int
Roomba::Shutdown()
{
  this->StopThread();

  if(roomba_close(this->roomba_dev))
  {
    PLAYER_ERROR("failed to close roomba connection");
  }
  roomba_destroy(this->roomba_dev);
  this->roomba_dev = NULL;
  return(0);
}

void
Roomba::Main()
{
  player_position2d_data_t posdata;

  for(;;)
  {
     this->ProcessMessages();

     if(roomba_get_sensors(this->roomba_dev, -1) < 0)
     {
       PLAYER_ERROR("failed to get sensor data from roomba");
       roomba_close(this->roomba_dev);
       return;
     }

     memset(&posdata,0,sizeof(posdata));

     posdata.pos.px = this->roomba_dev->ox;
     posdata.pos.py = this->roomba_dev->oy;
     posdata.pos.pa = this->roomba_dev->oa;

     this->Publish(this->device_addr, NULL,
                   PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE,
                   (void*)&posdata, sizeof(posdata), NULL);
     usleep(CYCLE_TIME_US);
  }
}

int
Roomba::ProcessMessage(MessageQueue * resp_queue, 
		       player_msghdr * hdr, 
		       void * data)
{
  if(Message::MatchMessage(hdr,
                           PLAYER_MSGTYPE_CMD,
                           PLAYER_POSITION2D_CMD_VEL,
                           this->device_addr))
  {
    // get and send the latest motor command
    player_position2d_cmd_vel_t position_cmd;
    position_cmd = *(player_position2d_cmd_vel_t*)data;
    if(roomba_set_speeds(this->roomba_dev, 
                         position_cmd.vel.px, 
                         position_cmd.vel.pa) < 0)
    {
      PLAYER_ERROR("failed to set speeds to roomba");
    }
    return(0);
  }
  return(-1);
}
