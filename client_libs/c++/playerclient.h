/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *                      
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
 * the C++ client
 */

#ifndef PLAYERCLIENT_H
#define PLAYERCLIENT_H


#include <messages.h>       /* from the server; gives message types */
#include <playercclient.h>  /* pure C networking building blocks */

#include <laserproxy.h>  /* support for the laser device */
#include <sonarproxy.h>  /* support for the sonar device */
#include <miscproxy.h>  /* support for the misc device */
#include <positionproxy.h>  /* support for the position device */
#include <ptzproxy.h>  /* support for the ptz device */
#include <gripperproxy.h>  /* support for the gripper device */
#include <speechproxy.h>  /* support for the speech device */
#include <laserbeaconproxy.h>  /* support for the laserbeacon device */
#include <visionproxy.h>  /* support for the vision device */
#include <truthproxy.h>  /* support for the truth device */
#include <occupancyproxy.h>  /* support for guess which device */
#include <gpsproxy.h>  /* support for the GPS device */
#include <bpsproxy.h>  /* support for the BPS device */

// keep a linked list of proxies that we've got open
class ClientProxyNode
{
  public:
    ClientProxy* proxy;
    ClientProxyNode* next;
};

class PlayerClient
{
  private:
    // special flag to indicate that we are being destroyed
    bool destroyed;

    // list of proxies associated with us
    ClientProxyNode* proxies;
    int num_proxies;

    // count devices with access 'r' or 'a'
    int CountReadProxies();

    int reserved;
    
    // get the pointer to the proxy for the given device and index
    //
    // returns NULL if we can't find it.
    //
    ClientProxy* PlayerClient::GetProxy(unsigned short device, 
                                        unsigned short index);

  public:
    // our connection to Player
    player_connection_t conn;

    // are we connected?
    bool connected;

    void SetReserved(int res) { reserved = res; }
    int GetReserved() { return(reserved); }

    // flag set if data has just been read into this device
    bool fresh; 

    // store the name and port of the connected host
    char hostname[256]; 
    int port;

    // the current time on the server
    struct timeval timestamp;

    // constructors
    // 
    // make a client and connect it as indicated.
    PlayerClient(const char* hostname=NULL, int port=PLAYER_PORTNUM);

    // destructor
    ~PlayerClient();

    // 3 ways to connect to the server
    // Return:
    //    0 if everything is OK (connection opened)
    //   -1 if something went wrong (connection NOT opened)
    //  
    int Connect(const char* hostname, int port);
    int Connect(const char* hostname) { return(Connect(hostname,PLAYER_PORTNUM)); }
    int Connect() { return(Connect("localhost",PLAYER_PORTNUM)); }

    // disconnect from server
    // Return:
    //    0 if everything is OK (connection closed)
    //   -1 if something went wrong (connection was probably already closed)
    int Disconnect();

    // check if we are connected
    bool Connected() { return((conn.sock >= 0) ? true : false); }

    //
    // read from our connection.  put the data in the proper device object
    //
    // Returns:
    //    0 if everything went OK
    //   -1 if something went wrong (you should probably close the connection!)
    //
    int Read();
    
    //
    // write a message to our connection.  
    //
    // Returns:
    //    0 if everything went OK
    //   -1 if something went wrong (you should probably close the connection!)
    //
    int Write(unsigned short device, unsigned short index,
              const char* command, size_t commandlen);

    //
    // issue a request to the player server
    //
    int Request(unsigned short device,
                unsigned short index,
                const char* payload,
                size_t payloadlen,
                player_msghdr_t* replyhdr,
                char* reply, size_t replylen);

    // use this one if you don't want the reply
    int Request(unsigned short device,
                unsigned short index,
                const char* payload,
                size_t payloadlen)
    { return(Request(device,index,payload,payloadlen,NULL,NULL,0)); }
    
    // request access to a device, meant for use by client-side device
    // proxy constructors
    //
    // req_access is requested access
    // grant_access, if non-NULL, will be filled with the granted access
    //
    // Returns:
    //   0 if everything went OK
    //   -1 if something went wrong (you should probably close the connection!)
    int RequestDeviceAccess(unsigned short device,
                            unsigned short index,
                            unsigned char req_access,
                            unsigned char* grant_access);

    // Player device configurations

    // change continuous data rate (freq is in Hz)
    int SetFrequency(unsigned short freq);

    // change data delivery mode
    //   1 = request/reply
    //   0 = continuous (default)
    int SetDataMode(unsigned char mode);

    // request a round of data (only valid when in request/reply mode)
    int RequestData();

    // authenticate
    int Authenticate(char* key);
    
    // proxy list management methods

    // add a proxy to the list
    void AddProxy(ClientProxy* proxy);
    // remove a proxy from the list
    void RemoveProxy(ClientProxy* proxy);
};


#endif
