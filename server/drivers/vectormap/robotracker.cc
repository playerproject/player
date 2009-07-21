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
 *
 * Robot tracker that updates vectormap layer with shapes of given robots.
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_robotracker robotracker
 * @brief Robot tracker that updates vectormap layer with shapes of given robots.

@par Provides

- @ref interface_opaque

@par Requires

- @ref interface_vectormap
- @ref interface_position2d

@par Configuration requests

- None

@par Configuration file options

- names (string tuple)
  - non-empty list of robot names (vectormap layer objects)

- shape_x (float tuple)
  - Default: [ -0.01 0.01 ]
  - shape of the robot (x'es of points of linestring)
  - all robots are of the same shape

- shape_y (float tuple)
  - Default: [ -0.01 0.01 ]
  - shape of the robot (y's of points of linestring)
  - all robots are of the same shape

- interval (float)
  - Default: -0.01 (=NONE)
  - mimimal interval between map updates

- min_x_change (float)
  - Default: -0.01 (=NONE)
  - minimum change on X-axis to assume robot has moved

- min_y_change (float)
  - Default: -0.01 (=NONE)
  - minimum change on Y-axis to assume robot has moved

- layer_name (string)
  - Default: "NONE"
  - vectormap layer name to be updated

- workspaces_name (string)
  - Default: "NONE"
  - if not set to "NONE" this is the name of vectormap layer that will be
    filled-up with robots extents

- depletion_zone (float)
  - Default: 0.0
  - length of additional depletion zone in robot workspace

- first2last_extent_name (string)
  - Default: "NONE"
  - if not set to "NONE" this is the name of additional vectormap object
    that will be added to workspaces layer to denote extent of all robots and
    their workspaces

@par Example

@verbatim
driver
(
  name "robotracker"
  names ["roomba0" "roomba1" "roomba2" "roomba3" "roomba4"]
  requires ["vectormap:0" "roomba0:::position2d:1" "roomba1::6666:position2d:1" "roomba2::6667:position2d:1" "roomba3::6668:position2d:1" "roomba4::6669:position2d:1"]
  provides ["opaque:0"]
  shape_x [ 0.225 0.208 0.159 0.086 0.000 -0.086 -0.159 -0.208 -0.225 -0.208 -0.159 -0.086 -0.000  0.086  0.159  0.208 0.225 ]
  shape_y [ 0.000 0.086 0.159 0.208 0.225  0.208  0.159  0.086  0.000 -0.086 -0.159 -0.208 -0.225 -0.208 -0.159 -0.086 0.000 ]
  min_x_change 0.1
  min_y_change 0.1
  layer_name "obstacles"
  workspaces_name "workspaces"
  first2last_extent_name "formation_extent"
  depletion_zone 0.3
  alwayson 1
)
@endverbatim

@author Paul Osmialowski

*/

/** @} */

#include <libplayercore/playercore.h>
#include <libplayerwkb/playerwkb.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#if !defined (WIN32)
  #include <strings.h> /* strcasecmp() */
#endif

#define MAX_BOTS 32
#define MAX_SHAPE_POINTS 64

#define EPS 0.00000000000001

class RoboTracker : Driver
{
public:
  RoboTracker(ConfigFile * cf, int section);
  virtual ~RoboTracker();
  virtual int Setup();
  virtual int Shutdown();
  virtual int ProcessMessage(QueuePointer & resp_queue,
                             player_msghdr * hdr,
                             void * data);
private:
  playerwkbprocessor_t wkbProcessor;
  player_devaddr_t opaque_addr;
  player_devaddr_t position_addrs[MAX_BOTS];
  player_devaddr_t vectormap_addr;
  bool pos_data_valid[MAX_BOTS];
  bool prev_pos_data_valid[MAX_BOTS];
  player_position2d_data_t pos_data[MAX_BOTS];
  player_position2d_data_t prev_pos_data[MAX_BOTS];
  Device * pos_dev[MAX_BOTS];
  const char * names[MAX_BOTS];
  const char * layer_name;
  const char * workspaces_name;
  const char * first2last_extent_name;
  Device * vectormap_dev;
  int shape_points;
  double shape[MAX_SHAPE_POINTS][2];
  double extent[5][2];
  double depletion_zone;
  int position_devices;
  double min_x_change;
  double min_y_change;
  double interval;
  double last_update;
  double minx;
  double miny;
  double maxx;
  double maxy;
  enum { idle = 0, get_layer_data = 1, write_obstacles = 2, write_workspaces = 3 } state;
};

RoboTracker::RoboTracker(ConfigFile * cf, int section)
    : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  int i;

  this->wkbProcessor = player_wkb_create_processor(); // one per driver instance
  memset(&(this->opaque_addr), 0, sizeof(player_devaddr_t));
  memset(this->position_addrs, 0, sizeof this->position_addrs);
  memset(&(this->vectormap_addr), 0, sizeof(player_devaddr_t));
  memset(this->pos_data_valid, 0, sizeof this->pos_data_valid);
  memset(this->prev_pos_data_valid, 0, sizeof this->prev_pos_data_valid);
  memset(this->pos_data, 0, sizeof this->pos_data);
  memset(this->prev_pos_data, 0, sizeof this->prev_pos_data);
  memset(this->pos_dev, 0, sizeof this->pos_dev);
  memset(this->names, 0, sizeof this->names);
  this->layer_name = NULL;
  this->workspaces_name = NULL;
  first2last_extent_name = NULL;
  this->vectormap_dev = NULL;
  this->shape_points = 0;
  memset(this->shape, 0, sizeof this->shape);
  memset(this->extent, 0, sizeof this->extent);
  this->depletion_zone = 0.0;
  this->position_devices = 0;
  this->min_x_change = -1.0;
  this->min_y_change = -1.0;
  this->interval = -1.0;
  this->last_update = 0.0;
  this->minx = 0.0;
  this->miny = 0.0;
  this->maxx = 0.0;
  this->maxy = 0.0;
  this->state = idle;
  if (cf->ReadDeviceAddr(&(this->opaque_addr), section, "provides", PLAYER_OPAQUE_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->opaque_addr))
  {
    this->SetError(-1);
    return;  
  }
  this->layer_name = cf->ReadString(section, "layer_name", "NONE");
  if (!(this->layer_name))
  {
    this->SetError(-1);
    return;
  }
  if (!(strcmp(this->layer_name, "NONE")))
  {
    PLAYER_ERROR("layer_name not given");
    this->SetError(-1);
    return;
  }
  this->workspaces_name = cf->ReadString(section, "workspaces_name", "NONE");
  if (!(this->workspaces_name))
  {
    this->SetError(-1);
    return;
  }
  this->first2last_extent_name = cf->ReadString(section, "first2last_extent_name", "NONE");
  if (!(this->first2last_extent_name))
  {
    this->SetError(-1);
    return;
  }  
  this->position_devices = cf->GetTupleCount(section, "names");
  if (((this->position_devices) <= 0) || ((this->position_devices) > MAX_BOTS))
  {
    PLAYER_ERROR("invalid number of position devices");
    this->SetError(-1);
    return;
  }
  for (i = 0; i < (this->position_devices); i++)
  {
    pos_dev[i] = NULL;
    pos_data_valid[i] = false;
    prev_pos_data_valid[i] = false;
    this->names[i] = cf->ReadTupleString(section, "names", i, "NONE");
    if (!(this->names[i]))
    {
      this->SetError(-1);
      return;
    }
    if (!(strlen(this->names[i]) > 0))
    {
      PLAYER_ERROR1("empty names not allowed %d", i);
      this->SetError(-1);
      return;
    }
    if (!(strcmp(this->names[i], "NONE")))
    {
      PLAYER_ERROR1("name %d not given", i);
      this->SetError(-1);
      return;
    }
    if (cf->ReadDeviceAddr(&(this->position_addrs[i]), section, "requires", PLAYER_POSITION2D_CODE, -1, this->names[i]))
    {
      this->SetError(-1);
      return;
    }
  }
  if (cf->ReadDeviceAddr(&(this->vectormap_addr), section, "requires", PLAYER_VECTORMAP_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  this->shape_points = cf->GetTupleCount(section, "shape_x");
  if ((cf->GetTupleCount(section, "shape_y")) != (this->shape_points))
  {
    PLAYER_ERROR("size of shape_x and shape_y sets should be equal");
    this->SetError(-1);
    return;
  }
  if (!((this->shape_points) > 0))
  {
    this->minx = -0.01;
    this->miny = -0.01;
    this->maxx = 0.01;
    this->maxy = 0.01;
    this->shape_points = 2;
    this->shape[0][0] = minx;
    this->shape[0][1] = miny;
    this->shape[1][0] = maxx;
    this->shape[1][1] = maxy;
  } else
  {
    if ((this->shape_points) > MAX_SHAPE_POINTS)
    {
      PLAYER_ERROR("invalid size of shape_x set");
      this->SetError(-1);
      return;
    }
    for (i = 0; i < (this->shape_points); i++)
    {
      this->shape[i][0] = cf->ReadTupleFloat(section, "shape_x", i, 0.0);
      this->shape[i][1] = cf->ReadTupleFloat(section, "shape_y", i, 0.0);
      if (!i)
      {
        this->minx = this->shape[i][0];
        this->miny = this->shape[i][1];
        this->maxx = this->shape[i][0];
        this->maxy = this->shape[i][1];
      } else
      {
        if (this->shape[i][0] < this->minx) this->minx = this->shape[i][0];
        if (this->shape[i][1] < this->miny) this->miny = this->shape[i][1];
        if (this->shape[i][0] > this->maxx) this->maxx = this->shape[i][0];
        if (this->shape[i][1] > this->maxy) this->maxy = this->shape[i][1];
      }
    }
  }
  this->depletion_zone = cf->ReadFloat(section, "depletion_zone", 0.0);
  this->minx -= this->depletion_zone;
  this->miny -= this->depletion_zone;
  this->maxx += this->depletion_zone;
  this->maxy += this->depletion_zone;
  this->extent[0][0] = this->minx;
  this->extent[0][1] = this->miny;
  this->extent[1][0] = this->maxx;
  this->extent[1][1] = this->miny;
  this->extent[2][0] = this->maxx;
  this->extent[2][1] = this->maxy;
  this->extent[3][0] = this->minx;
  this->extent[3][1] = this->maxy;
  this->extent[4][0] = this->minx;
  this->extent[4][1] = this->miny;
  this->interval = cf->ReadFloat(section, "interval", -1.0);
  this->min_x_change = cf->ReadFloat(section, "min_x_change", -1.0);
  this->min_y_change = cf->ReadFloat(section, "min_y_change", -1.0);
}

RoboTracker::~RoboTracker()
{
  player_wkb_destroy_processor(this->wkbProcessor);
}

int RoboTracker::Setup()
{
  int i, j;

  memset(this->pos_data, 0, sizeof this->pos_data);
  memset(this->prev_pos_data, 0, sizeof this->prev_pos_data);
  for (i = 0; i < (this->position_devices); i++)
  {
    pos_data_valid[i] = false;
    prev_pos_data_valid[i] = false;
  }
  this->last_update = 0.0;
  this->state = idle;
  this->vectormap_dev = deviceTable->GetDevice(this->vectormap_addr);
  if (!(this->vectormap_dev))
  {
    PLAYER_ERROR("unable to locate suitable vectormap device");
    return -1;
  }
  if (this->vectormap_dev->Subscribe(this->InQueue))
  {
    PLAYER_ERROR("unable to subscribe to vectormap device");
    this->vectormap_dev = NULL;
    return -1;
  }
  for (i = 0; i < (this->position_devices); i++)
  {
    this->pos_dev[i] = deviceTable->GetDevice(this->position_addrs[i]);
    if (!(this->pos_dev[i]))
    {
      PLAYER_ERROR1("unable to locate suitable position2d device %d", i);
      for (j = 0; j < i; j++)
      {
        if (this->pos_dev[j]) this->pos_dev[j]->Unsubscribe(this->InQueue);
        this->pos_dev[j] = NULL;
      }
      if (this->vectormap_dev) this->vectormap_dev->Unsubscribe(this->InQueue);
      this->vectormap_dev = NULL;
      return -1;
    }
    if (this->pos_dev[i]->Subscribe(this->InQueue))
    {
      PLAYER_ERROR1("unable to subscribe to position2d device %d", i);
      this->pos_dev[i] = NULL;
      for (j = 0; j < i; j++)
      {
        if (this->pos_dev[j]) this->pos_dev[j]->Unsubscribe(this->InQueue);
        this->pos_dev[j] = NULL;
      }
      if (this->vectormap_dev) this->vectormap_dev->Unsubscribe(this->InQueue);
      this->vectormap_dev = NULL;
      return -1;
    }
  }
  return 0;
}

int RoboTracker::Shutdown()
{
  int i;

  for (i = 0; i < (this->position_devices); i++)
  {
    if (this->pos_dev[i]) this->pos_dev[i]->Unsubscribe(this->InQueue);
    this->pos_dev[i] = NULL;
  }
  if (this->vectormap_dev) this->vectormap_dev->Unsubscribe(this->InQueue);
  this->vectormap_dev = NULL;
  return 0;
}

int RoboTracker::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  int i, j;
  double d;
  player_vectormap_layer_data_t layer;
  player_vectormap_layer_data_t * layer_data;
  player_vectormap_layer_data_t * new_layer_data;
  int to_add;
  int added;
  size_t wkb_size;
  double eminx = 0.0;
  double eminy = 0.0;
  double emaxx = 0.0;
  double emaxy = 0.0;
  double first2last[5][2];
  player_opaque_data_t opdata;

  assert(hdr);
  for (i = 0; i < (this->position_devices); i++)
  {
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_NACK, -1, this->vectormap_addr))
    {
      assert((this->state) != idle);
      PLAYER_ERROR("request not accepted by vectormap device");
      this->state = idle;
      return 0;
    }
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, -1, this->position_addrs[i]))
    {
      assert(data);
      if ((this->state) != idle) return 0;
      this->pos_data[i] = *(reinterpret_cast<player_position2d_data_t *>(data));
      this->pos_data_valid[i] = true;
      for (i = 0; i < (this->position_devices); i++) if (!(this->pos_data_valid[i])) return 0;
      for (i = 0; i < (this->position_devices); i++)
      {
        if (!(this->prev_pos_data_valid[i])) break;
        if ((this->min_x_change) > EPS)
        {
          if (fabs(this->pos_data[i].pos.px - this->prev_pos_data[i].pos.px) >= (this->min_x_change)) break;
        } else break;
        if ((this->min_y_change) > EPS)
        {
          if (fabs(this->pos_data[i].pos.py - this->prev_pos_data[i].pos.py) >= (this->min_y_change)) break;
        } else break;
      }
      if (!(i < (this->position_devices))) return 0;
      if ((this->interval) > EPS)
      {
        GlobalTime->GetTimeDouble(&d);
        if ((d - (this->last_update)) >= (this->interval)) this->last_update = d;
        else return 0;
      }
      memset(&layer, 0, sizeof layer);
      assert(this->layer_name);
      layer.name = strdup(this->layer_name);
      if (!(layer.name))
      {
        PLAYER_ERROR("out of memory");
        return -1;
      }
      layer.name_count = strlen(layer.name) + 1;
      this->vectormap_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_REQ, PLAYER_VECTORMAP_REQ_GET_LAYER_DATA, reinterpret_cast<void *>(&layer), 0, NULL);
      assert(layer.name);
      free(layer.name);
      this->state = get_layer_data;
      return 0;
    }
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_ACK, -1, this->vectormap_addr))
    {
      switch (this->state)
      {
      case idle:
        assert(!"invalid state");
        break;
      case get_layer_data:
        assert(data);
        assert((hdr->subtype) == PLAYER_VECTORMAP_REQ_GET_LAYER_DATA);
        layer_data = reinterpret_cast<player_vectormap_layer_data_t *>(data);
        if (!(layer_data->name))
        {
          PLAYER_ERROR("internal error: no layer name");
          this->state = idle;
          return -1;
        }
        if (strcmp(this->layer_name, layer_data->name))
        {
          PLAYER_ERROR("internal error: wrong layer name");
          this->state = idle;
          return -1;
        }
        if ((layer_data->features_count > 0) && (!(layer_data->features)))
        {
          PLAYER_ERROR("internal error: NULL features");
          this->state = idle;
          return -1;
        }
        to_add = this->position_devices;
        for (j = 0; j < static_cast<int>(layer_data->features_count); j++)
        {
          assert(layer_data->features[j].name);
          for (i = 0; i < (this->position_devices); i++)
          {
#if defined (WIN32)
            if (!_stricmp(layer_data->features[j].name, this->names[i]))
#else
            if (!strcasecmp(layer_data->features[j].name, this->names[i]))
#endif
            {
              to_add--;
              break;
            }
          }
        }
        if (to_add < 0)
        {
          PLAYER_ERROR("invalid number of the same names");
          this->state = idle;
          return -1;
        }
        new_layer_data = reinterpret_cast<player_vectormap_layer_data_t *>(malloc(sizeof(player_vectormap_layer_data_t)));
        if (!new_layer_data)
        {
          PLAYER_ERROR("out of memory");
          this->state = idle;
          return -1;
        }
        memset(new_layer_data, 0, sizeof(player_vectormap_layer_data_t));
        new_layer_data->name = strdup(this->layer_name);
        if (!(new_layer_data->name))
        {
          PLAYER_ERROR("out of memory");
          free(new_layer_data);
          this->state = idle;
          return -1;
        }
        new_layer_data->name_count = strlen(new_layer_data->name) + 1;
        new_layer_data->features_count = layer_data->features_count + to_add;
        assert(new_layer_data->features_count > 0);
        new_layer_data->features = reinterpret_cast<player_vectormap_feature_data_t *>(malloc(sizeof(player_vectormap_feature_data_t) * (new_layer_data->features_count)));
        if (!(new_layer_data->features))
        {
          PLAYER_ERROR("out of memory");
          free(new_layer_data->name);
          free(new_layer_data);
          this->state = idle;
          return -1;
        }
        memset(new_layer_data->features, 0, sizeof(player_vectormap_feature_data_t) * (new_layer_data->features_count));
        added = 0;
        for (j = 0; j < static_cast<int>(layer_data->features_count); j++)
        {
          assert(layer_data->features[j].name);
          new_layer_data->features[j].name = strdup(layer_data->features[j].name);
          assert(new_layer_data->features[j].name);
          new_layer_data->features[j].name_count = strlen(new_layer_data->features[j].name) + 1;
          if (layer_data->features[j].attrib)
          {
            new_layer_data->features[j].attrib = strdup(layer_data->features[j].attrib);
            assert(new_layer_data->features[j].attrib);
            new_layer_data->features[j].attrib_count = strlen(new_layer_data->features[j].attrib) + 1;
          } else
          {
            new_layer_data->features[j].attrib = reinterpret_cast<char *>(malloc(sizeof(char) * 1));
            assert(new_layer_data->features[j].attrib);
            new_layer_data->features[j].attrib[0] = '\0';
            new_layer_data->features[j].attrib_count = 1;
          }
#if defined (WIN32)
          for (i = 0; i < (this->position_devices); i++) if (!_stricmp(layer_data->features[j].name, this->names[i])) break;
#else
          for (i = 0; i < (this->position_devices); i++) if (!strcasecmp(layer_data->features[j].name, this->names[i])) break;
#endif
          if (i < (this->position_devices))
          {
            new_layer_data->features[j].wkb_count = player_wkb_create_linestring(this->wkbProcessor, this->shape, this->shape_points, this->pos_data[i].pos.px, this->pos_data[i].pos.py, NULL, 0);
            assert(new_layer_data->features[j].wkb_count > 0);
            new_layer_data->features[j].wkb = reinterpret_cast<uint8_t *>(malloc(sizeof(uint8_t) * new_layer_data->features[j].wkb_count));
            assert(new_layer_data->features[j].wkb);
            wkb_size = player_wkb_create_linestring(this->wkbProcessor, this->shape, this->shape_points, this->pos_data[i].pos.px, this->pos_data[i].pos.py, new_layer_data->features[j].wkb, new_layer_data->features[j].wkb_count);
            assert(wkb_size == static_cast<size_t>(new_layer_data->features[j].wkb_count));
            this->prev_pos_data[i] = this->pos_data[i];
            this->pos_data_valid[i] = false;
            this->prev_pos_data_valid[i] = true;
            added++;
          } else
          {
            assert(layer_data->features[j].wkb);
            assert(layer_data->features[j].wkb_count > 0);
            new_layer_data->features[j].wkb_count = layer_data->features[j].wkb_count;
            new_layer_data->features[j].wkb = reinterpret_cast<uint8_t *>(malloc(sizeof(uint8_t) * new_layer_data->features[j].wkb_count));
            assert(new_layer_data->features[j].wkb);
            memcpy(new_layer_data->features[j].wkb, layer_data->features[j].wkb, new_layer_data->features[j].wkb_count);
          }
        }
        layer_data = NULL;
        assert(added == ((this->position_devices) - to_add));
        added = 0;
        for (i = 0; i < (this->position_devices); i++)
        {
          if (this->pos_data_valid[i])
          {
            assert(j < static_cast<int>(new_layer_data->features_count));
            new_layer_data->features[j].name = strdup(this->names[i]);
            assert(new_layer_data->features[j].name);
            new_layer_data->features[j].name_count = strlen(new_layer_data->features[j].name) + 1;
            new_layer_data->features[j].attrib = reinterpret_cast<char *>(malloc(sizeof(char) * 1));
            assert(new_layer_data->features[j].attrib);
            new_layer_data->features[j].attrib[0] = '\0';
            new_layer_data->features[j].attrib_count = 1;
            new_layer_data->features[j].wkb_count = player_wkb_create_linestring(this->wkbProcessor, this->shape, this->shape_points, this->pos_data[i].pos.px, this->pos_data[i].pos.py, NULL, 0);
            assert(new_layer_data->features[j].wkb_count > 0);
            new_layer_data->features[j].wkb = reinterpret_cast<uint8_t *>(malloc(sizeof(uint8_t) * new_layer_data->features[j].wkb_count));
            assert(new_layer_data->features[j].wkb);
            wkb_size = player_wkb_create_linestring(this->wkbProcessor, this->shape, this->shape_points, this->pos_data[i].pos.px, this->pos_data[i].pos.py, new_layer_data->features[j].wkb, new_layer_data->features[j].wkb_count);
            assert(wkb_size == static_cast<size_t>(new_layer_data->features[j].wkb_count));
            this->prev_pos_data[i] = this->pos_data[i];
            this->pos_data_valid[i] = false;
            this->prev_pos_data_valid[i] = true;
            added++; j++;
          }
        }
        assert(added == to_add);
        this->vectormap_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_REQ, PLAYER_VECTORMAP_REQ_WRITE_LAYER, reinterpret_cast<void *>(new_layer_data), 0, NULL);
        assert(new_layer_data->name);
        free(new_layer_data->name);
        assert(new_layer_data->features);
        assert(new_layer_data->features_count > 0);
        for (j = 0; j < static_cast<int>(new_layer_data->features_count); j++)
        {
          assert(new_layer_data->features[j].name);
          free(new_layer_data->features[j].name);
          assert(new_layer_data->features[j].attrib);
          free(new_layer_data->features[j].attrib);
          assert(new_layer_data->features[j].wkb);
          free(new_layer_data->features[j].wkb);
        }
        free(new_layer_data->features);
        free(new_layer_data);
        new_layer_data = NULL;
        this->state = write_obstacles;
        break;
      case write_obstacles:
        assert((hdr->subtype) == PLAYER_VECTORMAP_REQ_WRITE_LAYER);
        assert(this->workspaces_name);
        if (!(strcmp(this->workspaces_name, "NONE")))
        {
          memset(&opdata, 0, sizeof opdata);
          opdata.data_count = 0;
          opdata.data = NULL;        
          this->Publish(this->opaque_addr,
                        PLAYER_MSGTYPE_DATA, PLAYER_OPAQUE_DATA_STATE,
                        reinterpret_cast<void *>(&opdata), 0, NULL, true); // copy = true, do not dispose data placed on local stack!
          this->state = idle;
          break;
        }
        new_layer_data = reinterpret_cast<player_vectormap_layer_data_t *>(malloc(sizeof(player_vectormap_layer_data_t)));
        if (!new_layer_data)
        {
          PLAYER_ERROR("out of memory");
          this->state = idle;
          return -1;
        }
        memset(new_layer_data, 0, sizeof(player_vectormap_layer_data_t));
        new_layer_data->name = strdup(this->workspaces_name);
        if (!(new_layer_data->name))
        {
          PLAYER_ERROR("out of memory");
          free(new_layer_data);
          this->state = idle;
          return -1;
        }
        new_layer_data->name_count = strlen(new_layer_data->name) + 1;
        new_layer_data->features_count = this->position_devices;
        assert(this->first2last_extent_name);
        if (strcmp(this->first2last_extent_name, "NONE")) (new_layer_data->features_count)++;
        assert(new_layer_data->features_count > 0);
        new_layer_data->features = reinterpret_cast<player_vectormap_feature_data_t *>(malloc(sizeof(player_vectormap_feature_data_t) * (new_layer_data->features_count)));
        if (!(new_layer_data->features))
        {
          PLAYER_ERROR("out of memory");
          free(new_layer_data->name);
          free(new_layer_data);
          this->state = idle;
          return -1;
        }
        memset(new_layer_data->features, 0, sizeof(player_vectormap_feature_data_t) * (new_layer_data->features_count));
        for (i = 0; i < (this->position_devices); i++)
        {
          assert(this->prev_pos_data_valid[i]);
          assert(i < static_cast<int>(new_layer_data->features_count));
          if (!i)
          {
            eminx = this->prev_pos_data[i].pos.px;
            eminy = this->prev_pos_data[i].pos.py;
            emaxx = this->prev_pos_data[i].pos.px;
            emaxy = this->prev_pos_data[i].pos.py;
          } else
          {
            if (this->prev_pos_data[i].pos.px < eminx) eminx = this->prev_pos_data[i].pos.px;
            if (this->prev_pos_data[i].pos.py < eminy) eminy = this->prev_pos_data[i].pos.py;
            if (this->prev_pos_data[i].pos.px > emaxx) emaxx = this->prev_pos_data[i].pos.px;
            if (this->prev_pos_data[i].pos.py > emaxy) emaxy = this->prev_pos_data[i].pos.py;
          }
          new_layer_data->features[i].name = strdup(this->names[i]);
          assert(new_layer_data->features[i].name);
          new_layer_data->features[i].name_count = strlen(new_layer_data->features[i].name) + 1;
          new_layer_data->features[i].attrib = reinterpret_cast<char *>(malloc(sizeof(char) * 1));
          assert(new_layer_data->features[i].attrib);
          new_layer_data->features[i].attrib[0] = '\0';
          new_layer_data->features[i].attrib_count = 1;
          new_layer_data->features[i].wkb_count = player_wkb_create_linestring(this->wkbProcessor, this->extent, 5, this->prev_pos_data[i].pos.px, this->prev_pos_data[i].pos.py, NULL, 0);
          assert(new_layer_data->features[i].wkb_count > 0);
          new_layer_data->features[i].wkb = reinterpret_cast<uint8_t *>(malloc(sizeof(uint8_t) * new_layer_data->features[i].wkb_count));
          assert(new_layer_data->features[i].wkb);
          wkb_size = player_wkb_create_linestring(this->wkbProcessor, this->extent, 5, this->pos_data[i].pos.px, this->pos_data[i].pos.py, new_layer_data->features[i].wkb, new_layer_data->features[i].wkb_count);
          assert(wkb_size == static_cast<size_t>(new_layer_data->features[i].wkb_count));
        }
        eminx += this->minx;
        eminy += this->miny;
        emaxx += this->maxx;
        emaxy += this->maxy;
        first2last[0][0] = eminx;
        first2last[0][1] = eminy;
        first2last[1][0] = emaxx;
        first2last[1][1] = eminy;
        first2last[2][0] = emaxx;
        first2last[2][1] = emaxy;
        first2last[3][0] = eminx;
        first2last[3][1] = emaxy;
        first2last[4][0] = eminx;
        first2last[4][1] = eminy;      
        assert(this->first2last_extent_name);
        if (strcmp(this->first2last_extent_name, "NONE"))
        {
          assert(i < static_cast<int>(new_layer_data->features_count));
          new_layer_data->features[i].name = strdup(this->first2last_extent_name);
          assert(new_layer_data->features[i].name);
          new_layer_data->features[i].name_count = strlen(new_layer_data->features[i].name) + 1;
          new_layer_data->features[i].attrib = reinterpret_cast<char *>(malloc(sizeof(char) * 1));
          assert(new_layer_data->features[i].attrib);
          new_layer_data->features[i].attrib[0] = '\0';
          new_layer_data->features[i].attrib_count = 1;
          new_layer_data->features[i].wkb_count = player_wkb_create_linestring(this->wkbProcessor, first2last, 5, 0.0, 0.0, NULL, 0);
          assert(new_layer_data->features[i].wkb_count > 0);
          new_layer_data->features[i].wkb = reinterpret_cast<uint8_t *>(malloc(sizeof(uint8_t) * new_layer_data->features[i].wkb_count));
          assert(new_layer_data->features[i].wkb);
          wkb_size = player_wkb_create_linestring(this->wkbProcessor, first2last, 5, 0.0, 0.0, new_layer_data->features[i].wkb, new_layer_data->features[i].wkb_count);
          assert(wkb_size == static_cast<size_t>(new_layer_data->features[i].wkb_count));
        }
        this->vectormap_dev->PutMsg(this->InQueue, PLAYER_MSGTYPE_REQ, PLAYER_VECTORMAP_REQ_WRITE_LAYER, reinterpret_cast<void *>(new_layer_data), 0, NULL);
        assert(new_layer_data->name);
        free(new_layer_data->name);
        assert(new_layer_data->features);
        assert(new_layer_data->features_count > 0);
        for (j = 0; j < static_cast<int>(new_layer_data->features_count); j++)
        {
          assert(new_layer_data->features[j].name);
          free(new_layer_data->features[j].name);
          assert(new_layer_data->features[j].attrib);
          free(new_layer_data->features[j].attrib);
          assert(new_layer_data->features[j].wkb);
          free(new_layer_data->features[j].wkb);
        }
        free(new_layer_data->features);
        free(new_layer_data);
        new_layer_data = NULL;
        this->state = write_workspaces;
        break;
      case write_workspaces:
        assert((hdr->subtype) == PLAYER_VECTORMAP_REQ_WRITE_LAYER);
        memset(&opdata, 0, sizeof opdata);
        opdata.data_count = 0;
        opdata.data = NULL;        
        this->Publish(this->opaque_addr,
                      PLAYER_MSGTYPE_DATA, PLAYER_OPAQUE_DATA_STATE,
                      reinterpret_cast<void *>(&opdata), 0, NULL, true); // copy = true, do not dispose data placed on local stack!
        this->state = idle;
        break;
      default:
        assert(!"unknown state");
      }
      return 0;
    }
  }
  return -1;
}

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver * RoboTracker_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new RoboTracker(cf, section));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void robotracker_Register(DriverTable * table)
{
  table->AddDriver("robotracker", RoboTracker_Init);
}
