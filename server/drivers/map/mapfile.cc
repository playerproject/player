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
 * A driver to read an occupancy grid map from an image file.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <player.h>
#include <drivertable.h>
#include <driver.h>

// use gdk-pixbuf for image loading
#include <gdk-pixbuf/gdk-pixbuf.h>

// compute linear index for given map coords
#define MAP_IDX(mf, i, j) ((mf->size_x) * (j) + (i))

// check that given coords are valid (i.e., on the map)
#define MAP_VALID(mf, i, j) ((i >= 0) && (i < mf->size_x) && (j >= 0) && (j < mf->size_y))


extern int global_playerport;

class MapFile : public Driver
{
  private:
    const char* filename;
    double resolution;
    int negate;
    int size_x, size_y;
    char* mapdata;
    
    // Handle map info request
    void HandleGetMapInfo(void *client, void *request, int len);
    // Handle map data request
    void HandleGetMapData(void *client, void *request, int len);

  public:
    MapFile(ConfigFile* cf, int section, const char* file, double res, int neg);
    ~MapFile();
    virtual int Setup();
    virtual int Shutdown();
    virtual int PutConfig(player_device_id_t* device, void* client,
                          void* data, size_t len);
};

Driver*
MapFile_Init(ConfigFile* cf, int section)
{
  const char* filename;
  double resolution;
  int negate;

  if(!(filename = cf->ReadFilename(section,"filename", NULL)))
  {
    PLAYER_ERROR("must specify map filename");
    return(NULL);
  }
  if((resolution = cf->ReadFloat(section,"resolution",-1.0)) < 0)
  {
    PLAYER_ERROR("must specify positive map resolution");
    return(NULL);
  }
  negate = cf->ReadInt(section,"negate",0);

  return((Driver*)(new MapFile(cf, section, filename, resolution, negate)));
}

// a driver registration function
void 
MapFile_Register(DriverTable* table)
{
  table->AddDriver("mapfile", MapFile_Init);
}


// this one has no data or commands, just configs
MapFile::MapFile(ConfigFile* cf, int section, const char* file, double res, int neg) : 
  Driver(cf, section, PLAYER_MAP_CODE, PLAYER_READ_MODE,
         0,0,100,100)
{
  this->mapdata = NULL;
  this->size_x = this->size_y = 0;
  this->filename = file;
  this->resolution = res;
  this->negate = neg;
}

MapFile::~MapFile()
{
}

int
MapFile::Setup()
{
  GdkPixbuf* pixbuf;
  guchar* pixels;
  guchar* p;
  int rowstride, n_channels, bps;
  GError* error = NULL;
  int i,j,k;
  double occ;
  int color_sum;
  double color_avg;

  // Initialize glib
  g_type_init();

  printf("MapFile loading image file: %s...", this->filename);
  fflush(stdout);

  // Read the image
  if(!(pixbuf = gdk_pixbuf_new_from_file(this->filename, &error)))
  {
    PLAYER_ERROR1("failed to open image file %s", this->filename);
    return(-1);
  }

  this->size_x = gdk_pixbuf_get_width(pixbuf);
  this->size_y = gdk_pixbuf_get_height(pixbuf);

  assert(this->mapdata = (char*)malloc(sizeof(char) *
                                       this->size_x * this->size_y));

  rowstride = gdk_pixbuf_get_rowstride(pixbuf);
  bps = gdk_pixbuf_get_bits_per_sample(pixbuf)/8;
  n_channels = gdk_pixbuf_get_n_channels(pixbuf);
  if(gdk_pixbuf_get_has_alpha(pixbuf))
    n_channels++;

  // Read data
  pixels = gdk_pixbuf_get_pixels(pixbuf);
  for(j = 0; j < this->size_y; j++)
  {
    for (i = 0; i < this->size_x; i++)
    {
      p = pixels + j*rowstride + i*n_channels*bps;
      color_sum = 0;
      for(k=0;k<n_channels;k++)
        color_sum += *(p + (k * bps));
      color_avg = color_sum / (double)n_channels;

      if(this->negate)
        occ = color_avg / 255.0;
      else
        occ = (255 - color_avg) / 255.0;
      if(occ > 0.95)
        this->mapdata[MAP_IDX(this,i,this->size_y - j - 1)] = +1;
      else if(occ < 0.1)
        this->mapdata[MAP_IDX(this,i,this->size_y - j - 1)] = -1;
      else
        this->mapdata[MAP_IDX(this,i,this->size_y - j - 1)] = 0;
    }
  }

  gdk_pixbuf_unref(pixbuf);

  puts("Done.");
  printf("MapFile read a %d X %d map, at %.3f m/pix\n",
         this->size_x, this->size_y, this->resolution);
  return(0);
}

int
MapFile::Shutdown()
{
  free(this->mapdata);
  return(0);
}

// Process configuration requests
int 
MapFile::PutConfig(player_device_id_t* device, void* client,
                   void* data, size_t len)
{
  // Discard bogus empty packets
  if(len < 1)
  {
    PLAYER_WARN("got zero length configuration request; ignoring");
    if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
      PLAYER_ERROR("PutReply() failed");
    Unlock();
    return(0);
  }

  // Process some of the requests immediately
  switch(((char*) data)[0])
  {
    case PLAYER_MAP_GET_INFO_REQ:
      HandleGetMapInfo(client, data, len);
      break;
    case PLAYER_MAP_GET_DATA_REQ:
      HandleGetMapData(client, data, len);
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
MapFile::HandleGetMapInfo(void *client, void *request, int len)
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
  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, &info, sizeof(info), NULL) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}

// Handle map data request
void 
MapFile::HandleGetMapData(void *client, void *request, int len)
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
