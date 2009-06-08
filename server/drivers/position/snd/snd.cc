/*  Smooth ND driver for Player/Stage
 *  
 *  SND Authors:  Joey Durham (algorithm) , 
 *                Luca Invernizzi (driver implementation)
 *
 *  Implemented on top of Player - One Hell of a Robot Server
 *  Copyright (C) 2003  (Brian Gerkey, Andrew Howard)
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

/*
 * Desc: Implementation of Joey Durham's "Smooth ND" algorithm for 
 * obstacle avoidance.
 * The algorithm is described in:
 * - http://ieeexplore.ieee.org/xpls/abs_all.jsp?arnumber=4651071
 * - http://motion.mee.ucsb.edu/pdf/2008a-db.pdf
 * Author: Luca Invernizzi
 * Date: 10 March 2009
 */


/** @ingroup drivers */
/** @{ */
/** @defgroup driver_snd snd
 * @brief Smooth Nearness Diagram Navigation

This driver implements the Smooth Nearness Diagram Navigation algorithm,
an improvement over the @ref nd driver that removes oscillatory patterns
and improves overall driver performance. 
.
This algorithm handles local collision-avoidance and goal-seeking and is
designed for non-holonomic, non-circular robots operating in tight spaces.
The algorithm is in the following paper:

Durham, J. ; Bullo, F. "Smooth Nearness-Diagram Navigation" 2008,
    IEEE/RSJ International Conference on Intelligent Robots and Systems,
    2008. IROS 2008, 690-695

This driver reads pose information from a @ref interface_position2d
device, sensor data from a @ref interface_laser device and/or @ref
interface_sonar device, and writes commands to a @ref interface_position2d
device.  The two @ref interface_position2d devices can be the same.
At least one device of type @ref interface_laser must
be provided.

The driver itself supports the @interface_position2d interface.  Send
@ref PLAYER_POSITION2D_CMD_POS commands to set the goal pose.  The driver
also accepts @ref PLAYER_POSITION2D_CMD_VEL commands, simply passing them
through to the underlying output device.

This driver commands the underlying  @interface_position2d simply in velocity,
so it's suitable to be used on the robots for which the GoTo() function is not
yet implemented  (e.g.: the Erratic robot).


@par Compile-time dependencies

- none

@par Provides

- @ref interface_position2d

@par Requires

- "input" @ref interface_position2d : source of pose and velocity information
- "output" @ref interface_position2d : sink for velocity commands to control the robot
- @ref interface_laser : the laser to read from

@par Configuration requests

- all @ref interface_position2d requests are passed through to the
underlying "output" @ref interface_position2d device.

@par Configuration file options

- robot_radius (meters)
  - Default: 0.25 (m)
  - The radius of the minimum circle  which contains the robot

- min_gap_width (meters)
  - Default: 2*Robot radius
  - Minimum passage width the driver will try to exploit

- obstacle_avoid_dist(meters)
  - Default: 4*Robot radius
  - Maximum distance allowed from an obstacle


- max_speed (meters/s)
  - Default: 0.5
  - Maximum speed allowed 

- max_turn_rate (radiants/sec)
  - Default: 60 degrees
  - Maximum angular speed allowed 

- goal_position_tol (meters)
  - Default: Robot radius/2
  - Maximum distance allowed from the final goal for the algorithm to stop.

- goal_angle_tol (radiants)
  - Default: 30 degrees 
  - Maximum angular error from the final goal position for the algorithm to stop

@par Example

@verbatim
driver
(
  name "snd"
  provides ["position2d:1"]
  requires ["input:::position2d:0" "output:::position2d:0" "laser:0"]
  robot_radius 0.24
)
@endverbatim

@author Joey  Durham (underlying algorithm), Luca Invernizzi (driver integration and documentation)

*/
/** @} */

#if !defined (WIN32)
	#include <unistd.h>
	#include <netinet/in.h>
#endif
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <vector>
#include <libplayercore/playercore.h>

#include "snd.h"
#include "gap_nd_nav.h"

using namespace std;


////////////////////////////////////////////////////////////////////////////////
//                           SHARED OBJECT CREATION                           //
////////////////////////////////////////////////////////////////////////////////

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver* snd_Init(ConfigFile* cf, int section)
{
  // Create and return a new instance of this driver
  snd_Proxy* snd_proxy = new snd_Proxy(cf, section);
  return ((Driver*) snd_proxy);
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void snd_Register(DriverTable* table)
{
  table->AddDriver("snd", snd_Init);
}

// Extra stuff for building a shared object.
/* need the extern to avoid C++ name-mangling  */
extern "C"
{
  int player_driver_init(DriverTable* table)
  {
    snd_Register(table);
    return(0);
  }
}


////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
snd::snd(ConfigFile* cf, int section) 
  : ThreadedDriver(cf, section)
                                                  
{
    PLAYER_MSG0(3,"INITIALIZING INTERFACE ...");
    //Failsafe, shouldn't be needed
    this->goalX=0.0;
    this->goalY=0.0;
    this->data_odometry_ready=0;
    this->data_laser_ready=0;
    this->first_goal_has_been_set_to_init_position = 0;
  // Create my position interface
  if (cf->ReadDeviceAddr(&(this->m_position_addr), section, 
                         "provides", PLAYER_POSITION2D_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);
    return;
  }  
  if (this->AddInterface(this->m_position_addr))
  {
    this->SetError(-1);    
    return;
  }
  // Laser subscription address
  if (cf->ReadDeviceAddr(&(this->laser_addr), section, 
                         "requires", PLAYER_LASER_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);
    return;
  }
  // Odometry subscription address
  if (cf->ReadDeviceAddr(&(this->odom_in_addr), section, 
                         "requires", PLAYER_POSITION2D_CODE, -1, "input") != 0)
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->odom_out_addr), section, 
                         "requires", PLAYER_POSITION2D_CODE, -1, "output") != 0)
  {
    this->SetError(-1);
    return;
  }
    PLAYER_MSG0(3,"INTERFACE INITIALIZED");
    
    
    robot_radius = cf->ReadTupleLength(section, "robot_radius", 0, 0.25);
    min_gap_width = cf->ReadTupleLength(section, "min_gap_width", 0, 2*robot_radius);
	obstacle_avoid_dist = cf->ReadTupleLength(section, "obstacle_avoid_dist", 0, 4*robot_radius);
	max_speed = cf->ReadTupleLength(section, "max_speed", 0, 0.5);
	max_turn_rate = cf->ReadTupleLength(section, "max_turn_rate", 0, DTOR(60.0));
	goal_position_tol = cf->ReadTupleLength(section, "goal_tol", 0, robot_radius/2);
	goal_angle_tol = cf->ReadTupleLength(section, "goal_tol", 1, DTOR(30.0));
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.


int snd::Odometry_Setup()
{
    PLAYER_MSG0(3,"SETTING UP THE ODOMETRY  1...");
  // Subscribe to the odometry device
  if(!(this->odom_in_dev = deviceTable->GetDevice(this->odom_in_addr)))
  {
    PLAYER_ERROR("unable to locate suitable odometry device");
    return(-1);
  }
    PLAYER_MSG0(3,"SETTING UP THE ODOMETRY 2 ...");
  if(this->odom_in_dev->Subscribe(this->InQueue) != 0)
  {
    PLAYER_ERROR("unable to subscribe to odometry device");
    return(-1);
  }
    PLAYER_MSG0(3,"SETTING UP THE ODOMETRY 3 ...");
  if(!(this->odom_out_dev = deviceTable->GetDevice(this->odom_out_addr)))
  {
    PLAYER_ERROR("unable to locate suitable odometry device");
    return(-1);
  }
    PLAYER_MSG0(3,"SETTING UP THE ODOMETRY 4 ...");
  if(this->odom_out_dev->Subscribe(this->InQueue) != 0)
  {
    PLAYER_ERROR("unable to subscribe to odometry device");
    return(-1);
  }
  PLAYER_MSG0(3,"SETTING UP THE ODOMETRY 5 ...");

  return 0;
}


int snd::Laser_Setup()
{
  // Subscribe to the laser device
    PLAYER_MSG0(3,"SETTING UP THE LASER 1 ...");
  if(!(this->laser_dev = deviceTable->GetDevice(this->laser_addr)))
  {
    PLAYER_ERROR("unable to locate suitable laser device");
    return(-1);
  }
    PLAYER_MSG0(3,"SETTING UP THE LASER 2 ...");
  if(this->laser_dev->Subscribe(this->InQueue) != 0)
  {
      PLAYER_ERROR("unable to subscribe to laser device");
      return(-1);
  }
  PLAYER_MSG0(3,"SETTING UP THE LASER 3 ...");
  return 0;
}


int snd::Setup()
{   
    PLAYER_MSG0(3,"SETTING UP THE DRIVER ...");
    
    if (this->Odometry_Setup()!=0 ) return -1;
    if (this->Laser_Setup() !=0   ) return -1;
    PLAYER_MSG0(3,"DRIVER READY");

  // Start the device thread; spawns a new thread and executes
  // snd::Main(), which contains the main loop for the driver.
  this->StartThread();

  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int snd::Shutdown()
{

  // Stop and join the driver thread
  PLAYER_MSG0(3,"DRIVER SHUTDOWN");
  this->StopThread();

  // Unsubscribe from the laser
  this->laser_dev->Unsubscribe(this->InQueue);
  // Unsubscribe from the odometry
  this->odom_in_dev->Unsubscribe(this->InQueue);
  this->odom_out_dev->Unsubscribe(this->InQueue);
  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void snd::Main() 
{
    PLAYER_MSG0(3,"MAIN INIT");
	pthread_attr_t thread_attr;
    pthread_mutexattr_t goal_mutex_attr, data_mutex_attr;
    pthread_condattr_t goal_changed_cond_attr, data_changed_cond_attr;

    pthread_condattr_init(&goal_changed_cond_attr);
    pthread_cond_init(&(this->goal_changed_cond), &goal_changed_cond_attr);
    pthread_condattr_destroy(&goal_changed_cond_attr);
    goal_changed = true;

    pthread_mutexattr_init(&goal_mutex_attr);
    pthread_mutex_init(&(this->goal_mutex), &goal_mutex_attr);
    pthread_mutexattr_destroy(&goal_mutex_attr);

    pthread_condattr_init(&data_changed_cond_attr);
    pthread_cond_init(&(this->data_changed_cond), &data_changed_cond_attr);
    pthread_condattr_destroy(&data_changed_cond_attr);
    data_changed = true;

    pthread_mutexattr_init(&data_mutex_attr);
    pthread_mutex_init(&(this->data_mutex), &data_mutex_attr);
    pthread_mutexattr_destroy(&data_mutex_attr);


    // Wait for the first goal to be set to the initial position
    while (first_goal_has_been_set_to_init_position==0){
        this->Wait();
        this->ProcessMessages();
    }
	pthread_attr_init(&thread_attr);
	pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);




    if ( pthread_create( &(this->algorithm_thread), NULL, main_algorithm, (void*)this ) ) {
         fprintf (stderr, "error creating thread\n");
    }
    PLAYER_MSG0(3,"GOING");
	pthread_attr_destroy(&thread_attr);


  // The main loop; interact with the device here
  for(;;)
  {
    // test if we are supposed to cancel
    pthread_testcancel();

    // Process incoming messages.  Calls ProcessMessage() on each pending
    // message.
    this->ProcessMessages();

    // Sleep (or you might, for example, block on a read() instead)
    usleep(100000);
  }
  return;
}

////////////////////////////////////////////////////////////////////////////////

void snd::SetSpeedCmd(player_position2d_cmd_vel_t cmd)
{
//  cmd.state = 1;
  // cmd.type = 0;
  this->odom_out_dev->PutMsg(this->InQueue,
                             PLAYER_MSGTYPE_CMD,
                             PLAYER_POSITION2D_CMD_VEL,
                             (void*)&cmd,sizeof(cmd),NULL);

    PLAYER_MSG0(3,"speed cmd received");
}



////////////////////////////////////////////////////////////////////////////////


int snd::ProcessMessage(QueuePointer & resp_queue, 
                                player_msghdr * hdr, 
                                void * data)
{

  // Handle new data from the laser
    if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCAN, 
                this->laser_addr))
    {

        pthread_mutex_lock(&(this->data_mutex));
        this->laser__ranges_count = ((player_laser_data_t*) data)->ranges_count  ;
        this->laser__resolution=((player_laser_data_t*) data)->resolution;
        this->laser__max_range=((player_laser_data_t*) data)->max_range;
        this->laser__ranges.clear();
        for (unsigned int index=0;index<this->laser__ranges_count;index ++) 
        {
            this->laser__ranges.push_back(((player_laser_data_t*) data)->ranges[index]) ;
        }
        this->data_laser_ready = 1;
        pthread_cond_signal(&(this->data_changed_cond));
        pthread_mutex_unlock(&(this->data_mutex));
        return(0);
    }

    if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, 
                           PLAYER_POSITION2D_DATA_STATE, 
                           this->odom_in_addr))
  {
    pthread_mutex_lock(&(this->data_mutex));
    this->odom_pose= ((player_position2d_data_t*) data)->pos;
    if (this->first_goal_has_been_set_to_init_position ==0) {
        this->SignalNextGoal(this->odom_pose.px,this->odom_pose.py,this->odom_pose.pa);
        this->first_goal_has_been_set_to_init_position=1;
    }
    hdr->addr = this->device_addr;
    this->data_odometry_ready=1;
    pthread_cond_signal(&(this->data_changed_cond));
    pthread_mutex_unlock(&(this->data_mutex));
    this->Publish(hdr,data);
    PLAYER_MSG3(5, "Here I am: (%.3f %.3f %.3f)",this->odom_pose.px,this->odom_pose.py,this->odom_pose.pa);
    return(0);
  }


  
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, 
                           PLAYER_POSITION2D_DATA_STATE, 
                           this->odom_out_addr)) {
    hdr->addr = this->device_addr;
    this->Publish(hdr,data);
    return(0);
  }

  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, -1, this->device_addr))
  {
    Message* msg;
    if(!(msg = this->odom_out_dev->Request(this->InQueue,
                                       hdr->type,
                                       hdr->subtype,
                                       (void*)data, 
                                       hdr->size, 
                                       &hdr->timestamp)))
    {
      PLAYER_WARN1("failed to forward config request with subtype: %d\n", hdr->subtype);
      return(-1);
    }
    player_msghdr_t* rephdr = msg->GetHeader();
    void* repdata = msg->GetPayload();
    // Copy in our address and forward the response
    rephdr->addr = this->device_addr;
    this->Publish(resp_queue, rephdr, repdata);
    delete msg;
    return(0);
  }

  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, 
			       PLAYER_POSITION2D_CMD_POS,
                                this->device_addr))
  {
    player_position2d_cmd_pos_t* cmd = (player_position2d_cmd_pos_t*)data;
    this->SignalNextGoal(cmd->pos.px,cmd->pos.py,cmd->pos.pa);
    return 0;
    
  }

  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, 
			       PLAYER_POSITION2D_CMD_VEL,
                                this->device_addr))
  {
    player_position2d_cmd_vel_t* cmd = (player_position2d_cmd_vel_t*)data;
    this->SetSpeedCmd(*cmd);
    return 0;
    
  }
  // Tell the caller that you don't know how to handle this message
    PLAYER_MSG0(3,"Command unknown!");
  return(-1);
}



//////////////////////////////////////////////////////7

double snd_Proxy::GetScanRes() 
{

    PLAYER_MSG0(3,"GetScanRes");
    pthread_mutex_lock(&(this->data_mutex));
    double temp=this->laser__resolution;
    pthread_mutex_unlock(&(this->data_mutex));
    return temp;
}



 


double snd_Proxy::GetMaxRange() 
{
    PLAYER_MSG0(3,"GetMaxRange");
    pthread_mutex_lock(&(this->data_mutex));
    double temp=this->laser__max_range;
    pthread_mutex_unlock(&(this->data_mutex));
    return temp;
}


uint32_t snd_Proxy::GetCount()
{
    PLAYER_MSG0(3,"GetCount");
    pthread_mutex_lock(&(this->data_mutex));
    int temp=this->laser__ranges_count;
    pthread_mutex_unlock(&(this->data_mutex));
    return temp;
}

double snd_Proxy::range(const int index)
{
    pthread_mutex_lock(&(this->data_mutex));
    double temp= this->laser__ranges[index];
    pthread_mutex_unlock(&(this->data_mutex));
    return temp;
}





void   snd_Proxy::SetMotorEnable(int turnkey)
{
    /*Not implemented, passed on to lower driver*/
}
void   snd_Proxy::SetOdometry(double position_x0,
                                   double position_y0,
                                   double position_alpha0)
{
    /*Not implemented*/
}
double snd_Proxy::GetXPos() 
{
    pthread_mutex_lock(&(this->data_mutex));
    double temp= this->odom_pose.px;
    pthread_mutex_unlock(&(this->data_mutex));
    return temp;
}
double snd_Proxy::GetYPos()
{
    pthread_mutex_lock(&(this->data_mutex));
    double temp= this->odom_pose.py;
    pthread_mutex_unlock(&(this->data_mutex));
    return temp;
}
double snd_Proxy::GetYaw()
{
    pthread_mutex_lock(&(this->data_mutex));
    double temp= this->odom_pose.pa;
    pthread_mutex_unlock(&(this->data_mutex));
    return temp;
}
void   snd_Proxy::RequestGeom() 
{
    /*Not implemented, done automatically*/
}
void   snd_Proxy::SetSpeed(double velocity_modulus,
                                double velocity_angle) 
{
    player_position2d_cmd_vel_t cmd;
    cmd.vel.px=velocity_modulus;
    cmd.vel.py=0.0;
    cmd.vel.pa=velocity_angle;
    this->SetSpeedCmd(cmd);
}


void unlock_mutex(void *m) 
{
      pthread_mutex_unlock((pthread_mutex_t *)m);
}

void snd::WaitForNextGoal() 
{
    pthread_mutex_lock(&(this->goal_mutex));
    pthread_cleanup_push(unlock_mutex,(void *)&(this->goal_mutex));
    while (goal_changed == false)
    {
        pthread_cond_wait(&(this->goal_changed_cond), &(this->goal_mutex));
    }
    goal_changed=false;
    pthread_cleanup_pop(0);
    pthread_mutex_unlock(&(this->goal_mutex));
}

void snd::SignalNextGoal(double goalX,double goalY,double goalA) 
{
    pthread_mutex_lock(&(this->goal_mutex));
    this->goalX=goalX;
    this->goalY=goalY;
    this->goalA=goalA;
    goal_changed = true;
    pthread_cond_signal(&(this->goal_changed_cond));
    pthread_mutex_unlock(&(this->goal_mutex));
    PLAYER_MSG0(5,"Signal next goal command issue");
}
void snd::Read() 
{
    timespec ts;
    timeval    tp;

    PLAYER_MSG0(5,"Waiting for new data");
    pthread_mutex_lock(&(this->data_mutex));
    pthread_cleanup_push(unlock_mutex,(void *)&(this->data_mutex));
    while (this->data_odometry_ready!=1 ||this->data_laser_ready!=1)
    {
        gettimeofday(&tp, NULL);
        ts.tv_sec  = tp.tv_sec;
        ts.tv_nsec = tp.tv_usec * 1000;
        ts.tv_sec += 1;
        if (pthread_cond_timedwait(&(this->data_changed_cond), &(this->data_mutex),&ts) == ETIMEDOUT)
        {
            PLAYER_ERROR("SND driver is not receiving any data! Is Stage paused?");
        }
    }
      data_odometry_ready = 0;
      data_laser_ready = 0;
    PLAYER_MSG0(5,"Data acquired");
      pthread_cleanup_pop(0);
      pthread_mutex_unlock(&(this->data_mutex));
}

void snd::ReadIfWaiting() 
{
    /*Not implemented*/
}

