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

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>     // for atoi(3)
#include <netinet/in.h> // for htons(3)
#include "bpsdevice.h"
#include "devicetable.h"

extern CDeviceTable* deviceTable;
extern int global_playerport; // used to get at devices
static void *DummyMain(void *data);


////////////////////////////////////////////////////////////////////////////////
// Useful macros
// this one is defined in playercommon.h - BPG
//#define NORMALIZE(a) atan2(sin(a), cos(a))


////////////////////////////////////////////////////////////////////////////////
// Structures used internally

struct CBpsFrame
{
    double gx, gy, ga;
    double ox, oy, oa;
    double err;
    double fx, fy, fa;
    int obs_count;
};


struct CBpsObs
{
    CBpsFrame *a_frame, *b_frame;
    double ax, ay, aa;
    double bx, by, ba;
    double ux, uy, ua;
};



////////////////////////////////////////////////////////////////////////////////
// Constructor
//
CBpsDevice::CBpsDevice(int argc, char** argv) :
  CDevice(sizeof(player_bps_data_t),0,1,1)
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
#if INCLUDE_SELFTEST
    else if (!strcmp(argv[i], "test"))
    {
      if (i + 1 < argc)
      {
        Setup();
        Test(argv[i + 1]);
        Shutdown();
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
      fprintf(stderr, "CBpsDevice: ignoring unknown parameter \"%s\"\n",
              argv[i]);
  }
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device
int CBpsDevice::Setup()
{
    // Get pointers to other devices
    this->position = deviceTable->GetDevice(global_playerport,PLAYER_POSITION_CODE, this->index);
    this->laserbeacon = deviceTable->GetDevice(global_playerport,PLAYER_LASERBEACON_CODE, this->index);
    assert(this->position != NULL);
    assert(this->laserbeacon != NULL);

    // Subscribe to devices
    this->position->GetLock()->Subscribe(this->position);
    this->laserbeacon->GetLock()->Subscribe(this->laserbeacon);
    
    // Initialise configuration settings
    this->gain = 0.01;
    this->laser_px = this->laser_py = this->laser_pa = 0;
    memset(this->beacon, 0, sizeof(this->beacon));
    this->max_frames = 8;
    this->max_obs = 16;
    assert(this->max_obs * this->max_frames < ARRAYSIZE(this->obs));

    this->odo_px = this->odo_py = this->odo_pa = 0;

    // Initialise frame and observation lists
    this->frame_count = 0;
    this->obs_count = 0;

    // Create an initial frame
    this->current = AllocFrame();
    this->current->gx = 0;
    this->current->gy = 0;
    this->current->ga = 0;
    this->current->ox = 0;
    this->current->oy = 0;
    this->current->oa = 0;
    this->current->err = 0;
    
#ifdef INCLUDE_SELFTEST
    this->dumpfile = fopen("bpsdevice.out", "w+");
    if (!this->dumpfile)
        PLAYER_ERROR1("unable to open dump file, error [%s]", strerror(errno));
#endif  

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

#ifdef INCLUDE_SELFTEST
    if (this->dumpfile)
        fclose(this->dumpfile);
#endif

    // Clear the frame and observation lists
    while (this->obs_count > 0)
        DestroyObs(this->obs[0]);
    while (this->frame_count > 0)
        DestroyFrame(this->frames[0]);
    
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

    // Nice ourself down so we dont interfere with other threads
    nice(10);

    while (TRUE)
    {
        pthread_testcancel();

#ifdef INCLUDE_SELFTEST
        // Dump current frames, obs etc
        Dump();
#endif

        // Update our pose estimate
        // TODO: this should probably run for a set amount of time
        for (int i = 0; i < 100; i++)
            UpdateEstimate();

        // Now sleep for a while so we dont use all the cpu cycles
        usleep(10);
    

                
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

            if(!ntohs(lbdata.count))
              this->err = 1;
            else
              this->err = 0;
            for (int i = 0; i < ntohs(lbdata.count); i++)
            {
                int id = lbdata.beacon[i].id;
                if (id == 0)
                    continue;

                double r = ntohs(lbdata.beacon[i].range) / 1000.0;
                double b = ((short) ntohs(lbdata.beacon[i].bearing)) * M_PI / 180.0;
                double o = ((short) ntohs(lbdata.beacon[i].orient)) * M_PI / 180.0;
                PLAYER_TRACE4("beacon : %d %f %f %f", id, r, b, o);

                // Now process this beacon measurement
                ProcessBeacon(id, r, b, o);
            }
        }
        
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
            double ox = ((int) ntohl(posdata.xpos)) / 1000.0;
            double oy = ((int) ntohl(posdata.ypos)) / 1000.0;
            double oa = ntohs(posdata.theta) * M_PI / 180;

            // Process this odometry measurement
            ProcessOdometry(ox, oy, oa);
            
            // Update our data
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
    CBpsFrame *frame = this->current;

    // Get robot odometric pose
    double ox = this->odo_px;
    double oy = this->odo_py;
    double oa = this->odo_pa;

    // Compute robot pose relative to current frame
    double rx = +(ox - frame->ox) * cos(frame->oa) + (oy - frame->oy) * sin(frame->oa);
    double ry = -(ox - frame->ox) * sin(frame->oa) + (oy - frame->oy) * cos(frame->oa);
    double ra = oa - frame->oa;
    
    // Compute current global pose
    double gx = frame->gx + rx * cos(frame->ga) - ry * sin(frame->ga);
    double gy = frame->gy + rx * sin(frame->ga) + ry * cos(frame->ga);
    double ga = frame->ga + ra;

    // Construct data packet
    ((player_bps_data_t*)this->device_data)->px = 
            htonl((int) (gx * 1000));
    ((player_bps_data_t*)this->device_data)->py = 
            htonl((int) (gy * 1000));
    ((player_bps_data_t*)this->device_data)->pa = 
            htonl((int) (ga * 180 / M_PI));
    // TODO htonl((int) (this->err * 1e6));
    ((player_bps_data_t*)this->device_data)->err = 0; 
}


////////////////////////////////////////////////////////////////////////////////
// Get data from buffer (called by client thread)
//
/*
size_t CBpsDevice::GetData(unsigned char *dest, size_t maxsize) 
{
    assert(maxsize >= sizeof(this->data));
    memcpy(dest, &this->data, sizeof(this->data));
    return sizeof(this->data);
}
*/



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
        this->beacon[id].px = (int) ntohl(setbeacon->px) / 1000.0;
        this->beacon[id].py = (int) ntohl(setbeacon->py) / 1000.0;
        this->beacon[id].pa = (int) ntohl(setbeacon->pa) * M_PI / 180.0;
        this->beacon[id].ux = (int) ntohl(setbeacon->ux) / 1000.0;
        this->beacon[id].uy = (int) ntohl(setbeacon->uy) / 1000.0;
        this->beacon[id].ua = (int) ntohl(setbeacon->ua) * M_PI / 180.0;
        this->beacon[id].isset = true;

        PLAYER_TRACE4("set beacon %d to %f %f %f", id,
                      this->beacon[id].px, this->beacon[id].py, this->beacon[id].pa);
    }
    else
        PLAYER_ERROR("config packet size is incorrect");
}


////////////////////////////////////////////////////////////////////////////////
// Process odometry
// (ox, oy, oa) is the robot pose in the odometric cs
void CBpsDevice::ProcessOdometry(double ox, double oy, double oa)
{
    double kr = 1.0;
    double ka = 1 / M_PI;

    CBpsFrame *frame = this->current;
    
    // Compute distance travelled since last update
    double dx = ox - this->odo_px;
    double dy = oy - this->odo_py;
    double da = NORMALIZE(oa - this->odo_pa);

    // Store current odometric pose for later use
    this->odo_px = ox;
    this->odo_py = oy;
    this->odo_pa = oa;    
    
    // Compute accumulated uncertainty
    //frame->err += kr * dx * dx + kr * dy * dy + ka * da * da;
    frame->err += kr * fabs(dx) + kr * fabs(dy) + ka * fabs(da);
    
    // If the uncertainty is still small,
    // do nothing; otherwise, continue on to create new frame.
    if (frame->err <= 1)
        return;

    // Create a new frame
    CBpsFrame *newframe = AllocFrame();

    // Create a new observation
    CBpsObs *obs = AllocObs(frame, newframe);

    // Compute current robot pose relative to current frame
    obs->ax = +(ox - frame->ox) * cos(frame->oa) + (oy - frame->oy) * sin(frame->oa);
    obs->ay = -(ox - frame->ox) * sin(frame->oa) + (oy - frame->oy) * cos(frame->oa);
    obs->aa = oa - frame->oa;

    // Compute current robot pose relative to new frame
    // Is zero by definition.
    obs->bx = 0;
    obs->by = 0;
    obs->ba = 0;

    // Compute uncertainty in observation
    obs->ux = 0.10; // TODO
    obs->uy = 0.10;
    obs->ua = 0.10;

    // Compute initial pose of new frame in global cs
    newframe->gx = frame->gx + obs->ax * cos(frame->ga) - obs->ay * sin(frame->ga);
    newframe->gy = frame->gy + obs->ax * sin(frame->ga) + obs->ay * cos(frame->ga);
    newframe->ga = frame->ga + obs->aa;

    // Set pose of new frame in odometric cs
    newframe->ox = ox;
    newframe->oy = oy;
    newframe->oa = oa;

    newframe->err = 0;
    
    // Now start using the new frame
    this->current = newframe;
}


////////////////////////////////////////////////////////////////////////////////
// Process a single beacon
// (r, b, o) is the beacon range, bearing and orientation of the beacon,
// relative to the sensor.
void CBpsDevice::ProcessBeacon(int id, double r, double b, double o)
{
    CBpsFrame *frame = this->current;

    // Check that the beacon id is valid and informative
    assert(id > 0 && id <= 255);
    if (!this->beacon[id].isset)
        return;

    // Compute pose of laser in odometrics cs
    double lx = this->odo_px + this->laser_px * cos(this->odo_pa) -
        this->laser_py * sin(this->odo_pa);
    double ly = this->odo_py + this->laser_px * sin(this->odo_pa) +
        this->laser_py * cos(this->odo_pa);
    double la = this->odo_pa + this->laser_pa;
    
    // Compute pose of beacon in odometric cs
    double bx = lx + r * cos(la + b);
    double by = ly + r * sin(la + b);
    double ba = la + o;
    
    // Create a new observation
    CBpsObs *obs = AllocObs(frame, NULL);
    
    // Compute and store measured beacon pose relative to current frame
    obs->ax = +(bx - frame->ox) * cos(frame->oa) + (by - frame->oy) * sin(frame->oa);
    obs->ay = -(bx - frame->ox) * sin(frame->oa) + (by - frame->oy) * cos(frame->oa);
    obs->aa = ba - frame->oa;

    // Compute and store uncertainty in observation
    obs->ux = 0.10; // TODO
    obs->uy = 0.10;
    obs->ua = 0.10;

    // Compute and store modelled beacon pose relative to global cs
    obs->bx = this->beacon[id].px;
    obs->by = this->beacon[id].py;
    obs->ba = this->beacon[id].pa;

    // Compute and store uncertainty in model
    obs->ux += this->beacon[id].ux;
    obs->uy += this->beacon[id].uy;
    obs->ua += this->beacon[id].ua;
}


////////////////////////////////////////////////////////////////////////////////
// Update our estimate of the pose of each frame
// Returns: total energy
double CBpsDevice::UpdateEstimate()
{
    double kr = 1e-5;
    double ka = 1e-5;
    double k = 1;

    double u = 0;
    
    // Zero all forces
    for (int i = 0; i < this->frame_count; i++)
    {
        CBpsFrame *frame = this->frames[i];
        frame->fx = frame->fy = frame->fa = 0;
    }

    // Compute force generated by each observation
    for (int i = 0; i < this->obs_count; i++)
    {
        CBpsObs *obs = this->obs[i];
        u += ComputeForce(obs);
    }

    // Now apply the forces
    for (int i = 0; i < this->frame_count; i++)
    {
        CBpsFrame *frame = this->frames[i];

        //*** printf("%f %f %f\n", frame->fx, frame->fy, frame->fa);

        frame->gx += k * kr * frame->fx;
        frame->gy += k * kr * frame->fy;
        frame->ga += k * ka * frame->fa;
    }

    return u;
}


////////////////////////////////////////////////////////////////////////////////
// Compute the force generated by an observation
// Returns: energy stored in observation
double CBpsDevice::ComputeForce(CBpsObs *obs)
{
    double ax, ay, aa;
    double bx, by, ba;
    double cx, cy, ca;
    double kx, ky, ka, u;
    double du_dcx, du_dcy, du_dca;
    double dax_dgx, dax_dgy, dax_dga;
    double day_dgx, day_dgy, day_dga;
    double daa_dgx, daa_dgy, daa_dga;
    // i added the following initializations to quash the compiler warnings
    //       - BPG
    double dbx_dgx=0, dbx_dgy=0, dbx_dga=0;
    double dby_dgx=0, dby_dgy=0, dby_dga=0;
    double dba_dgx=0, dba_dgy=0, dba_dga=0;
    double fax, fay, faa;
    double fbx, fby, fba;
    
    // Compute pose in global cs according to a
    ax = obs->a_frame->gx +
        obs->ax * cos(obs->a_frame->ga) - obs->ay * sin(obs->a_frame->ga);
    ay = obs->a_frame->gy +
        obs->ax * sin(obs->a_frame->ga) + obs->ay * cos(obs->a_frame->ga);
    aa = obs->a_frame->ga + obs->aa;

    dax_dgx = 1;
    dax_dgy = 0;
    dax_dga = -obs->ax * sin(obs->a_frame->ga) - obs->ay * cos(obs->a_frame->ga);
    day_dgx = 0;
    day_dgy = 1;
    day_dga = +obs->ax * cos(obs->a_frame->ga) - obs->ay * sin(obs->a_frame->ga);
    daa_dgx = 0;
    daa_dgy = 0;
    daa_dga = 1;
    
    // Compute pose in global cs according to b
    if (obs->b_frame)
    {
        bx = obs->b_frame->gx +
            obs->bx * cos(obs->b_frame->ga) - obs->by * sin(obs->b_frame->ga);
        by = obs->b_frame->gy +
            obs->bx * sin(obs->b_frame->ga) + obs->by * cos(obs->b_frame->ga);
        ba = obs->b_frame->ga + obs->ba;

        dbx_dgx = 1;
        dbx_dgy = 0;
        dbx_dga = -obs->bx * sin(obs->b_frame->ga) - obs->by * cos(obs->b_frame->ga);
        dby_dgx = 0;
        dby_dgy = 1;
        dby_dga = +obs->bx * cos(obs->b_frame->ga) - obs->by * sin(obs->b_frame->ga);
        dba_dgx = 0;
        dba_dgy = 0;
        dba_dga = 1;
    }
    else
    {
        bx = obs->bx;
        by = obs->by;
        ba = obs->ba;
    }

    // Compute spring constants
    kx = 1 / (obs->ux * obs->ux + 0.01);
    ky = 1 / (obs->uy * obs->uy + 0.01);
    ka = 1 / (obs->ua * obs->ua + 0.01);

    // Compute the difference
    cx = bx - ax;
    cy = by - ay;
    ca = NORMALIZE(ba - aa);

    // Compute weighted energy term
    u = kx * cx * cx / 2 + ky * cy * cy / 2 + ka * ca * ca / 2;

    // Compute basic derivatives
    du_dcx = kx * cx;
    du_dcy = ky * cy;
    du_dca = ka * ca;

    // Now compute total derivatives
    fax = +du_dcx * dax_dgx + du_dcy * day_dgx + du_dca * daa_dgx;
    fay = +du_dcx * dax_dgy + du_dcy * day_dgy + du_dca * daa_dgy;
    faa = +du_dcx * dax_dga + du_dcy * day_dga + du_dca * daa_dga;

    obs->a_frame->fx += fax;
    obs->a_frame->fy += fay;
    obs->a_frame->fa += faa;

    if (obs->b_frame)
    {
        fbx = -du_dcx * dbx_dgx - du_dcy * dby_dgx - du_dca * dba_dgx;
        fby = -du_dcx * dbx_dgy - du_dcy * dby_dgy - du_dca * dba_dgy;
        fba = -du_dcx * dbx_dga - du_dcy * dby_dga - du_dca * dba_dga;

        obs->b_frame->fx += fbx;
        obs->b_frame->fy += fby;
        obs->b_frame->fa += fba;
    }

    return u;
}


////////////////////////////////////////////////////////////////////////////////
// Allocate a new frame
// This may destroy an old frame to make room.
CBpsFrame *CBpsDevice::AllocFrame()
{
    // If we alread have lots of frames, remove an old one
    if (this->frame_count >= this->max_frames)
    {
        // Pick a frame to remove
        CBpsFrame *frame = this->frames[0];

        // Remove all observations pointing to this frame
        for (int i = 0; i < this->obs_count; )
        {
            CBpsObs *obs = this->obs[i];

            if (obs->a_frame == frame || obs->b_frame == frame)
                DestroyObs(obs);
            else
                i++;
        }

        // Destroy the frame
        DestroyFrame(frame);
    }

    CBpsFrame *frame = CreateFrame();
    frame->obs_count = 0;

    return frame;
}


////////////////////////////////////////////////////////////////////////////////
// Allocate a new observation
// This may destroy an old observation to make room
CBpsObs *CBpsDevice::AllocObs(CBpsFrame *a_frame, CBpsFrame *b_frame)
{
    // If we already have lots of observations for this frame,
    // remove an old one
    if (a_frame->obs_count >= this->max_obs)
    {
        for (int i = 0; i < this->obs_count; i++)
        {
            CBpsObs *obs = this->obs[i];

            if (obs->a_frame == a_frame)
            {
                a_frame->obs_count--;
                DestroyObs(obs);
                break;
            }
        }
    }
    
    CBpsObs *obs = CreateObs();
    obs->a_frame = a_frame;
    obs->b_frame = b_frame;
    a_frame->obs_count++;
  
    return obs;
}


////////////////////////////////////////////////////////////////////////////////
// Create a frame
CBpsFrame *CBpsDevice::CreateFrame()
{
    // Create a new frame
    CBpsFrame *frame = new CBpsFrame;

    // Store it in the list
    assert(this->frame_count < ARRAYSIZE(this->frames));
    this->frames[this->frame_count++] = frame;

    return frame;
}


////////////////////////////////////////////////////////////////////////////////
// Destroy a frame
void CBpsDevice::DestroyFrame(CBpsFrame *frame)
{
    for (int i = 0; i < this->frame_count; i++)
    {
        if (this->frames[i] == frame)
        {
            memmove(this->frames + i, this->frames + i + 1,
                    (this->frame_count - i - 1) * sizeof(this->frames[0]));
            this->frame_count--;
            break;
        }
    }
    delete frame;
}


////////////////////////////////////////////////////////////////////////////////
// Create an observation
CBpsObs *CBpsDevice::CreateObs()
{
    // Create a new obstacle
    CBpsObs *obs = new CBpsObs;

    // Store it in the list
    assert(this->obs_count < ARRAYSIZE(this->obs));
    this->obs[this->obs_count++] = obs;

    return obs;
}


////////////////////////////////////////////////////////////////////////////////
// Destroy an observation
void CBpsDevice::DestroyObs(CBpsObs *obs)
{
    for (int i = 0; i < this->obs_count; i++)
    {
        if (this->obs[i] == obs)
        {
            memmove(this->obs + i, this->obs + i + 1,
                    (this->obs_count - i - 1) * sizeof(this->obs[0]));
            this->obs_count--;
            break;
        }
    }
    delete obs;
}

#ifdef INCLUDE_SELFTEST

////////////////////////////////////////////////////////////////////////////////
// Read in and process a log-file
void CBpsDevice::Test(const char *filename)
{
    double gps_px, gps_py, gps_pa;
    
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        PLAYER_ERROR2("unable to open [%s] : error [%s]", filename, strerror(errno));
        return;
    }

    while (true)
    {
        char line[4096];
        if (!fgets(line, sizeof(line), file))
            break;
        
        char *type = strtok(line, " ");

        // Look for 'beacon' entries;
        // this is a sneaky way of loading the map
        if (strcmp(type, "beacon") == 0)
        {
            int id = atoi(strtok(NULL, " "));
            this->beacon[id].px = atof(strtok(NULL, " "));
            this->beacon[id].py = atof(strtok(NULL, " "));
            this->beacon[id].pa = atof(strtok(NULL, " ")) * M_PI / 180;
            this->beacon[id].ux = 0;
            this->beacon[id].uy = 0;
            this->beacon[id].ua = 0;
            this->beacon[id].isset = true;
        }

        // Look for gps entries;
        // this is useful comparing with ground-truth
        if (strcmp(type, "gps") == 0)
        {
            strtok(NULL, " ");
            strtok(NULL, " ");
            gps_px = atof(strtok(NULL, " ")) / 1000;
            gps_py = atof(strtok(NULL, " ")) / 1000;
            gps_pa = atof(strtok(NULL, " ")) * M_PI / 180;
        }
        
        if (strcmp(type, "position") == 0)
        {
            strtok(NULL, " ");
            strtok(NULL, " ");
            double ox = atof(strtok(NULL, " ")) / 1000;
            double oy = atof(strtok(NULL, " ")) / 1000;
            double oa = atof(strtok(NULL, " ")) * M_PI / 180;
            ProcessOdometry(ox, oy, oa);

            // Update our pose
            PutData(NULL, 0);
        }
        
        if (strcmp(type, "laser_beacon") == 0)
        {
            strtok(NULL, " ");
            strtok(NULL, " ");

            while (true)
            {
                char *sid = strtok(NULL, " \n");
                if (!sid)
                    break;
                int id = atoi(sid);
                double range = atof(strtok(NULL, " ")) / 1000;
                double bearing = atof(strtok(NULL, " ")) * M_PI / 180;
                double orient = atof(strtok(NULL, " ")) * M_PI / 180;

                if (id > 0)
                    ProcessBeacon(id, range, bearing, orient);
            }

            for (int i = 0; i < 100; i++)
                UpdateEstimate();
        }

        double gx = (int) ntohl(this->data.px) / 1000.0;
        double gy = (int) ntohl(this->data.py) / 1000.0;
        double ga = (int) ntohl(this->data.pa) * M_PI / 180.0;    
        printf("%f %f %f %f %f %f\n", gps_px, gps_py, gps_pa, gx, gy, ga);
                      
        Dump();
    }
}


////////////////////////////////////////////////////////////////////////////////
// Dump out frames, observations
void CBpsDevice::Dump()
{
    if (!this->dumpfile)
        return;
    
    for (int i = 0; i < this->frame_count; i++)
    {
        CBpsFrame *frame = this->frames[i];

        fprintf(this->dumpfile, "frame %p %f %f %f %f\n",
                frame, frame->gx, frame->gy, frame->ga,
                frame->err);
    }

    for (int i = 0; i < this->obs_count; i++)
    {
        CBpsObs *obs = this->obs[i];

        fprintf(this->dumpfile, "obs %p %p %f %f %f %f %f %f\n",
                obs->a_frame, obs->b_frame, obs->ax, obs->ay, obs->aa,
                obs->bx, obs->by, obs->ba);
    }

    fprintf(this->dumpfile, "\n");
}

#endif



/*
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
}

*/
