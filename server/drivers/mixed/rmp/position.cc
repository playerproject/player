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
 * $Id$
 *
 * The position (and position3d) interface to the Segway RMP.  This driver
 * just forwards commands to and data from the underlying segway driver.
 */

#include "segwayrmp.h"
#include "player.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"

class SegwayRMPPosition : public CDevice
{
  private:
    uint16_t interface_code;
    CDevice* segwayrmp;

  public:
    SegwayRMPPosition(uint16_t code, ConfigFile* cf, int section);
    ~SegwayRMPPosition();

    virtual int Setup();
    virtual int Shutdown();

    virtual void PutCommand(void* client, unsigned char* src, size_t len);

    virtual void Main();
};

// Initialization function
CDevice* SegwayRMPPosition_Init(char* interface, ConfigFile* cf, int section)
{
  uint16_t code;

  if(!strcmp(interface, PLAYER_POSITION_STRING))
    code = PLAYER_POSITION_CODE;
  else if(!strcmp(interface, PLAYER_POSITION3D_STRING))
    code = PLAYER_POSITION3D_CODE;
  else
  {
    PLAYER_ERROR1("driver \"segwayrmpposition\" does not support "
                  "interface \"%s\"\n", interface);
    return (NULL);
  }

  return((CDevice*)(new SegwayRMPPosition(code,cf,section)));
}

// a driver registration function
void SegwayRMPPosition_Register(DriverTable* table)
{
  table->AddDriver("segwayrmpposition", PLAYER_ALL_MODE, 
                   SegwayRMPPosition_Init);
}

SegwayRMPPosition::SegwayRMPPosition(uint16_t code, 
                                     ConfigFile* cf, 
                                     int section)
        : CDevice(sizeof(player_position3d_data_t), 
                  sizeof(player_position3d_cmd_t), 10, 10)
{
  this->interface_code = code;
  assert(this->segwayrmp = SegwayRMP::Instance(cf,section));
}

SegwayRMPPosition::~SegwayRMPPosition()
{
}

int
SegwayRMPPosition::Setup()
{
  player_position3d_data_t data3d;
  player_position3d_cmd_t cmd3d;

  memset(&data3d,0,sizeof(data3d));
  memset(&cmd3d,0,sizeof(cmd3d));

  PutCommand(this,(unsigned char*)&cmd3d,sizeof(cmd3d));
  PutData((unsigned char*)&data3d,sizeof(data3d),0,0);

  if(this->segwayrmp->Subscribe(this))
    return(-1);

  StartThread();

  return(0);
}

int
SegwayRMPPosition::Shutdown()
{
  StopThread();

  return(this->segwayrmp->Unsubscribe(this));
}

void
SegwayRMPPosition::Main()
{
  player_segwayrmp_data_t data;
  uint32_t time_sec, time_usec;

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

  for(;;)
  {
    pthread_testcancel();

    this->segwayrmp->Wait();

    this->segwayrmp->GetData(this,(unsigned char*)&data,sizeof(data),
                             &time_sec,&time_usec);

    if(this->interface_code == PLAYER_POSITION_CODE)
      PutData(&(data.position_data),sizeof(data.position_data),
              time_sec, time_usec);
    else if(this->interface_code == PLAYER_POSITION3D_CODE)
      PutData(&(data.position3d_data),sizeof(data.position3d_data),
              time_sec, time_usec);
    else
    {
      PLAYER_ERROR1("can't provide data for interface %d; bailing", 
                    interface_code);
      pthread_exit(NULL);
    }
  }
}

void 
SegwayRMPPosition::PutCommand(void* client, unsigned char* src, size_t len)
{
  player_segwayrmp_cmd_t cmd;

  Lock();
  cmd.code = this->interface_code;
  if(this->interface_code==PLAYER_POSITION_CODE)
  {
    cmd.position_cmd = *((player_position_cmd_t*)src);
    this->segwayrmp->PutCommand(client, (unsigned char*)&src, sizeof(cmd));
  }
  else if(this->interface_code==PLAYER_POSITION3D_CODE)
  {
    cmd.position3d_cmd = *((player_position3d_cmd_t*)src);
    this->segwayrmp->PutCommand(client, (unsigned char*)&src, sizeof(cmd));
  }
  Unlock();
}

