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
//  The first and second bits of the beacon must be '1' and '0' respectively.
//  More significant bits can be used to encode a unique id.
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

#define PLAYER_ENABLE_TRACE 1

#include <string.h>
#include <math.h>
#include <netinet/in.h>  /* for htons(3) */
#include "laserbeacondevice.hh"


////////////////////////////////////////////////////////////////////////////////
// Constructor
//
CLaserBeaconDevice::CLaserBeaconDevice(CDevice *laser) 
{
    m_laser = laser;
    ASSERT(m_laser != NULL);
}


////////////////////////////////////////////////////////////////////////////////
// Get data from buffer (called by client thread)
//
size_t CLaserBeaconDevice::GetData(unsigned char *dest, size_t maxsize) 
{
    // Get the laser data
    //
    player_laser_data_t laser_data;
    m_laser->GetLock()->GetData(m_laser,
                                (uint8_t*) &laser_data, sizeof(laser_data),
                                NULL, NULL);

    // Do some byte swapping
    //
    laser_data.resolution = ntohs(laser_data.resolution);
    laser_data.min_angle = ntohs(laser_data.min_angle);
    laser_data.max_angle = ntohs(laser_data.max_angle);
    laser_data.range_count = ntohs(laser_data.range_count);
    for (int i = 0; i < laser_data.range_count; i++)
        laser_data.ranges[i] = ntohs(laser_data.ranges[i]);

    // Analyse the laser data
    //
    player_laserbeacon_data_t beacon_data;
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
    if (maxsize != sizeof(player_laserbeacon_config_t))
        PLAYER_ERROR("config packet size is incorrect");
    
    player_laserbeacon_config_t *beacon_config = (player_laserbeacon_config_t*) src;

    // Number of bits and size of each bit
    //
    m_max_bits = beacon_config->bit_count;
    m_bit_width = ntohs(beacon_config->bit_size) / 1000.0;

    // Expected width of beacon
    //
    m_min_width = m_max_bits * m_bit_width - m_bit_width;  
    m_max_width = m_max_bits * m_bit_width + m_bit_width;

    // Tolerance
    //
    m_one_thresh = +beacon_config->one_thresh;
    m_zero_thresh = -beacon_config->zero_thresh;

    PLAYER_TRACE2("bits %d, width %f", m_max_bits, m_bit_width);
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device
//
int CLaserBeaconDevice::Setup()
{
    // Maximum variance in the flatness of the beacon
    //
    m_max_depth = 0.05;

    // Number of bits and size of each bit
    //
    m_max_bits = 8;
    m_bit_width = 0.05;

    // Expected width of beacon
    //
    m_min_width = m_max_bits * m_bit_width - m_bit_width;  
    m_max_width = m_max_bits * m_bit_width + m_bit_width;
    
    // Histogram thresholds
    //
    m_one_thresh = +3;
    m_zero_thresh = -3;
    
    // Hack to get around mutex on GetData
    //
    player_laserbeacon_data_t beacon_data;
    beacon_data.count = 0;
    GetLock()->PutData(this, (uint8_t*) &beacon_data, sizeof(beacon_data));
    
    PLAYER_MSG0("laser beacon device: setup");
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
//
int CLaserBeaconDevice::Shutdown()
{
    PLAYER_MSG0("laser beacon device: shutdown");
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Analyze the laser data and return beacon data
//
void CLaserBeaconDevice::FindBeacons(const player_laser_data_t *laser_data,
                                     player_laserbeacon_data_t *beacon_data)
{
    beacon_data->count = 0;

    double max_width = m_max_bits * m_bit_width;
    
    int ai = -1;
    int bi = -1;
    double ax, ay;
    double bx, by;
    
    for (int i = 0; i < laser_data->range_count; i++)
    {
        int intensity = (int) (laser_data->ranges[i] >> 13);
        double range = (double) (laser_data->ranges[i] & 0x1FFF) / 1000;
        double bearing = (double) (laser_data->min_angle + i * laser_data->resolution)
            / 100.0 * M_PI / 180;

        double px = range * cos(bearing);
        double py = range * sin(bearing);
        
        if (intensity > 0)
        {
            if (ai < 0)
            {
                ai = i;
                ax = px;
                ay = py;
            }
        }
        if (ai < 0)
            continue;
        
        double width = sqrt((px - ax) * (px - ax) + (py - ay) * (py - ay));
        if (width < max_width)
            continue;

        bi = i;
        bx = px;
        by = py;
        
        printf("%d %d\n", ai, bi);
        double orient = atan2(by - ay, bx - ax);
        int id = IdentBeacon(ai, bi, ax, ay, orient, laser_data);
        ai = -1;
        bi = -1;

        if (id < 0)
            continue;
        
        // Check for array overflow
        //
        if (beacon_data->count < ARRAYSIZE(beacon_data->beacon))
        {
            double ox = (bx + ax) / 2;
            double oy = (by + ay) / 2;
            double range = sqrt(ox * ox + oy * oy);
            double bearing = atan2(oy, ox);

            // Create an entry for this beacon
            // Note that we return the surface normal for the beacon orientation.
            //
            beacon_data->beacon[beacon_data->count].id = id;
            beacon_data->beacon[beacon_data->count].range = (int) (range * 1000);
            beacon_data->beacon[beacon_data->count].bearing = (int) (bearing * 180 / M_PI);
            beacon_data->beacon[beacon_data->count].orient = (int) (orient * 180 / M_PI + 90);
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
    int term_count = 0;
    double term[PLAYER_NUM_LASER_SAMPLES][2];

    // Scan through the readings that make up the candidate
    //
    for (int i = a; i <= b; i++)
    {
        int intensity = (int) (laser_data->ranges[i] >> 13);
        double range = (double) (laser_data->ranges[i] & 0x1FFF) / 1000;
        double bearing = (double) (laser_data->min_angle + i * laser_data->resolution)
            / 100.0 * M_PI / 180;
        double res = (double) laser_data->resolution / 100.0 * M_PI / 180;
        
        double cx = range * cos(bearing) - ox;
        double cy = range * sin(bearing) - oy;

        double ax = range * cos(bearing - res) - ox;
        double ay = range * sin(bearing - res) - oy;

        double bx = range * cos(bearing + res) - ox;
        double by = range * sin(bearing + res) - oy;

        // Rotate so that points all lie along x-axis
        // with the origin at the first point
        //
        double tcx = cx * cos(-oth) - cy * sin(-oth);
        double tcy = cx * sin(-oth) + cy * cos(-oth);
        double tax = ax * cos(-oth) - ay * sin(-oth);
        double tbx = bx * cos(-oth) - by * sin(-oth);

        // Discard candidate points are not close to x-axis
        // (ie candidate is not flat).
        //
        if (fabs(tcy) > m_max_depth)
            return -1;

        // Add a term to our histogram
        //
        if (intensity > 0)
        {
            term[term_count][0] = tcx / m_bit_width;
            term[term_count][1] = fabs(tbx - tax) / m_bit_width;
            term_count++;
        }
    }

    /* for testing
    for (double x = -0.5; x <= m_max_bits + 0.5; x += 0.1)
    {
        double v = 0;
        for (int j = 0; j < term_count; j++)
        {
            double z = term[j][0];
            double s = term[j][1];
            v += exp(-(x - z) * (x - z) / (2 * s * s));
        }
        printf("%1.3f %1.3f\n", (double) x, (double) v);
    }
    printf("\n");
    printf("\n");
    */

    int id = 0;
    double one_thresh;
    double zero_thresh;

    // Identify bits
    // Assumes first two bits are 1 and 0.
    //
    for (int bit = 0; bit < m_max_bits; bit++)
    {
        double x = (double) bit + 0.5;
        double v = 0;
        for (int j = 0; j < term_count; j++)
        {
            double z = term[j][0];
            double s = term[j][1];
            v += exp(-(x - z) * (x - z) / (2 * s * s));
        }

        if (bit == 0)
            one_thresh = v - 0.5;
        else if (bit == 1)
            zero_thresh = v + 0.5;
        
        if (v > one_thresh)
            id |= (1 << bit);
        else if (v > zero_thresh)
            return 0;
    }
    if (one_thresh - zero_thresh < 0)
        return 0;
    
    return id;
}


