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
 * convolve it with a robot to create the C-space.
 */

#include <sys/types.h> // required by Darwin
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <player.h>
#include <drivertable.h>
#include <driver.h>
#include <devicetable.h>
#include <error.h>

// compute linear index for given map coords
#define MAP_IDX(mf, i, j) ((mf->size_x) * (j) + (i))

// check that given coords are valid (i.e., on the map)
#define MAP_VALID(mf, i, j) ((i >= 0) && (i < mf->size_x) && (j >= 0) && (j < mf->size_y))

extern int global_playerport;

typedef enum
{
  CIRCLE,
} robot_shape_t;
         
class MapCspace : public Driver
{
  private:
    double resolution;
    int size_x, size_y;
    char* mapdata;
    int map_index;
    robot_shape_t robot_shape;
    double robot_radius;

    // get the map from the underlying map device
    int GetMap();
    // convolve the map with a circular robot to produce the cspace
    int CreateCspaceCircle();
    
    // Handle map info request
    void HandleGetMapInfo(void *client, void *request, int len);
    // Handle map data request
    void HandleGetMapData(void *client, void *request, int len);

  public:
    MapCspace(ConfigFile* cf, int section, int index, robot_shape_t shape, double radius);
    ~MapCspace();
    int Setup();
    int Shutdown();
    int PutConfig(player_device_id_t id, void *client, 
                  void* src, size_t len,
                  struct timeval* timestamp);
};

Driver*
MapCspace_Init(ConfigFile* cf, int section)
{
  const char* shapestring;
  int index;
  double radius;
  robot_shape_t shape;

  if((index = cf->ReadInt(section,"map_index",-1)) < 0)
  {
    PLAYER_ERROR("must specify positive map index");
    return(NULL);
  }
  if((radius = cf->ReadLength(section,"robot_radius",-1.0)) < 0)
  {
    PLAYER_ERROR("must specify positive robot radius");
    return(NULL);
  }
  if(!(shapestring = cf->ReadString(section,"robot_shape",NULL)))
  {
    PLAYER_ERROR("must specify robot shape");
    return(NULL);
  }
  if(!strcmp(shapestring,"circle"))
  {
    shape = CIRCLE;
  }
  else
  {
    PLAYER_ERROR1("unknown robot shape \"%s\"", shapestring);
    return(NULL);
  }

  return((Driver*)(new MapCspace(cf, section, index, shape, radius)));
}

// a driver registration function
void 
MapCspace_Register(DriverTable* table)
{
  table->AddDriver("mapcspace", MapCspace_Init);
}


// this one has no data or commands, just configs
MapCspace::MapCspace(ConfigFile* cf, int section,
                     int index, robot_shape_t shape, double radius) :
  Driver(cf, section, PLAYER_MAP_CODE, PLAYER_READ_MODE,
         0,0,100,100)
{
  this->mapdata = NULL;
  this->size_x = this->size_y = 0;
  this->map_index = index;
  this->robot_shape = shape;
  this->robot_radius = radius;
}

MapCspace::~MapCspace()
{
}

int
MapCspace::Setup()
{
  if(this->GetMap() < 0)
    return(-1);
  if(this->CreateCspaceCircle() < 0)
    return(-1);

  return(0);
}

// get the map from the underlying map device
// TODO: should Unsubscribe from the map on error returns in the function
int
MapCspace::GetMap()
{
  player_device_id_t map_id;
  Driver* mapdevice;

  // Subscribe to the map device
  map_id.port = global_playerport;
  map_id.code = PLAYER_MAP_CODE;
  map_id.index = this->map_index;

  if(!(mapdevice = deviceTable->GetDriver(map_id)))
  {
    PLAYER_ERROR("unable to locate suitable map device");
    return(-1);
  }
  if(mapdevice == this)
  {
    PLAYER_ERROR("tried to subscribe to self; specify a *different* map index");
    return(-1);
  }
  if(mapdevice->Subscribe(map_id) != 0)
  {
    PLAYER_ERROR("unable to subscribe to map device");
    return(-1);
  }

  printf("MapCspace: Loading map from map:%d...\n", this->map_index);
  fflush(NULL);

  // Fill in the map structure

  // first, get the map info
  int replen;
  unsigned short reptype;
  player_map_info_t info;
  struct timeval ts;
  info.subtype = PLAYER_MAP_GET_INFO_REQ;
  if((replen = mapdevice->Request(map_id, this, 
                                  &info, sizeof(info.subtype), NULL,
                                  &reptype, &info, sizeof(info), &ts)) == 0)
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

  // now, get the map data
  player_map_data_t data_req;
  int reqlen;
  int i,j;
  int oi,oj;
  int sx,sy;
  int si,sj;

  data_req.subtype = PLAYER_MAP_GET_DATA_REQ;
  
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

    if((replen = mapdevice->Request(map_id, this, 
                                    &data_req, reqlen, NULL,
                                    &reptype, &data_req, 
                                    sizeof(data_req), &ts)) == 0)
    {
      PLAYER_ERROR("failed to get map info");
      return(-1);
    }
    else if(replen != (reqlen + si * sj))
    {
      PLAYER_ERROR2("got less map data than expected (%d != %d)",
                    replen, reqlen + si*sj);
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
  if(mapdevice->Unsubscribe(map_id) != 0)
    PLAYER_WARN("unable to unsubscribe from map device");

  // Read data

  puts("Done.");
  printf("MapCspace read a %d X %d map, at %.3f m/pix\n",
         this->size_x, this->size_y, this->resolution);
  return(0);
}

// convolve the map with a circular robot to produce the cspace
int
MapCspace::CreateCspaceCircle()
{
  int i,j;
  int di,dj;
  int r;
  char state;

  // a parallel map, telling which cells have already been updated
  unsigned char* updated;

  printf("MapCspace creating C-space for circular robot with radius %.3fm...",
         this->robot_radius);
  fflush(NULL);

  assert(updated = (unsigned char*)calloc(this->size_x * this->size_y,
                                          sizeof(unsigned char*)));

  // compute robot radius in map cells
  r = (int)rint(this->robot_radius / this->resolution);

  for(j=0; j < this->size_y; j++)
  {
    for(i=0; i < this->size_x; i++)
    {
      // don't double-update
      if(updated[MAP_IDX(this,i,j)])
        continue;

      state = this->mapdata[MAP_IDX(this,i,j)];

      // grow both occupied and unknown regions
      if(state >= 0)
      {
        for(dj = -r; dj <= r; dj++)
        {
          for(di = -r; di <= r; di++)
          {
            // stay within the radius
            if((int)rint(sqrt(di*di + dj*dj)) > r)
              continue;

            // make sure we stay on the map
            if(!MAP_VALID(this,i+di,j+dj))
              continue;

            // don't change occupied to uknown
            if(this->mapdata[MAP_IDX(this,i+di,j+dj)] < state)
            {
              this->mapdata[MAP_IDX(this,i+di,j+dj)] = state;
              updated[MAP_IDX(this,i+di,j+dj)] = 1;
            }
          }
        }
      }
    }
  }

  free(updated);

  puts("Done.");

  return(0);
}

int
MapCspace::Shutdown()
{
  free(this->mapdata);
  return(0);
}


// Process configuration requests
int 
MapCspace::PutConfig(player_device_id_t id, void *client, 
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
MapCspace::HandleGetMapInfo(void *client, void *request, int len)
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

  if(this->mapdata == NULL)
  {
    PLAYER_ERROR("NULL map data");
    if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
      PLAYER_ERROR("PutReply() failed");
    return;
  }

  // convert to pixels / kilometer
  info.scale = htonl((uint32_t)rint(1e3 / this->resolution));

  info.width = htonl((uint32_t) (this->size_x));
  info.height = htonl((uint32_t) (this->size_y));

  // Send map info to the client
  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, &info, sizeof(info),NULL) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}

// Handle map data request
void 
MapCspace::HandleGetMapData(void *client, void *request, int len)
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
        if(MAP_VALID(this, i + oi, j + oj))
          data.data[i + j * si] = this->mapdata[MAP_IDX(this, i+oi, j+oj)];
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
