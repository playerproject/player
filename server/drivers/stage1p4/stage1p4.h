/*
 *  Stage-1.4 driver for Player
 *  Copyright (C) 2003  Richard Vaughan (rtv) vaughan@hrl.com 
 *
 * This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
x *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: stage1p4.cc,v 1.5 2003/08/02 02:
 *
*/

// STAGE-1.4 DRIVER CLASS  /////////////////////////////////
// creates a single static Stage client

#include "device.h"
#include "configfile.h"
#include "stage.h"
#include "worldfile.h"

class Stage1p4 : public CDevice
{
 public:
  Stage1p4(char* interface, ConfigFile* cf, int section, 
		   size_t datasz, size_t cmdsz, int rqlen, int rplen);
  virtual ~Stage1p4();
  
  virtual int Setup();
  virtual int Shutdown(); 
  virtual void Update();

 protected:

  static ConfigFile* config;
  static CWorldFile wf;
  static char worldfile_name[MAXPATHLEN]; // filename
  static stg_client_t* stage_client;
  static char* world_name;
  static bool init;
  static pthread_mutex_t reply_mutex; // used to block until a device until a reply is received
  static pthread_mutex_t model_mutex; // used to protect the model tree data

  stg_model_t* model; // points inside the shared stg_client_t to our
		      // individual model data
  
  // the property we automatically subscribe to on Setup();
  stg_id_t subscribe_prop;


  void ModelsLock(){ pthread_mutex_lock( &model_mutex ); }
  void ModelsUnlock(){ pthread_mutex_unlock( &model_mutex ); }

  //void AwaitReply(){ pthread_mutex_lock( &reply_mutex ) 
  //	     pthread_mutex_w  };
};
