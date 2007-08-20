#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "dbconn.h"
#ifdef HAVE_GEOS
#include <geos_c.h>
#endif

using namespace std;

bool PostgresConn::Connect(const char* dbname, const char* host, const char* user, const char* password, const char* port)
{
  conn = PQsetdbLogin(host, port, NULL, NULL, dbname, user, password);
  return (PQstatus(conn) != CONNECTION_BAD);
}

bool PostgresConn::Disconnect()
{
  PQfinish(conn);
  conn = NULL;
  //return (PQstatus(conn) == CONNECTION_BAD);
  return true;
}

VectorMapInfoHolder PostgresConn::GetVectorMapInfo(vector<string> layerNames)
{
  // Get the extent in the first query
  string query_string = "SELECT asbinary(extent(geom)) FROM (SELECT geom FROM ";

  for (uint ii=0; ii<layerNames.size(); ++ii)
  {
    if (ii == 0)
    {
      query_string += layerNames[ii];
    }
    else
    {
      query_string += " UNION SELECT geom FROM " + string(layerNames[ii]);
    }
  }
  query_string += ") AS layer_extent;";

  //PGresult* res = PQexec(conn, query_string.c_str());
  PGresult* res = PQexecParams(conn,
                               query_string.c_str(),
                               0,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               1);

  if (PQresultStatus(res) != PGRES_TUPLES_OK)
  {
    cout << "Error performing select query on database!" << endl;
    cout << "No extent value found." << endl;
    cout << "GetVectorMapInfo() failed" << endl;
  }
 
  uint8_t* wkb_temp = reinterpret_cast<uint8_t*>(PQgetvalue(res, 0, 0));
  uint length = PQgetlength(res, 0, 0);
  uint8_t* wkb = new uint8_t[length];
  memcpy(wkb, wkb_temp, length);
  BoundingBox extent = BinaryToBBox(wkb, length);
  delete[] wkb;

  // Get the srid in the second query
  res = PQexec(conn, "SELECT srid FROM geometry_columns LIMIT 1;");
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
  {
    cout << "Error performing select query on database!" << endl;
    cout << "No srid value found." << endl;
    cout << "GetVectorMapInfo() failed" << endl;
  }

  uint32_t srid = atoi(PQgetvalue(res, 0, 0));
  PQclear(res);

  VectorMapInfoHolder info(srid, extent);
  for (uint i=0; i<layerNames.size(); ++i)
  {
    LayerInfoHolder layer_info(layerNames[i]);
    LayerDataHolder layer_data(layer_info);
    info.layers.push_back(layer_data);
  }

  return info;
}

LayerInfoHolder PostgresConn::GetLayerInfo(const char* layer_name)
{
  LayerInfoHolder info;

  // Retrieve the extent of the layer in binary form
  const char* query_template = "SELECT asbinary(extent(geom)) AS extent FROM %s;";

  char query_string[MAX_PSQL_STRING];
  memset(query_string, 0, MAX_PSQL_STRING);
  snprintf(query_string, MAX_PSQL_STRING, query_template, layer_name);

  //PGresult* res = PQexec(conn, query_string);
  PGresult* res = PQexecParams(conn,
                               query_string,
                               0,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               1);

  if (PQresultStatus(res) != PGRES_TUPLES_OK)
  {
    cout << "Error performing select query on database!" << endl;
    cout << "GetLayerInfo() failed" << endl;
  }

  info.name = layer_name;
  uint length = PQgetlength(res, 0, 0);
  uint8_t* wkb = new uint8_t[length];
  memcpy(wkb, PQgetvalue(res, 0, 0), length);

  info.extent = BinaryToBBox(wkb, length);

  PQclear(res);

  return info;
}

LayerDataHolder PostgresConn::GetLayerData(const char* layer_name)
{
  // Split into two queries. First get the layer meta-data, then get the feature data.
  // need to get layer name count, layer name, feature count and exteny
  LayerDataHolder data;

  data.info.name = layer_name;

  // need to get name count, name, wkb count, wkb
  const char* template_data = "SELECT name, asbinary(geom) FROM %s;";
  char query_data[MAX_PSQL_STRING];
  memset(query_data, 0, sizeof(MAX_PSQL_STRING));
  snprintf(query_data, MAX_PSQL_STRING, template_data, layer_name);

  //res = PQexec(conn, query_data);
  PGresult* res = PQexecParams(conn,
                         query_data,
                         0,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         1);
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
  {
    cout << "Error performing select query on database!" << endl;
    cout << "GetLayerData() data failed, returned NULL" << endl;
  }

  int num_rows = PQntuples(res);

  for (int i=0; i<num_rows; ++i)
  {
    FeatureDataHolder fd(string(PQgetvalue(res, i, 0)));
    uint8_t *wkb = reinterpret_cast<uint8_t *>(PQgetvalue(res, i, 1));
    uint32_t length = PQgetlength(res, i, 1);
    fd.wkb.assign(wkb, &wkb[length]);
    data.features.push_back(fd);
  }

  PQclear(res);

  return data;
}

BoundingBox PostgresConn::BinaryToBBox(const uint8_t* wkb, uint length)
{
  BoundingBox res;
  memset(&res, 0, sizeof(BoundingBox));
#ifdef HAVE_GEOS
  GEOSGeom polygon = NULL;
  polygon = GEOSGeomFromWKB_buf(wkb, length);
  if (polygon == NULL)
  {
    printf("GEOSGeomFromWKB_buf returned NULL!\n");
    return res;
  }
  GEOSGeom linestring = GEOSGetExteriorRing(polygon);
  if (linestring == NULL)
  {
    printf("GEOSGetExteriorRing returned NULL!\n");
    return res;
  }
  GEOSCoordSeq coords = GEOSGeom_getCoordSeq(linestring);
  if (coords == NULL)
  {
    printf("GEOSGeom_getCoordSeq returned NULL!\n");
    return res;
  }

  double xmin = INT_MAX, ymin = INT_MAX;
  double xmax = INT_MIN, ymax = INT_MIN;
  double tempX, tempY = 0;

  for (int ii=0; ii<GEOSGetNumCoordinates(linestring); ++ii)
  {
    GEOSCoordSeq_getX(coords, ii, &tempX);
    GEOSCoordSeq_getY(coords, ii, &tempY);
    if (tempX > xmax)
      xmax = tempX;
    if (tempX < xmin)
      xmin = tempX;
    if (tempY > ymax)
      ymax = tempY;
    if (tempY < ymin)
      ymin = tempY;
  }

  res.x0 = xmin;
  res.y0 = ymin;
  res.x1 = xmax;
  res.y1 = ymax;

#endif
  return res;
}

const player_vectormap_info_t* VectorMapInfoHolder::Convert()
{
  info.srid = srid;
  info.extent.x0 = extent.x0;
  info.extent.y0 = extent.y0;
  info.extent.x1 = extent.x1;
  info.extent.y1 = extent.y1;
  info.layers_count = layers.size();
  info.layers = new player_vectormap_layer_data_t[layers.size()];
  for (uint ii=0; ii<layers.size(); ++ii)
  {
    info.layers[ii] = *(layers[ii].Convert());
  }
  return &info;
}

VectorMapInfoHolder::~VectorMapInfoHolder()
{
//   if (info.layers)
//   {
//     delete[] info.layers;
//   }
}

const player_vectormap_layer_info_t* LayerInfoHolder::Convert()
{
  layer_info.name = strdup(name.c_str());
  layer_info.name_count = name.size() + 1;
  layer_info.extent.x0 = extent.x0;
  layer_info.extent.y0 = extent.y0;
  layer_info.extent.x1 = extent.x1;
  layer_info.extent.y1 = extent.y1;
  return &layer_info;
}

const player_vectormap_feature_data_t* FeatureDataHolder::Convert()
{
  feature_data.name = strdup(name.c_str());
  feature_data.name_count = name.size() + 1;
  feature_data.wkb = new uint8_t[wkb.size()];
  feature_data.wkb_count = wkb.size();
  ///TODO: Make more efficient
  for (uint ii=0; ii<wkb.size(); ++ii)
  {
    feature_data.wkb[ii] = wkb[ii];
  }
  return &feature_data;
}

FeatureDataHolder::~FeatureDataHolder()
{
//   if (feature_data.wkb)
//   {
//     delete[] feature_data.wkb;
//   }
}

const player_vectormap_layer_data_t* LayerDataHolder::Convert()
{
  layer_data.info = *(info.Convert());
  layer_data.features_count = features.size();
  layer_data.features = new player_vectormap_feature_data_t[features.size()];
  for (uint ii=0; ii<features.size(); ++ii)
  {
    layer_data.features[ii] = *(features[ii].Convert());
  }
  return &layer_data;
}

LayerDataHolder::~LayerDataHolder()
{
//   if (layer_data.features)
//   {
//     delete[] layer_data.features;
//   }
}
