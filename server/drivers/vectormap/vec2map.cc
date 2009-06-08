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
 * A driver to convert a vector map to a regular grid map.
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_Vec2Map Vec2Map
 * @brief A driver to convert a vector map to a regular grid map.

@par Provides

- @ref interface_map

@par Requires

- @ref interface_vectormap

@par Configuration requests

- None

@par Configuration file options

- cells_per_unit (float)
  - Default: 0.0 (Should be set to something greater than zero!)
  - How many cells are occupied for one vectormap unit (for example 50.0 cells for one meter)
- full_extent (integer)
  - Default: 1
  - If set to 1, extent will be computed as if point (0, 0) were in the middle. Warning! It can produce large grid map!
- draw_border (integer)
  - Default: 1
  - If set to 1, border will be drawn around whole map. Border line will be marked as occupied place in the grid.
- skip_feature (string)
  - Default: ""
  - Do not draw feature with that name on final grid map

@par Example

@verbatim
driver
(
  name "vec2map"
  requires ["vectormap:0"]
  provides ["map:0"]
  cells_per_unit 50.0
)
@endverbatim

@author Paul Osmialowski

*/

/** @} */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <assert.h>
#include <libplayercore/playercore.h>
#include <libplayerwkb/playerwkb.h>

#define EPS 0.0000001
#define MAXFABS(a, b) ((fabs(a) > fabs(b)) ? fabs(a) : fabs(b))

class Vec2Map : public ThreadedDriver
{
  public:
    // Constructor; need that
    Vec2Map(ConfigFile * cf, int section);

    virtual ~Vec2Map();

    // Must implement the following methods.
    virtual int MainSetup();
    virtual void MainQuit();

    // This method will be invoked on each incoming message
    virtual int ProcessMessage(QueuePointer & resp_queue,
                               player_msghdr * hdr,
                               void * data);

  private:
    struct seglist
    {
      player_segment_t seg;
      struct Vec2Map::seglist * next; // segment is valid only if next is not NULL
      struct Vec2Map::seglist * last; // for the first element only
    };
#define SEGLIST(ptr) (reinterpret_cast<struct Vec2Map::seglist *>(ptr))

    // Main function for device thread.
    virtual void Main();

    // some helper functions
    static void line(int a, int b, int c, int d, int8_t * cells, int maxx, int maxy);
    static int over(int x, int min, int max);
    static void add_segment(void * segptr, double x0, double y0, double x1, double y1);

    // The address of the vectormap device to which we will
    // subscribe
    player_devaddr_t vectormap_addr;

    player_devaddr_t map_addr;

    // Handle for the device that have the address given above
    Device * vectormap_dev;

    double cells_per_unit;
    int full_extent;
    int draw_border;
    const char * skip_feature;
    playerwkbprocessor_t wkbProcessor;
};

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
//
// This driver will support the map interface, and it will subscribe
// to the vectormap interface.
//
Vec2Map::Vec2Map(ConfigFile * cf, int section)
    : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  memset(&(this->vectormap_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->map_addr), 0, sizeof(player_devaddr_t));
  this->skip_feature = NULL;
  this->wkbProcessor = player_wkb_create_processor(); // one per driver instance
  this->cells_per_unit = cf->ReadFloat(section, "cells_per_unit", 0.0);
  if ((this->cells_per_unit) <= 0.0)
  {
    PLAYER_ERROR("Invalid cells_per_unit value");
    this->SetError(-1);
    return;
  }
  this->full_extent = cf->ReadInt(section, "full_extent", 1);
  this->draw_border = cf->ReadInt(section, "draw_border", 1);
  this->skip_feature = cf->ReadString(section, "skip_feature", "");
  if (cf->ReadDeviceAddr(&(this->map_addr), section, "provides",
                         PLAYER_MAP_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->map_addr))
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->vectormap_addr), section, "requires",
                         PLAYER_VECTORMAP_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
}

Vec2Map::~Vec2Map()
{
  player_wkb_destroy_processor(this->wkbProcessor);
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int Vec2Map::MainSetup()
{
  // Retrieve the handle to the vectormap device.
  this->vectormap_dev = deviceTable->GetDevice(this->vectormap_addr);
  if (!(this->vectormap_dev))
  {
    PLAYER_ERROR("unable to locate suitable vectormap device");
    return -1;
  }
  // Subscribe my message queue the vectormap device.
  if (this->vectormap_dev->Subscribe(this->InQueue))
  {
    PLAYER_ERROR("unable to subscribe to vectormap device");
    return -1;
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
void Vec2Map::MainQuit()
{
  // Unsubscribe from the vectormap
  this->vectormap_dev->Unsubscribe(this->InQueue);
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void Vec2Map::Main()
{
  struct timespec tspec;

  // The main loop
  for(;;)
  {
    // Wait till we get new messages
    this->InQueue->Wait();

    pthread_testcancel();

    // Process incoming messages
    this->ProcessMessages();

    // sleep for a while
    tspec.tv_sec = 0;
    tspec.tv_nsec = 5000;
    nanosleep(&tspec, NULL);
  }
}

void Vec2Map::add_segment(void * segptr, double x0, double y0, double x1, double y1)
{
  struct Vec2Map::seglist * tmp;

  assert(segptr);
  tmp = SEGLIST(segptr)->last;
  if (!tmp) tmp = SEGLIST(segptr);
  memset(&(tmp->seg), 0, sizeof tmp->seg);
  tmp->seg.x0 = x0;
  tmp->seg.y0 = y0;
  tmp->seg.x1 = x1;
  tmp->seg.y1 = y1;
  tmp->next = reinterpret_cast<struct Vec2Map::seglist *>(malloc(sizeof(struct Vec2Map::seglist)));
  assert(tmp->next);
  memset(tmp->next, 0, sizeof(struct Vec2Map::seglist));
  memset(&(tmp->next->seg), 0, sizeof tmp->next->seg);
  tmp->next->last = NULL;
  tmp->next->next = NULL;
  SEGLIST(segptr)->last = tmp->next;
}

int Vec2Map::over(int x, int min, int max)
{
    if (x < min) return -1;
    if (x >= max) return 1;
    return 0;
}

void Vec2Map::line(int a, int b, int c, int d, int8_t * cells, int width, int height)
{
    double x, y;
    int distX = abs(a - c);
    int distY = abs(b - d);
    double wspX;
    double wspY;

    if (distX > distY)
    {
	if (!distX)
	{
	    wspX = 0.0;
	    wspY = 0.0;
	} else
	{
	    wspX = 1.0;
	    if (!distY) wspY = 0.0; else wspY = 1.0 / ((static_cast<double>(distX)) / (static_cast<double>(distY)));
	}
    } else
    {
	if (!distY)
	{
	    wspX = 0.0;
	    wspY = 0.0;
	} else
	{
	    wspY = 1.0;
	    if (!distX) wspX = 0.0; else wspX = 1.0 / ((static_cast<double>(distY)) / (static_cast<double>(distX)));
	}
    }
    if (c < a) wspX = -wspX;
    if (d < b) wspY = -wspY;

    x = static_cast<double>(a); y = static_cast<double>(b);
    if (static_cast<int>(x) < 0) x = 0.0;
    if (static_cast<int>(y) < 0) y = 0.0;
    if (static_cast<int>(x) >= width) x = static_cast<double>(width - 1);
    if (static_cast<int>(y) >= height) y = static_cast<double>(height - 1);
    cells[(static_cast<int>(y) * width) + (static_cast<int>(x))] = 1;
    if ((fabs(wspX) < EPS) && (fabs(wspY) < EPS)) return;

    while (!((fabs(x - static_cast<double>(c)) < EPS) && (fabs(y - static_cast<double>(d)) < EPS)))
    {
	x += wspX;
	y += wspY;
	if (Vec2Map::over(static_cast<int>(x), 0, width)) break;
	if (Vec2Map::over(static_cast<int>(y), 0, height)) break;
	cells[(static_cast<int>(y) * width) + (static_cast<int>(x))] = 1;
    }
}

int Vec2Map::ProcessMessage(QueuePointer & resp_queue,
                            player_msghdr * hdr,
                            void * data)
{
  player_map_info_t map_info;
  player_map_data_t map_data, map_data_request;
  player_map_data_vector_t map_data_vector;
  player_vectormap_layer_data_t layer, * layer_data;
  player_vectormap_info_t * vectormap_info;
  Message * msg;
  Message * rep;
  uint32_t width, height, data_count, ii, jj;
  int8_t * cells;
  struct Vec2Map::seglist segments, * tmp;
  double basex, basey;

  memset(&(segments.seg), 0, sizeof segments.seg);
  segments.last = NULL;
  segments.next = NULL;

  // Process messages here.  Send a response if necessary, using Publish().
  // If you handle the message successfully, return 0.  Otherwise,
  // return -1, and a NACK will be sent for you, if a response is required.

  // Is it new vectormap data?
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA,
                            -1, // -1 means 'all message subtypes'
                            this->vectormap_addr))
  {
    // we don't expect any data
    return 0;
  }

  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                            PLAYER_MAP_REQ_GET_INFO,
                            this->map_addr))
  {
    memset(&map_info, 0, sizeof map_info);
    msg = this->vectormap_dev->Request(this->InQueue,
                                       PLAYER_MSGTYPE_REQ,
                                       PLAYER_VECTORMAP_REQ_GET_MAP_INFO,
                                       NULL, 0, NULL, true);
    if (!msg)
    {
      PLAYER_WARN("failed to acquire vectormap info");
      return -1;
    }
    if ((msg->GetDataSize()) < (sizeof(player_vectormap_info_t)))
    {
      PLAYER_WARN2("invalid acqired data size %d vs %d", msg->GetDataSize(), sizeof(player_vectormap_info_t));
      delete msg;
      return -1;
    }
    vectormap_info = reinterpret_cast<player_vectormap_info_t *>(msg->GetPayload());
    if (!vectormap_info)
    {
      PLAYER_WARN("no data acquired");
      delete msg;
      return -1;
    }
    map_info.scale = 1.0 / this->cells_per_unit;
    if (this->full_extent)
    {
      map_info.width = static_cast<uint32_t>((MAXFABS(vectormap_info->extent.x0, vectormap_info->extent.x1) * 2.0) * this->cells_per_unit);
      map_info.height = static_cast<uint32_t>((MAXFABS(vectormap_info->extent.y0, vectormap_info->extent.y1) * 2.0) * this->cells_per_unit);
      map_info.origin.pa = 0.0;
      map_info.origin.px = -MAXFABS(vectormap_info->extent.x0, vectormap_info->extent.x1);
      map_info.origin.py = -MAXFABS(vectormap_info->extent.y0, vectormap_info->extent.y1);
    } else
    {
      map_info.width = static_cast<uint32_t>(fabs((vectormap_info->extent.x1) - (vectormap_info->extent.x0)) * this->cells_per_unit);
      map_info.height = static_cast<uint32_t>(fabs((vectormap_info->extent.y1) - (vectormap_info->extent.y0)) * this->cells_per_unit);
      map_info.origin.pa = 0.0;
      map_info.origin.px = vectormap_info->extent.x0;
      map_info.origin.py = vectormap_info->extent.y0;
    }
    delete msg;
    msg = NULL;
    this->Publish(this->map_addr,
                  resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_MAP_REQ_GET_INFO,
                  reinterpret_cast<void *>(&map_info));
    return 0;
  }

  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                            PLAYER_MAP_REQ_GET_DATA,
                            this->map_addr))
  {
    memset(&map_data, 0, sizeof map_data);
    if (!data)
    {
      PLAYER_WARN("request incomplete");
      return -1;
    }
    memcpy(&map_data_request, data, sizeof map_data_request);
    msg = this->vectormap_dev->Request(this->InQueue,
                                       PLAYER_MSGTYPE_REQ,
                                       PLAYER_VECTORMAP_REQ_GET_MAP_INFO,
                                       NULL, 0, NULL, true);
    if (!msg)
    {
      PLAYER_WARN("failed to acquire vectormap info");
      return -1;
    }
    if ((msg->GetDataSize()) < (sizeof(player_vectormap_info_t)))
    {
      PLAYER_WARN2("invalid acqired data size %d vs %d", msg->GetDataSize(), sizeof(player_vectormap_info_t));
      delete msg;
      return -1;
    }
    vectormap_info = reinterpret_cast<player_vectormap_info_t *>(msg->GetPayload());
    if (!vectormap_info)
    {
      PLAYER_WARN("no data acquired");
      delete msg;
      return -1;
    }
    if (this->full_extent)
    {
      width = static_cast<uint32_t>((MAXFABS(vectormap_info->extent.x0, vectormap_info->extent.x1) * 2.0) * this->cells_per_unit);
      height = static_cast<uint32_t>((MAXFABS(vectormap_info->extent.y0, vectormap_info->extent.y1) * 2.0) * this->cells_per_unit);
      basex = -MAXFABS(vectormap_info->extent.x0, vectormap_info->extent.x1);
      basey = -MAXFABS(vectormap_info->extent.y0, vectormap_info->extent.y1);
    } else
    {
      width = static_cast<uint32_t>(fabs((vectormap_info->extent.x1) - (vectormap_info->extent.x0)) * this->cells_per_unit);
      height = static_cast<uint32_t>(fabs((vectormap_info->extent.y1) - (vectormap_info->extent.y0)) * this->cells_per_unit);
      basex = vectormap_info->extent.x0;
      basey = vectormap_info->extent.y0;
    }
    data_count = width * height;
    if (!((data_count > 0) && ((vectormap_info->layers_count) > 0)))
    {
      PLAYER_WARN("Invalid map");
      delete msg;
      return -1;
    }
    for (ii = 0; ii < (vectormap_info->layers_count); ii++)
    {
      memset(&layer, 0, sizeof layer);
      layer.name_count = strlen(vectormap_info->layers[ii].name) + 1;
      assert((layer.name_count) > 0);
      layer.name = reinterpret_cast<char *>(malloc(layer.name_count));
      if (!(layer.name))
      {
        PLAYER_ERROR("cannot allocate space for layer.name");
	delete msg;
	return -1;
      }
      strcpy(layer.name, vectormap_info->layers[ii].name);
      rep = this->vectormap_dev->Request(this->InQueue,
                                         PLAYER_MSGTYPE_REQ,
                                         PLAYER_VECTORMAP_REQ_GET_LAYER_DATA,
                                         reinterpret_cast<void *>(&layer), 0, NULL, true);
      free(layer.name);
      layer.name = NULL;
      if (!rep)
      {
        PLAYER_WARN("failed to acquire layer data");
        return -1;
      }
      if ((rep->GetDataSize()) < (sizeof(player_vectormap_layer_data_t)))
      {
        PLAYER_WARN2("invalid acqired data size %d vs %d", rep->GetDataSize(), sizeof(player_vectormap_layer_data_t));
	delete rep;
        delete msg;
        return -1;
      }
      layer_data = reinterpret_cast<player_vectormap_layer_data_t *>(rep->GetPayload());
      if (!layer_data)
      {
        PLAYER_WARN("no data acquired");
	delete rep;
        delete msg;
        return -1;
      }
      for (jj = 0; jj < (layer_data->features_count); jj++)
      {
        if (this->skip_feature)
          if ((strlen(this->skip_feature)) && (layer_data->features[jj].name_count > 0))
            if (!strcmp(this->skip_feature, layer_data->features[jj].name)) continue;
	if (!player_wkb_process_wkb(this->wkbProcessor, layer_data->features[jj].wkb, static_cast<size_t>(layer_data->features[jj].wkb_count), reinterpret_cast<playerwkbcallback_t>(Vec2Map::add_segment), reinterpret_cast<void *>(&segments)))
	{
	  PLAYER_ERROR("Error while processing wkb!");
	}
      }
      delete rep;
      rep = NULL;
    }
    vectormap_info = NULL;
    delete msg;
    msg = NULL;
    map_data.data = NULL;
    cells = reinterpret_cast<int8_t *>(malloc(data_count * sizeof(int8_t)));
    if (!cells)
    {
      PLAYER_ERROR("cannot allocate space for cells");
      return -1;
    }
    memset(cells, -1, data_count);
    if (this->draw_border)
    {
      Vec2Map::line(0, 0, width - 1, 0, cells, width, height);
      Vec2Map::line(width - 1, 0, width - 1, height - 1, cells, width, height);
      Vec2Map::line(width - 1, height - 1, 0, height - 1, cells, width, height);
      Vec2Map::line(0, height - 1, 0, 0, cells, width, height);
    }
    for (tmp = &segments; tmp->next; tmp = tmp->next)
    {
      Vec2Map::line(static_cast<int>((tmp->seg.x0 - basex) * this->cells_per_unit), static_cast<int>((tmp->seg.y0 - basey) * this->cells_per_unit), static_cast<int>((tmp->seg.x1 - basex) * this->cells_per_unit), static_cast<int>((tmp->seg.y1 - basey) * this->cells_per_unit) , cells, width, height);
    }
    if (!(segments.next)) assert(!(segments.last));
    if (!(segments.last)) assert(!(segments.next));
    while (segments.next)
    {
      tmp = segments.next->next;
      free(segments.next);
      segments.next = tmp;
    }
    segments.last = NULL;
    if (map_data_request.col >= width) map_data_request.col = width - 1;
    if (map_data_request.row >= height) map_data_request.row = height - 1;
    if (map_data_request.col + map_data_request.width >= width) map_data_request.width = width - map_data_request.col;
    if (map_data_request.row + map_data_request.height >= height) map_data_request.height = height - map_data_request.row;
    if ((map_data_request.width > 0) && (map_data_request.height > 0))
    {
      map_data.data_count = map_data_request.width * map_data_request.height;
      assert(map_data.data_count > 0);
      map_data.data = reinterpret_cast<int8_t *>(malloc(map_data.data_count * sizeof(int8_t)));
      if (!(map_data.data))
      {
        PLAYER_ERROR("cannot allocate space for map data");
        if (cells) free(cells);
        return -1;
      }
      for (ii = 0; ii < (map_data_request.height); ii++) memcpy(map_data.data + (ii * (map_data_request.width)), cells + (ii * width) + map_data_request.col, map_data_request.width);
    }

    this->Publish(this->map_addr,
                  resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_MAP_REQ_GET_DATA,
                  reinterpret_cast<void *>(&map_data));
    if (cells) free(cells);
    cells = NULL;
    if (map_data.data) free(map_data.data);
    map_data.data = NULL;
    return 0;
  }

  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                            PLAYER_MAP_REQ_GET_VECTOR,
                            this->map_addr))
  {
    memset(&map_data_vector, 0, sizeof map_data_vector);
    msg = this->vectormap_dev->Request(this->InQueue,
                                       PLAYER_MSGTYPE_REQ,
                                       PLAYER_VECTORMAP_REQ_GET_MAP_INFO,
                                       NULL, 0, NULL, true);
    if (!msg)
    {
      PLAYER_WARN("failed to acquire vectormap info");
      return -1;
    }
    if ((msg->GetDataSize()) < (sizeof(player_vectormap_info_t)))
    {
      PLAYER_WARN2("invalid acqired data size %d vs %d", msg->GetDataSize(), sizeof(player_vectormap_info_t));
      delete msg;
      return -1;
    }
    vectormap_info = reinterpret_cast<player_vectormap_info_t *>(msg->GetPayload());
    if (!vectormap_info)
    {
      PLAYER_WARN("no data acquired");
      delete msg;
      return -1;
    }
    map_data_vector.minx = vectormap_info->extent.x0;
    map_data_vector.miny = vectormap_info->extent.y0;
    map_data_vector.maxx = vectormap_info->extent.x1;
    map_data_vector.maxy = vectormap_info->extent.y1;
    for (ii = 0; ii < (vectormap_info->layers_count); ii++)
    {
      memset(&layer, 0, sizeof layer);
      layer.name_count = strlen(vectormap_info->layers[ii].name) + 1;
      assert((layer.name_count) > 0);
      layer.name = reinterpret_cast<char *>(malloc(layer.name_count));
      if (!(layer.name))
      {
        PLAYER_ERROR("cannot allocate space for layer.name");
	delete msg;
	return -1;
      }
      strcpy(layer.name, vectormap_info->layers[ii].name);
      rep = this->vectormap_dev->Request(this->InQueue,
                                         PLAYER_MSGTYPE_REQ,
                                         PLAYER_VECTORMAP_REQ_GET_LAYER_DATA,
                                         reinterpret_cast<void *>(&layer), 0, NULL, true);
      free(layer.name);
      layer.name = NULL;
      if (!rep)
      {
        PLAYER_WARN("failed to acquire layer data");
	delete msg;
        return -1;
      }
      if ((rep->GetDataSize()) < (sizeof(player_vectormap_layer_data_t)))
      {
        PLAYER_WARN2("invalid acqired data size %d vs %d", rep->GetDataSize(), sizeof(player_vectormap_layer_data_t));
	delete rep;
        delete msg;
        return -1;
      }
      layer_data = reinterpret_cast<player_vectormap_layer_data_t *>(rep->GetPayload());
      if (!layer_data)
      {
        PLAYER_WARN("no data acquired");
	delete rep;
        delete msg;
        return -1;
      }
      for (jj = 0; jj < (layer_data->features_count); jj++)
      {
        if (this->skip_feature)
          if ((strlen(this->skip_feature)) && (layer_data->features[jj].name_count > 0))
            if (!strcmp(this->skip_feature, layer_data->features[jj].name)) continue;
	if (!player_wkb_process_wkb(this->wkbProcessor, layer_data->features[jj].wkb, static_cast<size_t>(layer_data->features[jj].wkb_count), reinterpret_cast<playerwkbcallback_t>(Vec2Map::add_segment), reinterpret_cast<void *>(&segments)))
	{
	  PLAYER_ERROR("Error while processing wkb!");
	}
      }
      delete rep;
      rep = NULL;
    }
    vectormap_info = NULL;
    delete msg;
    msg = NULL;
    map_data_vector.segments = NULL;
    map_data_vector.segments_count = 0;
    for (tmp = &segments; tmp->next; tmp = tmp->next) map_data_vector.segments_count++;
    if ((map_data_vector.segments_count) > 0)
    {
      map_data_vector.segments = reinterpret_cast<player_segment_t *>(malloc(map_data_vector.segments_count * sizeof(player_segment_t)));
      if (!(map_data_vector.segments))
      {
        PLAYER_ERROR("cannot allocate space for map_data_vector.segments");
	return -1;
      }
      for (ii = 0, tmp = &segments; (ii < map_data_vector.segments_count) && (tmp->next); ii++, tmp = tmp->next)
      {
        map_data_vector.segments[ii] = tmp->seg;
      }
      assert((ii == (map_data_vector.segments_count)) && (!(tmp->next)));
    }
    if (!(segments.next)) assert(!(segments.last));
    if (!(segments.last)) assert(!(segments.next));
    while (segments.next)
    {
      tmp = segments.next->next;
      free(segments.next);
      segments.next = tmp;
    }
    segments.last = NULL;
    this->Publish(this->map_addr,
                  resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_MAP_REQ_GET_VECTOR,
                  reinterpret_cast<void *>(&map_data_vector));
    if (map_data_vector.segments) free(map_data_vector.segments);
    map_data_vector.segments = NULL;
    return 0;
  }
  return -1;
}

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver * Vec2Map_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return (Driver *)(new Vec2Map(cf, section));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void vec2map_Register(DriverTable * table)
{
  table->AddDriver("vec2map", Vec2Map_Init);
}
