#ifndef __DBCONN_H_
#define __DBCONN_H_

#include <libpq-fe.h>
#include <libplayercore/playercore.h>
#include <libplayercore/error.h>
#include <vector>
#include <string>

#define MAX_PSQL_STRING 256

using namespace std;

typedef struct
{
  double x0, y0, x1, y1;
}BoundingBox;

class FeatureDataHolder
{
  public:
    FeatureDataHolder(){};
    ~FeatureDataHolder();
    FeatureDataHolder(string name)
    { 
      this->name = name;
    };

    const player_vectormap_feature_data_t* Convert();

    string name;
    vector<uint8_t> wkb;
    string attrib;
    player_vectormap_feature_data_t feature_data;
};

class LayerInfoHolder
{
  public:
    LayerInfoHolder(){memset(&layer_info,0,sizeof(layer_info));};
    LayerInfoHolder(string name)
    {
      this->name = name;
      layer_info.name = strdup(name.c_str());
      layer_info.name_count = name.size() +1;
      memset(&extent, 0, sizeof(extent));
    };
    ~LayerInfoHolder()
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
    LayerDataHolder(){memset(&layer_data,0,sizeof(layer_data));};
    LayerDataHolder(string name)
    {
      memset(&layer_data,0,sizeof(layer_data));
      this->name = name;
      layer_data.name = strdup(name.c_str());
      layer_data.name_count = name.size() +1;
    };
    ~LayerDataHolder();

    const player_vectormap_layer_data_t* Convert();

    vector<FeatureDataHolder> features;
    player_vectormap_layer_data_t layer_data;
    string name;
};

class VectorMapInfoHolder
{
  public:
    VectorMapInfoHolder(){};
    ~VectorMapInfoHolder();
    VectorMapInfoHolder(uint32_t srid, BoundingBox extent) { this->srid = srid; this->extent = extent; };

    const player_vectormap_info_t* Convert();

    uint32_t srid;
    vector<LayerInfoHolder> layers;
    BoundingBox extent;
    player_vectormap_info_t info;
};

class PostgresConn
{
  public:
    PostgresConn(){ conn = NULL; };
    ~PostgresConn(){ if (Connected()) Disconnect(); };
    bool Connect(const char* dbname, const char* host, const char* user, const char* password, const char* port);
    bool Disconnect();
    bool Connected() { return (conn != NULL) && (PQstatus(conn) != CONNECTION_BAD); };
    const char* ErrorMessage() { return strdup(PQerrorMessage(conn)); };

    VectorMapInfoHolder GetVectorMapInfo(vector<string> layerNames);
    LayerInfoHolder GetLayerInfo(const char *layer_name);
    LayerDataHolder GetLayerData(const char *layer_name);

  private:
    BoundingBox BinaryToBBox(const uint8_t *binary, uint32_t length);
    PGconn *conn;

};

#endif /* __DBCONN_H_ */
