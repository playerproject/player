/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2010
 *     Alejandro R. Mosteo
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_differential differential
 * @brief Differential steering (also skid-steering)

This driver takes two @ref interface_position1d sources
and considers them the two motors of a differential steer machine,
providing in turn a @ref interface_position2d for control of such machine.

@par Compile-time dependencies

- none

@par Requires

- @ref interface_position1d
    - Two, with "left" and "right" keys
    - Velocity commands (x, 0, ω) are transformed and delivered to these interfaces.
    - Position commands not supported.

@par Provides

- @ref interface_position2d
    - The differential steer interface obtained coupling the two position1d interfaces.

@par Configuration file options

- axis_length (float [length] default 25cm)
    - Distance between wheels at its pivot point.

- period (float [s] default 0.05)
    - Period used for integration of odometry, since we have unsyncronized sources for each wheel.

@par Example

@verbatim

# Example with the differential LEGO driver with two actuators.
# Standard configured brick with B and C motors in use.

unit_length "m"
unit_angle  "radians"

# The NXT driver
driver
(
  name "differential"
  provides [ "B:::position1d:0" "C:::position1d:1" ]
)

# The differential driver that provides simplified position2d management
driver
(
  name "differential"
  requires [ "left:::position1d:0" "right:::position1d:1" ]
  provides [ "position2d:0" ]

  axis_length 0.25
)

@endverbatim

@author Alejandro R. Mosteo
*/
/** @} */
#include "libplayercore/playercore.h"

#include "chronos.hh"
#include <cstring>
#include <cmath>
#include <stdexcept>

#if defined(__SUNPRO_CC)
using namespace std;
#endif

using namespace driver_differential;

const int kNumMotors = 2;
const int kL = 0;
const int kR = 1;
const char *kMotorNames[kNumMotors] = { "left", "right" };


class Differential : public ThreadedDriver
  {
  public:
    Differential ( ConfigFile* cf, int section );

    virtual void Main ( void );
    virtual int  MainSetup ( void );
    // virtual void MainQuit ( void );

    virtual int ProcessMessage ( QueuePointer &resp_queue, player_msghdr * hdr, void * data );

  private:
    player_devaddr_t p1d_addr_[kNumMotors];
    player_devaddr_t p2d_addr_;

    Device          *p1d_dev_[kNumMotors];

    player_position2d_data_t p2d_state_;

    player_position1d_data_t p1d_state_     [kNumMotors]; // Just read status.
    player_position1d_data_t p1d_state_prev_[kNumMotors]; // Previous status to integrate speed.

    double           axis_length_;

    double           period_;
    Chronos          timer_period_;

    void             CheckMotors ( void );
    int              GetMotor ( const player_devaddr_t & addr ) const;
    void             SetVel ( const player_pose2d_t & vel );
  };

Driver* differential_Init ( ConfigFile* cf, int section )
{
  return static_cast<Driver*> ( new Differential ( cf, section ) );
}

void differential_Register ( DriverTable* table )
{
  table->AddDriver ( "differential", differential_Init );
}

Differential::Differential ( ConfigFile *cf, int section )
    : ThreadedDriver ( cf, section ),
    axis_length_ ( cf->ReadLength ( section, "axis_length", 0.25 ) ),
    period_ ( cf->ReadFloat ( section, "period", 0.05 ) )
{
  for ( int i = 0; i < kNumMotors; i++ )
    {

      if ( cf->ReadDeviceAddr ( &p1d_addr_[i], section, "requires", PLAYER_POSITION1D_CODE, -1, kMotorNames[i] ) != 0 )
        {
          PLAYER_ERROR1 ( "position1d required for motor %s not found", kMotorNames[i] );
          SetError ( -1 );
          return;
        }
      else
        {
          memset ( &p1d_state_[i],      0, sizeof ( p1d_state_[i] ) );
          memset ( &p1d_state_prev_[i], 0, sizeof ( p1d_state_[i] ) );
        }
    }

  if ( cf->ReadDeviceAddr ( &p2d_addr_, section, "provides", PLAYER_POSITION2D_CODE, -1, NULL ) == 0 )
    {
      if ( AddInterface ( p2d_addr_ ) != 0 )
        throw std::runtime_error ( "Cannot add position2d interface" );
      else
        memset ( &p2d_state_, 0, sizeof ( p2d_state_ ) );
    }
  else
    throw std::runtime_error ( "Cannot find position2d interface" );
}

int Differential::MainSetup ( void )
{
  for ( int i = 0 ; i < kNumMotors; i++ )
    if ( ! ( p1d_dev_[i] = deviceTable->GetDevice ( p1d_addr_[i] ) ) )
      {
        PLAYER_ERROR1 ( "Unable to locate position1d device at given address with key: %s", kMotorNames[i] );
        return -1;
      }
    else if ( p1d_dev_[i]->Subscribe ( InQueue ) != 0 )
      {
        PLAYER_ERROR1 ( "Unable to subscribe to position1d driver with key: %s", kMotorNames[i] );
        return -1;
      }

  return 0;
}

void Differential::Main ( void )
{
  while ( true )
    {
      // Wait till we get new data or we need to measure something
      Wait ( period_ );

      pthread_testcancel();

      ProcessMessages ( 0 );

      CheckMotors();
    }
}

void Differential::CheckMotors ( void )
{
  // THESE CALCULATIONS ARE mostly TAKEN FROM
  // http://rossum.sourceforge.net/papers/DiffSteer/

  if ( timer_period_.elapsed() < period_ )
    return;

  timer_period_.reset();

  p2d_state_.stall = p1d_state_[kL].stall || p1d_state_[kR].stall;

  p2d_state_.vel.px = ( p1d_state_[kL].vel + p1d_state_[kR].vel ) / 2.0;
  p2d_state_.vel.py = 0.0;
  p2d_state_.vel.pa = ( p1d_state_[kR].vel - p1d_state_[kL].vel ) / axis_length_;

  const double dist = ( p1d_state_[kL].pos - p1d_state_prev_[kL].pos + p1d_state_[kR].pos - p1d_state_prev_[kR].pos ) / 2.0;
  p2d_state_.pos.pa += ( ( p1d_state_[kR].pos - p1d_state_prev_[kR].pos ) -
                         ( p1d_state_[kL].pos - p1d_state_prev_[kL].pos ) ) / ( 2.0 * axis_length_ );
  p2d_state_.pos.px += dist * cos ( p2d_state_.pos.pa );
  p2d_state_.pos.py += dist * sin ( p2d_state_.pos.pa );

  if ( HasSubscriptions() )
    Publish ( p2d_addr_,
              PLAYER_MSGTYPE_DATA,
              PLAYER_POSITION2D_DATA_STATE,
              static_cast<void*> ( &p2d_state_ ) );

  for ( int i = 0; i < kNumMotors; i++ )
    p1d_state_prev_[i] = p1d_state_[i];

  PLAYER_MSG5 ( 4, "differential: odom update is ( px, py, pa )( vx, 0.0, va) = ( %7.2f, %7.2f, %7.2f )( %7.2f, 0.0, %7.2f)",
                p2d_state_.pos.px, p2d_state_.pos.py, p2d_state_.pos.pa, p2d_state_.vel.px, p2d_state_.vel.pa );

}

int Differential::ProcessMessage ( QueuePointer  & resp_queue,
                                   player_msghdr * hdr,
                                   void          * data )
{
  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION1D_DATA_STATE ) )
    {
      // Store last odometry until next integration deadline
      p1d_state_[ GetMotor ( hdr->addr ) ] = *static_cast<player_position1d_data_t*> ( data );
      return 0;
    }

  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_POS ) ||
       Message::MatchMessage ( hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_POSITION_PID ) )
    {
      PLAYER_WARN ( "differential: position commands not supported" );
      return 0;
    }

  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL ) )
    {
      player_position2d_cmd_vel_t &vel = *static_cast<player_position2d_cmd_vel_t*> ( data );
      SetVel ( vel.vel );

      return 0;
    }

  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_GET_GEOM ) )
    {      
      player_position2d_geom_t geom;
      memset(&geom, 0, sizeof(geom));
      geom.pose.px = p2d_state_.pos.px;
      geom.pose.py = p2d_state_.pos.py;
      geom.pose.pyaw = p2d_state_.pos.pa;

      geom.size.sw = axis_length_;
      
      Publish(hdr->addr, PLAYER_MSGTYPE_RESP_ACK, hdr->subtype, &geom);
      PLAYER_WARN ( "differential: geometry only partially supported" );
      return 0;
    }

  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_POSITION_MODE ) )
    {
      PLAYER_WARN ( "differential: mode is always speed" );
      return 0;
    }

  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_MOTOR_POWER ) )
    {
      PLAYER_WARN ( "differential: motors are always on" );
      return 0;
    }

  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_RESET_ODOM ) )
    {
      p1d_dev_[kL]->PutMsg ( InQueue, PLAYER_MSGTYPE_REQ, PLAYER_POSITION1D_REQ_RESET_ODOM, NULL, 0, NULL );
      p1d_dev_[kR]->PutMsg ( InQueue, PLAYER_MSGTYPE_REQ, PLAYER_POSITION1D_REQ_RESET_ODOM, NULL, 0, NULL );
      return 0;
    }

  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_SET_ODOM ) )
    {
      PLAYER_WARN ( "differential: odometry setting to arbitrary values not supported" );
      return 0;
    }

  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_SPEED_PID ) )
    {
      PLAYER_WARN ( "differential: speed profiles not supported" );
      return 0;
    }

  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_SPEED_PROF ) )
    {
      player_position2d_speed_prof_req_t & req = *static_cast<player_position2d_speed_prof_req_t*> ( data );
      player_position1d_speed_prof_req_t cmd = { req.speed, req.acc };

      p1d_dev_[kL]->PutMsg ( InQueue, PLAYER_MSGTYPE_REQ, PLAYER_POSITION1D_REQ_SPEED_PROF,
                             static_cast<void*> ( &cmd ), 0, NULL );
      p1d_dev_[kR]->PutMsg ( InQueue, PLAYER_MSGTYPE_REQ, PLAYER_POSITION1D_REQ_SPEED_PROF,
                             static_cast<void*> ( &cmd ), 0, NULL );

      return 0;
    }

  PLAYER_WARN4 ( "differential: Message not processed idx:%d type:%d sub:%d seq:%d\n", hdr->addr.index, hdr->type, hdr->subtype, hdr->seq );
  return -1;
}


int Differential::GetMotor ( const player_devaddr_t &addr ) const
  {
    for ( int i = 0; i < kNumMotors; i++ )
      if ( ( addr.host   == p1d_addr_[i].host ) &&
           ( addr.robot  == p1d_addr_[i].robot ) &&
           ( addr.index  == p1d_addr_[i].index ) &&
           ( addr.interf == p1d_addr_[i].interf ) )
        return i;

    throw std::runtime_error ( "differential: received request for unknown motor" );
  }

void Differential::SetVel ( const player_pose2d_t &vel )
{
  player_position1d_cmd_vel_t sl = { vel.px - vel.pa * axis_length_ / 2.0, 0 };
  player_position1d_cmd_vel_t sr = { vel.px + vel.pa * axis_length_ / 2.0, 0 };

  PLAYER_MSG4 ( 4,  "differential: speed CMD: [vx, va --> vl, vr] = [ %8.2f, %8.2f --> %8.2f, %8.2f ]",
                vel.px, vel.pa, sl.vel, sr.vel );

  p1d_dev_[kL]->PutMsg ( InQueue,
                         PLAYER_MSGTYPE_CMD,
                         PLAYER_POSITION1D_CMD_VEL,
                         static_cast<void*> ( &sl ), 0, NULL );
  p1d_dev_[kR]->PutMsg ( InQueue,
                         PLAYER_MSGTYPE_CMD,
                         PLAYER_POSITION1D_CMD_VEL,
                         static_cast<void*> ( &sr ), 0, NULL );

  if ( vel.py != 0 )
    PLAYER_WARN1 ( "differential: Y speed requested is not null; impossible with skid-steering: %8.2f (ignored)", vel.py );
}

