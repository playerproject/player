/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2004  Brian Gerkey gerkey@stanford.edu
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

/*
 * $Id$
 *
 * A driver to read a vector map from a text file
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_vmapfile vmapfile
 * @brief Read vector maps from text files

The vmapfile driver reads a vector map from a text file and
provides the map to others via the @ref interface_map interface.

The text file should contain lines in the format

x1 y1 x2 y2

where (x1,y1) and (x2,y2) are the cartesian coordinates representing
the endpoints of each vector.  The endpoints are read in as floating
point numbers, so they can have decimal values.

@par Compile-time dependencies

- None

@par Provides

- @ref interface_map

@par Requires

- None

@par Configuration requests

- PLAYER_MAP_REQ_GET_VECTOR

@par Configuration file options

- filename (string)
  - Default: NULL
  - The file to read.

- scale (tuple [double double])
  - Default [1.0 1.0]
  - Multipliers for X,Y of each vector, for the final result to be in meters

@par Example

@verbatim
driver
(
  name "vmapfile"
  provides ["map:0"]
  filename "mymap.wld"
  scale [1.0 1.0]
)
@endverbatim

@author Brian Gerkey

*/
/** @} */

#include <math.h>
#include <sys/types.h> // required by Darwin
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <libplayercore/playercore.h>

#include <config.h>
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#ifndef FMIN
#define FMIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef FMAX
#define FMAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#if defined WIN32
  #define strcasecmp _stricmp
#endif

class VMapFile : public Driver
{
  private:
    const char* filename;
    player_map_data_vector_t* vmap;
    size_t vmapsize;

    // Handle map data request
    void HandleGetMapVector(void *client, void *request, int len);

    // Add a vector
    void AddVector(double x0, double y0, double x1, double y1);

    float scalex_, scaley_; // scaling of input vectors

  public:
    VMapFile(ConfigFile* cf, int section, const char* file);
    ~VMapFile();
    int Setup();
    int Shutdown();

    // MessageHandler
    int ProcessMessage(QueuePointer & resp_queue,
		       player_msghdr * hdr,
		       void * data);

};

Driver*
VMapFile_Init(ConfigFile* cf, int section)
{
  const char* filename;

  if(!(filename = cf->ReadFilename(section,"filename", NULL)))
  {
    PLAYER_ERROR("must specify map filename");
    return(NULL);
  }
  return((Driver*)(new VMapFile(cf, section, filename)));
}

// a driver registration function
void
vmapfile_Register(DriverTable* table)
{
  table->AddDriver("vmapfile", VMapFile_Init);
}


// this one has no data or commands, just configs
VMapFile::VMapFile(ConfigFile* cf, int section, const char* file)
  : Driver(cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_MAP_CODE)
{
  this->vmap = NULL;
  this->filename = file;
  scalex_ = cf->ReadTupleFloat(section, "scale", 0, 1.0);
  scaley_ = cf->ReadTupleFloat(section, "scale", 1, 1.0);
}

VMapFile::~VMapFile()
{
}

void
VMapFile::AddVector(double x0, double y0, double x1, double y1)
{
      this->vmap->segments = (player_segment_t*) realloc(
        this->vmap->segments,
        (this->vmap->segments_count+1)*sizeof(this->vmap->segments[0])
      );

    if (this->vmap->segments_count == 0)
    {
        this->vmap->minx = FMIN(x0 * scalex_, x1 * scalex_);
        this->vmap->miny = FMIN(y0 * scaley_, y1 * scaley_);
        this->vmap->maxx = FMAX(x0 * scalex_, x1 * scalex_);
        this->vmap->maxy = FMAX(y0 * scaley_, y1 * scaley_);
    }
    else
    {
        this->vmap->minx = FMIN(this->vmap->minx, FMIN(x0 * scalex_, x1 * scalex_));
        this->vmap->miny = FMIN(this->vmap->minx, FMIN(y0 * scaley_, y1 * scaley_));
        this->vmap->maxx = FMAX(this->vmap->maxx, FMAX(x0 * scalex_, x1 * scalex_));
        this->vmap->maxy = FMAX(this->vmap->maxy, FMAX(y0 * scaley_, y1 * scaley_));
    }
      
    this->vmap->segments[this->vmap->segments_count].x0 = x0 * scalex_;
    this->vmap->segments[this->vmap->segments_count].y0 = y0 * scaley_;
    this->vmap->segments[this->vmap->segments_count].x1 = x1 * scalex_;
    this->vmap->segments[this->vmap->segments_count].y1 = y1 * scaley_;
    this->vmap->segments_count++;
}

int
VMapFile::Setup()
{
  FILE* fp;
  
  float x0,y0,x1,y1;
  
  char linebuf[512];
  char keyword [512];
  

  printf("VMapFile loading file: %s...", this->filename);
  fflush(stdout);

  if(!(fp = fopen(this->filename, "r")))
  {
    PLAYER_ERROR1("failed to open file %s", this->filename);
    return(-1);
  }

  // Allocate space for the biggest possible vector map; we'll realloc
  // later
  this->vmap =
          (player_map_data_vector_t*)malloc(sizeof(player_map_data_vector_t));
  assert(this->vmap);

  this->vmap->segments_count = 0;
  this->vmap->segments = NULL;
  while(!feof(fp))
  {
    if(!fgets(linebuf, sizeof(linebuf), fp))
      break;
    if(!strlen(linebuf) || (linebuf[0] == '#'))
      continue;

    if(sscanf(linebuf,"%s",keyword) == 1)
    {
      if(!strcasecmp(keyword, "origin"))
      {
        PLAYER_WARN1("origin line is deprecated: %s",linebuf);
        continue;
      }
      else if(!strcasecmp(keyword, "width"))
      {
        PLAYER_WARN1("width line is deprecated: %s:",linebuf);
        continue;
      }
      else if(!strcasecmp(keyword, "height"))
      {
        PLAYER_WARN1("height line is deprecated: %s:",linebuf);
        continue;
      }
    }

    if(sscanf(linebuf, " %f %f %f %f", &x0, &y0, &x1, &y1) == 4)
        AddVector(x0, y0, x1, y1);
    else
      PLAYER_WARN1("ignoring line:%s:", linebuf);
  }

/*
  if(!got_origin || !got_width || !got_height)
  {
    PLAYER_ERROR("file is missing meta-data");
    return(-1);
  }
*/

  assert(this->vmap);

  puts("Done.");
  printf("VMapFile read a %d-segment map\n", this->vmap->segments_count);
  return(0);
}

int
VMapFile::Shutdown()
{
  free(this->vmap->segments);
  free(this->vmap);
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
// Process an incoming message
int VMapFile::ProcessMessage(QueuePointer & resp_queue,
                             player_msghdr * hdr,
                             void * data)
{
  HANDLE_CAPABILITY_REQUEST(device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILITIES_REQ);
  HANDLE_CAPABILITY_REQUEST(device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_MAP_REQ_GET_VECTOR);
  
  // Is it a request for the map?
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                           PLAYER_MAP_REQ_GET_VECTOR,
                           this->device_addr))
  {
    // Give it the map.
    this->Publish(this->device_addr, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_MAP_REQ_GET_VECTOR,
                  (void*)this->vmap);
    return(0);
  }
  else
    return(-1);
}

