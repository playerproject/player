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
// File: bpsdevice.h
// Author: Andrew Howard
// Date: 27 Aug 2001
// Desc: Beacon-based positioning system device
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

#ifndef BPSDEVICE_H
#define BPSDEVICE_H

#include <pthread.h>
#include <unistd.h>

#include <device.h>
#include <messages.h>


// Forward declare some structures
struct CBpsFrame;
struct CBpsObs;


// The bps device class
class CBpsDevice : public CDevice
{
    // Init function
  public:
    static CDevice* Init(int argc, char** argv)
    {
      return((CDevice*)(new CBpsDevice(argc,argv)));
    }

    // Constructor
    public: CBpsDevice(int argc, char** argv);

    // Setup/shutdown routines
    public: virtual int Setup();
    public: virtual int Shutdown();

    // Device thread
    public: virtual void Main();
    
    // Client interface
    public: virtual void PutData(unsigned char *, size_t maxsize,
                                 uint32_t timestamp_sec, 
                                 uint32_t timestamp_usec);
    public: virtual int PutConfig(CClientData*, unsigned char *, 
                                  size_t maxsize);

    // Estimateion functions
    private: void ProcessOdometry(double ox, double oy, double oa);
    private: void ProcessBeacon(int id, double r, double b, double o);
    private: double UpdateEstimate();
    private: double ComputeForce(CBpsObs *obs);
    private: CBpsFrame *AllocFrame();
    private: CBpsObs *AllocObs(CBpsFrame *a_frame, CBpsFrame *b_frame);
    private: CBpsFrame *CreateFrame();
    private: void DestroyFrame(CBpsFrame *frame);
    private: CBpsObs *CreateObs();
    private: void DestroyObs(CBpsObs *obs);

#ifdef INCLUDE_SELFTEST
    // Self-test functions
    private: void Test(const char *filename);
    private: void Dump();
    private: FILE *dumpfile;
#endif
      
    // BPG
    private: double err;

    // Our index
    private: uint16_t index;
    
    // Pointer to beacon detector to get data from
    private: CDevice *position;   
    private: CDevice *laserbeacon;

    // Timestamps
    private: uint32_t position_sec, position_usec;
    private: uint32_t beacon_sec, beacon_usec;

    // Gain term
    private: double gain;
    
    // Pose of laser relative to robot
    private: double laser_px, laser_py, laser_pa;
    
    // List of true beacon poses
    private: struct
    {
        bool isset;
        double px, py, pa;
        double ux, uy, ua;
    } beacon[256];

    // Limits
    private: int max_frames, max_obs;
    
    // Frame list
    private: int frame_count;
    private: CBpsFrame* frames[64];
    private: int obs_count;
    private: CBpsObs* obs[256];

    // Current frame
    private: CBpsFrame *current;

    // Current odometric pose
    private: double odo_px, odo_py, odo_pa;

    // Current bps data
    //private: player_bps_data_t data;
};


#endif
