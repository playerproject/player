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

#include <cstdio>
#include "nxtdc.hh"
#include <unistd.h>

using namespace NXT;
using namespace std;

void set_power ( brick &b, int power )
{
  const int i = power;

  b.set_motor ( B,  i );
  b.set_motor ( C, -i );
 
  usleep ( 100000 );
  const output_state st_b = b.get_motor_state ( B );
  const output_state st_c = b.get_motor_state ( C );
  printf ( "Power: %4d B:[%6d/%6d/%6d/%6d] C:[%6d/%6d/%6d/%6d]\n",
           i,
           st_b.tacho_limit, st_b.tacho_count, st_b.block_tacho_count, st_b.rotation_count,
           st_c.tacho_limit, st_c.tacho_count, st_c.block_tacho_count, st_c.rotation_count );
}

int main ( void )
{
  NXT::brick b;

  const NXT::versions v = b.get_version();

  printf ( "Connected to NXT brick named %s, protocol %d.%d firmware %d.%d\n",
           b.get_device_info().brick_name,
           v.protocol_major, v.protocol_minor,
           v.firmware_major, v.firmware_minor );

  printf ( "Battery: %d\n", b.get_battery_level() );

  b.play_tone ( 700, 1000 );

  for ( int i = 0; i <= 100; i++ )
    set_power ( b, i );
  for ( int i = 100; i >= -100; i-- )
    set_power ( b, i );
  for ( int i = -100; i <= 0; i++ )
    set_power ( b, i );

  printf ( "Battery: %d\n", b.get_battery_level() );

  // b.msg_rate_check();

  return 0;
}
