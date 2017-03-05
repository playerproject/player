/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */
///////////////////////////////////////////////////////////////////////////
//
// Desc: Log file time
// Author: Andrew Howard
// Date: 28 May 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <math.h>
#include <stdio.h>

#if defined (WIN32)
  #include <winsock2.h> // For struct timeval
#endif

#include "readlog_time.h"


////////////////////////////////////////////////////////////////////////////////
// The global time for readlog
struct timeval ReadLogTime_time;
double ReadLogTime_timeDouble;


////////////////////////////////////////////////////////////////////////////////
// Constructor
ReadLogTime::ReadLogTime()
{
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
ReadLogTime::~ReadLogTime()
{
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Get the simulator time
int ReadLogTime::GetTime(struct timeval* time)
{
  *time = ReadLogTime_time;    
  return(0);
}

int 
ReadLogTime::GetTimeDouble(double* time)
{
  *time = ReadLogTime_timeDouble;
  return(0);
}
