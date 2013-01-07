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

#include <libusb.h>
#include <stdexcept>
#include <vector>

namespace NXT
  {

  using namespace std;

  typedef vector<unsigned char> protobuffer;

  class buffer : public protobuffer
    {
    public:
      buffer & append_byte ( uint8_t byte );
      buffer & append_word ( uint16_t word );
      buffer & append ( const buffer & buf );
      // return self to chain calls

      void dump ( const string & header ) const; // Debug to stdout
    };

  class transport
    {
    public:
      virtual void write ( const buffer &buf ) = 0;
      virtual buffer read ( void ) = 0;
    };

  class USB_transport : public transport
    {
    public:
      USB_transport ( void );
      ~USB_transport ( void );
      virtual void write ( const buffer &buf );
      virtual buffer read ( void );
    private:
      libusb_context *context_;
      libusb_device_handle *handle_;

      void usb_check ( int usb_error );
    };

  class nxt_error : public runtime_error
    {
    public :
      nxt_error ( const char *s )    : runtime_error ( s ) {};
      nxt_error ( const string & s ) : runtime_error ( s ) {};
    };

  enum motors
  {
    A   = 0x00,
    B   = 0x01,
    C   = 0x02,
    All = 0xFF
  };

  enum motor_modes
  {
    motor_on        = 0x01,
    motor_brake     = 0x02,
    motor_regulated = 0x04
  };

  enum regulation_modes
  {
    regulation_motor_idle  = 0x00,
    regulation_motor_speed = 0x01,
    regulation_motor_sync  = 0x02
  };

  enum motor_run_states
  {
    motor_run_state_idle     = 0x00,
    motor_run_state_ramp_up  = 0x10,
    motor_run_state_running  = 0x20,
    motor_run_state_rampdown = 0x40
  };

  typedef struct
    {
      uint8_t protocol_minor;
      uint8_t protocol_major;
      uint8_t firmware_minor;
      uint8_t firmware_major;
    } versions;

  typedef struct
    {
      char brick_name[15];       // Null-terminated
      char bluetooth_address[7]; // Null-terminated
    } device_info;

  typedef struct
    {
      uint8_t          motor;
      int8_t           power_pct;
      motor_modes      mode;
      regulation_modes regulation;
      int8_t           turn_ratio;
      motor_run_states state;
      int32_t          tacho_limit;       // programmed limit for current movement, if any
      int32_t          tacho_count;       // current tacho count since last reset (acummulated odometry)
      int32_t          block_tacho_count; // current pos relative to last programmed pos (delta odometry)
      int32_t          rotation_count;    // current pos relative to the last reset of rot. counter
    } output_state;
    // Beware: the delta is since last command, not since last reading!    

  class brick
    {

    public:

      // Connect to the first brick found
      // TODO: give some means of connecting to a particular brick.
      // As is, this library serves only for using with a single brick.
      brick ( void );

      ~brick ( void );

      // Execute a prepared command
      // When with_feedback, the brick is asked to confirm proper execution
      // Returns the reply buffer, with the reply flag byte, status, and etc.
      //   or an empty buffer if !with_feedback
      buffer execute ( const buffer &command, bool with_feedback = false );

      // PREPARED COMMANDS
      // That you an store and execute with or without feedback

      buffer prepare_play_tone ( uint16_t tone_Hz, uint16_t duration_ms );

      buffer prepare_output_state (
        motors           motor,
        int8_t           power_pct,
        motor_modes      mode        = motor_brake,
        regulation_modes regulation  = regulation_motor_speed,
        int8_t           turn_ratio  = 0,
        motor_run_states state       = motor_run_state_running,
        uint32_t         tacho_count = 0 );
      // Full motor control; refer to NXT docs for precise meanings...

      buffer prepare_reset_motor_position ( motors motor, bool relative_to_last_position = false );
      buffer prepare_stop_sound_playback ( void );
      buffer prepare_keep_alive ( void );

      // DIRECT PERFORMING (WITH FEEDBACK)
      // If you don't want the feedback overhead, use execute with prepared commands
      // Errors will be reported as thrown nxt_error

      void play_tone ( uint16_t tone_Hz, uint16_t duration_ms );

      // Simple motor control
      void set_motor ( motors motor, int8_t power_pct );

      output_state get_motor_state ( motors motor );

      // In millivolts
      uint16_t get_battery_level ( void );

      versions get_version ( void );

      device_info get_device_info ( void );

      // Run a 10-second loop of play_tone, to get the average time per commands
      // For testing purposes, yes.
      void msg_rate_check ( void );

    private:

      enum telegram_types
      {
        direct_command_with_response    = 0x00,
        system_command_with_response    = 0x01,
        reply                           = 0x02,
        direct_command_without_response = 0x80,
        system_command_without_response = 0x80
      };

      buffer assemble ( telegram_types teltype,
                        uint8_t        command,
                        const buffer & payload );
      //  Assembles the full telegram to be sent over usb or bluetooth.

      USB_transport link_;
      //  For now is a fixed USB transport, but we could easily add bluetooth here

    };

}
