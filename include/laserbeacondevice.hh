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

#include <unistd.h>

#include "device.h"
#include "messages.h"


// The laser beacon device class
class CLaserBeaconDevice : public CDevice
{
  // Initialization function
  public:
    static CDevice* Init(int argc, char** argv)
    {
      return((CDevice*)(new CLaserBeaconDevice(argc,argv)));
    }

  // Constructor
  public: CLaserBeaconDevice(int argc, char** argv);

  // Setup/shutdown routines
  //
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Client interface
  public: virtual size_t GetData(unsigned char *, size_t maxsize,
                                 uint32_t* timestamp_sec,
                                 uint32_t* timestamp_usec);
  public: virtual int PutConfig(player_device_id_t* device, void *client, 
                                void *data, size_t len);

  // Analyze the laser data and return beacon data
  private: void FindBeacons(const player_laser_data_t *laser_data,
                            player_laserbeacon_data_t *beacon_data);

  // Analyze the candidate beacon and return its id (0 == none)
  private: int IdentBeacon(int a, int b, double ox, double oy, double oth,
                           const player_laser_data_t *laser_data);

#ifdef INCLUDE_SELFTEST
  // Beacon detector self-test
  private: int SelfTest(const char *filename);

  // Filename for self-test
  private: char *test_filename;
#endif

  // Pointer to laser to get data from
  private: int index;
  private: CDevice *laser;

  // Defaults
  private: int default_bitcount;
  private: double default_bitsize;
  
  // Magic numbers
  // See setup for definitions
  private: int max_bits;
  private: double bit_width;
  private: double max_depth;
  private: double accept_thresh, zero_thresh, one_thresh;

  // Filter array (beacons must appear in multiple frames to be accepted)
  private: double filter[256];

  // The current data
  //
  player_laserbeacon_data_t beacon_data;
};


#endif
