/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2004
 *     Brian Gerkey
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

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "playernav.h"

extern double dumpfreq;
extern double mapupdatefreq;

/* Parse command line arguments, of the form host:port */
int
parse_args(int argc, char** argv, size_t* num_bots, char** hostnames,
           int* ports, double* zoom, int* aa, int* map_idx, int* planner_idx)
{
  char* idx;
  int port;
  int hostlen;
  int i;

  // first look for -foo options
  for(i=0; (i<argc) && (argv[i][0] == '-'); i++)
  {
    if(!strcmp(argv[i],"-fps"))
    {
      if(++i < argc)
        dumpfreq = atof(argv[i]);
      else
        return(-1);
    }
    else if(!strcmp(argv[i],"-zoom"))
    {
      if(++i < argc)
        *zoom = atof(argv[i]);
      else
        return(-1);
    }
    else if(!strcmp(argv[i],"-mapidx"))
    {
      if(++i < argc)
        *map_idx = atoi(argv[i]);
      else
        return(-1);
    }
    else if(!strcmp(argv[i],"-planneridx"))
    {
      if(++i < argc)
        *planner_idx = atoi(argv[i]);
      else
        return(-1);
    }
    else if(!strcmp(argv[i],"-mapupdate"))
    {
      if(++i < argc)
        mapupdatefreq = atof(argv[i]);
      else
        return(-1);
    }
    else if(!strcmp(argv[i],"-aa"))
    {
      if(++i < argc)
        *aa = atoi(argv[i]);
      else
        return(-1);
    }
    else
      return(-1);
  }

//  if(i>=argc)
//    return(-1);

  *num_bots=0;

  if (i< argc)
  {
    while(i<argc)
    {
      // Look for ':' (colon), and extract the trailing port number.  If not
      // given, then use the default Player port (6665)
      if((idx = strchr(argv[i],':')) && (strlen(idx) > 1))
      {
        port = atoi(idx+1);
        hostlen = idx - argv[i];
      }
      else
      {
        port = 6665;
        hostlen = strlen(argv[i]);
      }

      // Store the hostnames and port numbers
      assert((hostlen > 0) && (hostlen < (MAX_HOSTNAME_LEN - 1)));
      argv[i][hostlen] = '\0';
      hostnames[*num_bots] = strdup(argv[i]);
      ports[*num_bots] = port;
      (*num_bots)++;
      i++;
    }
  }
  else
  {
    hostnames[*num_bots] = strdup("localhost");
    ports[*num_bots] = 6665;
    *num_bots=1;
  }

  return(0);
}


