/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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

///////////////////////////////////////////////////////////////////////////
//
// File: laserdevice.h
// Author: Andrew Howard
// Date: 7 Nov 2000
// Desc: Driver for the SICK laser
//
// CVS info:
//  $Source$
//  $Author$
//  $Revision$
//
// Usage:
//  (empty)
//
// Theory of operation:
//  (empty)
//
// Known bugs:
//  (empty)
//
// Possible enhancements:
//  (empty)
//
///////////////////////////////////////////////////////////////////////////

#ifndef LASERDEVICE
#define LASERDEVICE
#include <pthread.h>
#include <unistd.h>

#include "lock.h"
#include "device.h"

// Some length-specific types
//
#ifndef BYTE
#define BYTE unsigned char
#endif
#ifndef UINT16
#define UINT16 unsigned short
#endif


// The laser device class
//
class CLaserDevice : public CDevice
{
  public:
    
    CLaserDevice(char *port);
      
    int Setup();
    int Shutdown();
    void Run();
      
    CLock* GetLock( void ) {return &lock;};

    // Client interface
    //
    int GetData( unsigned char * );
    void PutData( unsigned char * );
    void GetCommand( unsigned char * );
    void PutCommand( unsigned char *,int );
    int GetConfig( unsigned char * );
    void PutConfig( unsigned char *,int );
    
  private:

    // Dummy main (just calls real main)
    //
    static void* DummyMain(void *laserdevice);

    // Main function for device thread
    //
    int Main();
    
    // Open the terminal
    // Returns 0 on success
    //
    int OpenTerm();

    // Close the terminal
    // Returns 0 on success
    //
    int CloseTerm();
    
    // Set the terminal speed
    // Valid values are 9600 and 38400
    // Returns 0 on success
    //
    int ChangeTermSpeed(int speed);

    // Put the laser into configuration mode
    //
    int SetLaserMode();

    // Set the laser data rate
    // Valid values are 9600 and 38400
    // Returns 0 on success
    //
    int SetLaserSpeed(int speed);

    // Set the laser configuration
    // Returns 0 on success
    //
    int SetLaserConfig();

    // Request data from the laser
    // Returns 0 on success
    //
    int RequestLaserData();

    // Read laser range data
    // Runs in the device thread.
    //
    int ProcessLaserData();

    // Write a packet to the laser
    //
    ssize_t WriteToLaser(BYTE *data, ssize_t len); 
    
    // Read a packet from the laser
    //
    ssize_t ReadFromLaser(BYTE *data, ssize_t maxlen, bool ack = false, int timeout = -1);

    // Calculates CRC for a telegram
    //
    unsigned short CreateCRC(BYTE *data, ssize_t len);

    // Get the time (in ms)
    //
    int GetTime();
    
  protected:
    bool data_swapped;     // whether or not the data buffer has been byteswapped
    pthread_t thread;           // the thread that continuously reads from the laser 
    CLock lock;

    // laser device file descriptor
    //
    int laser_fd;           

    // Most recent scan data
    //
    BYTE* data;

    // Config data
    // PutConfig sets the data and the size, and GetConfig reads and zeros it
    //
    ssize_t config_size;
    BYTE* config;

    // Turn intensity data on/off
    //
    bool intensity;
    
    // Scan range  (for restricted scan)
    //
    int min_segment, max_segment;

 public:
    
    // device used to communicate with the laserk
    char LASER_SERIAL_PORT[LASER_SERIAL_PORT_NAME_SIZE];
};


#endif
