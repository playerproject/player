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

#define PLAYER_ENABLE_TRACE 0

#include <string.h>
#include <math.h>
#include <stdlib.h>     // for atoi(3)
#include <netinet/in.h> // for htons(3)
#include "bpsdevice.h"
#include "devicetable.h"

extern CDeviceTable* deviceTable;
static void *DummyMain(void *data);
    

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
    this->gain = 0.01;
    this->laser_px = this->laser_py = this->laser_pa = 0;
    memset(this->beacon, 0, sizeof(this->beacon));
    this->odo_px = this->odo_py = this->odo_pa = 0;
    this->org_px = this->org_py = this->org_pa = 0;
    this->err = 0;

    // Hack to get around mutex on GetData
    GetLock()->PutData(this, NULL, 0);
    
    // Start our own thread
    pthread_create(&this->thread, NULL, DummyMain, this );
    
    PLAYER_TRACE0("setup");
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
//
int CBpsDevice::Shutdown()
{
    // Stop the thread
    //
    pthread_cancel(this->thread);
    pthread_join(this->thread, NULL);
    
    // Unsubscribe from the laser device
    //
    this->position->GetLock()->Unsubscribe(this->position);    
    this->laserbeacon->GetLock()->Unsubscribe(this->laserbeacon);

    PLAYER_TRACE0("shutdown");
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Dummy thread entry point
static void *DummyMain(void *data)
{
    ((CBpsDevice*) data)->Main();
    return NULL;
}


////////////////////////////////////////////////////////////////////////////////
// Device thread
//
void CBpsDevice::Main()
{
    uint32_t sec, usec;
    
    PLAYER_TRACE0("main");

    while (TRUE)
    {
        pthread_testcancel();
        usleep(10);
    
        // Get the odometry data
        player_position_data_t posdata;
        this->position->GetLock()->GetData(this->position, (uint8_t*) &posdata,
                                           sizeof(posdata), &sec, &usec);

        // If odometry data is new, process it...
        if (!(sec == this->position_sec && usec == this->position_usec))
        {
            PLAYER_TRACE2("odometry time : %u.%06u", sec, usec);
            
            this->position_sec = sec;
            this->position_usec = usec;
            
            // Compute odometric pose in SI units
            this->odo_px = ((int) ntohl(posdata.xpos)) / 1000.0;
            this->odo_py = ((int) ntohl(posdata.ypos)) / 1000.0;
            this->odo_pa = ntohs(posdata.theta) * M_PI / 180;

            PLAYER_TRACE3("odometry : %f %f %f",
                          this->odo_px, this->odo_py, this->odo_pa);

            // Update our data
            //
            GetLock()->PutData(this, NULL, 0);
        }
                
        // Get the beacon data
        player_laserbeacon_data_t lbdata;
        this->laserbeacon->GetLock()->GetData(this->laserbeacon,
                                              (uint8_t*) &lbdata,
                                              sizeof(lbdata), &sec, &usec);

        // If beacon data is new, process it...
        if (!(sec == this->beacon_sec && usec == this->beacon_usec))
        {
            PLAYER_TRACE2("beacon time : %u.%06u", sec, usec);
            
            this->beacon_sec = sec;
            this->beacon_usec = usec;

            for (int i = 0; i < ntohs(lbdata.count); i++)
            {
                int id = lbdata.beacon[i].id;
                if (id == 0)
                    continue;

                double r = ntohs(lbdata.beacon[i].range) / 1000.0;
                double b = ((short) ntohs(lbdata.beacon[i].bearing)) * M_PI / 180.0;
                double o = ((short) ntohs(lbdata.beacon[i].orient)) * M_PI / 180.0;

                PLAYER_TRACE4("beacon : %d %f %f %f", id, r, b, o);

                // Now process this beacon
                err = ProcessBeacon(id, r, b, o);
    
                // Totally bogus filter on error term
                if (err >= 0)
                {
                    double tc = 0.5;
                    this->err = (1 - tc) * this->err + tc * err;
                }
            }

            // Update our data
            //
            GetLock()->PutData(this, NULL, 0);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// Put data in buffer (called by device thread)
// Note that the arguments are ignored.
//
void CBpsDevice::PutData(unsigned char *src, size_t maxsize)
{
    // Compute current global pose
    double gx = this->org_px +
        this->odo_px * cos(this->org_pa) - this->odo_py * sin(this->org_pa);
    double gy = this->org_py +
        this->odo_px * sin(this->org_pa) + this->odo_py * cos(this->org_pa);
    double ga = this->org_pa + this->odo_pa;

    // Construct data packet
    this->data.px = htonl((int) (gx * 1000));
    this->data.py = htonl((int) (gy * 1000));
    this->data.pa = htonl((int) (ga * 180 / M_PI));
    this->data.err = htonl((int) (this->err * 1e6));
}


////////////////////////////////////////////////////////////////////////////////
// Get data from buffer (called by client thread)
//
size_t CBpsDevice::GetData(unsigned char *dest, size_t maxsize) 
{
    assert(maxsize >= sizeof(this->data));
    memcpy(dest, &this->data, sizeof(this->data));
    return sizeof(this->data);
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
    if (maxsize == sizeof(player_bps_setgain_t))
    {
        player_bps_setgain_t *setgain = (player_bps_setgain_t*) src;
        if (setgain->subtype != PLAYER_BPS_SUBTYPE_SETGAIN)
        {
            PLAYER_ERROR("config packet has incorrect subtype");
            return;
        }
        this->gain = ntohl(setgain->gain) / 1e6;
    }
    else if (maxsize == sizeof(player_bps_setlaser_t))
    {
        player_bps_setlaser_t *setlaser = (player_bps_setlaser_t*) src;
        if (setlaser->subtype != PLAYER_BPS_SUBTYPE_SETLASER)
        {
            PLAYER_ERROR("config packet has incorrect subtype");
            return;
        }

        this->laser_px = (int) ntohl(setlaser->px) / 1000.0;
        this->laser_py = (int) ntohl(setlaser->py) / 1000.0;
        this->laser_pa = (int) ntohl(setlaser->pa) * M_PI / 180.0;

        PLAYER_TRACE3("set laser to %f %f %f",
                      this->laser_px, this->laser_py, this->laser_pa);
    }
    else if (maxsize == sizeof(player_bps_setbeacon_t))
    {
        player_bps_setbeacon_t *setbeacon = (player_bps_setbeacon_t*) src;
        if (setbeacon->subtype != PLAYER_BPS_SUBTYPE_SETBEACON)
        {
            PLAYER_ERROR("config packet has incorrect subtype");
            return;
        }

        int id = setbeacon->id;
        this->beacon[id].isset = true;
        this->beacon[id].px = (int) ntohl(setbeacon->px) / 1000.0;
        this->beacon[id].py = (int) ntohl(setbeacon->py) / 1000.0;
        this->beacon[id].pa = (int) ntohl(setbeacon->pa) * M_PI / 180.0;
        this->beacon[id].ux = (int) ntohl(setbeacon->ux) / 1000.0;
        this->beacon[id].uy = (int) ntohl(setbeacon->uy) / 1000.0;
        this->beacon[id].ua = (int) ntohl(setbeacon->ua) * M_PI / 180.0;

        PLAYER_TRACE4("set beacon %d to %f %f %f", id,
                      this->beacon[id].px, this->beacon[id].py, this->beacon[id].pa);
    }
    else
        PLAYER_ERROR("config packet size is incorrect");
}


////////////////////////////////////////////////////////////////////////////////
// Process a single beacon
// This function tries to minimize the error between the measured pose of
// the beacon (in global cs) and the true pose of the beacon (in global cs)
// by shifting the origin of the odometric cs. 
//
double CBpsDevice::ProcessBeacon(int id, double r, double b, double o)
{
    assert(id > 0 && id <= 255);
    if (!this->beacon[id].isset)
        return -1;

    PLAYER_TRACE4("beacon in laser cs: %d %f %f %f", id, r * cos(b), r * sin(b), o);
    
    // Get robot pose in odometric cs
    double ox = this->odo_px;
    double oy = this->odo_py;
    double oa = this->odo_pa;
    
    // Compute robot pose in global cs
    double rx = this->org_px + ox * cos(this->org_pa) - oy * sin(this->org_pa);
    double ry = this->org_py + ox * sin(this->org_pa) + oy * cos(this->org_pa);
    double ra = this->org_pa + oa;
    PLAYER_TRACE3("robot in global cs : %f %f %f", rx, ry, ra);

    // Compute laser pose in global cs
    double lx = rx + this->laser_px * cos(ra) - this->laser_py * sin(ra);
    double ly = ry + this->laser_px * sin(ra) + this->laser_py * cos(ra);
    double la = ra + this->laser_pa;
    PLAYER_TRACE3("laser in global cs : %f %f %f", lx, ly, la);
    
    // Compute measured beacon pose in global cs
    double ax = lx + r * cos(ra + b);
    double ay = ly + r * sin(ra + b);
    double aa = la + o;
    PLAYER_TRACE4("beacon in global cs: %d %f %f %f", id, ax, ay, aa);
    
    // Get true beacon pose in global cs
    double bx = this->beacon[id].px;
    double by = this->beacon[id].py;
    double ba = this->beacon[id].pa;
    PLAYER_TRACE4("true beacon pose   : %d %f %f %f", id, bx, by, ba);

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

    // Compute partials wrt laser pose
    double derr_dax = kx * cx;
    double derr_day = ky * cy;
    double derr_daa = ka * ca;
    double dax_dlx = 1;
    double dax_dly = 0;
    double dax_dla = -r * sin(la + b);
    double day_dlx = 0;
    double day_dly = 1;
    double day_dla = +r * cos(la + b);
    double daa_dlx = 0;
    double daa_dly = 0;
    double daa_dla = 1;

    // Compute full derivatives wrt laser pose
    double derr_dlx = derr_dax * dax_dlx + derr_day * day_dlx + derr_daa * daa_dlx;
    double derr_dly = derr_dax * dax_dly + derr_day * day_dly + derr_daa * daa_dly;
    double derr_dla = derr_dax * dax_dla + derr_day * day_dla + derr_daa * daa_dla;

    // Compute new laser pose in global cs
    lx -= this->gain * derr_dlx;
    ly -= this->gain * derr_dly;
    la -= this->gain * derr_dla;

    // Compute new robot pose in global cs
    ra = la - this->laser_pa;
    rx = lx - this->laser_px * cos(ra) + this->laser_py * sin(ra);
    ry = ly - this->laser_px * sin(ra) - this->laser_py * cos(ra);

    PLAYER_TRACE3("robot in global cs : %f %f %f", rx, ry, ra);

    // Compute odometric origin needed to yield this pose
    this->org_pa = ra - oa;
    this->org_px = rx - ox * cos(this->org_pa) + oy * sin(this->org_pa);
    this->org_py = ry - ox * sin(this->org_pa) - oy * cos(this->org_pa);

    PLAYER_TRACE3("org = %f %f %f", this->org_px, this->org_py, this->org_pa);
    PLAYER_TRACE1("err = %f", err);

    return err;

    /*
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
    this->org_px -= this->gain * derr_dox;
    this->org_py -= this->gain * derr_doy;
    this->org_pa -= this->gain * derr_doa;

    PLAYER_TRACE3("org = %f %f %f", this->org_px, this->org_py, this->org_pa);
    PLAYER_TRACE1("err = %f", err);
    */
}

