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

#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// Shouldn't really need this; <libplayercore/playercommon.h> includes
// it...
#include <limits.h> // for PATH_MAX

#include <libplayercore/drivertable.h>
#include <libplayercore/error.h>
#include <libplayercore/globals.h>
#include <libplayercore/playercommon.h>

#include <replace/replace.h>

#include "plugins.h"

class plugin_path_list
{
public:
	plugin_path_list(plugin_path_list * parent)
	{
		memset(fullpath,0,sizeof(fullpath));
		next = NULL;
		if (parent)
		{
			parent->next = this;
		}
	}

	~plugin_path_list()
	{
		delete next;
	}

	plugin_path_list * last()
	{
		plugin_path_list * ret = NULL;
		for (ret = this; ret->next != NULL; ret = ret->next);
		return ret;
	}

	char fullpath[PATH_MAX];
	plugin_path_list * next;
};


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

  lt_dlhandle handle=NULL;
  plugin_path_list paths(NULL);

  char* playerpath;
  char* tmp;
  char* cfgdir;
  unsigned int i,j;

  // allocate a buffer to put the searched paths in so we can just display the error at the end
  // rather than during plugin loading
  // see if we got an absolute path
  if(pluginname[0] == '/' || pluginname[0] == '~')
  {
    strncpy(paths.fullpath,pluginname,PATH_MAX);
  }
  else
  {
    // we got a relative path, so search for the module
    // did the user set PLAYERPATH?
    if((playerpath = getenv("PLAYERPATH")))
    {
      PLAYER_MSG1(1,"PLAYERPATH: %s\n", playerpath);

      // yep, now parse it, as a colon-separated list of directories
      i=0;
      while(i<strlen(playerpath))
      {
        j=i;
        while(j<strlen(playerpath))
        {
          if(playerpath[j] == ':')
          {
            break;
          }
          j++;
        }
        plugin_path_list * path = paths.last();
        new plugin_path_list(path);
        strncpy(path->fullpath,playerpath+i,j-i);
        strcat(path->fullpath,"/");
        strcat(path->fullpath,pluginname);

        i=j+1;
      }
    }

    // try to load it from the directory where the config file is
    if(cfgfile)
    {
      // Note that dirname() modifies the contents, so
      // we need to make a copy of the filename.
      tmp = strdup(cfgfile);
      assert(tmp);
      plugin_path_list * path = paths.last();
      new plugin_path_list(path);
      cfgdir = dirname(tmp);
      if(cfgdir[0] != '/' && cfgdir[0] != '~')
      {
        getcwd(path->fullpath, PATH_MAX);
        strncat(path->fullpath,"/", PATH_MAX);
      }
      strncat(path->fullpath,cfgdir, PATH_MAX);
      strncat(path->fullpath,"/", PATH_MAX);
      strncat(path->fullpath,pluginname, PATH_MAX);
      free(tmp); // this should ha
    }

    // try to load it from prefix/lib
    plugin_path_list * path = paths.last();
    new plugin_path_list(path);
    strncpy(path->fullpath,PLAYER_INSTALL_PREFIX, PATH_MAX);
    strncat(path->fullpath,"/lib/", PATH_MAX);
    strncat(path->fullpath,pluginname, PATH_MAX);

    // just pass the libname to lt_dlopenext, to see if it can handle it
    // (this may work when the plugin is installed in a default system
    // location).
    path = paths.last();
    strncpy(path->fullpath,pluginname,PATH_MAX);

    PLAYER_MSG1(3, "loading plugin %s", pluginname);
  }
  for (plugin_path_list *path = &paths;!handle && path;path = path->next)
  {
    if((handle = lt_dlopenext(path->fullpath)))
    {
      break;
    }
  }

  if(!handle)
  {
    PLAYER_ERROR1("failed to load plugin %s, tried paths:",pluginname);
    for (plugin_path_list *path = &paths;path;path = path->next)
      PLAYER_ERROR1("\t%s", path->fullpath);
  }

  return handle;

#else
  PLAYER_ERROR("Sorry, no support for shared libraries, so can't load plugins.");
  PLAYER_ERROR("You should install libltdl, which is part of GNU libtool, then re-compile player.");
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
  return(NULL);
#endif
}
