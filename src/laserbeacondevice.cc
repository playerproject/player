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
    // Dont do anything if the laser doesnt have new data
    //
    if (m_laser->data_timestamp_sec == this->data_timestamp_sec &&
        m_laser->data_timestamp_usec == this->data_timestamp_usec)
    {
        return sizeof(player_laserbeacon_data_t);
    }
    
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

    // Copy the laser timestamp
    //
    this->data_timestamp_sec = m_laser->data_timestamp_sec;
    this->data_timestamp_usec  = m_laser->data_timestamp_usec;
    
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
    m_max_bits = max(m_max_bits, 3);
    m_max_bits = min(m_max_bits, 8);
    m_bit_width = ntohs(beacon_config->bit_size) / 1000.0;

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

    // Zero the filters
    //
    for (int i = 0; i < ARRAYSIZE(m_filter); i++)
        m_filter[i] = 0;
    
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

    int ai = -1;
    int bi = -1;
    double ax, ay;
    double bx, by;

    // Expected width of beacon
    //
    double max_width = (m_max_bits + 1) * m_bit_width;
    
    // Update the filters
    // We filter ids across multiple frames.
    //
    double filter_gain = 0.50;
    double filter_thresh = 0.80;
    for (int i = 0; i < ARRAYSIZE(m_filter); i++)
        m_filter[i] *= (1 - filter_gain);

    // Find the beacons in this scan
    //
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
            bi = i;
            bx = px;
            by = py;
        }
        if (ai < 0)
            continue;

        double width = sqrt((px - ax) * (px - ax) + (py - ay) * (py - ay));
        if (width < max_width)
            continue;

        // Assign an id to the beacon
        //
        double orient = atan2(by - ay, bx - ax);
        int id = IdentBeacon(ai, bi, ax, ay, orient, laser_data);

        // Reset counters so we can find new beacons
        //
        ai = -1;
        bi = -1;

        // Ignore invalid ids
        //
        if (id < 0)
            continue;
        
        // Do some additional filtering on the id
        // We make sure the id is present in multiple laser scans,
        // thereby eliminating most false matches.
        //
        if (id > 0)
        {
            assert(id >= 0 && id < ARRAYSIZE(m_filter));
            m_filter[id] += filter_gain;
            if (m_filter[id] < filter_thresh)
                id = 0;
        }
        
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

    // Initialise our probability distribution
    // We determine the probability that each bit is set using Bayes law.
    //
    double prob[8][2];
    for (int bit = 0; bit < ARRAYSIZE(prob); bit++)
    {
        prob[bit][0] = 0.5;
        prob[bit][1] = 0.5;
    }

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

        // Update our probability distribution
        // Use Bayes law
        //
        for (int bit = 0; bit < m_max_bits; bit++)
        {
            // Assume a simple gaussian for the sensor model
            // The width of the gaussian is determined by the laser resolution
            // and the orientation of the beacon wrt the laser.
            //
            double x = bit + 0.5;
            double m = tcx / m_bit_width;
            double s = fabs(tbx - tax) / m_bit_width / 2;
            double pa = exp(-(x - m) * (x - m) / (2 * s * s)) / 2 + 0.5;
            double pb = 1 - pa;

            assert(bit >= 0 && bit < ARRAYSIZE(prob));
            
            if (intensity == 0)
            {
                prob[bit][0] *= pa;
                prob[bit][1] *= pb;
            }
            else
            {
                prob[bit][0] *= pb;
                prob[bit][1] *= pa;
            }

            // Normalize
            //
            double n = prob[bit][0] + prob[bit][1];
            prob[bit][0] /= n;
            prob[bit][1] /= n;
        }
    }

    int id = 0;
    double one_thresh = 0.99;
    double zero_thresh = 0.01;

    // Now assign the id
    //
    for (int bit = 0; bit < m_max_bits; bit++)
    {
        //printf("%f ", (double) (prob[bit][1]));
        if (prob[bit][1] > one_thresh)
            id |= (1 << bit);
        else if (prob[bit][1] > zero_thresh)
        {
            id = 0;
            break;
        }
    }
    //printf("\n");
    
    return id;
}


