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


#include "device.h"
#include "devicetable.h"
#include "drivertable.h"
#include "player.h"
#include "segwayio.h"


class RMPPower : public CDevice
{
public:
  RMPPower(char *interface, ConfigFile *cf, int section);

  size_t GetData(void *, unsigned char *dest, size_t maxsize,
		 uint32_t *ts_sec, uint32_t *ts_usec);

  virtual int Setup();
  virtual int Shutdown();
  
protected:
  SegwayIO *segway;

};

CDevice *
RMPPower_Init(char *interface, ConfigFile *cf, int section)
{
  if (strcmp(interface, PLAYER_POWER_STRING) != 0) {
    PLAYER_ERROR1("driver \"rmppower\" does not support interface "
		  "\"%s\"\n", interface);
    return NULL;
  }

  return ((CDevice *) (new RMPPower(interface, cf, section)));
}

void
RMPPower_Register(DriverTable *table)
{
  table->AddDriver("rmppower", PLAYER_READ_MODE, RMPPower_Init);
}

RMPPower::RMPPower(char *interface, ConfigFile *cf, int section) :
  CDevice(sizeof(player_power_data_t), 
	  sizeof(player_power_data_t), 5, 5), segway(NULL)
{
}

int
RMPPower::Setup()
{
  int ret;

  segway = SegwayIO::Instance();

  if ((ret = segway->Init()) < 0) {
    PLAYER_ERROR1("RMPPOWER: error on segwayIO init (%d)\n",ret);
    return -1;
  }

  return 0;
}

int
RMPPower::Shutdown()
{
  int ret;

  if ((ret = segway->Shutdown()) < 0) {
    PLAYER_ERROR1("RMPPOWER: error on canio shutdown (%d)\n",ret);
    return -1;
  }

  return 0;
}

size_t
RMPPower::GetData(void *client, unsigned char *dest, size_t maxsize,
		  uint32_t *ts_sec, uint32_t *ts_usec)
{
  if (maxsize < sizeof(player_power_data_t)) {
    return 0;
  }

  segway->GetData( (player_power_data_t *)dest);

  return sizeof(player_power_data_t);
}
