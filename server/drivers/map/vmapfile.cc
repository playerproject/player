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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 *
 * A driver to read a vector map from a text file
 */

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_vmapfile vmapfile

The vmapfile driver reads a vector map from a text file and
provides the map to others via the @ref player_interface_map interface.

The format of the text file is...

@par Compile-time dependencies

- None

@par Provides

- @ref player_interface_map

@par Requires

- None

@par Configuration requests

- PLAYER_MAP_REQ_GET_VECTOR

@par Configuration file options

- filename (string)
  - Default: NULL
  - The file to read.
 
@par Example 

@verbatim
driver
(
  name "vmapfile"
  provides ["map:0"]
  filename "mymap.wld"
)
@endverbatim

@par Authors

Brian Gerkey

*/
/** @} */

#include <sys/types.h> // required by Darwin
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <libplayercore/playercore.h>

class VMapFile : public Driver
{
  private:
    const char* filename;
    player_map_data_vector_t* vmap;
    
    // Handle map data request
    void HandleGetMapVector(void *client, void *request, int len);

  public:
    VMapFile(ConfigFile* cf, int section, const char* file);
    ~VMapFile();
    int Setup();
    int Shutdown();

    // MessageHandler
    int ProcessMessage(MessageQueue * resp_queue, 
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
  return((Driver*)(new MapFile(cf, section, filename)));
}

// a driver registration function
void 
VMapFile_Register(DriverTable* table)
{
  table->AddDriver("vmapfile", VMapFile_Init);
}


// this one has no data or commands, just configs
VMapFile::VMapFile(ConfigFile* cf, int section, const char* file)
  : Driver(cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_MAP_CODE)
{
  this->vmap = NULL;
  this->filename = file;
}

VMapFile::~VMapFile()
{
}

int
VMapFile::Setup()
{
  FILE* fp;
  double ox, oy, w, h;
  double x0,y0,x1,y1;
  char linebuf[512];
  char keyword [512];
  int got_origin, got_width, got_height;

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
  got_origin = got_width = got_height = 0;
  while(!feof(fp))
  {
    if(!fgets(linebuf, sizeof(linebuf), fp))
      break;
    if(!strlen(linebuf) || (linebuf[0] == '#'))
      continue;

    if((sscanf(linebuf,"%s %f %f", keyword, &ox, &oy) == 3) &&
       !strcmp(keyword, "origin"))
    {
      got_origin = 1;
    }
    else if((sscanf(linebuf,"%s %f", keyword, &w) == 2) &&
            !strcmp(keyword, "width"))
    {
      got_width = 1;
    }
    else if((sscanf(linebuf,"%s %f", keyword, &h) == 2) &&
            !strcmp(keyword, "height"))
    {
      got_height = 1;
    }
    else if(sscanf(linebuf, "%f %f %f %f", &x0, &y0, &x1, &y1) == 4)
    {
      this->vmap->segments[this->vmap->segments_count].x0 = (float)x0;
      this->vmap->segments[this->vmap->segments_count].y0 = (float)y0;
      this->vmap->segments[this->vmap->segments_count].x1 = (float)x1;
      this->vmap->segments[this->vmap->segments_count].y1 = (float)y1;
      this->vmap->segments_count++;
      if(this->vmap->segments_count == PLAYER_MAP_MAX_SEGMENTS)
      {
        PLAYER_WARN("too many segments in file; truncating");
        break;
      }
    }
    else
      PLAYER_WARN1("ignoring line:%s:", linebuf);
  }

  if(!got_origin || !got_width || !got_height)
  {
    PLAYER_ERROR("file is missing meta-data");
    return(-1);
  }

  this->vmap->minx = (float)ox;
  this->vmap->miny = (float)oy;
  this->vmap->maxx = (float)(w + ox);
  this->vmap->maxy = (float)(h + oy);

  this->vmap = (player_map_data_vector_t*)realloc(this->vmap,
                                                  sizeof(player_map_data_vector_t) - 
                                                  ((PLAYER_MAP_MAX_SEGMENTS - 
                                                    this->vmap->segments_count) * 
                                                   sizeof(player_segment_t)));
  assert(this->vmap);

  puts("Done.");
  printf("VMapFile read a %d-segment map\n", this->vmap->segments_count);
  return(0);
}

int
VMapFile::Shutdown()
{
  free(this->vmap);
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
// Process an incoming message
int VMapFile::ProcessMessage(MessageQueue * resp_queue, 
                            player_msghdr * hdr, 
                            void * data)
{
  // Is it a request for map meta-data?
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_MAP_REQ_GET_INFO, 
                           this->device_addr))
  {
    if(hdr->size != 0)
    {
      PLAYER_ERROR2("request is wrong length (%d != %d); ignoring",
                    hdr->size, sizeof(player_laser_config_t));
      return(-1);
    }
    player_map_info_t info;
    info.scale = this->resolution;
    info.width = this->size_x;
    info.height = this->size_y;
    info.origin.px = -(this->size_x / 2.0) * this->resolution;
    info.origin.py = -(this->size_y / 2.0) * this->resolution;
    info.origin.pa = 0.0;

    this->Publish(this->device_addr, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_MAP_REQ_GET_INFO,
                  (void*)&info, sizeof(info), NULL);
    return(0);
  }
  // Is it a request for a map tile?
  else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, 
                           PLAYER_MAP_REQ_GET_DATA,
                           this->device_addr))
  {
    player_map_data_t* mapreq = (player_map_data_t*)data;

    // Can't declare a map tile on the stack (it's too big)
    size_t mapsize = (sizeof(player_map_data_t) - PLAYER_MAP_MAX_TILE_SIZE + 
                      (mapreq->width * mapreq->height));
    player_map_data_t* mapresp = (player_map_data_t*)calloc(1,mapsize);
    assert(mapresp);
    
    int i, j;
    int oi, oj, si, sj;

    // Construct reply
    oi = mapresp->col = mapreq->col;
    oj = mapresp->row = mapreq->row;
    si = mapresp->width = mapreq->width;
    sj = mapresp->height = mapreq->height;

    // Grab the pixels from the map
    for(j = 0; j < sj; j++)
    {
      for(i = 0; i < si; i++)
      {
        if((i * j) <= PLAYER_MAP_MAX_TILE_SIZE)
        {
          if(MAP_VALID(this, i + oi, j + oj))
            mapresp->data[i + j * si] = this->mapdata[MAP_IDX(this, i+oi, j+oj)];
          else
          {
            PLAYER_WARN2("requested cell (%d,%d) is offmap", i+oi, j+oj);
            mapresp->data[i + j * si] = 0;
          }
        }
        else
        {
          PLAYER_WARN("requested tile is too large; truncating");
          if(i == 0)
          {
            mapresp->width = si-1;
            mapresp->height = j-1;
          }
          else
          {
            mapresp->width = i;
            mapresp->height = j;
          }
        }
      }
    }

    // recompute size, in case the tile got truncated
    mapsize = (sizeof(player_map_data_t) - PLAYER_MAP_MAX_TILE_SIZE + 
               (mapresp->width * mapresp->height));
    mapresp->data_count = mapresp->width * mapresp->height;
    this->Publish(this->device_addr, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_MAP_REQ_GET_DATA,
                  (void*)mapresp, mapsize, NULL);
    free(mapresp);
    return(0);
  }
  return(-1);
}

