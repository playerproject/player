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

#ifndef _LASERDEVICE_H
#define _LASERDEVICE_H
#include <pthread.h>
#include <unistd.h>

#include <lock.h>
#include <device.h>

#include <playercommon.h>
#include <messages.h>

// The laser device class
//
class CLaserDevice : public CDevice
{
  public:
    
    CLaserDevice(char *port);
      
    int Setup();
    int Shutdown();
    void Run();
      
    CLock* GetLock( void ) {return &m_lock;};

    // Client interface
    //
    size_t GetData( unsigned char *, size_t maxsize);
    void PutData( unsigned char *, size_t maxsize);
    void GetCommand( unsigned char *, size_t maxsize);
    void PutCommand( unsigned char *, size_t maxsize);
    size_t GetConfig( unsigned char *, size_t maxsize);
    void PutConfig( unsigned char *, size_t maxsize);
    
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

    // Get the laser type
    //
    int GetLaserType(char *buffer, size_t bufflen);

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
    int SetLaserConfig(bool intensity);

    // Change the resolution of the laser
    // 
    int SetLaserRes(int angle, int res);
    
    // Request data from the laser
    // Returns 0 on success
    //
    int RequestLaserData(int min_segment, int max_segment);

    // Read range data from laser
    //
    int ReadLaserData(uint16_t *data, size_t datalen);

    // Write a packet to the laser
    //
    ssize_t WriteToLaser(uint8_t *data, ssize_t len); 
    
    // Read a packet from the laser
    //
    ssize_t ReadFromLaser(uint8_t *data, ssize_t maxlen, bool ack = false, int timeout = -1);

    // Calculates CRC for a telegram
    //
    unsigned short CreateCRC(uint8_t *data, ssize_t len);

    // Get the time (in ms)
    //
    int64_t GetTime();
    
  protected:
    // Side effects class
    //
    CLock m_lock;

    // Laser driver thread
    //
    pthread_t m_thread;

    // Name of device used to communicate with the laser
    //
    char m_laser_name[MAX_FILENAME_SIZE];
    
    // laser device file descriptor
    //
    int m_laser_fd;           

    // Config data
    // PutConfig sets the data and the size, and GetConfig reads and zeros it
    //
    ssize_t m_config_size;
    player_laser_config_t m_config;

    // Most recent scan data
    // PutData sets the data, GetData reads it
    //
    player_laser_data_t m_data;
    
    // Scan width and resolution
    //
    int m_scan_width, m_scan_res;
    
    // Scan range  (for restricted scan)
    //
    int m_scan_min_segment, m_scan_max_segment;

    // Turn intensity data on/off
    //
    bool m_intensity;
};


#endif
