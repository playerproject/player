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
// File: bpsdevice.cc
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

#define PLAYER_ENABLE_TRACE 1

#include <string.h>
#include <math.h>
#include <stdlib.h>     // for atoi(3)
#include <netinet/in.h> // for htons(3)
#include "bpsdevice.h"
#include "devicetable.h"

extern CDeviceTable* deviceTable;


////////////////////////////////////////////////////////////////////////////////
// Constructor
//
CBpsDevice::CBpsDevice(int argc, char** argv)
{
  this->index = 0;
  for(int i=0;i<argc;i++)
  {
    if(!strcmp(argv[i],"index"))
    {
      if(++i<argc)
        this->index = atoi(argv[i]);
      else
        fprintf(stderr, "CBpsDevice: missing index; "
                "using default: %d\n", index);
    }
    else
      fprintf(stderr, "CBpsDevice: ignoring unknown parameter \"%s\"\n",
              argv[i]);
  }
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device
//
int CBpsDevice::Setup()
{
    // Get pointers to other devices
    this->position = deviceTable->GetDevice(PLAYER_POSITION_CODE, this->index);
    this->laserbeacon = deviceTable->GetDevice(PLAYER_LASERBEACON_CODE, this->index);
    ASSERT(this->position != NULL);
    ASSERT(this->laserbeacon != NULL);
    
    // Subscribe to devices
    this->position->GetLock()->Subscribe(this->position);
    this->laserbeacon->GetLock()->Subscribe(this->laserbeacon);

    // Initialise some stuff
    memset(this->beacon, 0, sizeof(this->beacon));
    this->odo_px = this->odo_py = this->odo_pa = 0;
    this->org_px = this->org_py = this->org_pa = 0;

    // *** TESTING
    // Put some stuff in the map here
    this->beacon[1].isset = true;
    this->beacon[1].px = 2;
    this->beacon[1].py = 0.5;
    this->beacon[1].pa = 0;
    this->beacon[2].isset = true;
    this->beacon[2].px = 4;
    this->beacon[2].py = 2;
    this->beacon[2].pa = M_PI/2;
    this->beacon[3].isset = true;
    this->beacon[3].px = 6;
    this->beacon[3].py = 0.5;
    this->beacon[3].pa = M_PI;
    this->beacon[4].isset = true;
    this->beacon[4].px = 8;
    this->beacon[4].py = 2;
    this->beacon[4].pa = -M_PI/2;

    // Hack to get around mutex on GetData
    player_bps_data_t bps_data;
    GetLock()->PutData(this, (uint8_t*) &bps_data, sizeof(bps_data));
    
    PLAYER_TRACE0("bps device: setup");
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
//
int CBpsDevice::Shutdown()
{
    // Unsubscribe from the laser device
    //
    this->position->GetLock()->Unsubscribe(this->position);    
    this->laserbeacon->GetLock()->Unsubscribe(this->laserbeacon);

    PLAYER_TRACE0("bps device: shutdown");
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get data from buffer (called by client thread)
//
size_t CBpsDevice::GetData(unsigned char *dest, size_t maxsize) 
{
    uint32_t sec, usec;
    
    // Get the odometry data
    player_position_data_t position_data;
    this->position->GetLock()->GetData(this->position, (uint8_t*) &position_data,
                                       sizeof(position_data), &sec, &usec);

    // If odometry data is new, process it...
    if (!(sec == this->position_sec && usec == this->position_usec))
    {
        this->position_sec = sec;
        this->position_usec = usec;
            
        // Compute odometric pose in SI units
        this->odo_px = ((int) ntohl(position_data.xpos)) / 1000.0;
        this->odo_py = ((int) ntohl(position_data.ypos)) / 1000.0;
        this->odo_pa = ntohs(position_data.theta) * M_PI / 180;

        PLAYER_TRACE3("odometry : %f %f %f", this->odo_px, this->odo_py, this->odo_pa);
    }
        
    // Get the beacon data
    player_laserbeacon_data_t laserbeacon_data;
    this->laserbeacon->GetLock()->GetData(this->laserbeacon, (uint8_t*) &laserbeacon_data,
                                          sizeof(laserbeacon_data), &sec, &usec);

    // If beacon data is new, process it...
    if (!(sec == this->beacon_sec && usec == this->beacon_usec))
    {
        this->beacon_sec = sec;
        this->beacon_usec = usec;

        // Process the beacons one-by-one
        for (int i = 0; i < ntohs(laserbeacon_data.count); i++)
        {
            int id = laserbeacon_data.beacon[i].id;
            if (id == 0)
                continue;
            double r = ntohs(laserbeacon_data.beacon[i].range) / 1000.0;
            double b = ((short) ntohs(laserbeacon_data.beacon[i].bearing)) * M_PI / 180.0;
            double o = ((short) ntohs(laserbeacon_data.beacon[i].orient)) * M_PI / 180.0;
            ProcessBeacon(id, r, b, o);
        }
    }
    
    // Compute current global pose
    double gx = this->org_px +
        this->odo_px * cos(this->org_pa) - this->odo_py * sin(this->org_pa);
    double gy = this->org_py +
        this->odo_px * sin(this->org_pa) + this->odo_py * cos(this->org_pa);
    double ga = this->org_pa + this->odo_pa;

    // Construct data packet
    player_bps_data_t data;
    data.px = htonl((int) (gx * 1000));
    data.py = htonl((int) (gy * 1000));
    data.pa = htonl((int) (ga * 180 / M_PI));

    // Copy results
    ASSERT(maxsize >= sizeof(data));
    memcpy(dest, &data, sizeof(data));
    
    return sizeof(data);
}


////////////////////////////////////////////////////////////////////////////////
// Put data in buffer (called by device thread)
//
void CBpsDevice::PutData(unsigned char *src, size_t maxsize)
{
}


////////////////////////////////////////////////////////////////////////////////
// Get command from buffer (called by device thread)
//
void CBpsDevice::GetCommand(unsigned char *dest, size_t maxsize) 
{
}


////////////////////////////////////////////////////////////////////////////////
// Put command in buffer (called by client thread)
//
void CBpsDevice::PutCommand(unsigned char *src, size_t maxsize) 
{
}


////////////////////////////////////////////////////////////////////////////////
// Get configuration from buffer (called by device thread)
//
size_t CBpsDevice::GetConfig(unsigned char *dest, size_t maxsize) 
{
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Put configuration in buffer (called by client thread)
//
void CBpsDevice::PutConfig( unsigned char *src, size_t maxsize) 
{
    /*
    if (maxsize != sizeof(player_laserbeacon_config_t))
        PLAYER_ERROR("config packet size is incorrect");
    
    player_laserbeacon_config_t *beacon_config = (player_laserbeacon_config_t*) src;

    // Number of bits and size of each bit
    //
    m_max_bits = beacon_config->bit_count;
    m_max_bits = max(m_max_bits, 3);
    m_max_bits = min(m_max_bits, 8);
    m_bit_width = ntohs(beacon_config->bit_size) / 1000.0;
    m_zero_thresh = ntohs(beacon_config->zero_thresh) / 100.0;
    m_one_thresh = ntohs(beacon_config->one_thresh) / 100.0;

    PLAYER_TRACE2("bits %d, width %f", m_max_bits, m_bit_width);
    */
}


////////////////////////////////////////////////////////////////////////////////
// Process a single beacon
// This function tries to minimize the error between the measured pose of
// the beacon (in global cs) and the true pose of the beacon (in global cs)
// by shifting the origin of the odometric cs. 
//
void CBpsDevice::ProcessBeacon(int id, double r, double b, double o)
{
    double step = 0.01;

    assert(id > 0 && id <= 255);
    if (!this->beacon[id].isset)
        return;

    PLAYER_TRACE4("beacon in las cs: %d %f %f %f", id, r * cos(b), r * sin(b), o);
    
    // Get origin of odometrics cs in global cs
    double ox = this->org_px;
    double oy = this->org_py;
    double oa = this->org_pa;

    // Get robot pose in odometric cs
    double rx = this->odo_px;
    double ry = this->odo_py;
    double ra = this->odo_pa;
    
    // Compute measured beacon pose in odometric cs
    double box = rx + r * cos(ra + b);
    double boy = ry + r * sin(ra + b);
    double boa = ra + o;

    PLAYER_TRACE4("beacon in odo cs: %d %f %f %f", id, box, boy, boa);
    
    // Compute measured beacon pose in global cs
    double ax = ox + box * cos(oa) - boy * sin(oa);
    double ay = oy + box * sin(oa) + boy * cos(oa);
    double aa = oa + boa;

    PLAYER_TRACE4("beacon in glo cs: %d %f %f %f", id, ax, ay, aa);

    // Get true pose of beacon in global cs
    double bx = this->beacon[id].px;
    double by = this->beacon[id].py;
    double ba = this->beacon[id].pa;

    PLAYER_TRACE4("true beacon pose: %d %f %f %f", id, bx, by, ba);

    // Compute difference in pose
    // The angle is normalize to domain [-pi, pi]
    double cx = ax - bx;
    double cy = ay - by;
    double ca = atan2(sin(aa - ba), cos(aa - ba));

    // Compute weights
    double kx = 1;
    double ky = 1;
    double ka = 1;
        
    // Compute weighted error
    double err = kx * cx * cx + ky * cy * cy + ka * ca * ca;

    // Compute partials
    double derr_dax = kx * cx;
    double derr_day = ky * cy;
    double derr_daa = ka * ca;
    double dax_dox = 1;
    double dax_doy = 0;
    double dax_doa = -box * sin(oa) - boy * cos(oa);
    double day_dox = 0;
    double day_doy = 1;
    double day_doa = +box * cos(oa) - boy * sin(oa);
    double daa_dox = 0;
    double daa_doy = 0;
    double daa_doa = 1;

    // Compute full derivatives
    double derr_dox = derr_dax * dax_dox + derr_day * day_dox + derr_daa * daa_dox;
    double derr_doy = derr_dax * dax_doy + derr_day * day_doy + derr_daa * daa_doy;
    double derr_doa = derr_dax * dax_doa + derr_day * day_doa + derr_daa * daa_doa;

    // Shift the odometric origin to reduce the error term
    this->org_px -= step * derr_dox;
    this->org_py -= step * derr_doy;
    this->org_pa -= step * derr_doa;

    PLAYER_TRACE3("org = %f %f %f", this->org_px, this->org_py, this->org_pa);
    PLAYER_TRACE1("err = %f", err);
}

