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

#include "player.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"

#define DEFAULT_SEGWAYRMP_PORT "/dev/can0"

// copied from can.h
#define CAN_MSG_LENGTH 8

struct canmsg_t 
{       
  short           flags;
  int             cob;
  unsigned long   id;
  unsigned long   timestamp;
  unsigned int    length;
  char            data[CAN_MSG_LENGTH];
} __attribute__ ((packed));



// Driver for robotic Segway
class SegwayRMP : public CDevice
{
  private: 
    // name of can port
    const char* can_port;
    // file descriptor to can port
    int can_fd;
    // Main function for device thread.
    virtual void Main();

  public: 
    // Constructor	  
    SegwayRMP(char* interface, ConfigFile* cf, int section);

    // Setup/shutdown routines.
    virtual int Setup();
    virtual int Shutdown();
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
  can_port = cf->ReadString(section, "port", DEFAULT_SEGWAYRMP_PORT);
  can_fd=-1;
}

int
SegwayRMP::Setup()
{
  if(can_fd >= 0)
    close(can_fd);

  if((can_fd = open(can_port, O_RDWR)) < 0)
  {
    PLAYER_ERROR2("couldn't open CAN port %s: %s", can_port,strerror(errno));
    return(-1);
  }
  printf("opened port %s\n", can_port);
  StartThread();
  return(0);
}

int
SegwayRMP::Shutdown()
{
  StopThread();
  if(can_fd >= 0)
    close(can_fd);
  return(0);
}

// Main function for device thread.
void 
SegwayRMP::Main()
{
  canmsg_t msg;
  int numread;

  for(;;)
  {
    puts("calling read");
    if((numread = read(can_fd,(void*)&msg, sizeof(msg))) < 0)
    {
      PLAYER_ERROR1("read errored: %s",strerror(errno));
      pthread_exit(NULL);
    }

    printf("read %d bytes\n", numread);
  }
}
