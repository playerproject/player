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
#include <sonarproxy.h>  /* support for all sonar devices */
#include <miscproxy.h>  /* support for the misc device */
#include <positionproxy.h>  /* support for all position devices */
#include <ptzproxy.h>  /* support for the ptz device */
#include <gripperproxy.h>  /* support for the gripper device */
#include <speechproxy.h>  /* support for the speech device */
#include <laserbeaconproxy.h>  /* support for the laserbeacon device */
#include <visionproxy.h>  /* support for the vision device */
//BROKEN #include <truthproxy.h>  /* support for the truth device */
//REMOVE? #include <occupancyproxy.h>  /* support for guess which device */
#include <gpsproxy.h>  /* support for the GPS device */
#include <bpsproxy.h>  /* support for the BPS device */
#include <broadcastproxy.h> /* support for the broadcast device */
#include <moteproxy.h> /* support for the broadcast device */

#include <bumperproxy.h> /* support for all bumper devices */
//#include <joystickproxy.h> /* support for all joystick devices */
#include <powerproxy.h> /* support for all power devices */

//#include <idarproxy.h>  /* support for the IDAR device */
//#include <descartesproxy.h>  /* support for the IDAR device */

// keep a linked list of proxies that we've got open
class ClientProxyNode
{
  public:
    ClientProxy* proxy;
    ClientProxyNode* next;
};

/** One {\tt PlayerClient} object is used to control each connection to
    a Player server.  Contained within this object are methods for changing the
    connection parameters and obtaining access to devices, which we explain 
    next.
 */
class PlayerClient
{
  private:
    // special flag to indicate that we are being destroyed
    bool destroyed;

    // list of proxies associated with us
    ClientProxyNode* proxies;
    int num_proxies;

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
    // store the IP of the host, too - more efficient for
    // matching, etc, then the string
    struct in_addr hostaddr;

    // the current time on the server
    struct timeval timestamp;

    // constructors
    
    /** Make a client and connect it as indicated.
        If {\tt hostname} is omitted (or NULL) then the client will {\em not}
        be connected.  In that cast, call {\tt Connect()} yourself later.
     */
    PlayerClient(const char* hostname=NULL, const int port=PLAYER_PORTNUM);

    /** Make a client and connect it as indicated, using a binary IP instead 
        of a hostname
      */
    PlayerClient(const struct in_addr* hostaddr, const int port);

    // destructor
    ~PlayerClient();

    /** Connect to the indicated host and port.\\
        Returns 0 on success; -1 on error.
     */
    int Connect(const char* hostname="localhost", int port=PLAYER_PORTNUM);

    /** Connect to the indicated host and port, using a binary IP.\\
        Returns 0 on success; -1 on error.
     */
    int Connect(const struct in_addr* addr, int port);

    /** Disconnect from server.\\
        Returns 0 on success; -1 on error.
      */
    int Disconnect();

    /** Check if we are connected.
      */
    bool Connected() { return((conn.sock >= 0) ? true : false); }

    //
    // read from our connection.  put the data in the proper device object
    //
    // Returns:
    //    0 if everything went OK
    //   -1 if something went wrong (you should probably close the connection!)
    //
    /**
    This method will read one round of data; that is, it will read one data
    packet for each device that is currently open for reading.  The data from
    each device will be processed by the appropriate device proxy and stored
    there for access by your program.  If no errors occurred 0 is returned.
    Otherwise, -1 is returned and diagnostic information is printed to {\tt
    stderr} (you should probably close the connection!).
    */
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

    // use this one if you don't want the reply. it will return -1 if 
    // the request failed outright or if the response type is not ACK
    int Request(unsigned short device,
                unsigned short index,
                const char* payload,
                size_t payloadlen);
    
    /**
      Request access to a device; meant mostly for use by client-side device 
      proxy constructors.\\
      {\tt req\_access} is requested access.\\
      {\tt grant\_access}, if non-NULL, will be filled with the granted access.\\
      Returns 0 if everything went OK or -1 if something went wrong.
    */
    int RequestDeviceAccess(unsigned short device,
                            unsigned short index,
                            unsigned char req_access,
                            unsigned char* grant_access );

    // Player device configurations

    // change continuous data rate (freq is in Hz)
    /**
      You can change the rate at which your client receives data from the 
      server with this method.  The value of {\tt freq} is interpreted as Hz; 
      this will be the new rate at which your client receives data (when in 
      continuous mode).  On error, -1 is returned; otherwise 0.
     */
    int SetFrequency(unsigned short freq);

    /** You can toggle the mode in which the server sends data to your 
        client with this method.  The {\tt mode} should be either 1 
        (for request/reply) or 0 (for continuous).  On error, -1 is 
        returned; otherwise 0.
      */
    int SetDataMode(unsigned char mode);

    /** When in request/reply mode, you can request a single round of data 
        using this method.  On error -1 is returned; otherwise 0.
      */
    int RequestData();

    /** Attempt to authenticate your client using the provided key.  If 
        authentication fails, the server will close your connection
      */
    int Authenticate(char* key);
    
    // proxy list management methods

    // add a proxy to the list
    void AddProxy(ClientProxy* proxy);
    // remove a proxy from the list
    void RemoveProxy(ClientProxy* proxy);
};


#endif
