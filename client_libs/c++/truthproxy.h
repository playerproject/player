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

#ifndef TRUTHPROXY_H
#define TRUTHPROXY_H

#include <clientproxy.h>
#include <playerclient.h>


// Convert radians to degrees
//
#ifndef RTOD
#define RTOD(r) ((r) * 180 / M_PI)
#endif

// Convert degrees to radians
//
#ifndef DTOR
#define DTOR(d) ((d) * M_PI / 180)
#endif

// Normalize angle to domain -pi, pi
//
#ifndef NORMALIZE
#define NORMALIZE(z) atan2(sin(z), cos(z))
#endif

class TruthProxy; //forward declaration

//#define truth_t player_generic_truth_t

// truth-like struct with SI units and  a freshness flag
typedef struct
{
  // identify this entity to Player
  player_device_id_t id;
  player_device_id_t parent;

  double x, y, th, w, h;

  unsigned char modified_remotely;
  unsigned char modified_locally;

} truth_t;


#define MAX_TRUTHS 1024 // too small?

//class CXGui;

typedef void (*truth_callback_t)(truth_t*);

class TruthProxy : public ClientProxy
{
  /* BOGUS: This is all bogus now; should be like gps now. AH */
  truth_t database[ MAX_TRUTHS ]; // may be too small?
  
  truth_callback_t preUpdateCallback; // pointers to functions
  truth_callback_t postUpdateCallback;

  int num_truths;
  
  public:
  
  // the client calls this method to make a new proxy
  //   leave access empty to start unconnected
  TruthProxy(PlayerClient* pc, unsigned short index, 
             unsigned char access = 'c');

  void AttachPreUpdateCallback( truth_callback_t func )
    {
      preUpdateCallback = func;
    };
  
  void AttachPostUpdateCallback( truth_callback_t func )
    {
      postUpdateCallback = func;
    };

  // these methods are the user's interface to this device
    
  // interface that all proxies must provide
  void FillData(player_msghdr_t hdr, const char* buffer);
    
  // interface that all proxies SHOULD provide
  void Print();

  // get a pointer to an object that has been updated
  // and unset the object's fresh flag
  //truth_t* GetAnUpdatedObject( void );

  void FreshenAll( void );

  // supply the address and size of an array containing all the truth data
  int GetDevices( truth_t *devs, int& num );

  // return a pointer to a device; successive calls rotate through
  // the database
  truth_t* GetNextDevice( void );

  // return a pointer to a device with id matching the parameter
  truth_t* GetDeviceByID( player_device_id_t* id );

  // return a pointer to a device with fresh data
  truth_t* GetNextFreshDevice( void );

  // return a pointer to the device nearest x,y (meters)
  truth_t* NearestDevice( double x, double y );

  int SendModifiedTruthsToStage();
  int SendSingleTruthToStage( truth_t& truth );

  void ImportTruth( truth_t &si, player_generic_truth_t &pl );
  void ExportTruth( player_generic_truth_t &si, truth_t &pl );
};

#endif




