/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
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

/*
 * $Id$
 *
 * client-side sonar device 
 */

//#define DEBUG

#include <truthproxy.h>
#include <netinet/in.h>
#include <string.h>
#include <math.h>

#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif

  // the client calls this method to make a new proxy
  //   leave access empty to start unconnected
TruthProxy::TruthProxy(PlayerClient* pc, unsigned short index, 
	   unsigned char access = 'c') :
  ClientProxy(pc,PLAYER_TRUTH_CODE,index,access)
{
  // initialize data
  memset( database, 0, sizeof( database ) );
 
  num_truths = 0;

  preUpdateCallback = 0;
  postUpdateCallback = 0;

};

void TruthProxy::ImportTruth( truth_t &si, player_generic_truth_t &pl )
{
  si.id.port = pl.id.port;
  si.id.type = pl.id.type;
  si.id.index = pl.id.index;

  si.parent.port = pl.parent.port;
  si.parent.type = pl.parent.type;
  si.parent.index = pl.parent.index;

  pl.x  == 0 ? si.x  = 0 : si.x  = (double)(pl.x  / 1000.0); // mm -> m
  pl.y  == 0 ? si.y  = 0 : si.y  = (double)(pl.y  / 1000.0);
  pl.w  == 0 ? si.w  = 0 : si.w  = (double)(pl.w  / 1000.0);
  pl.h  == 0 ? si.h  = 0 : si.h  = (double)(pl.h  / 1000.0);
  pl.th == 0 ? si.th = 0 : si.th = (double)DTOR(pl.th); //deg -> rad
}

void TruthProxy::ExportTruth( player_generic_truth_t &pl, truth_t &si )
{
  pl.id.port = si.id.port;
  pl.id.type = si.id.type;
  pl.id.index = si.id.index;

  pl.parent.port = si.parent.port;
  pl.parent.type = si.parent.type;
  pl.parent.index = si.parent.index;

  si.x  == 0 ? pl.x  = 0 : pl.x  = (uint32_t)(si.x * 1000.0); // m -> mm
  si.y  == 0 ? pl.y  = 0 : pl.y  = (uint32_t)(si.y * 1000.0);
  si.w  == 0 ? pl.w  = 0 : pl.w  = (uint16_t)(si.w * 1000.0);
  si.h  == 0 ? pl.h  = 0 : pl.h  = (uint16_t)(si.h * 1000.0);
  si.th == 0 ? pl.th = 0 : pl.th = (uint16_t)RTOD(si.th); //rad -> deg
}

    
int TruthProxy::SendModifiedTruthsToStage()
{
#ifdef DEBUG
  puts( "TruthProxy::SendTruthsToStage()" ); 
#endif

  if(!client)
    return(-1);

  player_generic_truth_t player_truth;
  
  int res;
  int truths_sent = 0;
  
  // for all the truths
  for( int i=0; i<num_truths; i++ )
    {
      if( database[i].modified_locally ) // we changed it
	{
#ifdef DEBUG
	  printf( "TruthProxy: database[%d] has changed. Updating Stage\n",
		  i ); 
#endif
	  // pack it into stage format
	  ExportTruth( player_truth, database[i] );
	  
	  // and send the truth to stage
	  res = client->Write( PLAYER_TRUTH_CODE, index,
			       (const char*)&player_truth,
			       sizeof(player_truth));
	  
	  if( res == -1 ) puts( "Warning: truth proxy failed to write truth" );
	  else truths_sent++;
	  
	  database[i].modified_locally = false;
	}
    }

  return truths_sent;
}

int TruthProxy::SendSingleTruthToStage( truth_t& truth )
{
#ifdef DEBUG
  puts( "TruthProxy::SendSingleTruthToStage()" ); 
#endif

  if(!client)
    return(-1);

  player_generic_truth_t player_truth;
  
  int res;
  int truths_sent = 0;

#ifdef DEBUG
  printf( "TruthProxy: converting "
	  "(%d,%d,%d) [%.2f,%.2f,%.2f]\n", 
	  truth.id.port, truth.id.type, truth.id.index,
	  truth.x, truth.y, truth.th );
#endif		

  
  // pack it into stage format
  ExportTruth( player_truth, truth );

#ifdef DEBUG
  printf( "                    to "
	  "(%d,%d,%d) [%u,%u,%u] for sending to Player\n", 
	  player_truth.id.port, player_truth.id.type,
	  player_truth.id.index,
	  player_truth.x, player_truth.y, player_truth.th );
#endif		

  // and send the truth to stage
  res = client->Write( PLAYER_TRUTH_CODE, index,
		       (const char*)&player_truth,
		       sizeof(player_truth));
  
  if( res == -1 ) puts( "Warning: truth proxy failed to write truth" );
  else truths_sent++;

  return truths_sent;
}


void TruthProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  //#ifdef DEBUG
  // puts( "TruthProxy::FillData()" ); 
  //#endif

  // how many truth structures do we have in the buffer?
  int truth_count = hdr.size / sizeof(player_generic_truth_t);

  if( truth_count < 1 ) return; // nothing to see here

#ifdef DEBUG
  printf( "TruthProxy received %d player truth structs at %p\n", 
	  truth_count, buffer );
#endif

  player_generic_truth_t *ptruth = (player_generic_truth_t *)buffer;

  // for each truth struct in the buffer
  for( int t=0; t< truth_count; t++ )
    {
#ifdef DEBUG
      printf( "TruthProxy: Processing item %d\n", t ); fflush( stdout );
#endif

      bool found = false;

      // if we have some records already
      if( num_truths > 0 )
	// check the database to see if we have this already
	// for every entry in the database
	for( int i=0; i<num_truths; i++ )
	  {
	    // is this the same object?
	    if(    database[i].id.port ==  ptruth[t].id.port 
		&& database[i].id.type ==  ptruth[t].id.type 
		&& database[i].id.index == ptruth[t].id.index )
	      {
		// seen it before!
		// if there is a pre-update callback, call it.
		if( preUpdateCallback )
		  (*preUpdateCallback)( &(database[i]) );
		
		// so copy the new truth into the database
		ImportTruth( database[i], ptruth[t] );
		
#ifdef DEBUG
		printf( "TruthProxy: updating device "
			"(%d,%d,%d) [%.2f,%.2f,%.2f]\n", 
			database[i].id.port, 
			database[i].id.type, 
			database[i].id.index,
			database[i].x, database[i].y, database[i].th );
#endif		
		
		database[i].modified_remotely = true; // flag as fresh
		found = true;
		
		// if there is a post-update callback, call it.
		if( postUpdateCallback )
		  (*postUpdateCallback)( &(database[i]) );
		
	  break;
	      }
	  }
      
      if( !found ) // it wasn;t in the database
	{
#ifdef DEBUG
	  printf( "TruthProxy: adding device (%d,%d,%d)\n", 
		ptruth[t].id.port, ptruth[t].id.type, ptruth[t].id.index );
#endif
	  //memcpy( &(database[num_truths]), &(truth[t]), sizeof(truth[t]) );
	  ImportTruth( database[num_truths], ptruth[t] );
	  database[num_truths].modified_remotely = true; // flag data as fresh
	  num_truths++;
	}
    }
}

// interface that all proxies SHOULD provide
void TruthProxy::Print()
{
  puts( "Truth Proxy device database:" );
  for( int i=0; i<num_truths; i++)
    printf( "[%d] (%d,%d,%d) x:%.2f y:%.2f th:%.2f w:%.2f h:%.2f\n",
	    i, 
	    database[i].id.port, 
	    database[i].id.type, 
	    database[i].id.index, 
	    //truth[t].desc, 
	    database[i].x, 
	    database[i].y, 
	    database[i].th, 
	    database[i].w, 
	    database[i].h );
  
  fflush( stdout );
}

// supply the address and size of an array containing all the truth data
int TruthProxy::GetDevices( truth_t *devs, int& num )
{
  devs = database;
  num = num_truths;
  return num_truths;
}


// return a pointer to a device; successive calls rotate through
// the database
truth_t* TruthProxy::GetNextDevice( void )
{
  static int i = 0;
  
  i %= num_truths; // cycle from 0 to (num_truths-1)
  
#ifdef DEBUG
  printf( "TruthProxy::GetNextDevice() returning database[%d] of %d\n", 
	  i, num_truths );
#endif
  
  return( &(database[i++]) ); // return the pointer and increment the index
}

// flag everything as fresh so we can read it all again
void TruthProxy::FreshenAll( void )
{
  //memset( modified_remotely, true, sizeof(modified_remotely) );
  
  for( int i=0; i<num_truths; i++ )
      database[i].modified_remotely = true;
}
 
// return the nearest device to thge given point
truth_t* TruthProxy::NearestDevice( double x, double y )
{
  double dist, nearDist = 9999999.9;
  truth_t* nearest = 0;

  for( int i=0; i<num_truths; i++ )
    {
      dist = hypot( x - database[i].x, y - database[i].y );
      
        if( dist < nearDist ) 
    	{
    	  nearDist = dist;
    	  nearest = &(database[i]);
    	}
    }
  return nearest; 
}

// return the nearest device to thge given point
truth_t* TruthProxy::GetDeviceByID( player_id_t* id )
{
  for( int i=0; i<num_truths; i++ )
    {
//        printf( "TP: comparing (%d,%d,%d) with (%d,%d,%d)\n",
//  	      id->port, 
//  	      id->type, 
//  	      id->index,
//  	      database[i].id.port, 
//  	      database[i].id.type, 
//  	      database[i].id.index );

      if( database[i].id.type == id->type &&
	  database[i].id.index == id->index &&
	  database[i].id.port == id->port )
	return &(database[i]); // success!
    }

  //  printf( "checked all %d truths - not found!", num_truths );
  return 0; // failed!
}

// return a pointer to a device that has been updated; 0 if none fresh
truth_t* TruthProxy::GetNextFreshDevice( void )
{
 
  // return the first fresh truth in the database
  for( int i=0; i<num_truths; i++ )
    if( database[i].modified_remotely )
      {
#ifdef DEBUG
	printf( "TruthProxy::GetNextFreshDevice() "
		"returning database[%d] of %d\n", 
		i, num_truths );
#endif
	// unset the fresh flag so we don't get this object again
	// until it has been updatd
	database[i].modified_remotely = false;

	return &(database[i]);
      }
  return 0; // nothing fresh :(
}














