/*
 *  Player - One Hell of a Robot Server
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
///////////////////////////////////////////////////////////////////////////
//
// Desc: Stage (simulator) time
// Author: Richard Vaughan
// Date: 7 May 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

//#define DEBUG

#include "stage1p4.h"
#include "stg_time.h"
#include "math.h"

////////////////////////////////////////////////////////////////////////////////
// Constructor
StgTime::StgTime( stg_client_t* cli )
{
  this->client = cli;
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
StgTime::~StgTime()
{
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Get the simulator time
int StgTime::GetTime(struct timeval* time)
{
  PRINT_DEBUG( "get time" );
  
  // handle any packets coming in from Stage - there might be a new
  // time in the pipe
  if( this->client )
    {
      PRINT_DEBUG( "stg_time checking for new data" );

      stg_msg_t* msg = NULL;      
      while( (msg = stg_client_read( client )) )
	{
	  stg_client_handle_message( client, msg );
	  free( msg );
	}
      
      PRINT_DEBUG( "stg_time done checking" );
      time->tv_sec =  (long)client->stagetime;
      time->tv_usec =  (long)(fmod( client->stagetime, 1.0) * 1000000.0);
    }
  else // no time data available yet. 
    memset( time, 0, sizeof(struct timeval) );
  
  PRINT_DEBUG2( "time now %d sec %d usec", time->tv_sec, time->tv_usec );
  
  return 0;
}
