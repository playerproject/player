/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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
 * a pass-through driver that acts as a client to another Player server,
 * shifting data (and maybe commands) back and forth, making it seem as if 
 * remote devices are actually local
 */

#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

// we'll use the C client facilities to connect to the remote server
#include <playercclient.h>
#include <deviceregistry.h>  // for lookup_interface()
#include <device.h>
#include <drivertable.h>
#include <devicetable.h>
#include <player.h>

extern CDeviceTable* deviceTable;

extern int global_playerport;

class PassThrough:public CDevice 
{
  private:
    // info for the server/device to which we will connect
    const char* remote_hostname;
    int remote_port;
    player_device_id_t remote_device_id;
    unsigned char remote_access;
    char remote_drivername[PLAYER_MAX_DEVICE_STRING_LEN];

    // bookkeeping for the client connection
    player_connection_t conn;
    char* remote_data;
    char* remote_command;
    char* remote_config;
    char* remote_reply;

    // this function will be run in a separate thread
    virtual void Main();

    // close and cleanup
    void CloseConnection();

  public:
    PassThrough(const char* hostname, int port,
                player_device_id_t id, char* interface, 
                ConfigFile* cf, int section);
    ~PassThrough();
    virtual int Setup();
    virtual int Shutdown();
};

// initialization function
CDevice* 
PassThrough_Init(char* interface, ConfigFile* cf, int section)
{
  player_interface_t interf;
  player_device_id_t id;
  int port;
  const char* host;
  // determine the code for the interface that we have been asked to support
  if(lookup_interface(interface, &interf) < 0)
  {
    PLAYER_ERROR1("couldn't find interface code for \"%s\"", interface);
    return(NULL);
  }
  host = cf->ReadString(section, "host", "localhost");
  port = cf->ReadInt(section, "port", global_playerport);
  id.port = cf->ReadInt(section, "port", 0);
  id.code = interf.code;
  id.index = cf->ReadInt(section, "index", 0);

  if(!strcmp(host,"localhost") && (port == global_playerport))
  {
    PLAYER_ERROR("passthrough connected to itself; you should specify\n the hostname and/or port of the remote server in the configuration file");
    return(NULL);
  }

  return((CDevice*)(new PassThrough(host, port, id, interface, cf, section)));
}

// a driver registration function
void 
PassThrough_Register(DriverTable* table)
{
  table->AddDriver("passthrough", PLAYER_ALL_MODE, PassThrough_Init);
}

PassThrough::PassThrough(const char* hostname, int port,
                         player_device_id_t id, char* interface, 
                         ConfigFile* cf, int section) :
  CDevice(PLAYER_MAX_PAYLOAD_SIZE,PLAYER_MAX_PAYLOAD_SIZE,1,1)
{
  this->remote_hostname = hostname;
  this->remote_port = port;
  this->remote_device_id = id;
  this->remote_access = (unsigned char)cf->ReadString(section, "access", "a")[0];
  this->conn.protocol = PLAYER_TRANSPORT_TCP;

  assert(this->remote_data = (char*)calloc(PLAYER_MAX_PAYLOAD_SIZE,1));
  assert(this->remote_command = (char*)calloc(PLAYER_MAX_PAYLOAD_SIZE,1));
  assert(this->remote_config = (char*)calloc(PLAYER_MAX_PAYLOAD_SIZE,1));
  assert(this->remote_reply = (char*)calloc(PLAYER_MAX_PAYLOAD_SIZE,1));
}

void
PassThrough::CloseConnection()
{
  if(this->conn.sock >=0)
    player_disconnect(&this->conn);
  PutData(NULL,0,0,0);
}

PassThrough::~PassThrough()
{
  //CloseConnection();
  free(this->remote_data);
  free(this->remote_command);
  free(this->remote_config);
  free(this->remote_reply);
}

int 
PassThrough::Setup()
{
  unsigned char grant_access;
  CDeviceEntry* devp;
  player_msghdr_t hdr;

  // zero out the buffers
  //PutData(NULL,0,0,0);
  PutCommand(NULL,NULL,0);

  printf("Passthrough connecting to server at %s:%d...", this->remote_hostname,
         this->remote_port);
  // connect to the server
  if(player_connect(&this->conn,this->remote_hostname,this->remote_port) < 0)
  {
    PLAYER_ERROR1("couldn't connect to remote host \"%s\"", 
                  this->remote_hostname);
    return(-1);
  }

  puts("Done");

  printf("Passthrough opening device %d:%d:%d...", 
         this->remote_device_id.port,
         this->remote_device_id.code,
         this->remote_device_id.index);
  // open the device
  if((player_request_device_access(&this->conn,
                                   this->remote_device_id.code,
                                   this->remote_device_id.index,
                                   this->remote_access,
                                   &grant_access,
                                   this->remote_drivername,
                                   sizeof(this->remote_drivername)) < 0) ||
     (grant_access != this->remote_access))
  {
    PLAYER_ERROR("couldn't get requested access to remote device");
    CloseConnection();
    return(-1);
  }
  puts("Done");

  // set the driver name in the devicetable
  if(!(devp = deviceTable->GetDeviceEntry(this->device_id)))
  {
    PLAYER_ERROR("couldn't find my own entry in the deviceTable");
    CloseConnection();
    return(-1);
  }
  strncpy(devp->drivername,this->remote_drivername,PLAYER_MAX_DEVICE_STRING_LEN);

  for(;;)
  {
    // wait for one data packet from the remote server, to avoid sending
    // zero length packets to our clients
    if(player_read(&this->conn,&hdr,this->remote_data,PLAYER_MAX_PAYLOAD_SIZE))
    {
      PLAYER_ERROR("got error while reading data; bailing");
      CloseConnection();
      return(-1);
    }

    if((hdr.type == PLAYER_MSGTYPE_DATA) &&
       (hdr.device == this->remote_device_id.code) &&
       (hdr.device_index == this->remote_device_id.index))
    {
      PutData(this->remote_data,hdr.size,
              hdr.timestamp_sec,hdr.timestamp_usec);
      break;
    }
  }

  StartThread();

  return(0);
}

int 
PassThrough::Shutdown()
{
  StopThread();
  CloseConnection();
  return(0);
}

void
PassThrough::Main()
{
  size_t len_command;
  size_t len_config;
  player_msghdr_t hdr;
  void* client;
  player_msghdr_t replyhdr;
  player_device_id_t id;
  struct timeval ts;

  for(;;)
  {
    // did we get a config request?
    if((len_config = 
        GetConfig(&id,&client,this->remote_config,PLAYER_MAX_PAYLOAD_SIZE)) > 0)
    {
      // send it
      if(player_request(&this->conn,this->remote_device_id.code,
                        this->remote_device_id.index,
                        this->remote_config,len_config,&replyhdr,
                        this->remote_reply,PLAYER_MAX_PAYLOAD_SIZE) < 0)
      {
        PLAYER_ERROR("got error while sending request; bailing");
        CloseConnection();
        pthread_exit(NULL);
      }

      ts.tv_sec = replyhdr.timestamp_sec;
      ts.tv_usec = replyhdr.timestamp_usec;

      // return the reply
      PutReply(&id,client,replyhdr.type,&ts,this->remote_reply,replyhdr.size);
    }

    // did we get a new command to send?
    if((len_command = GetCommand((unsigned char*)this->remote_command,
                         PLAYER_MAX_PAYLOAD_SIZE)) > 0)
    {
      if(player_write(&this->conn,this->remote_device_id.code,
                      this->remote_device_id.index,
                      this->remote_command,len_command) < 0)
      {
        PLAYER_ERROR("got error while writing command; bailing");
        CloseConnection();
        pthread_exit(NULL);
      }
    }

    // get new data from the remote server
    if(player_read(&this->conn,&hdr,this->remote_data,PLAYER_MAX_PAYLOAD_SIZE))
    {
      PLAYER_ERROR("got error while reading data; bailing");
      CloseConnection();
      pthread_exit(NULL);
    }

    if((hdr.type == PLAYER_MSGTYPE_DATA) &&
       //(hdr.robot == this->remote_device_id.robot) &&
       (hdr.device == this->remote_device_id.code) &&
       (hdr.device_index == this->remote_device_id.index))
    {
      PutData(this->remote_data,hdr.size,
              hdr.timestamp_sec,hdr.timestamp_usec);
    }
  }
}

