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

#include "player.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"

// Driver for robotic Segway
class SegwayRMP : public CDevice
{
  // Constructor
  public: SegwayRMP(char* interface, ConfigFile* cf, int section);

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Main function for device thread.
  private: virtual void Main();
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
              sizeof(player_position_cmd_t), 0, 0)
{
}

int
SegwayRMP::Setup()
{
  return(-1);
}

int
SegwayRMP::Shutdown()
{
  return(-1);
}
