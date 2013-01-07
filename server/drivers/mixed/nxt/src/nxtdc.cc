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

#include <cassert>
#include <cstdio>
#include <cstring>
#if defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define le32toh OSSwapLittleToHostInt32
#define le16toh OSSwapLittleToHostInt16
#define htole16 OSSwapHostToLittleInt16
#define htole32 OSSwapHostToLittleInt32
#else
#include <endian.h>
#endif
#include "nxtdc.hh"
#include <sstream>
#include <sys/time.h>

using namespace NXT;
using namespace std;

const uint16_t VENDOR_LEGO = 0x0694;
const uint16_t PRODUCT_NXT = 0x0002;

const int kNxtConfig    = 1;
const int kNxtInterface = 0;

const uint8_t kMaxTelegramSize = 64; // Per NXT spec.

const unsigned char kOutEndpoint = 0x1;
const unsigned char kInEndpoint  = 0x82;

enum direct_commands
{
  command_play_tone            = 0x03,
  command_set_output_state     = 0x04,
  command_get_output_state     = 0x06,
  command_reset_motor_position = 0x0A,
  command_get_battery_level    = 0x0B,
  command_stop_sound_playback  = 0x0C,
  command_keep_alive           = 0x0D
};

enum system_commands
{
  system_get_firmware_version  = 0x88,
  system_get_device_info       = 0x9B
};

const char *usberr_to_str ( int err )
{
  switch ( err )
    {
    case 0:
      return "LIBUSB_SUCCESS";
    case -1:
      return "LIBUSB_ERROR_IO";
    case -2:
      return "LIBUSB_ERROR_INVALID_PARAM";
    case -3:
      return "LIBUSB_ERROR_ACCESS";
    case -4:
      return "LIBUSB_ERROR_NO_DEVICE";
    case -5:
      return "LIBUSB_ERROR_NOT_FOUND";
    case -6:
      return "LIBUSB_ERROR_BUSY";
    case -7:
      return "LIBUSB_ERROR_TIMEOUT";
    case -8:
      return "LIBUSB_ERROR_OVERFLOW";
    case -9:
      return "LIBUSB_ERROR_PIPE";
    case -10:
      return "LIBUSB_ERROR_INTERRUPTED";
    case -11:
      return "LIBUSB_ERROR_NO_MEM";
    case -12:
      return "LIBUSB_ERROR_NOT_SUPPORTED";
    case -99:
      return "LIBUSB_ERROR_OTHER";
    default:
      return "LIBUSB_ERROR_UNKNOWN";
    }
}

string nxterr_to_str ( int err )
{
  switch ( err )
    {
    case 0x20:
      return "Pending communication transaction in progress";
    case 0x40:
      return "Specified mailbox queue is empty";
    case 0xBD:
      return "Request failed (e.g. specified file not found)";
    case 0xBE:
      return "Unknown command opcode";
    case 0xBF:
      return "Insane packet";
    case 0xC0:
      return "Data contains out-of-range values";
    case 0xDD:
      return "Communication bus error";
    case 0xDE:
      return "No free memory in communication buffer";
    case 0xDF:
      return "Specified channel/connection is not valid";
    case 0xE0:
      return "Specified channel/connection not configured or busy";
    case 0xEC:
      return "No active program";
    case 0xED:
      return "Illegal size specified";
    case 0xEE:
      return "Illegal mailbox queue ID specified";
    case 0xEF:
      return "Attempted to access invalid field or structure";
    case 0xF0:
      return "Bad input or output specified";
    case 0xFB:
      return "Insufficient memory available";
    case 0xFF:
      return "Bad arguments";
    default:
      char s[100];
      snprintf ( s, 100, "NXT_UNCATEGORIZED_ERROR: 0x%02x", err );
      return string ( s );
    }
}

buffer & buffer::append_byte ( uint8_t byte )
{
  push_back ( static_cast<unsigned char> ( byte ) );
  return *this;
}

buffer & buffer::append_word ( uint16_t word )
{
  union
    {
      unsigned char bytes[2];
      uint16_t      le_word;
    } tmp;

  tmp.le_word = htole16 ( word );

  push_back ( tmp.bytes[0] );
  push_back ( tmp.bytes[1] );

  return *this;
}

buffer & buffer::append ( const buffer & buf )
{
  for ( size_t i = 0; i < buf.size(); i++ )
    push_back ( buf[i] );

  return *this;
}

void buffer::dump ( const string & header ) const
  {
    printf ( "%s\n", header.c_str() );

    for ( size_type i = 0; i < this->size(); i++ )
      printf ( "%2d = 0x%02x\n", i, this->at ( i ) );
  }

void USB_transport::usb_check ( int usb_error )
{
  if ( usb_error != LIBUSB_SUCCESS )
    throw runtime_error ( string ( "USB error: " ) + usberr_to_str ( usb_error ) );
}

USB_transport::USB_transport ( void )
{
  usb_check ( libusb_init ( &context_ ) );
  libusb_set_debug ( context_, 3 );

  handle_ = libusb_open_device_with_vid_pid ( context_, VENDOR_LEGO, PRODUCT_NXT );
  if ( handle_ == NULL )
    throw runtime_error ( "USB_transport: brick not found." );

  usb_check ( libusb_set_configuration ( handle_, kNxtConfig ) );
  usb_check ( libusb_claim_interface ( handle_, kNxtInterface ) );
  usb_check ( libusb_reset_device ( handle_ ) );
}

USB_transport::~USB_transport ( void )
{
  //    usb_check(libusb_release_interface(handle_, kNxtInterface));
  // Fails, why?

  libusb_close ( handle_ );
  libusb_exit ( context_ );
}

void USB_transport::write ( const buffer &buf )
{
  int transferred;

  // buf.dump ( "write" );

  usb_check ( libusb_bulk_transfer
              ( handle_, kOutEndpoint,
                ( unsigned char* ) &buf[0], buf.size(),
                &transferred, 0 ) );
  // printf ( "T:%d\n", transferred );
}

buffer USB_transport::read ( void )
{
  unsigned char buf[kMaxTelegramSize];
  int           transferred;

  usb_check ( libusb_bulk_transfer
              ( handle_, kInEndpoint,
                &buf[0], kMaxTelegramSize,
                &transferred, 0 ) );
  // printf ( "%2x %2x %2x (%d read)\n", buf[0], buf[1], buf[2], transferred );

  buffer result;
  result.reserve ( transferred );

  for ( int i = 0; i < transferred; i++ )
    result.push_back ( buf[i] );

  return result ;
}

brick::brick ( void )
{
  ;
}

brick::~brick ( void )
{
  ;
}

buffer brick::execute ( const buffer &command, bool with_feedback )
{
  assert ( command.size() >= 2 );

  if ( with_feedback && ( ! ( command[0] & 0x80 ) ) )
    link_.write ( command );
  else if ( ( !with_feedback ) && ( command[0] & 0x80 ) )
    link_.write ( command );
  else
    {
      buffer newcomm ( command );

      // Set or reset 0x80 bit (confirmation request)
      newcomm[0] = ( with_feedback ? command[0] & 0x7F : command[0] | 0x80 );

      link_.write ( newcomm );
    }

  if ( with_feedback )
    {
      const buffer reply = link_.read ();
      if ( reply.size() < 3 )
        {
          stringstream s;
          s << "Reply too short: " << reply.size() << " bytes";
          throw nxt_error ( s.str () );
        }
      else if ( reply[0] != brick::reply )
        {
          char s[100];
          snprintf ( s, 100, "Unexpected telegram: 0x%02x != 0x%02x", reply[0], brick::reply );
          throw nxt_error ( s );
        }
      else if ( reply[1] != command[1] )
        {
          char s[100];
          snprintf ( s, 100, "Unexpected reply type: 0x%02x != 0x%02x", reply[1], command[1] );
          throw nxt_error ( s );
        }
      else if ( reply[2] != 0 )
        throw nxt_error ( nxterr_to_str ( reply[2] ) );
      else
        return reply;
    }
  else
    return buffer();
}

buffer brick::prepare_play_tone ( uint16_t tone_Hz, uint16_t duration_ms )
{
  return assemble ( direct_command_without_response,
                    command_play_tone,
                    buffer().
                    append_word ( tone_Hz ).
                    append_word ( duration_ms ) ) ;
}

buffer brick::prepare_output_state (
  motors           motor       ,
  int8_t           power_pct   ,
  motor_modes      mode        ,
  regulation_modes regulation  ,
  int8_t           turn_ratio  ,
  motor_run_states state       ,
  uint32_t         tacho_count )
{
  union
    {
      uint8_t  bytes[4];
      uint32_t tacho;
    } aux;

  aux.tacho = htole32 ( tacho_count );

  return
    assemble
    ( direct_command_without_response,
      command_set_output_state,
      buffer().
      append_byte ( motor ).
      append_byte ( power_pct ).
      append_byte ( mode ).
      append_byte ( regulation ).
      append_byte ( turn_ratio ).
      append_byte ( state ).
      append_byte ( aux.bytes[0] ).append_byte ( aux.bytes[1] ).
      append_byte ( aux.bytes[2] ).append_byte ( aux.bytes[3] ) );
}

buffer brick::prepare_reset_motor_position ( motors motor, bool relative_to_last_position )
{
  return
    assemble ( direct_command_without_response,
               command_reset_motor_position,
               buffer().
               append_byte ( motor ).
               append_byte ( relative_to_last_position ) );
}

buffer brick::prepare_stop_sound_playback ( void )
{
  return
    assemble ( direct_command_without_response,
               command_stop_sound_playback,
               buffer() );
}

buffer brick::prepare_keep_alive ( void )
{
  return
    assemble ( direct_command_without_response,
               command_keep_alive,
               buffer() );
}

void brick::play_tone ( uint16_t tone_Hz, uint16_t duration_ms )
{
  execute ( prepare_play_tone ( tone_Hz, duration_ms ), false );
}

void brick::set_motor ( motors motor, int8_t power_pct )
{
  execute
  ( assemble
    ( direct_command_without_response,
      command_set_output_state,
      buffer().
      append_byte ( motor ).
      append_byte ( power_pct ).
      append_byte ( motor_brake ).		// Brake uses a bit more power but gives finer control at low speeds
      append_byte ( regulation_motor_speed ). // Better Idle than Running, which tries to compensate loads?
      append_byte ( 0 ). 			// TURN_RATIO
      append_byte ( power_pct == 0 ? motor_run_state_idle : motor_run_state_running ).
      append_word ( 0 ).append_word ( 0 ) ),	// Tacho count (unlimited)
    false );
}

output_state brick::get_motor_state ( motors motor )
{
  const buffer reply =
    execute
    ( assemble
      ( direct_command_with_response,
        command_get_output_state,
        buffer().append_byte ( motor ) ) , true );

  const output_state state =
  {
    reply[3],                                   // motor
    reply[4],                                   // power_pct
    static_cast<motor_modes> ( reply[5] ),      // motor_modes
    static_cast<regulation_modes> ( reply[6] ), // regulation_modes
    reply[7],                                   // turn_ratio
    static_cast<motor_run_states> ( reply[8] ), // motor_run_states
    le32toh ( *reinterpret_cast<const int32_t*> ( &reply[9] ) ),
    le32toh ( *reinterpret_cast<const int32_t*> ( &reply[13] ) ),
    le32toh ( *reinterpret_cast<const int32_t*> ( &reply[17] ) ),
    le32toh ( *reinterpret_cast<const int32_t*> ( &reply[21] ) ),
  };
  return state;

}

versions brick::get_version ( void )
{
  const buffer reply =
    execute
    ( assemble
      ( system_command_with_response,
        system_get_firmware_version,
        buffer() ) , true );

  const versions v = { reply.at ( 3 ), reply.at ( 4 ), reply.at ( 5 ), reply.at ( 6 ) };
  return v;
}

device_info brick::get_device_info ( void )
{
  const buffer reply =
    execute
    ( assemble
      ( system_command_with_response,
        system_get_device_info,
        buffer() ) , true );

  device_info info;
  strncpy ( info.brick_name, reinterpret_cast<const char*> ( &reply[3] ), 14 );
  info.brick_name[14] = '\0';
  strncpy ( info.bluetooth_address, reinterpret_cast<const char*> ( &reply[18] ), 6 );
  info.bluetooth_address[6] = '\0';

  return info;
}

uint16_t brick::get_battery_level ( void )
{
  const buffer reply =
    execute ( assemble ( direct_command_with_response,
                         command_get_battery_level,
                         buffer() ),
              true );

  union
    { const unsigned char * bytes;
      const uint16_t      * level;
    } aux;

  aux.bytes = &reply[3];
  return le16toh ( *aux.level );
}

void brick::msg_rate_check ( void )
{
  struct timeval start, now;
  int calls=0;

  assert ( gettimeofday ( &start, NULL ) == 0 );

  do
    {
      execute ( prepare_play_tone ( 440, 0 ), true );
      calls++;

      assert ( gettimeofday ( &now, NULL ) == 0 );
    }
  while ( ( double ) start.tv_sec + ( double ) start.tv_usec/1000000.0 -
          ( double ) now.tv_sec - ( double ) now.tv_usec/1000000.0 > -10.0 );

  printf ( "%d calls in 10s (%dms per call)\n", calls, 10000 / calls );
}

buffer brick::assemble ( telegram_types teltype,
                         uint8_t        command,
                         const buffer & payload )
{
  return
    buffer ().
    append_byte ( teltype ).
    append_byte ( command ).
    append ( payload );
}

