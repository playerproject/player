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
// Author: Richard Vaughan, based on Andrew Howard's gz_time.cc
// Date: 7 May 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#include "stage1p4.h"
#include "stg_time.h"
#include "math.h"

////////////////////////////////////////////////////////////////////////////////
// Constructor
StgTime::StgTime()
{
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
 puts( "get time" );

  // get the time from Stage. 

  // this mechanism could be more efficient,
  // but it's nice and simple for now.
  
 if( Stage1p4::created_models_count < 3 )
   {
     memset( time, 0, sizeof(struct timeval) );
     return 0;
   }

 stg_property_t *prop = 
   stg_send_property( Stage1p4::stage_client, 
		      Stage1p4::created_models[2].stage_id, 
		      STG_PROP_TIME, STG_GET, NULL, 0 );
 
 printf( "time is %.4f\n", prop->timestamp );
 
 assert( prop );
 
 time->tv_sec = (int) floor(prop->timestamp);
 time->tv_usec = (int) floor(fmod(prop->timestamp, 1.0) * 1e6);
 
 stg_property_free(prop);
 
 return 0;
}
