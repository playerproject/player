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
/** @defgroup driver_nxt nxt
 * @brief Lego Mindstorms NXT

This driver implements partial interaction with a USB-connected Lego Mindstorms NXT brick.\n
Motors are implemented.\n
Sensors are unimplemented.

@par Compile-time dependencies

- libusb-1.0 or newer (www.libusb.org)

@par Provides

- @ref interface_position1d
    - One per each of the A, B, C motors
    - These can be aggregated in a position2d using, e.g., @ref driver_differential
    - Velocity commands are accepted. Position commands are not.
- @ref interface_power
    - Battery level of the brick.

@par Configuration file options

- max_power (tuple of float [%] default: [100 100 100])
  - Power applied when maximum vel is requested for each motor.

- max_speed (tuple of float [length/s] default: [0.5 0.5 0.5])
  - Speed that each motor provides at max_power (must be calibrated/measured somehow depending on the LEGO model built).

- odom_rate (tuple of float default [0.0005 0.0005 0.0005])
  - Multiplier for the tachometer in the lego motor. tacho_count x odom_rate = real_distance (must be calibrated also).
  - Default is somewhat close to the standard small wheels with direct motor drive.

- period (float [s] default 0.05)
  - Seconds between reads of motor encoders. Since this requires polling and affects CPU use, each app can set an adequate timing.
  - Note that a polling roundtrip via USB takes (empirically measured) around 2ms per motor.

@par Example

@verbatim

# Standard configured brick with B and C motors in use

unit_length "m"
unit_angle  "radians"

# The NXT driver
driver
(
  name "nxt"
  provides [ "B:::position1d:0" "C:::position1d:1" "power:0" ]

  max_power [100 100 100] # 100% power is to be used
  max_speed [0.5 0.5 0.5] # in order to achieve 0.5 m/s linearly
  odom_rate [0.1 0.1 0.1] # multiplier for odometry

  period 0.05
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

#include "chronos.hh"
#include "libplayercore/playercore.h"
#include "nxtdc.hh"

using namespace nxt_driver;

const int kNumMotors = 3;

class Nxt : public ThreadedDriver
  {
  public:
    Nxt ( ConfigFile* cf, int section );

    virtual void Main ( void );
    virtual int  MainSetup ( void );
    virtual void MainQuit ( void );

    virtual int ProcessMessage ( QueuePointer &resp_queue, player_msghdr * hdr, void * data );

  private:
    player_devaddr_t motor_addr_[kNumMotors];
    player_devaddr_t power_addr_;

    player_position1d_data_t data_state_     [kNumMotors]; // Just read status.
    player_position1d_data_t data_state_prev_[kNumMotors]; // Previous status to integrate speed.

    double           max_power_[kNumMotors];
    double           max_speed_[kNumMotors];
    double           odom_rate_[kNumMotors];

    bool             publish_motor_[kNumMotors];
    bool             publish_power_;

    player_power_data_t juice_;

    double           period_;
    Chronos          timer_battery_;
    Chronos          timer_period_;

    NXT::brick       *brick_;

    void             CheckBattery ( void );
    void             CheckMotors ( void );
    NXT::motors      GetMotor ( const player_devaddr_t &addr ) const;
    int8_t           GetPower ( float vel, NXT::motors motor ) const;
  };

Driver* nxt_Init ( ConfigFile* cf, int section )
{
  return static_cast<Driver*> ( new Nxt ( cf, section ) );
}

void nxt_Register ( DriverTable* table )
{
  table->AddDriver ( "nxt", nxt_Init );
}

const char *motor_names[kNumMotors] = { "A", "B", "C" };

Nxt::Nxt ( ConfigFile *cf, int section )
    : ThreadedDriver ( cf, section ),
    period_ ( cf->ReadFloat ( section, "period", 0.05 ) ),
    timer_battery_ ( -666.0 )   // Ensure first update to be sent immediately
{
  for ( int i = 0; i < kNumMotors; i++ )
    {
      // Read them regardless of motor usage to placate player unused warnings
      max_power_[i] = cf->ReadTupleFloat ( section, "max_power", i, 100.0 );
      max_speed_[i] = cf->ReadTupleFloat ( section, "max_speed", i, 0.5 );
      odom_rate_[i] = cf->ReadTupleFloat ( section, "odom_rate", i, 0.0005 );

      if ( cf->ReadDeviceAddr ( &motor_addr_[i], section, "provides", PLAYER_POSITION1D_CODE, -1, motor_names[i] ) == 0 )
        {
          PLAYER_MSG1 ( 3, "nxt: Providing motor %s", motor_names[i] );

          if ( AddInterface ( motor_addr_[i] ) != 0 )
            throw std::runtime_error ( "Cannot add position1d interface" );
          else
            {
              publish_motor_[i] = true;

              data_state_[i].pos    = 0.0f;
              data_state_[i].vel    = 0.0f;
              data_state_[i].stall  = 0;
              data_state_[i].status = ( 1 << PLAYER_POSITION1D_STATUS_ENABLED );
            }
        }
    }

  if ( cf->ReadDeviceAddr ( &power_addr_, section, "provides", PLAYER_POWER_CODE, -1, NULL ) == 0 )
    {
      if ( AddInterface ( power_addr_ ) != 0 )
        throw std::runtime_error ( "Cannot add power interface" );
      else
        publish_power_ = true;
    }
  else
    {
      publish_power_ = false;
    }

}

int Nxt::MainSetup ( void )
{
  brick_ = new NXT::brick();

  // Reset odometries to origin
  for ( int i = 0; i < kNumMotors; i++ )
    if ( publish_motor_[i] )
      brick_->execute ( brick_->prepare_reset_motor_position ( static_cast<NXT::motors> ( i ), false ) );

  return 0;
}

void Nxt::MainQuit ( void )
{
  // Stop motors just in case they're running.
  // The brick has no watchdog, so they will keep its last commanded speed forever
  for ( int i = 0; i < kNumMotors; i++ )
    if ( publish_motor_[i] )
      brick_->set_motor ( static_cast<NXT::motors> ( i ), 0 );

  delete brick_;
}

void Nxt::Main ( void )
{
  while ( true )
    {
      // Wait till we get new data or we need to measure something
      Wait ( period_ );

      pthread_testcancel();

      ProcessMessages ( 0 );

      CheckBattery();
      CheckMotors();
    }
}

void Nxt::CheckBattery ( void )
{
  if ( ! publish_power_ )
    return;

  // We don't want to poll battery level innecesarily often (how much is this, really?)
  if ( timer_battery_.elapsed() > 10.0 )
    {
      timer_battery_.reset();

      juice_.valid = PLAYER_POWER_MASK_VOLTS;
      juice_.volts = static_cast<float> ( brick_->get_battery_level() ) / 1000.0f;
      // Omitted 4 unknown values here
    };

  // Publish value
  if ( HasSubscriptions() )
    Publish ( power_addr_,
              PLAYER_MSGTYPE_DATA,
              PLAYER_POWER_DATA_STATE,
              static_cast<void*> ( &juice_ ) );

  PLAYER_MSG1 ( 3, "Publishing power: %8.2f\n", juice_.volts );
}

void Nxt::CheckMotors ( void )
{
  if ( timer_period_.elapsed() < period_ )
    return;

  timer_period_.reset();

  // First we get odometry updates from brick
  for ( int i = 0; i < kNumMotors; i++ )
    {
      if ( ! publish_motor_[i] )
        continue;

      const NXT::output_state state = brick_->get_motor_state ( static_cast<NXT::motors> ( i ) );

      data_state_[i].pos = state.tacho_count * odom_rate_[i];
      data_state_[i].vel = ( data_state_[i].pos - data_state_prev_[i].pos ) / period_;

      PLAYER_MSG3 ( 5, "nxt: odom read is [raw/adjusted/vel] = [ %8d / %8.2f / %8.2f ]",
                    state.tacho_count, data_state_[i].pos, data_state_[i].vel );

      data_state_prev_[i] = data_state_[i];
    }

  // Then we publish them together, to minimize unsyncing in a consuming driver (e.g. differential driver)
  for ( int i = 0; i < kNumMotors; i++ )
    if ( publish_motor_[i] && HasSubscriptions() )
      Publish ( motor_addr_[i],
                PLAYER_MSGTYPE_DATA,
                PLAYER_POSITION1D_DATA_STATE,
                static_cast<void*> ( &data_state_[i] ) );

}

int Nxt::ProcessMessage ( QueuePointer  & resp_queue,
                          player_msghdr * hdr,
                          void          * data )
{
  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_REQ, PLAYER_POWER_REQ_SET_CHARGING_POLICY, power_addr_ ) )
    {
      PLAYER_WARN ( "nxt: there are no charging policies." );
      return 0;
    }

  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION1D_CMD_POS ) ||
       Message::MatchMessage ( hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION1D_REQ_POSITION_PID ) )
    {
      PLAYER_WARN ( "nxt: position commands not supported" );
      return 0;
    }

  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION1D_CMD_VEL ) )
    {
      player_position1d_cmd_vel_t &vel = *static_cast<player_position1d_cmd_vel_t*> ( data );

      const NXT::motors motor = GetMotor ( hdr->addr );
      brick_->set_motor ( motor, GetPower ( vel.vel, motor ) );

      return 0;
    }

  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION1D_REQ_GET_GEOM ) )
    {
      PLAYER_WARN ( "nxt: geometry not supported" );
      return 0;
    }

  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION1D_REQ_POSITION_MODE ) )
    {
      PLAYER_WARN ( "nxt: mode is always speed" );
      return 0;
    }

  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION1D_REQ_MOTOR_POWER ) )
    {
      PLAYER_WARN ( "nxt: motors are always on" );
      return 0;
    }

  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION1D_REQ_RESET_ODOM ) )
    {
      brick_->execute ( brick_->prepare_reset_motor_position ( GetMotor ( hdr->addr ) ) );
      return 0;
    }

  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION1D_REQ_SET_ODOM ) )
    {
      PLAYER_WARN ( "nxt: odometry setting to arbitrary values not supported" );
      return 0;
    }

  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION1D_REQ_SPEED_PID ) )
    {
      PLAYER_WARN ( "nxt: speed profiles not supported" );
      return 0;
    }

  if ( Message::MatchMessage ( hdr, PLAYER_MSGTYPE_REQ, PLAYER_POSITION1D_REQ_SPEED_PROF ) )
    {
      PLAYER_WARN ( "nxt: acceleration ignored, adjusting max speed only" );
      const NXT::motors motor = GetMotor ( hdr->addr );
      const player_position1d_speed_prof_req_t &prof = *static_cast<player_position1d_speed_prof_req_t*> ( data );

      max_power_[motor] *= ( prof.speed / max_speed_[motor] ); // Adjust power proportionally
      max_speed_[motor]  = prof.speed;

      if ( abs ( max_power_[motor] ) > 100 )
        PLAYER_WARN2 ( "nxt: requested speed would require excess power: [speed/power] = [ %8.2f / %8.2f ]",
                       max_speed_[motor], max_power_[motor] );
      return 0;
    }

  PLAYER_WARN4 ( "nxt: Message not processed idx:%d type:%d sub:%d seq:%d\n", hdr->addr.index, hdr->type, hdr->subtype, hdr->seq );
  return -1;
}


NXT::motors Nxt::GetMotor ( const player_devaddr_t &addr ) const
  {
    for ( int i = 0; i < kNumMotors; i++ )
      if ( ( addr.host   == motor_addr_[i].host ) &&
           ( addr.robot  == motor_addr_[i].robot ) &&
           ( addr.index  == motor_addr_[i].index ) &&
           ( addr.interf == motor_addr_[i].interf ) )
        return static_cast<NXT::motors> ( i );

    throw std::runtime_error ( "nxt: received request for unknown motor" );
  }

template<class T>
T sign ( T x )
{
  return x >= 0 ? 1 : -1;
}

int8_t Nxt::GetPower ( float vel, NXT::motors motor ) const
{
  const int8_t power = vel / max_speed_[motor] * max_power_[motor];
  if ( abs ( power ) > abs ( max_power_[motor] ) )
    {
      PLAYER_WARN3 ( "nxt: exceeded max power [motor/reqvel/reqpwr] = [ %s / %8.2f / %d ]",
                     motor_names[motor], vel, power );
      return max_power_[motor] * sign<float> ( power );
    }
  else
    return power;
}

