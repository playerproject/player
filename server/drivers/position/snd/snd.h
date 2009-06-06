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

#ifndef snd_H
#define snd_H


#include <pthread.h>
#include <semaphore.h>
#include <vector>
#include <libplayercore/playercore.h>


class snd_Proxy; 

////////////////////////////////////////////////////////////////////////////////
// The class for the driver
class snd : public ThreadedDriver
{
public:
    snd(ConfigFile* cf, int section);
    // Must implement the following methods.
    virtual int Setup();
    virtual int Shutdown();
    virtual int ProcessMessage(QueuePointer & resp_queue, 
                               player_msghdr * hdr, 
                               void * data);


private:
    // Main function for device thread.
    virtual void Main();
    int Odometry_Setup();
    int Laser_Setup();

    // My position interface
    player_devaddr_t m_position_addr;
    // My laser interface
    player_devaddr_t m_laser_addr;

    // Address of and pointer to the laser device to which I'll subscribe
    player_devaddr_t laser_addr;
    player_devaddr_t odom_in_addr;
    player_devaddr_t odom_out_addr;
    Device* laser_dev;
    Device *odom_in_dev;
    Device *odom_out_dev;
    player_position2d_geom_t robot_geom;
    int first_goal_has_been_set_to_init_position;
protected:
    void SetSpeedCmd(player_position2d_cmd_vel_t cmd);

    player_pose2d_t odom_pose;

    std::vector<double> laser__ranges;
    double laser__resolution;
    double laser__max_range;
    uint32_t laser__ranges_count;
public:
	double robot_radius;
	double min_gap_width;
	double obstacle_avoid_dist;
	double max_speed;
	double max_turn_rate;
	double goal_position_tol;
	double goal_angle_tol;
    double goalX,goalY,goalA;
	pthread_t algorithm_thread;
    pthread_mutex_t goal_mutex;
    pthread_cond_t goal_changed_cond;
    double goal_changed;
    pthread_mutex_t data_mutex;
    pthread_cond_t data_changed_cond;
    double data_changed;
    int data_odometry_ready;
    int data_laser_ready;
 
    void WaitForNextGoal();
    void SignalNextGoal(double goalX,double goalY,double goalA);
    void Read();
    void ReadIfWaiting();
};

/*This class acts as a substitute for libplayerc++ proxies.
 * This has been done to reuse the code of the algorithm (which is in 
 * gap_nd_nav.cc)
 */
class snd_Proxy : public snd
{
public:
    double   GetScanRes()  ;
    double   GetMaxRange() ;
    uint32_t GetCount()    ;
    double   range(const int index);

    void   SetMotorEnable(int turnkey);
    void   SetOdometry(double position_x0,
                                double position_y0,
                                double position_alpha0);
    double GetXPos() ;
    double GetYPos() ;
    double GetYaw()  ;
    void   RequestGeom();
    void   SetSpeed(double velocity_modulus,
                             double velocity_angle);
    snd_Proxy(ConfigFile* cf, int section):snd(cf,section){}
};



////////////////////////////////////////////////////////////////////////////////
// Extra stuff for building a shared object.

/* need the extern to avoid C++ name-mangling  */
extern "C" int player_driver_init(DriverTable* table);


#endif //snd_H
