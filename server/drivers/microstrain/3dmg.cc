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
 * Desc: Driver for the MicroStrain 3DM-G IMU
 * Author: Andrew Howard
 * Date: 19 Nov 2002
 * CVS: $Id$
 */

#if HAVE_CONFIG_H
  #include <config.h>
#endif

//#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include "playercommon.h"
#include "drivertable.h"
#include "player.h"


// MicroStraing 3DM-G IMU driver
class MicroStrain3DMG : public CDevice
{
  // Constructor
  public: MicroStrain3DMG(char* interface, ConfigFile* cf, int section);

  // Initialise device
  public: virtual int Setup();

  // Shutdown the device
  public: virtual int Shutdown();

  // Main function for device thread.
  private: virtual void Main();

  // Open the serial port
  // Returns 0 on success
  private: int OpenPort();

  // Close the serial port
  // Returns 0 on success
  private: int ClosePort();
    
  // Name of port used to communicate with the laser;
  // e.g. /dev/ttyS1
  private: const char *port_name;

  // Port file descriptor
  private: int fd;
};


// Factory creation function
CDevice* MicroStrain3DMG_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_POSITION_STRING))
  {
    PLAYER_ERROR1("driver \"MicroStrain3DMG\" does not support interface \"%s\"\n",
                  interface);
    return(NULL);
  }
  else
    return ((CDevice*) (new MicroStrain3DMG(interface, cf, section)));
}

// Driver registration function
void MicroStrain3DMG_Register(DriverTable* table)
{
  table->AddDriver("MicroStrain3DMG", PLAYER_READ_MODE, MicroStrain3DMG_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Device codes



////////////////////////////////////////////////////////////////////////////////
// Constructor
MicroStrain3DMG::MicroStrain3DMG(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_position_data_t), 0, 0, 0)
{
  // Default serial port
  this->port_name = cf->ReadString(section, "port", "/dev/ttyS1");

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device
int MicroStrain3DMG::Setup()
{   
  printf("IMU initialising (%s)\n", this->port_name);
    
  // Open the port
  if (OpenPort())
    return -1;
  
  // Start driver thread
  StartThread();

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int MicroStrain3DMG::Shutdown()
{
  // Stop driver thread
  StopThread();

  // Close the port
  ClosePort();

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void MicroStrain3DMG::Main() 
{
  while (true)
  {
    // Test if we are supposed to cancel
    pthread_testcancel();
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Open the terminal
// Returns 0 on success
int MicroStrain3DMG::OpenPort()
{
  // Open the port
#if PLAYER_LINUX
  this->fd = open(this->port_name, O_RDWR | O_SYNC , S_IRUSR | S_IWUSR );
#else
  this->fd = open(this->port_name, O_RDWR, S_IRUSR | S_IWUSR );
#endif
  if (this->fd < 0)
  {
    PLAYER_ERROR2("unable to open serial port [%s]; [%s]",
                  (char*) this->port_name, strerror(errno));
    return -1;
  }

  // Change port settings
  struct termios term;
  if (tcgetattr(this->fd, &term) < 0)
  {
    PLAYER_ERROR("Unable to get serial port attributes");
    return -1;
  }

#if HAVE_CFMAKERAW
  cfmakeraw( &term );
#endif
  cfsetispeed(&term, B38400);
  cfsetospeed(&term, B38400);

  if (tcsetattr(this->fd, TCSAFLUSH, &term) < 0 )
  {
    PLAYER_ERROR("Unable to set serial port attributes");
    return -1;
  }

  // Make sure queues are empty before we begin
  tcflush(this->fd, TCIOFLUSH);
    
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Close the serial port
// Returns 0 on success
int MicroStrain3DMG::ClosePort()
{
  close(this->fd);
  return 0;
}


