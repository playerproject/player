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
#include <stagecpp.hh> // from the Stage 1.4 distro

#define DEFAULT_STG_WORLDFILE "default.world"
#define DEFAULT_STG_HOST "localhost"


class Stage1p4 : public CDevice
{
public:
  stg_id_t stage_id; // associates this device with a Stage model
  int section; // worldfile section

  static ConfigFile* config;
  static char* world_file;
  static stg_client_t* stage_client;
  static stg_name_id_t* created_models;
  static int created_models_count;

  Stage1p4(char* interface, ConfigFile* cf, int section, 
		   size_t datasz, size_t cmdsz, int rqlen, int rplen);
  virtual ~Stage1p4();

  stg_client_t* CreateStageClient( char* host, int port, char* world );

  int DestroyStageClient( stg_client_t* cli );

  virtual int Setup();
  virtual int Shutdown();
};

