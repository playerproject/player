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

#define PLAYER_ENABLE_TRACE 0

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>  // for atoi(3)
#include <netinet/in.h>  /* for htons(3) */
#include "laserbeacondevice.hh"
#include "devicetable.h"

extern CDeviceTable* deviceTable;
extern int global_playerport; // used to get at devices

////////////////////////////////////////////////////////////////////////////////
// Constructor
//
CLaserBeaconDevice::CLaserBeaconDevice(int argc, char** argv)
{
  this->index = 0;

  for(int i=0;i<argc;i++)
  {
    if(!strcmp(argv[i],"index"))
    {
      if(++i<argc)
        this->index = atoi(argv[i]);
      else
        fprintf(stderr, "CLaserBeaconDevice: missing index; "
                "using default: %d\n", index);
    }
#if INCLUDE_SELFTEST
    else if (!strcmp(argv[i], "test"))
    {
      if (i + 5 < argc)
      {
        this->max_depth = 0.05;
        this->max_bits = atoi(argv[i + 1]);
        this->bit_width = atof(argv[i + 2]);
        this->zero_thresh = atof(argv[i + 3]);
        this->one_thresh = atof(argv[i + 4]);
        SelfTest(argv[i + 5]);
        exit(0);
      }
      else
      {
        fprintf(stderr, "CLaserBeaconDevice: missing parameters\n");
        exit(0);
      }
    }
#endif
    else
      fprintf(stderr, "CLaserBeaconDevice: ignoring unknown parameter \"%s\"\n",
              argv[i]);
  }
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device
//
int CLaserBeaconDevice::Setup()
{
    // get the pointer to the laser
    this->laser = deviceTable->GetDevice(global_playerport,PLAYER_LASER_CODE,index);
    ASSERT(this->laser != NULL);
    
    // Subscribe to the laser device
    //
    this->laser->GetLock()->Subscribe(this->laser);

    // Maximum variance in the flatness of the beacon
    //
    this->max_depth = 0.05;

    // Number of bits and size of each bit
    //
    this->max_bits = 8;
    this->bit_width = 0.05;

    // Default thresholds
    //
    this->accept_thresh = 1.0;
    this->zero_thresh = 0.60;
    this->one_thresh = 0.60;

    // Zero the filters
    //
    for (int i = 0; i < ARRAYSIZE(this->filter); i++)
        this->filter[i] = 0;
    
    // Hack to get around mutex on GetData
    //
    this->beacon_data.count = 0;
    GetLock()->PutData(this, (uint8_t*) &this->beacon_data, sizeof(this->beacon_data));

    PLAYER_MSG0("laser beacon device: setup");
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
//
int CLaserBeaconDevice::Shutdown()
{
    // Unsubscribe from the laser device
    //
    this->laser->GetLock()->Unsubscribe(this->laser);

    PLAYER_MSG0("laser beacon device: shutdown");
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get data from buffer (called by client thread)
//
size_t CLaserBeaconDevice::GetData(unsigned char *dest, size_t maxsize) 
{
    // If the laser doesnt have new data,
    // just return a copy of our old data.
    //
    if (this->laser->data_timestamp_sec == this->data_timestamp_sec &&
        this->laser->data_timestamp_usec == this->data_timestamp_usec)
    {
        ASSERT(maxsize >= sizeof(this->beacon_data));
        memcpy(dest, &this->beacon_data, sizeof(this->beacon_data));
        return sizeof(this->beacon_data);
    }
    
    // Get the laser data
    //
    player_laser_data_t laser_data;
    this->laser->GetLock()->GetData(this->laser,
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
    FindBeacons(&laser_data, &this->beacon_data);
    
    // Do some byte-swapping
    //
    for (int i = 0; i < this->beacon_data.count; i++)
    {
        this->beacon_data.beacon[i].range = htons(this->beacon_data.beacon[i].range);
        this->beacon_data.beacon[i].bearing = htons(this->beacon_data.beacon[i].bearing);
        this->beacon_data.beacon[i].orient = htons(this->beacon_data.beacon[i].orient);
    }
    PLAYER_TRACE1("setting beacon count: %u",this->beacon_data.count);
    this->beacon_data.count = htons(this->beacon_data.count);
    
    // Copy results
    //
    ASSERT(maxsize >= sizeof(this->beacon_data));
    memcpy(dest, &this->beacon_data, sizeof(this->beacon_data));

    // Copy the laser timestamp
    //
    this->data_timestamp_sec = this->laser->data_timestamp_sec;
    this->data_timestamp_usec  = this->laser->data_timestamp_usec;
    
    return sizeof(this->beacon_data);
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
    this->max_bits = beacon_config->bit_count;
    this->max_bits = max(this->max_bits, 3);
    this->max_bits = min(this->max_bits, 8);
    this->bit_width = ntohs(beacon_config->bit_size) / 1000.0;
    this->zero_thresh = ntohs(beacon_config->zero_thresh) / 100.0;
    this->one_thresh = ntohs(beacon_config->one_thresh) / 100.0;

    PLAYER_TRACE2("bits %d, width %f", this->max_bits, this->bit_width);
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
    double min_width = (this->max_bits - 1) * this->bit_width;
    double max_width = (this->max_bits + 1) * this->bit_width;
    
    // Update the filters
    // We filter ids across multiple frames.
    //
    double filter_gain = 0.50;
    //double filter_thresh = 0.80;
    for (int i = 0; i < ARRAYSIZE(this->filter); i++)
        this->filter[i] *= (1 - filter_gain);

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

        width = sqrt((bx - ax) * (bx - ax) + (by - ay) * (by - ay));
        if (width < min_width)
            continue;
        if (width > max_width)
        {
            ai = -1;
            continue;
        }

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
        /* *** TESTING
        if (id > 0)
        {
            assert(id >= 0 && id < ARRAYSIZE(this->filter));
            this->filter[id] += filter_gain;
            if (this->filter[id] < filter_thresh)
                id = 0;
        }
        */
        
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
            // normalizing orient to [-pi, pi] and adding 90 deg like it was
            // done before 
            orient += M_PI*0.5;
            orient = NORMALIZE(orient);
            beacon_data->beacon[beacon_data->count].orient = (int) (orient * 180 / M_PI );
            //beacon_data->beacon[beacon_data->count].orient = (int) (orient * 180 / M_PI + 90);
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
    // Compute pose of laser relative to beacon
    double lx = -ox * cos(-oth) + oy * sin(-oth);
    double ly = -ox * sin(-oth) - oy * cos(-oth);
    double la = -oth;

    // Initialise our probability distribution
    // We determine the probability that each bit is set using Bayes law.
    //
    double prob[8][2];
    for (int bit = 0; bit < ARRAYSIZE(prob); bit++)
        prob[bit][0] = prob[bit][1] = 0;

    // Scan through the readings that make up the candidate
    //
    for (int i = a; i <= b; i++)
    {
        int intensity = (int) (laser_data->ranges[i] >> 13);
        double range = (double) (laser_data->ranges[i] & 0x1FFF) / 1000;
        double bearing = (double) (laser_data->min_angle + i * laser_data->resolution)
            / 100.0 * M_PI / 180;
        double res = (double) laser_data->resolution / 100.0 * M_PI / 180;

        // Compute point relative to beacon
        double py = ly + range * sin(la + bearing);

        // Discard candidate points are not close to x-axis
        // (ie candidate is not flat).
        //
        if (fabs(py) > this->max_depth)
            return -1;

        // Compute intercept with beacon
        //double cx = lx + ly * tan(la + bearing + M_PI/2);
        double ax = lx + ly * tan(la + bearing - res/2 + M_PI/2);
        double bx = lx + ly * tan(la + bearing + res/2 + M_PI/2);

#ifdef INCLUDE_SELFTEST
        //if (intensity > 0)
        //    printf("%f %f %f %f\n", cx, py, ax, bx);
#endif

        // Update our probability distribution
        // Use Bayes law
        for (int bit = 0; bit < this->max_bits; bit++)
        {
            // Use a rectangular distribution
            double a = (bit + 0.0) * this->bit_width;
            double b = (bit + 1.0) * this->bit_width;

            double p = 0;
            if (ax < a && bx > b)
                p = 1;
            else if (ax > a && bx < b)
                p = 1;
            else if (ax > b || bx < a)
                p = 0;
            else if (ax < a && bx < b)
                p = 1 - (a - ax) / (bx - ax);
            else if (ax > a && bx > b)
                p = 1 - (bx - b) / (bx - ax);
            else
                assert(0);

            //printf("prob : %f %f %f %f %f\n", ax, bx, a, b, p);
            
            if (intensity == 0)
            {
                assert(bit >= 0 && bit < ARRAYSIZE(prob));            
                prob[bit][0] += p;
                prob[bit][1] += 0;
            }
            else
            {
                assert(bit >= 0 && bit < ARRAYSIZE(prob));            
                prob[bit][0] += 0;
                prob[bit][1] += p;
            }
        }
    }

#ifdef INCLUDE_SELFTEST
    //printf("\n\n");
#endif

    // Now assign the id
    //
    int id = 0;
    for (int bit = 0; bit < this->max_bits; bit++)
    {
        double pn = prob[bit][0] + prob[bit][1];
        double p0 = prob[bit][0] / pn;
        double p1 = prob[bit][1] / pn;
       
        if (pn < this->accept_thresh)
            id = -1;
        else if (p0 > this->zero_thresh)
            id |= (0 << bit);
        else if (p1 > this->one_thresh)
            id |= (1 << bit);
        else
            id = -1;

        //printf("%d %f %f : %f %f %f\n", bit, prob[bit][0], prob[bit][1], p0, p1, pn);
    }
    //printf("\n");

    if (id < 0)
        id = 0;
    
    return id;
}


#ifdef INCLUDE_SELFTEST

////////////////////////////////////////////////////////////////////////////////
// Beacon detector self-test
int CLaserBeaconDevice::SelfTest(const char *filename)
{
    int id = 21;
    
    // Zero the filters
    for (int i = 0; i < ARRAYSIZE(this->filter); i++)
        this->filter[i] = 0;
    
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        PLAYER_ERROR2("unable to open [%s] : error [%s]", filename, strerror(errno));
        return -1;
    }

    // Histogram of errors
    int hist[3] = {0};
    
    printf("# self test -- start\n");
    while (TRUE)
    {
        char line[4096];
        if (!fgets(line, sizeof(line), file))
            break;
        
        char *type = strtok(line, " ");
        if (strcmp(type, "laser") != 0)
            continue;

        player_laser_data_t laser_data;
        strtok(NULL, " ");
        strtok(NULL, " ");
        laser_data.resolution = atoi(strtok(NULL, " "));
        laser_data.min_angle = atoi(strtok(NULL, " "));
        laser_data.max_angle = atoi(strtok(NULL, " "));
        laser_data.range_count = atoi(strtok(NULL, " "));
        for (int i = 0; i < laser_data.range_count; i++)
            laser_data.ranges[i] = atoi(strtok(NULL, " "));

        player_laserbeacon_data_t beacon_data;
        FindBeacons(&laser_data, &beacon_data);

        for (int i = 0; i < beacon_data.count; i++)
        {
            if (beacon_data.beacon[i].id == id)
                hist[0] += 1;
            else if (beacon_data.beacon[i].id == 0)
                hist[1] += 1;
            else
                hist[2] += 1;

            if (beacon_data.beacon[i].id == id)            
                printf("beacon %d %d %d %d 0 0 0 0 0 0\n", (int) beacon_data.beacon[i].id,
                       beacon_data.beacon[i].range, beacon_data.beacon[i].bearing,
                       beacon_data.beacon[i].orient);
            else if (beacon_data.beacon[i].id == 0)
                printf("beacon %d 0 0 0 %d %d %d 0 0 0\n", (int) beacon_data.beacon[i].id,
                       beacon_data.beacon[i].range, beacon_data.beacon[i].bearing,
                       beacon_data.beacon[i].orient);
            else
                printf("beacon %d 0 0 0 0 0 0 0 %d %d %d\n", (int) beacon_data.beacon[i].id,
                       beacon_data.beacon[i].range, beacon_data.beacon[i].bearing,
                       beacon_data.beacon[i].orient);
        }
    }

    printf("# hist : %d %d %d\n", hist[0], hist[1], hist[2]);
    
    printf("# self test -- end\n");
    
    fclose(file);

    return 0;
}

#endif
















