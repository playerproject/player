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
// File: laserbeacondevice.h
// Author: Andrew Howard
// Date: 29 Jan 2000
// Desc: Looks for beacons in laser data
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

#ifndef LASERBEACONDEVICE
#define LASERBEACONDEVICE

#include <pthread.h>
#include <unistd.h>

#include "lock.h"
#include "device.h"
#include "messages.h"


// The laser beacon device class
//
class CLaserBeaconDevice : public CDevice
{
    // Constructor
    //
    public: CLaserBeaconDevice(int argc, char** argv);

    // Setup/shutdown routines
    //
    public: virtual int Setup();
    public: virtual int Shutdown();
    public: virtual CLock* GetLock() {return &lock;};

    // Client interface
    //
    public: virtual size_t GetData(unsigned char *, size_t maxsize);
    public: virtual void PutData(unsigned char *, size_t maxsize);
    public: virtual void GetCommand(unsigned char *, size_t maxsize);
    public: virtual void PutCommand(unsigned char *, size_t maxsize);
    public: virtual size_t GetConfig(unsigned char *, size_t maxsize);
    public: virtual void PutConfig(unsigned char *, size_t maxsize);

    // Analyze the laser data and return beacon data
    //
    private: void FindBeacons(const player_laser_data_t *laser_data,
                              player_laserbeacon_data_t *beacon_data);

    // Analyze the candidate beacon and return its id (0 == none)
    //
    private: int IdentBeacon(int a, int b, double ox, double oy, double oth,
                             const player_laser_data_t *laser_data);

    // Lock object for synchronization
    //
    private: CLock lock;

    // Pointer to laser to get data from
    //
    private: uint16_t index;
    private: CDevice *m_laser;

    // Magic numbers
    // See setup for definitions
    //
    private: int m_max_bits;
    private: double m_bit_width;
    private: double m_max_depth;
    private: double m_zero_thresh, m_one_thresh;

    // Filter array (beacons must appear in multiple frames to be accepted
    //
    private: double m_filter[256];

    // The current data
    //
    player_laserbeacon_data_t m_beacon_data;
};


#endif
