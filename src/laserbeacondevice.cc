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
// File: laserbeacondevice.cc
// Author: Andrew Howard
// Date: 29 Jan 2001
// Desc: Detects beacons in laser data
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
//  Will detect binary coded beacons (i.e. bar-codes) in laser data.
//  Reflectors represent '1' bits, non-reflectors represent '0' bits.
//  The first and last bits of the beacon must be '1'; these bits
//  are used to locate candidate beacons, determine their size and orientation.
//  Intermediate bits can be used to encode a unique id.
//
//  Will return an id of zero if it can see a beacon, but cannot identify it.
//
// Known bugs:
//  (empty)
//
// Possible enhancements:
//  (empty)
//
///////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <math.h>
#include <netinet/in.h>  /* for htons(3) */
#include "laserbeacondevice.hh"

 // *** TESTING -- remove this later
//#define ENABLE_TRACE 0
//#include "rtk-types.hh"


////////////////////////////////////////////////////////////////////////////////
// Constructor
//
CLaserBeaconDevice::CLaserBeaconDevice(CDevice *laser) 
{
    m_laser = laser;
    ASSERT(m_laser != NULL);

    // Expected width of beacon
    //
    m_min_width = 0.40 - 0.05;  
    m_max_width = 0.40 + 0.05;

    // Maximum variance in the flatness of the beacon
    //
    m_max_depth = 0.05;

    // Number of bits and size of each bit
    //
    m_max_bits = 8;
    m_bit_width = 0.05;
    
    // Histogram thresholds
    //
    m_one_thresh = +3;
    m_zero_thresh = -3;    
}


////////////////////////////////////////////////////////////////////////////////
// Get data from buffer (called by client thread)
//
size_t CLaserBeaconDevice::GetData(unsigned char *dest, size_t maxsize) 
{
    // Get the laser data
    //
    player_laser_data_t laser_data;
    m_laser->GetLock()->GetData(m_laser, (BYTE*) &laser_data, sizeof(laser_data),&data_timestamp);
    
    // Analyse the laser data
    //
    BeaconData beacon_data;
    FindBeacons(&laser_data, &beacon_data);
    
    // Do some byte-swapping
    //
    for (int i = 0; i < beacon_data.count; i++)
    {
        beacon_data.beacon[i].range = htons(beacon_data.beacon[i].range);
        beacon_data.beacon[i].bearing = htons(beacon_data.beacon[i].bearing);
        beacon_data.beacon[i].orient = htons(beacon_data.beacon[i].orient);
    }
    beacon_data.count = htons(beacon_data.count);
    
    // Copy results
    //
    ASSERT(maxsize >= sizeof(beacon_data));
    memcpy(dest, &beacon_data, sizeof(beacon_data));
    
    return sizeof(beacon_data);
}


////////////////////////////////////////////////////////////////////////////////
// Put data in buffer (called by device thread)
//
void CLaserBeaconDevice::PutData(unsigned char *src, size_t maxsize)
{
}


////////////////////////////////////////////////////////////////////////////////
// Get command from buffer (called by device thread)
//
void CLaserBeaconDevice::GetCommand(unsigned char *dest, size_t maxsize) 
{
}


////////////////////////////////////////////////////////////////////////////////
// Put command in buffer (called by client thread)
//
void CLaserBeaconDevice::PutCommand(unsigned char *src, size_t maxsize) 
{
}


////////////////////////////////////////////////////////////////////////////////
// Get configuration from buffer (called by device thread)
//
size_t CLaserBeaconDevice::GetConfig(unsigned char *dest, size_t maxsize) 
{
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Put configuration in buffer (called by client thread)
//
void CLaserBeaconDevice::PutConfig( unsigned char *src, size_t maxsize) 
{
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device
//
int CLaserBeaconDevice::Setup()
{
    // Hack to get around mutex on GetData
    //
    BeaconData beacon_data;
    beacon_data.count = 0;
    GetLock()->PutData(this, (BYTE*) &beacon_data, sizeof(beacon_data));
    
    MSG("laser beacon device: setup");
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
//
int CLaserBeaconDevice::Shutdown()
{
    MSG("laser beacon device: shutdown");
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Analyze the laser data and return beacon data
//
void CLaserBeaconDevice::FindBeacons(const player_laser_data_t *laser_data,
                               BeaconData *beacon_data)
{
    //TRACE0("searching");
    
    beacon_data->count = 0;

    int marker_count = 0;
    double marker[PLAYER_NUM_LASER_SAMPLES][3];

    // Extract all the markers (reflectors)
    //
    for (int i = 0; i < PLAYER_NUM_LASER_SAMPLES; i++)
    {
        int intensity = (int) (ntohs(laser_data->ranges[i]) >> 13);
        double range = (double) (ntohs(laser_data->ranges[i]) & 0x1FFF) / 1000;
        double bearing = ((double) i / 2  - 90) * M_PI / 180;

        if (intensity > 0)
        {
            marker[marker_count][0] = i;
            marker[marker_count][1] = range;
            marker[marker_count][2] = bearing;
            marker_count++;
        }
    }

    // Now look for pairs of markers that are the correct
    // distance apart
    //
    for (int i = 0; i < marker_count; i++)
    {
        int ai = (int) marker[i][0];
        double arange = marker[i][1];
        double abearing = marker[i][2];
        double ax = marker[i][1] * cos(marker[i][2]);
        double ay = marker[i][1] * sin(marker[i][2]);
            
        for (int j = i + 1; j < marker_count; j++)
        {
            int bi = (int) marker[j][0];
            double brange = marker[j][1];
            double bbearing = marker[j][2];
            double bx = marker[j][1] * cos(marker[j][2]);
            double by = marker[j][1] * sin(marker[j][2]);

            double dx = bx - ax;
            double dy = by - ay;
            double width = sqrt(dx * dx + dy * dy);
            double orient = atan2(dy, dx);

            // Check the width
            //
            if (width < m_min_width || width > m_max_width)
                continue;

            // Assign an id to this candidate beacon
            // Will return -1 if this is not a beacon.
            // Will return  0 if this is a beacon, but it cannot be identified.
            // Will return beacon id otherwise
            //
            int id = IdentBeacon(ai, bi, ax, ay, orient, laser_data);
            if (id < 0)
                continue;

            // Check for array overflow
            //
            if (beacon_data->count >= ARRAYSIZE(beacon_data->beacon))
                continue;
            
            double range = (arange + brange) / 2;
            double bearing = (abearing + bbearing) / 2;
            
            beacon_data->beacon[beacon_data->count].id = id;
            beacon_data->beacon[beacon_data->count].range = (int) (range * 1000);
            beacon_data->beacon[beacon_data->count].bearing = (int) (bearing * 180 / M_PI);
            beacon_data->beacon[beacon_data->count].orient = (int) (orient * 180 / M_PI);
            beacon_data->count++;
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// Analyze the candidate beacon and return its id
// Will return -1 if this is not a beacon.
// Will return  0 if this is a beacon, but it cannot be identified.
// Will return beacon id otherwise
//
int CLaserBeaconDevice::IdentBeacon(int a, int b, double ox, double oy, double oth,
                                    const player_laser_data_t *laser_data)
{
    int hist[PLAYER_NUM_LASER_SAMPLES];
    memset(hist, 0, sizeof(hist));
    
    //printf("candidate %d:%d  ", (int) a, (int) b);
    
    // Scan through the readings that make up the candidate
    //
    for (int i = a; i <= b; i++)
    {
        int intensity = (int) (ntohs(laser_data->ranges[i]) >> 13);
        double range = (double) (ntohs(laser_data->ranges[i]) & 0x1FFF) / 1000;
        double bearing = ((double) i / 2  - 90) * M_PI / 180;

        double tx = range * cos(bearing) - ox;
        double ty = range * sin(bearing) - oy;

        // Rotate so that points all lie along x-axis
        // with the origin at the first point
        //
        double px = tx * cos(-oth) - ty * sin(-oth);
        double py = tx * sin(-oth) + ty * cos(-oth);

        // Discard candidate points are not close to x-axis
        // (ie candidate is not flat).
        //
        if (fabs(py) > m_max_depth)
            return -1;

        // Work out which bit this point corresponds to
        //
        int bin = (int) (px / m_bit_width);
        if (bin < 0 || bin >= m_max_bits)
            continue;

        // Update histogram
        //
        if (intensity > 0)
            hist[bin] += 1;
        else
            hist[bin] -= 1;
        
        //printf("%d", (int) intensity);
    }
    //printf("\n");

    /*
    // Display histogram
    //
    printf("histogram ");
    for (int i = m_max_bits - 1; i >= 0; i--)
        printf("%+d", (int) hist[i]);
    printf("\n");
   
    // Display thresholded histogram
    //
    printf("threshold ");
    for (int i = m_max_bits - 1; i >= 0; i--)
    {
        if (hist[i] <= m_zero_thresh)
            printf(" 0");
        else if (hist[i] >= m_one_thresh)
            printf(" 1");
        else
            printf(" ?");
    }
    printf("\n");
    */

    // Assign id by thresholding the bit-histogram.
    //
    int id = 0;
    for (int i = 0; i < m_max_bits; i++)
    {
        if (hist[i] <= m_zero_thresh)
            id |= 0;
        else if (hist[i] >= m_one_thresh)
            id |= (1 << i);
        else
        {
            id = 0;
            break;
        }
    }

    //printf("id %d\n", (int) id);

    return id;
}



