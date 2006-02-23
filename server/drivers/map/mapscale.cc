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
 * A driver to read an occupancy grid map from another map device and
 * scale it to produce a map with a different given resolution.
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_mapscale mapscale
 * @brief Scale grid maps

@todo This driver is currently disabled because it needs to be updated to
the Player 2.0 API.

The mapscale driver reads a occupancy grid map from another @ref
interface_map device and scales it to produce a new map
with a different resolution.  The scaling is accomplished with the
gdk_pixbuf_scale_simple() function, using the GDK_INTERP_HYPER algorithm.

@par Compile-time dependencies

- gdk-pixbuf-2.0 (usually installed as part of GTK)

@par Provides

- @ref interface_map : the resulting scaled map

@par Requires

- @ref interface_map : the raw map, to be scaled

@par Configuration requests

- PLAYER_MAP_GET_INFO_REQ
- PLAYER_MAP_GET_DATA_REQ

@par Configuration file options

- resolution (length)
  - Default: -1.0
  - The new scale (length / pixel).
 
@par Example 

@verbatim
driver
(
  name "mapfile"
  provides ["map:0"]
  filename "mymap.pgm"
  resolution 0.1  # 10cm per pixel
)
driver
(
  name "mapscale"
  requires ["map:0"]  # read from map:0
  provides ["map:1"]  # output scaled map on map:1
  resolution 0.5 # scale to 50cm per pixel
)
@endverbatim

@author Brian Gerkey

*/
/** @} */


#include <sys/types.h> // required by Darwin
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// use gdk to interpolate
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <player.h>
#include <drivertable.h>
#include <devicetable.h>
#include <error.h>

// compute linear index for given map coords
#define MAP_IDX(mf, i, j) ((mf->size_x) * (j) + (i))
#define NEW_MAP_IDX(mf, i, j) ((mf->new_size_x) * (j) + (i))

// check that given coords are valid (i.e., on the map)
#define MAP_VALID(mf, i, j) ((i >= 0) && (i < mf->size_x) && (j >= 0) && (j < mf->size_y))
#define NEW_MAP_VALID(mf, i, j) ((i >= 0) && (i < mf->new_size_x) && (j >= 0) && (j < mf->new_size_y))

extern int global_playerport;

class MapScale : public Driver
{
  private:
    double resolution;
    unsigned int size_x, size_y;
    char* mapdata;
    player_device_id_t map_id;

    double new_resolution;
    unsigned int new_size_x, new_size_y;
    char* new_mapdata;

    // get the map from the underlying map device
    int GetMap();
    // interpolate the map
    int Scale();
    
    // Handle map info request
    void HandleGetMapInfo(void *client, void *request, int len);
    // Handle map data request
    void HandleGetMapData(void *client, void *request, int len);

  public:
    MapScale(ConfigFile* cf, int section, player_device_id_t id, double new_resolution);
    ~MapScale();
    int ProcessMessage(ClientData * client, player_msghdr * hdr, uint8_t * data, uint8_t * resp_data, size_t * resp_len);
    int Setup();
    int Shutdown();
/*    int PutConfig(player_device_id_t id, void *client, 
                  void* src, size_t len,
                  struct timeval* timestamp);*/
};

Driver*
MapScale_Init(ConfigFile* cf, int section)
{
  player_device_id_t map_id;
  double resolution;

  // Must have an input map
  if (cf->ReadDeviceId(&map_id, section, "requires",
                       PLAYER_MAP_CODE, -1, NULL) != 0)
  {
    PLAYER_ERROR("must specify input map");
    return(NULL);
  }
  if((resolution = cf->ReadLength(section,"resolution",-1.0)) < 0)
  {
    PLAYER_ERROR("must specify positive map resolution");
    return(NULL);
  }

  return((Driver*)(new MapScale(cf, section, map_id, resolution)));
}

// a driver registration function
void 
MapScale_Register(DriverTable* table)
{
  table->AddDriver("mapscale", MapScale_Init);
}


// this one has no data or commands, just configs
MapScale::MapScale(ConfigFile* cf, int section, player_device_id_t id, double res)
  : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_MAP_CODE, PLAYER_READ_MODE)
{
  this->mapdata = this->new_mapdata = NULL;
  this->size_x = this->size_y = 0;
  this->new_size_x = this->new_size_y = 0;
  this->map_id = id;
  this->new_resolution = res;
}

MapScale::~MapScale()
{
}

int
MapScale::Setup()
{
  if(this->GetMap() < 0)
    return(-1);
  if(this->Scale() < 0)
    return(-1);

  return(0);
}

// get the map from the underlying map device
// TODO: should Unsubscribe from the map on error returns in the function
int
MapScale::GetMap()
{
  Driver* mapdevice;

  // Subscribe to the map device
  if(!(mapdevice = SubscribeInternal(map_id)))
  {
    PLAYER_ERROR("unable to locate suitable map device");
    return(-1);
  }
/*  if(mapdevice == this)
  {
    PLAYER_ERROR("tried to subscribe to self; specify a *different* map index");
    return(-1);
  }
  if(mapdevice->Subscribe(map_id) != 0)
  {
    PLAYER_ERROR("unable to subscribe to map device");
    return(-1);
  }*/

  printf("MapScale: Loading map from map:%d...\n", this->map_id.index);
  fflush(NULL);

  // Fill in the map structure

  // first, get the map info
  //int replen;
  unsigned short reptype;
  player_map_info_t info;
  //struct timeval ts;
/*  info.subtype = PLAYER_MAP_GET_INFO_REQ;
  if((replen = mapdevice->Request(map_id, this, 
                                  &info, sizeof(info.subtype), NULL,
                                  &reptype, &info, sizeof(info), &ts)) == 0)
  {
    PLAYER_ERROR("failed to get map info");
    return(-1);
  }*/
  size_t resp_size = sizeof(info);
  reptype = mapdevice->ProcessMessage(PLAYER_MSGTYPE_REQ, PLAYER_MAP_GET_INFO, 
                        map_id, 0, (uint8_t *)&info, (uint8_t *)&info, &resp_size);

  if (reptype != PLAYER_MSGTYPE_RESP_ACK)
  {
    PLAYER_ERROR("failed to get map info");
    return(-1);	
  }
  
  // copy in the map info
  this->resolution = 1/(ntohl(info.scale) / 1e3);
  this->size_x = ntohl(info.width);
  this->size_y = ntohl(info.height);
  
  // allocate space for map cells
  assert(this->mapdata = (char*)malloc(sizeof(char) *
                                       this->size_x *
                                       this->size_y));

  // allocate space for map cells
  this->mapdata = (char*)malloc(sizeof(char) *
                                       this->size_x *
                                       this->size_y);

  assert(this->mapdata);

  // now, get the map data
  player_map_data_t data_req;
  unsigned int reqlen;
  unsigned int i,j;
  unsigned int oi,oj;
  unsigned int sx,sy;
  unsigned int si,sj;

  //data_req.subtype = PLAYER_MAP_GET_DATA_REQ;
  
  // Tile size
  sy = sx = (int)sqrt(sizeof(data_req.data));
  assert(sx * sy < (int)sizeof(data_req.data));
  oi=oj=0;
  while((oi < this->size_x) && (oj < this->size_y))
  {
    si = MIN(sx, this->size_x - oi);
    sj = MIN(sy, this->size_y - oj);

    data_req.col = htonl(oi);
    data_req.row = htonl(oj);
    data_req.width = htonl(si);
    data_req.height = htonl(sj);

    reqlen = sizeof(data_req) - sizeof(data_req.data);
    resp_size = sizeof(data_req);
    reptype = mapdevice->ProcessMessage(PLAYER_MSGTYPE_REQ, PLAYER_MAP_GET_DATA, 
                        map_id, reqlen, (uint8_t *)&data_req, (uint8_t *)&data_req, &resp_size);



/*    if((replen = mapdevice->Request(map_id, this, 
                                    &data_req, reqlen, NULL,
                                    &reptype, &data_req, 
                                    sizeof(data_req), &ts)) == 0)*/
    if (reptype != PLAYER_MSGTYPE_RESP_ACK)
    {
      PLAYER_ERROR("failed to get map info");
      return(-1);
    }
    else if(resp_size != (reqlen + si * sj))
    {
      PLAYER_ERROR2("got less map data than expected (%d != %d)",
                    resp_size, reqlen + si*sj);
      return(-1);
    }

    // copy the map data
    for(j=0;j<sj;j++)
    {
      for(i=0;i<si;i++)
      {
        this->mapdata[MAP_IDX(this,oi+i,oj+j)] = data_req.data[j*si + i];
      }
    }

    oi += si;
    if(oi >= this->size_x)
    {
      oi = 0;
      oj += sj;
    }
  }

  // we're done with the map device now
  UnsubscribeInternal(map_id);

  puts("Done.");
  printf("MapScale read a %d X %d map, at %.3f m/pix\n",
         this->size_x, this->size_y, this->resolution);
  return(0);
}

// interpolate the map
int
MapScale::Scale()
{
  unsigned int i,j;
  double scale_factor;
  GdkPixbuf* pixbuf;
  GdkPixbuf* new_pixbuf;
  int rowstride;
  int bits_per_sample;
  int n_channels;
  gboolean has_alpha;
  guchar* map_pixels;
  guchar* new_map_pixels;
  guchar* p;
  int new_rowstride;

  printf("MapScale scaling to resolution %.3fm...",
         this->new_resolution);
  fflush(NULL);

  has_alpha = FALSE;
  
  bits_per_sample = 8;
  n_channels = 3;
  rowstride = size_x * sizeof(guchar) * n_channels;
  assert(map_pixels = (guchar*)calloc(this->size_x * this->size_y,
                                      sizeof(guchar)*n_channels));

  // fill in the image from the map
  for(j=0; j<this->size_y; j++)
  {
    for(i=0; i<this->size_x; i++)
    {
      // fill the corresponding image pixel
      p = map_pixels + (this->size_y - j - 1)*rowstride + i*n_channels;

      if(this->mapdata[MAP_IDX(this,i,j)] == -1)
        *p = 255;
      else if(this->mapdata[MAP_IDX(this,i,j)] == 0)
        *p = 127;
      else
        *p = 0;
    }
  }

  g_assert((pixbuf = gdk_pixbuf_new_from_data((const guchar*)map_pixels,
                                              GDK_COLORSPACE_RGB,
                                              has_alpha,
                                              bits_per_sample,
                                              this->size_x,
                                              this->size_y,
                                              rowstride,
                                              NULL,NULL)));

  scale_factor = this->resolution / this->new_resolution;
  this->new_size_x = (int)rint(this->size_x * scale_factor);
  this->new_size_y = (int)rint(this->size_y * scale_factor);

  g_assert((new_pixbuf = gdk_pixbuf_scale_simple(pixbuf, 
                                                 this->new_size_x,
                                                 this->new_size_y,
                                                 GDK_INTERP_HYPER)));
  new_map_pixels = gdk_pixbuf_get_pixels(new_pixbuf);
  new_rowstride = gdk_pixbuf_get_rowstride(new_pixbuf);

  assert(this->new_mapdata = (char*)calloc(this->new_size_x * this->new_size_y,
                                           sizeof(char)));
  // fill in the map from the scaled image
  for(j=0; j<this->new_size_y; j++)
  {
    for(i=0; i<this->new_size_x; i++)
    {
      // fill the corresponding map cell
      p = new_map_pixels + (this->new_size_y-j-1)*new_rowstride + i*n_channels;

      if(*p > 0.9 * 255)
        this->new_mapdata[NEW_MAP_IDX(this,i,j)] = -1;
      else if(*p < 0.1 * 255)
        this->new_mapdata[NEW_MAP_IDX(this,i,j)] = 1;
      else
        this->new_mapdata[NEW_MAP_IDX(this,i,j)] = 0;
    }
  }

  g_object_unref((GObject*)pixbuf);
  // TODO: create a GdkPixbufDestroyNotify function, and pass it to
  // gdk_pixbuf_new_from_data() above, so that this buffer is free()d
  // automatically.
  free(map_pixels);
  g_object_unref((GObject*)new_pixbuf);
  free(this->mapdata);

  printf("Done. New map is %d X %d.\n", this->new_size_x, this->new_size_y);

  return(0);
}

int
MapScale::Shutdown()
{
  free(this->new_mapdata);
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
// Process an incoming message
int MapScale::ProcessMessage(ClientData * client, player_msghdr * hdr, uint8_t * data, uint8_t * resp_data, size_t * resp_len)
{
  assert(hdr);
  assert(data);
  assert(resp_data);
  assert(resp_len);

 
  if (MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_MAP_GET_INFO, device_id))
  {
  	assert(*resp_len > sizeof(player_map_info_t));
  	*resp_len = sizeof(player_map_info_t);
  	player_map_info_t & info = *reinterpret_cast<player_map_info_t *> (resp_data);

    info.scale = htonl((uint32_t)rint(1e3 / this->resolution));

    info.width = htonl((uint32_t) (this->size_x));
    info.height = htonl((uint32_t) (this->size_y));
  	
  	return PLAYER_MSGTYPE_RESP_ACK;
  }
  
  if (MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_MAP_GET_DATA, device_id))
  {
  	player_map_data_t & map_data = *reinterpret_cast<player_map_data_t *> (resp_data);

    unsigned int i, j;
    unsigned int oi, oj, si, sj;

    // Construct reply
    memcpy(resp_data, data, hdr->size);

    oi = ntohl(map_data.col);
    oj = ntohl(map_data.row);
    si = ntohl(map_data.width);
    sj = ntohl(map_data.height);

    // Grab the pixels from the map
    for(j = 0; j < sj; j++)
    {
      for(i = 0; i < si; i++)
      {
        if((i * j) <= PLAYER_MAP_MAX_CELLS_PER_TILE)
        {
          if(MAP_VALID(this, i + oi, j + oj))
            map_data.data[i + j * si] = this->mapdata[MAP_IDX(this, i+oi, j+oj)];
          else
          {
            PLAYER_WARN2("requested cell (%d,%d) is offmap", i+oi, j+oj);
            map_data.data[i + j * si] = 0;
          }
        }
        else
        {
          PLAYER_WARN("requested tile is too large; truncating");
          if(i == 0)
          {
            map_data.width = htonl(si-1);
            map_data.height = htonl(j-1);
          }
          else
          {
            map_data.width = htonl(i);
            map_data.height = htonl(j);
          }
        }
      }
    }

    size_t size=sizeof(map_data) - sizeof(map_data.data) + ntohl(map_data.width) * ntohl(map_data.height);
  	assert(*resp_len >= size);
  	*resp_len = size;
  	return PLAYER_MSGTYPE_RESP_ACK;
  }

  return -1;
}

// Process configuration requests
/*int 
MapScale::PutConfig(player_device_id_t id, void *client, 
                   void* src, size_t len,
                   struct timeval* timestamp)
{
  // Discard bogus empty packets
  if(len < 1)
  {
    PLAYER_WARN("got zero length configuration request; ignoring");
    if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
      PLAYER_ERROR("PutReply() failed");
    return(0);
  }

  // Process some of the requests immediately
  switch(((unsigned char*) src)[0])
  {
    case PLAYER_MAP_GET_INFO_REQ:
      HandleGetMapInfo(client, src, len);
      break;
    case PLAYER_MAP_GET_DATA_REQ:
      HandleGetMapData(client, src, len);
      break;
    default:
      PLAYER_ERROR("got unknown config request; ignoring");
      if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
  }

  return(0);
}

// Handle map info request
void 
MapScale::HandleGetMapInfo(void *client, void *request, int len)
{
  int reqlen;
  player_map_info_t info;
  
  // Expected length of request
  reqlen = sizeof(info.subtype);
  
  // check if the config request is valid
  if(len != reqlen)
  {
    PLAYER_ERROR2("config request len is invalid (%d != %d)", len, reqlen);
    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
      PLAYER_ERROR("PutReply() failed");
    return;
  }

  if(this->new_mapdata == NULL)
  {
    PLAYER_ERROR("NULL map data");
    if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
      PLAYER_ERROR("PutReply() failed");
    return;
  }

  // copy in subtype
  info.subtype = ((player_map_info_t*)request)->subtype;

  // convert to pixels / kilometer
  info.scale = htonl((uint32_t)rint(1e3 / this->new_resolution));

  info.width = htonl((uint32_t) (this->new_size_x));
  info.height = htonl((uint32_t) (this->new_size_y));

  // Send map info to the client
  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, &info, sizeof(info),NULL) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}

// Handle map data request
void 
MapScale::HandleGetMapData(void *client, void *request, int len)
{
  int i, j;
  int oi, oj, si, sj;
  int reqlen;
  player_map_data_t data;

  // Expected length of request
  reqlen = sizeof(data) - sizeof(data.data);

  // check if the config request is valid
  if(len != reqlen)
  {
    PLAYER_ERROR2("config request len is invalid (%d != %d)", len, reqlen);
    if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
      PLAYER_ERROR("PutReply() failed");
    return;
  }

  // Construct reply
  memcpy(&data, request, len);

  oi = ntohl(data.col);
  oj = ntohl(data.row);
  si = ntohl(data.width);
  sj = ntohl(data.height);

  // Grab the pixels from the map
  for(j = 0; j < sj; j++)
  {
    for(i = 0; i < si; i++)
    {
      if((i * j) <= PLAYER_MAP_MAX_CELLS_PER_TILE)
      {
        if(NEW_MAP_VALID(this, i + oi, j + oj))
          data.data[i + j * si] = 
                  this->new_mapdata[NEW_MAP_IDX(this, i+oi, j+oj)];
        else
        {
          PLAYER_WARN2("requested cell (%d,%d) is offmap", i+oi, j+oj);
          data.data[i + j * si] = 0;
        }
      }
      else
      {
        PLAYER_WARN("requested tile is too large; truncating");
        if(i == 0)
        {
          data.width = htonl(si-1);
          data.height = htonl(j-1);
        }
        else
        {
          data.width = htonl(i);
          data.height = htonl(j);
        }
      }
    }
  }
    
  // Send map info to the client
  if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, &data, 
              sizeof(data) - sizeof(data.data) + 
              ntohl(data.width) * ntohl(data.height),NULL) != 0)
    PLAYER_ERROR("PutReply() failed");
  return;
}
*/
