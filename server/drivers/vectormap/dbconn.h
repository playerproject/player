/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2007
 *     Ben Morelli
 *
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
 */

#ifndef __DBCONN_H_
#define __DBCONN_H_

#include <libpq-fe.h>
#include <libplayercore/playercore.h>
#include <libplayerwkb/playerwkb.h>
#include <vector>
#include <string>
#include <cstring>
#include <cstddef>

#define MAX_PSQL_STRING 256

using namespace std;

typedef struct
{
  double x0, y0, x1, y1;
} BoundingBox;

#define BBOX(ptr) (reinterpret_cast<BoundingBox *>(ptr))

class FeatureDataHolder
{
  public:
    FeatureDataHolder() { memset(&feature_data, 0, sizeof feature_data); }
    FeatureDataHolder(const FeatureDataHolder & orig)
    {
      memset(&feature_data, 0, sizeof feature_data);
      name = orig.name;
      wkb = orig.wkb;
      attrib = orig.attrib;
    }
    virtual ~FeatureDataHolder();
    FeatureDataHolder(string name)
    {
      memset(&feature_data, 0, sizeof feature_data);
      this->name = name;
    }
    FeatureDataHolder(const player_vectormap_feature_data_t * feature)
    {
      memset(&feature_data, 0, sizeof feature_data);
      name = string(feature->name);
      attrib = string(feature->attrib);
      wkb.assign(feature->wkb, (feature->wkb) + (feature->wkb_count));
    }

    const player_vectormap_feature_data_t* Convert();

    string name;
    vector<uint8_t> wkb;
    string attrib;
    player_vectormap_feature_data_t feature_data;
};

class LayerInfoHolder
{
  public:
    LayerInfoHolder() { memset(&layer_info,0,sizeof(layer_info)); memset(&extent, 0, sizeof(extent)); };
    LayerInfoHolder(const LayerInfoHolder & orig)
    {
      memset(&layer_info,0,sizeof(layer_info));
      name = orig.name;
      extent = orig.extent;
    }
    LayerInfoHolder(string name)
    {
      memset(&layer_info,0,sizeof(layer_info));
      this->name = name;
      memset(&extent, 0, sizeof(extent));
    };
    virtual ~LayerInfoHolder()
    {
      free(layer_info.name);
    }

    const player_vectormap_layer_info_t* Convert();

    string name;
    BoundingBox extent;
    player_vectormap_layer_info_t layer_info;
};

class LayerDataHolder
{
  public:
    LayerDataHolder() { memset(&layer_data, 0, sizeof layer_data); }
    LayerDataHolder(const LayerDataHolder & orig)
    {
      memset(&layer_data, 0, sizeof layer_data);
      name = orig.name;
      features = orig.features;
    }
    LayerDataHolder(string name)
    {
      memset(&layer_data,0,sizeof(layer_data));
      this->name = name;
    }
    LayerDataHolder(const player_vectormap_layer_data_t * layer)
    {
      memset(&layer_data, 0, sizeof layer_data);
      name = string(layer->name);
      for (uint32_t ii = 0; ii < layer->features_count; ii++)
      {
        FeatureDataHolder fd(&(layer->features[ii]));
        features.push_back(fd);
      }
    }
    virtual ~LayerDataHolder();

    const player_vectormap_layer_data_t* Convert();

    vector<FeatureDataHolder> features;
    player_vectormap_layer_data_t layer_data;
    string name;
};

class VectorMapInfoHolder
{
  public:
    VectorMapInfoHolder() { memset(&info, 0, sizeof info); memset(&extent, 0, sizeof extent); };
    VectorMapInfoHolder(const VectorMapInfoHolder & orig)
    {
      memset(&info, 0, sizeof info);
      srid = orig.srid; layers = orig.layers; extent = orig.extent;
    }
    virtual ~VectorMapInfoHolder();
    VectorMapInfoHolder(uint32_t srid, BoundingBox extent)
    {
	this->srid = srid; this->extent = extent;
	memset(&info, 0, sizeof info);
    };

    const player_vectormap_info_t* Convert();

    uint32_t srid;
    vector<LayerInfoHolder> layers;
    BoundingBox extent;
    player_vectormap_info_t info;
};

class PostgresConn
{
  public:
    PostgresConn(int debug = 0){ this->wkbprocessor = player_wkb_create_processor(); this->conn = NULL; this->debug = debug; };
    virtual ~PostgresConn(){ if (Connected()) Disconnect(); player_wkb_destroy_processor(this->wkbprocessor); };
    bool Connect(const char* dbname, const char* host, const char* user, const char* password, const char* port);
    bool Disconnect();
    bool Connected() { return (conn != NULL) && (PQstatus(conn) != CONNECTION_BAD); };

    VectorMapInfoHolder GetVectorMapInfo(vector<string> layerNames);
    LayerInfoHolder GetLayerInfo(const char *layer_name);
    LayerDataHolder GetLayerData(const char *layer_name);
    int WriteLayerData(LayerDataHolder & data);

  private:
    BoundingBox BinaryToBBox(const uint8_t *binary, uint32_t length);
    uint32_t Text2Bin(const char * text, unsigned char * bin, uint32_t maxlen);
    playerwkbprocessor_t wkbprocessor;
    PGconn *conn;
    int debug;
    static void bbcb(void * bbox, double x0, double y0, double x1, double y1);
};

#endif /* __DBCONN_H_ */
