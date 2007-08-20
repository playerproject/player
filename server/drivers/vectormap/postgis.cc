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
 * A driver to use a vector map with a PostGIS backend.
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_PostGIS PostGIS
 * @brief Vectormap driver based on a Postgresql database with the PostGIS plugin.

@par Provides

- @ref interface_vectormap

@par Requires

- None

@par Configuration requests

- None

@par Configuration file options

- dbname (string)
  - Default: template1
  - The name of the Postgresql database to connect to.
- host (string)
  - Default: localhost
  - The name of the database host.
- user (string)
  - Default: postgres
  - The name of the database user.
- port (string)
  - Default: 5432
  - The port of the database.
- password (string)
  - Default: empty string
  - The password for the database user
- layers (string tuple)
  - Default: Field required
  - Names of the layers. The layers are named after the corresponding tables in the database.
@par Example 

@verbatim
driver
(
  dbname "gis"
  host "192.168.0.2"
  port "5433"
  user "postgres"
  password "secret"
  layers ["obstacles_geom" "markers_geom"]
)
@endverbatim

@par Creating a PostGIS Database
///TODO: Add documentation
For more information see http://postgis.refractions.net/

@par Database schema
///TODO: Add documentation

@author Ben Morelli

*/

/** @} */

#include <sys/types.h> // required by Darwin
#include <stdlib.h>
#include <libplayercore/playercore.h>
#include <libplayercore/error.h>
#include <playererror.h>
#include "dbconn.h"
#ifdef HAVE_GEOS
#include <geos_c.h>
#endif

/** Dummy function passed as a function pointer GEOS when it is initialised. GEOS uses this for logging. */
void geosprint(const char *text, ...)
{
  return;
}
////////////////////////////////////////////////////////////////////////////////
class PostGIS : public Driver
{
  public:
    PostGIS(ConfigFile* cf, int section,
            const char* dbname,
            const char* host,
            const char* user,
            const char* password,
            const char* port,
            vector<string> layerNames);
    ~PostGIS();
    int Setup();
    int Shutdown();

    // MessageHandler
    int ProcessMessage(MessageQueue * resp_queue,
		       player_msghdr * hdr,
		       void * data);

  private:
    void RequestLayerWrite(player_vectormap_layer_data_t* data);

    VectorMapInfoHolder RequestVectorMapInfo();
    LayerInfoHolder RequestLayerInfo(const char* layer_name);
    LayerDataHolder RequestLayerData(const char* layer_name);

    const char* dbname;
    const char* host;
    const char* user;
    const char* password;
    const char* port;
    vector<string> layerNames;

    PostgresConn *conn;
};

////////////////////////////////////////////////////////////////////////////////
Driver* PostGIS_Init(ConfigFile* cf, int section)
{
  const char* dbname = cf->ReadString(section,"dbname", "template1");
  const char* host = cf->ReadString(section,"host", "localhost");
  const char* user = cf->ReadString(section,"user", "postgres");
  const char* password = cf->ReadString(section,"password", "");
  const char* port = cf->ReadString(section,"port", "5432");

  vector<string> layerNames;

  int layers_count = cf->GetTupleCount(section, "layers");
  if (layers_count < 1)
  {
    PLAYER_ERROR("There must be at least one layer defined in the 'layers' configuration field.");
    return NULL;
  }

  for (int i=0; i<layers_count; ++i)
  {
    const char* layer_name = cf->ReadTupleString(section, "layers", i, "");
    layerNames.push_back(string(layer_name));
  }
  return((Driver*)(new PostGIS(cf, section, dbname, host, user, password, port, layerNames)));
}

////////////////////////////////////////////////////////////////////////////////
// a driver registration function
void PostGIS_Register(DriverTable* table)
{
  table->AddDriver("postgis", PostGIS_Init);
}

////////////////////////////////////////////////////////////////////////////////
PostGIS::PostGIS(ConfigFile* cf, int section,
                 const char* dbname,
                 const char* host,
                 const char* user,
                 const char* password,
                 const char* port,
                 vector<string> layerNames)
  : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_VECTORMAP_CODE)
{
  this->dbname = dbname;
  this->host = host;
  this->user = user;
  this->password = password;
  this->port = port;
  this->layerNames = layerNames;
  this->conn = NULL;

}

////////////////////////////////////////////////////////////////////////////////
PostGIS::~PostGIS()
{
  /*if (conn != NULL && conn->Connected())
  {
    conn->Disconnect();
  }
  if (conn != NULL)
  {
    printf("DELETING CONNECTION\n");
    delete conn;
    conn = NULL;
  }*/
}

////////////////////////////////////////////////////////////////////////////////
// Load resources
int PostGIS::Setup()
{
  PLAYER_MSG0(2, "PostGIS vectormap initialising");

#ifdef HAVE_GEOS
  PLAYER_MSG0(2, "Initialising GEOS");
  initGEOS(geosprint, geosprint);
  PLAYER_MSG0(2, "GEOS Initialised");
#endif

  conn = new PostgresConn();
  conn->Connect(dbname, host, user, password, port);

  if (!conn->Connected())
  {
    PLAYER_ERROR("Could not connect to Postgres database!");
    PLAYER_ERROR1("%s",conn->ErrorMessage());
    return(1);
  }

  PLAYER_MSG0(2, "PostGIS vectormap ready");
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
// Clean up resources
int PostGIS::Shutdown()
{
  PLAYER_MSG0(2, "PostGIS vectormap shutting down");
  if (conn != NULL && conn->Connected())
  {
    PLAYER_MSG0(2, "Disconnecting database");
    conn->Disconnect();
  }
  if (conn != NULL)
  {
    delete conn;
    //conn == NULL;
  }
  
#ifdef HAVE_GEOS
  PLAYER_MSG0(2, "Shutting down GEOS");
  finishGEOS();
#endif
  PLAYER_MSG0(2, "PostGIS vectormap stopped");
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
// Process an incoming message
int PostGIS::ProcessMessage(MessageQueue * resp_queue,
                            player_msghdr * hdr,
                            void * data)
{
  // Request for map info ///////////////////////////////////////////////////////////
  if (Message::MatchMessage(hdr,  PLAYER_MSGTYPE_REQ,
                                  PLAYER_VECTORMAP_REQ_GET_MAP_INFO,
                                  this->device_addr))
  {

    if (hdr->size != 0)
    {
      PLAYER_ERROR2("request is wrong length (%d != %d); ignoring",
                 hdr->size, 0);
      return -1;
    }

    VectorMapInfoHolder info = RequestVectorMapInfo();
    const player_vectormap_info_t* response = info.Convert();

    this->Publish(this->device_addr,
                  resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_VECTORMAP_REQ_GET_MAP_INFO,
                  (void*)response,
                  sizeof(player_vectormap_info_t),
                  NULL);
    return(0);
  }
  // Request for layer info /////////////////////////////////////////////////////////
  else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
           PLAYER_VECTORMAP_REQ_GET_LAYER_INFO,
           this->device_addr))
  {

    if (hdr->size != sizeof(player_vectormap_layer_info_t))
    {
      PLAYER_ERROR2("request is wrong length (%d != %d); ignoring",
                    hdr->size, 0);
      return -1;
    }

    const player_vectormap_layer_info_t* request = reinterpret_cast<player_vectormap_layer_info_t*>(data);
    LayerInfoHolder info = RequestLayerInfo(request->name);
    const player_vectormap_layer_info_t* response = info.Convert();

    this->Publish(this->device_addr,
                  resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_VECTORMAP_REQ_GET_LAYER_INFO,
                  (void*)response,
                   sizeof(player_vectormap_layer_info_t),
                          NULL);

    return(0);
  }
  // Request for layer data /////////////////////////////////////////////////////////
  else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                                      PLAYER_VECTORMAP_REQ_GET_LAYER_DATA,
                                      this->device_addr))
  {
    if (hdr->size != sizeof(player_vectormap_layer_data_t))
    {
      PLAYER_ERROR2("request is wrong length (%d != %d); ignoring",
                    hdr->size, 0);
      return -1;
    }

    player_vectormap_layer_data_t* request = reinterpret_cast<player_vectormap_layer_data_t*>(data);
    LayerDataHolder ldata = RequestLayerData(request->info.name);
    const player_vectormap_layer_data_t* response = ldata.Convert();

    this->Publish(this->device_addr,
                  resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_VECTORMAP_REQ_GET_LAYER_DATA,
                  (void*)response,
                  sizeof(player_vectormap_layer_data_t),
                  NULL);

    return(0);
  }
  // Request to write layer data ///////////////////////////////////////////////////
  else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                                      PLAYER_VECTORMAP_REQ_WRITE_LAYER,
                                      this->device_addr))
  {
    if (hdr->size != 0)
    {
      PLAYER_ERROR2("request is wrong length (%d != %d); ignoring",
                    hdr->size, 0);
      return -1;
    }
    player_vectormap_layer_data_t* request = reinterpret_cast<player_vectormap_layer_data_t*>(data);
    RequestLayerWrite(request);

    this->Publish(this->device_addr,
                  resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_VECTORMAP_REQ_WRITE_LAYER,
                  (void*)request,
                   sizeof(player_vectormap_layer_data_t),
                  NULL);
    return(0);
  }
  // Don't know how to handle this message /////////////////////////////////////////
  return(-1);
}

VectorMapInfoHolder PostGIS::RequestVectorMapInfo()
{
  if (conn == NULL || conn->Connected() == false)
  {
    PLAYER_ERROR("PostGis::RequestVectorMapInfo() failed! No db connection.");
  }

  VectorMapInfoHolder info = conn->GetVectorMapInfo(layerNames);

  return info;
}

LayerInfoHolder PostGIS::RequestLayerInfo(const char* layer_name)
{
  if (conn == NULL || conn->Connected() == false)
  {
    PLAYER_ERROR("PostGis::RequestLayerInfo() failed! No db connection.");
  }

  LayerInfoHolder info = conn->GetLayerInfo(layer_name);

  return info;
}

LayerDataHolder PostGIS::RequestLayerData(const char* layer_name)
{
  if (conn == NULL || conn->Connected() == false)
  {
    PLAYER_ERROR("PostGis::RequestLayerData() failed! No db connection.");
  }

  LayerDataHolder data = conn->GetLayerData(layer_name);

  return data;
}

void PostGIS::RequestLayerWrite(player_vectormap_layer_data_t* data)
{
  return;
}
