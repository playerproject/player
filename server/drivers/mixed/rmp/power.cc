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
 * The power interface to the Segway RMP.  This driver
 * just forwards commands to and data from the underlying segway driver.
 */

#include "segwayrmp.h"
#include "player.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"

class SegwayRMPPower : public CDevice
{
  private:
    uint16_t interface_code;
    CDevice* segwayrmp;

  public:
    SegwayRMPPower(ConfigFile* cf, int section);
    ~SegwayRMPPower();

    virtual int Setup();
    virtual int Shutdown();

    virtual void Main();
};

// Initialization function
CDevice* SegwayRMPPower_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_POWER_STRING))
  {
    PLAYER_ERROR1("driver \"rmppower\" does not support "
                  "interface \"%s\"\n", interface);
    return (NULL);
  }

  return((CDevice*)(new SegwayRMPPower(cf,section)));
}

// a driver registration function
void SegwayRMPPower_Register(DriverTable* table)
{
  table->AddDriver("rmppower", PLAYER_READ_MODE, 
                   SegwayRMPPower_Init);
}

SegwayRMPPower::SegwayRMPPower(ConfigFile* cf, int section)
        : CDevice(sizeof(player_power_data_t),0, 10, 10)
{
  assert(this->segwayrmp = SegwayRMP::Instance(cf,section));
}

SegwayRMPPower::~SegwayRMPPower()
{
}

int
SegwayRMPPower::Setup()
{
  player_power_data_t data;

  memset(&data,0,sizeof(data));

  PutData((unsigned char*)&data,sizeof(data),0,0);

  if(this->segwayrmp->Subscribe(this))
    return(-1);

  StartThread();

  return(0);
}

int
SegwayRMPPower::Shutdown()
{
  int retval;

  retval = this->segwayrmp->Unsubscribe(this);

  StopThread();

  return(retval);
}

void
SegwayRMPPower::Main()
{
  player_segwayrmp_data_t data;
  uint32_t time_sec, time_usec;
  unsigned char buffer[256];
  size_t buffer_len;
  void* client;
  player_device_id_t id;
  unsigned short reptype;
  struct timeval time;

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

  for(;;)
  {
    pthread_testcancel();

    this->segwayrmp->Wait();

    this->segwayrmp->GetData(this,(unsigned char*)&data,sizeof(data),
                             &time_sec,&time_usec);

    PutData((void*)&(data.power_data),sizeof(data.power_data),
            time_sec, time_usec);

    if((buffer_len = GetConfig(&id, &client,
                               (void*)buffer, sizeof(buffer))) > 0)
    {
      // pass requests on to the underlying device
      if((buffer_len= this->segwayrmp->Request(&id,client,(void*)buffer,
                                               buffer_len,&reptype,&time,
					       (void*)buffer,
					       sizeof(buffer))) < 0)
        PutReply(client, PLAYER_MSGTYPE_RESP_NACK);
      else
        PutReply(&id,client,reptype,&time,(void*)buffer,buffer_len);
    }
  }
}

