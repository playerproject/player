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

#include <cerrno>
#include "chronos.hh"
#include <cstring>
#include <stdexcept>
#include <sys/time.h>

using namespace nxt_driver;

Chronos::Chronos ( double seconds_since_epoch )
{
  clock_ = seconds_since_epoch;
}

double Chronos::elapsed ( void ) const
  {
    return now() - clock_;
  }

void Chronos::reset ( void )
{
  clock_ = now();
}

double Chronos::now ( void )
  {
    struct timeval clock;

    if ( gettimeofday ( &clock, NULL ) != 0 )
      throw std::runtime_error ( strerror ( errno ) );

    return
      static_cast<double> ( clock.tv_sec ) +
      static_cast<double> ( clock.tv_usec ) * 1e-6;
  }