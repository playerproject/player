/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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
/********************************************************************
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ********************************************************************/

#include <config.h>

#include <string.h>
#include <stdlib.h>
#if !defined WIN32
  #include <unistd.h>
#endif

// Shouldn't really need this; <libplayercore/playercommon.h> includes
// it...
#include <limits.h> // for PATH_MAX

#include <libplayercore/drivertable.h>
#include <libplayercore/error.h>
#include <libplayercore/globals.h>
#include <libplayercore/playercommon.h>

#include <replace/replace.h>

#include "plugins.h"

// Try to load a given plugin, using a particular search algorithm.
// Returns true on success and false on failure.
lt_dlhandle
LoadPlugin(const char* pluginname, const char* cfgfile)
{
#if HAVE_LIBLTDL
  static int init_done = 0;
  
  if( !init_done )
	 {
    int errors = 0;
    if((errors = lt_dlinit()))
    {
      PLAYER_ERROR2( "Error(s) initializing dynamic loader (%d, %s)",
                     errors, lt_dlerror() );
      return NULL;
    }
    else
      init_done = 1;
  }
 
  // allocate a buffer to put the searched paths in so we can just display the error at the end
  // rather than during plugin loading
  // see if we got an absolute path
  if(pluginname[0] == '/' || pluginname[0] == '~')
	 {
		PLAYER_WARN1( "absolute path plugin %s", pluginname );
	 }
  else
	 {
		// we got a relative path, so set up a search path
		char* playerpath = NULL;

		// start with $PLAYERPATH, if set
		if(( playerpath = getenv("PLAYERPATH")))
		  {
			 if( lt_dlsetsearchpath( playerpath ) ) 
				PLAYER_ERROR( "failed to initialize plugin path to $PLAYERPATH" );
		  }
		
		// add the working directory		
		char workingdir[PATH_MAX];
		getcwd( workingdir, PATH_MAX );
		if( lt_dladdsearchdir( workingdir  ) )
		  PLAYER_ERROR1( "failed to add working directory %s to the plugin path", workingdir );
		
		// add the directory where the config file is
		if(cfgfile)
		  {
			 // Note that dirname() modifies the contents on some
			 // platforms, so we need to make a copy of the filename.
			 char* tmp = strdup(cfgfile);
			 assert(tmp);
			 char* cfgdir = dirname(tmp);			 
			 if( lt_dladdsearchdir( cfgdir ) )
				PLAYER_ERROR1( "failed to add config file directory %s to the plugin path", cfgdir );			 
			 free(tmp);
		  }
		
		// add $PLAYER_INSTALL_PREFIX/lib		
		char installdir[ PATH_MAX ];	 
		strncpy( installdir, PLAYER_INSTALL_PREFIX, PATH_MAX);
		strncat( installdir, "/lib/", PATH_MAX);						
		if( lt_dladdsearchdir( installdir ) )
				PLAYER_ERROR1( "failed to add working directory %s to the plugin path", installdir );
	 }
  
  PLAYER_MSG1(3, "loading plugin %s", pluginname);
  
  lt_dlhandle handle = handle = lt_dlopenext( pluginname );
  
  if(!handle)
	 {
		PLAYER_ERROR1( "Failed to load plugin %s.",
							pluginname );
		PLAYER_ERROR1( "libtool reports error: %s",
							lt_dlerror() );
		PLAYER_ERROR1( "plugin search path: %s",
							lt_dlgetsearchpath() );
	 }
  
  return handle;

#else
  PLAYER_ERROR("Sorry, no support for shared libraries, so can't load plugins.");
  PLAYER_ERROR("You should install libltdl, which is part of GNU libtool, then re-compile player.");
  PLAYER_ERROR("If you're on Windows, wait a little while as plugin loading is still under development.");
  return 0;
#endif
}

// Initialise a driver plugin
bool InitDriverPlugin(lt_dlhandle handle)
{
#if HAVE_LIBLTDL
  DriverPluginInitFn initfunc;
  // Invoke the initialization function
  if(handle)
  {
    PLAYER_MSG0(1, "invoking player_driver_init()...");

    initfunc = (DriverPluginInitFn)lt_dlsym(handle,"player_driver_init");
    if( !initfunc )
    {
      PLAYER_ERROR1("failed to resolve player_driver_init: %s\n", lt_dlerror());
      return(false);
    }

    int initfunc_result = 0;
    if( (initfunc_result = (*initfunc)(driverTable)) != 0)
    {
      PLAYER_ERROR1("error returned by player_driver_init: %d", initfunc_result);
      return(false);
    }

    PLAYER_MSG0(1, "success");

    return(true);
  }
  else
    return(false);
#else
  PLAYER_ERROR("Sorry, no support for shared libraries, so can't load plugins.");
  PLAYER_ERROR("You should install libltdl, which is part of GNU libtool, then re-compile player.");
  PLAYER_ERROR("If you're on Windows, wait a little while as plugin loading is still under development.");
  return(false);
#endif
}

// Initialise an interface plugin
playerxdr_function_t* InitInterfacePlugin(lt_dlhandle handle)
{
#if HAVE_LIBLTDL
  InterfPluginInitFn initfunc;
  playerxdr_function_t *flist;

  // Invoke the initialization function
  if(handle)
  {
    PLAYER_MSG0(1, "invoking player_plugininterf_gettable()...");

    initfunc = (InterfPluginInitFn)lt_dlsym(handle,"player_plugininterf_gettable");
    if( !initfunc )
    {
      PLAYER_ERROR1("failed to resolve player_plugininterf_gettable: %s\n", lt_dlerror());
      return(NULL);
    }

    if( (flist = (*initfunc)()) == NULL)
    {
      PLAYER_ERROR("player_plugininterf_gettable returned NULL");
      return(NULL);
    }

    PLAYER_MSG0(1, "success");

    return(flist);
  }
  else
    return(NULL);
#else
  PLAYER_ERROR("Sorry, no support for shared libraries, so can't load plugins.");
  PLAYER_ERROR("You should install libltdl, which is part of GNU libtool, then re-compile player.");
  PLAYER_ERROR("If you're on Windows, wait a little while as plugin loading is still under development.");
  return(NULL);
#endif
}
