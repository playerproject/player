/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
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
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
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

/*
 * $Id$
 *
 * the C++ client
 */

/** @addtogroup clientlibs Client Libraries */
/** @{ */
/** @defgroup player_clientlib_cplusplus C++ client library
@{

The C++ client (@p client_libs/c++) is generally the most comprehensive
library, since it is used to test new features as they are implemented
in the server.  It is also the most widely used client library and thus
the best debugged.  Having said that, this client is not perfect, but
should be straightforward to use by anyone familiar with C++.

The C++ library is built on a "service proxy" model in which the client
maintains local objects that are proxies for remote services.  There are
two kinds of proxies: the special server proxy @p PlayerClient and the
various device-specific proxies.  Each kind of proxy is implemented as a
separate class.  The user first creates a @p PlayerClient proxy and uses
it to establish a connection to a Player server.  Next, the proxies of the
appropriate device-specific types are created and initialized using the
existing @p PlayerClient proxy.  To make this process concrete, consider
the following simple example (for clarity, we omit some error-checking):

@include example_from_manual.cc

This program performs simple (and bad) sonar-based obstacle avoidance with
a mobile robot .
First, a @p PlayerClient
proxy is created, using the default constructor to connect to the
server listening at @p localhost:6665.  Next, a @p SonarProxy is
created to control the sonars and a @p PositionProxy to control the
robot's motors.  The constructors for these objects use the existing @p
PlayerClient proxy to establish @b read (@p 'r') access and @b write (@p
'w') access to the 0th @p sonar and @p position devices, respectively.
Finally, we enter a simple loop that reads the current sonar state and
writes appropriate commands to the motors.
*/

#ifndef PLAYERCLIENT_H
#define PLAYERCLIENT_H

#include <player.h>       /* from the server; gives message types */
#include <playercclient.h>  /* pure C networking building blocks */

#if HAVE_STRINGS_H
  #include <strings.h>
#endif

#if PLAYERCLIENT_THREAD
  #include <pthread.h>
#endif

#include <sys/time.h>
struct pollfd;
#include <string.h>
#include <math.h>

#ifndef RTOD
/** Convert radians to degrees */
#define RTOD(r) ((r) * 180 / M_PI)
#endif

#ifndef DTOR
/** Convert degrees to radians */
#define DTOR(d) ((d) * M_PI / 180)
#endif

#ifndef NORMALIZE
/** Normalize angle to domain -pi, pi */
#define NORMALIZE(z) atan2(sin(z), cos(z))
#endif

// forward declaration for friending
class PlayerClient;

/** @defgroup player_clientlib_cplusplus_core Core functionality */
/** @{ */

/**

Base class for all proxy devices. Access to a device is provided by a
device-specific proxy class.  These classes all inherit from the @p
ClientProxy class which defines an interface for device proxies.  As such,
a few methods are common to all devices and we explain them here.

*/
class ClientProxy
{
  friend class PlayerClient; // ANSI C++ syntax?

  public:         
     /** The controlling client object.
      */
    PlayerClient* client;

    /** Have we yet received any data from this device?
     */
    bool valid;

    /** the last message header and body will be copied here by
        StoreData(), so that it's available for later use. */
    // functionality currently disabled, until it is fixed to handle
    // big messages.
#if 0
    unsigned char last_data[PLAYER_MAX_MESSAGE_SIZE];
    player_msghdr_t last_header;
#endif

    unsigned char access;   // 'r', 'w', or 'a' (others?)
    player_device_id_t m_device_id;

    /** The name of the driver used to implement this device in the server.
     */
    char driver_name[PLAYER_MAX_DEVICE_STRING_LEN]; // driver in use

    /** Time at which this data was generated by the device. 
     */
    struct timeval timestamp;  

    /** Time at which this data was sent by the server. 
      */
    struct timeval senttime;   

    /** Time at which this data was received by the client. 
     */
    struct timeval receivedtime;

    /** This constructor will try to get access to the device,
        unless @p req_device is 0 or @p req_access is 'c'.
        The pointer @p pc must refer to an already connected 
        @p PlayerClient proxy.  The @p index indicates which one of the
        devices to use (usually 0).  Note that a request executed by this
        the constructor can fail, but the constructor cannot indicate the
        failure.  Thus, if you request a particular access mode, you should 
        verify that the current access is identical to your requested access 
        using @p GetAccess().  In any case, you can use 
        @p ChangeAccess() later to change your access mode for the device.
    */
    ClientProxy(PlayerClient* pc, 
		unsigned short req_device,
		unsigned short req_index,
		unsigned char req_access = 'c');

    // destructor will try to close access to the device
    virtual ~ClientProxy();

    /**  Returns the current access mode for the device.
     */
    unsigned char GetAccess() { return(access); };  

    /** Request different access for the device.  If @p grant_access is
        non-NULL, then it is filled in with the granted access.  Returns 0
        on success, -1 otherwise. */
    int ChangeAccess(unsigned char req_access, 
                     unsigned char* grant_access=NULL );

    /** Convenience method for requesting 'c' access.
     */
    int Close() { return(ChangeAccess('c')); }

    /** All proxies must provide this method.  It is used internally to parse
        new data when it is received. */
    virtual void FillData(player_msghdr_t hdr, const char* buffer);

    /** This method is used internally to keep a copy of the last message from
        the device **/
    // functionality currently disabled, until it is fixed to handle
    // big messages.
#if 0
    void StoreData(player_msghdr_t hdr, const char* buffer);
#endif
    
    /** All proxies SHOULD provide this method, which should print out, in a
        human-readable form, the device's current state. */
    virtual void Print();


    /** Methods for providing the ability to achieve thread safety, 
        the class is not thread safe without additional protection
	in the client app. 
	if PLAYERCLIENT_THREAD is not defined then Lock and Unlock
	do nothing and always succeed 
	These are called by the playerclient during a read if 
	data is to be updated */
	int Lock();
	int Unlock();
  protected:
#ifdef PLAYERCLIENT_THREAD
    pthread_mutex_t update_lock;
#endif
};


/** Keep a linked list of proxies that we've got open */
class ClientProxyNode
{
  public:
    ClientProxy* proxy;
    ClientProxyNode* next;
};


/**

One @p PlayerClient object is used to control each connection to
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

    unsigned char data_delivery_mode;
    
    // get the pointer to the proxy for the given device and index
    //
    // returns NULL if we can't find it.
    //
    ClientProxy* GetProxy(unsigned short device, 
                                        unsigned short index);
    ClientProxy* GetProxy(player_device_id_t id);

  public:
    //  Struct containing information about the  connection to Player
    player_connection_t conn;

    // are we connected? 
    bool connected;

    //void SetReserved(int res) { reserved = res; }
    //int GetReserved() { return(reserved); }

    /** Flag set if data has just been read into this client.  If you
        use it, you must set it to false yourself after examining the data.
        */ 
    bool fresh; 

    /** The host of the Player server to which we are connected.
     */
    char hostname[256]; 

    /** The port of the Player server to which we are connected.
        */ 
    int port;
    
    // store the IP of the host, too - more efficient for
    // matching, etc, then the string
    struct in_addr hostaddr;

    /** The latest time received from the server.
       */
    struct timeval timestamp;

    /** List of ids for available devices. This list is villed in by 
     * GetDeviceList()
     */
    int id_count;
    player_device_id_t ids[PLAYER_MAX_DEVICES];
    char drivernames[PLAYER_MAX_DEVICES][PLAYER_MAX_DEVICE_STRING_LEN];

    // constructors
    
    /** Make a client and connect it as indicated.
        If @p hostname is omitted (or NULL) then the client will @e not
        be connected.  In that cast, call @p Connect() yourself later.
     */
    PlayerClient(const char* hostname=NULL,
                 const int port=PLAYER_PORTNUM,
                 const int protocol=PLAYER_TRANSPORT_TCP);

    /** Make a client and connect it as indicated, using a binary IP instead 
        of a hostname
      */
    PlayerClient(const struct in_addr* hostaddr,
                 const int port,
                 const int protocol=PLAYER_TRANSPORT_TCP);

    // destructor
    ~PlayerClient();

    /** Connect to the indicated host and port.
        Returns 0 on success; -1 on error.
     */
    int Connect(const char* hostname="localhost", int port=PLAYER_PORTNUM);

    /** Connect to the indicated host and port, using a binary IP.
        Returns 0 on success; -1 on error.
     */
    int Connect(const struct in_addr* addr, int port);

    /** Connect to a robot, based on its name, by using the Player robot name 
        service (RNS) on the indicated host and port.  Returns 0 on success; 
        -1 on error.
     */
    int ConnectRNS(const char* robotname, const char* hostname="localhost",
                   int port=PLAYER_PORTNUM);

    /** Disconnect from server.
        Returns 0 on success; -1 on error.
      */
    int Disconnect();

    /** Check if we are connected.
      */
    bool Connected() { return((conn.sock >= 0) ? true : false); }

    /**
    This method will read one round of data; that is, it will read until a 
    SYNC packet is received from the server.  Depending on which data delivery
    mode is in use, new data may or may not be received for each open device.
    The data that is received for each device device will be processed by the
    appropriate device proxy and stored there for access by your program.
    If no errors occurred 0 is returned.  Otherwise, -1 is returned and
    diagnostic information is printed to @p stderr (you should probably
    close the connection!).
    */
    int Read();
    
    /** Write a command to the server.  This method is @b not intended for
        direct use.  Rather, device proxies should implement higher-level 
        methods atop this one. Returns 0 on success, -1 otherwise. */
    int Write(player_device_id_t device_id,
              const char* command, size_t commandlen);

    /** Send a request to the server.  This method is @b not intended for
        direct use.  Rather, device proxies should implement higher-level 
        methods atop this one. Returns 0 on success, -1 otherwise. */
    int Request(player_device_id_t device_id,
                const char* payload,
                size_t payloadlen,
                player_msghdr_t* replyhdr,
                char* reply, size_t replylen);

    /** Another form of Request(), this one can be used if the caller is not
        interested in the reply.  This method is @b not intended for
        direct use.  Rather, device proxies should implement higher-level 
        methods atop this one. Returns 0 if an ACK is received, -1 
        otherwise. */
    int Request(player_device_id_t device_id,
                const char* payload,
                size_t payloadlen);
    
    /**
      Request access to a device; meant mostly for use by client-side device 
      proxy constructors.
      @p req_access is requested access.
      @p grant_access, if non-NULL, will be filled with the granted access.
      Returns 0 if everything went OK or -1 if something went wrong.
    */
    int RequestDeviceAccess(player_device_id_t device_id,
                            unsigned char req_access,
                            unsigned char* grant_access,
                            char* driver_name = NULL,
                            int driver_name_len = 0);

    // Player device configurations

    /**
      You can change the rate at which your client receives data from the 
      server with this method.  The value of @p freq is interpreted as Hz; 
      this will be the new rate at which your client receives data (when in 
      continuous mode).  On error, -1 is returned; otherwise 0.
     */
    int SetFrequency(unsigned short freq);

    /** You can toggle the mode in which the server sends data to your 
        client with this method.  The @p mode should be one of
          -  @p PLAYER_DATAMODE_PUSH_ALL (all data at fixed frequency)
          -  @p PLAYER_DATAMODE_PULL_ALL (all data on demand)
          -  @p PLAYER_DATAMODE_PUSH_NEW (only new new data at fixed freq)
          -  @p PLAYER_DATAMODE_PULL_NEW (only new data on demand)
          -  @p PLAYER_DATAMODE_PUSH_ASYNC (new data, as fast as it is produced)
        On error, -1 is returned; otherwise 0.
      */
    int SetDataMode(unsigned char mode);

    /** When in a @p PULL data delivery mode, you can request a single 
        round of data using this method.  On error -1 is returned; otherwise 0.
      */
    int RequestData();

    /** Attempt to authenticate your client using the provided key.  If 
        authentication fails, the server will close your connection.
      */
    int Authenticate(char* key);

    /** Documentation on LookupPort goes here
     */
    int LookupPort(const char* name);
    
    // proxy list management methods

    // add a proxy to the list
    void AddProxy(ClientProxy* proxy);
    // remove a proxy from the list
    void RemoveProxy(ClientProxy* proxy);

    // Get the list of available device ids. The data is written into the
    // proxy structure rather than retured to the caller.
    int GetDeviceList(); 
};

// forward declaration to avoid including <sys/poll.h>, which may not be
// available when people are building clients against this lib
struct pollfd;

/** 

The PlayerMultiClient makes it easy to control multiple Player connections
within one thread.   You can connect to any number of Player servers
and read from all of them with a single Read().

*/
class PlayerMultiClient
{
  private:
    // dynamically managed array of our clients
    PlayerClient** clients;
    //int num_clients;
    int size_clients;

    // dynamically managed array of structs that we'll give to poll(2)
    struct pollfd* ufds;
    int size_ufds;
    int num_ufds;

  public:
    /** Uninteresting constructor. 
    */
    PlayerMultiClient();
    
    // destructor
    ~PlayerMultiClient();

    /** How many clients are currently being managed? 
     */
    int GetNumClients(void) { return num_ufds; };

    /** Return a pointer to the client associated with the given host and port,
        or NULL if none is connected to that address. */
    PlayerClient* GetClient( char* host, int port );

    /** Return a pointer to the client associated with the given binary host 
        and port, or NULL if none is connected to that address. */
    PlayerClient* GetClient( struct in_addr* addr, int port );
    
    /** After creating and connecting a PlayerClient object, you should use 
        this method to hand it over to the PlayerMultiClient for management.
        */
    void AddClient(PlayerClient* client);
    
    /** Remove a client from PlayerMultiClient management.
     */
    void RemoveClient(PlayerClient* client);

    /** Read on one of the client connections.  This method will return after 
        reading from the server with first available data.  It will @b
        not read data from all servers.  You can use the @p fresh flag
        in each client object to determine who got new data.  You should
        then set that flag to false.  Returns 0 if everything went OK,
        -1 if something went wrong. */
    int Read();

    /** Same as Read(), but reads everything off the socket so we end
        up with the freshest data, subject to N maximum reads. */
    int ReadLatest( int max_reads );
};

/** @} (core) */

/** @defgroup player_clientlib_cplusplus_proxies Proxies */
/** @{ */

/** 

The @p AIOProxy class is used to read from a @ref player_interface_aio
(analog I/O) device.  */
class AIOProxy : public ClientProxy
{

public:
    /** Constructor.
        Leave the access field empty to start unconnected.
    */
    AIOProxy (PlayerClient* pc, unsigned short index,
                   unsigned char access = 'c')
            : ClientProxy(pc,PLAYER_AIO_CODE,index,access)
      {}

    ~AIOProxy()
      {}

    /// The number of valid digital inputs.
    char count;

    /// A bitfield of the current digital inputs.
    int anin[PLAYER_AIO_MAX_SAMPLES];

    // interface that all proxies must provide
    void FillData (player_msghdr_t hdr, const char* buffer);

    /// Print out the current digital input state.
    void Print ();
};


/** 

The @p GpsProxy class is used to control a @ref player_interface_gps
device.  The latest pose data is stored in three class attributes.  */
class GpsProxy : public ClientProxy
{
  public:
    
    /// Latitude and longitude, in degrees.
    double latitude;
    double longitude;

    /// Altitude, in meters.
    double altitude;

    /// Number of satellites in view.
    int satellites;

    /// Fix quality
    int quality;
    
    /// Horizontal dilution of position (HDOP)
    double hdop;

    /// Vertical dilution of position (HDOP)
    double vdop;

    /// UTM easting and northing (meters).
    double utm_easting;
    double utm_northing;

    /// Time, since the epoch
    struct timeval time;
   
    /** Constructor.  Leave the access field empty to start
        unconnected.*/
    GpsProxy(PlayerClient* pc, unsigned short index, 
              unsigned char access='c') :
            ClientProxy(pc,PLAYER_GPS_CODE,index,access) {}

    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    /// Print out current pose information.
    void Print();
};




/* Gripper stuff */
#define GRIPopen   1
#define GRIPclose  2
#define GRIPstop   3
#define LIFTup     4
#define LIFTdown   5
#define LIFTstop   6
#define GRIPstore  7
#define GRIPdeploy 8
#define GRIPhalt   15
#define GRIPpress  16
#define LIFTcarry  17

/** 

The @p GripperProxy class is used to control a @ref
player_interface_gripper device.  The latest gripper data held in a
handful of class attributes.  A single method provides user control.
*/
class GripperProxy : public ClientProxy
{

  public:
    
    /// The latest raw gripper data.
    unsigned char state,beams;

    /** These boolean variables indicate the state of the gripper
     */
    bool outer_break_beam,inner_break_beam,
         paddles_open,paddles_closed,paddles_moving,
         gripper_error,lift_up,lift_down,lift_moving,
         lift_error;

    /** The client calls this method to make a new proxy.  Leave access empty 
         to start unconnected. */
    GripperProxy(PlayerClient* pc, unsigned short index, 
                 unsigned char access='c') :
            ClientProxy(pc,PLAYER_GRIPPER_CODE,index,access) {}

    // these methods are the user's interface to this device

    /** Send a gripper command.  Look in the Player user manual for details
        on the command and argument.
        Returns 0 if everything's ok, and  -1 otherwise (that's bad).
    */
    int SetGrip(unsigned char cmd, unsigned char arg=0);

    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    /// Print out current gripper state.
    void Print();
};


/** 

The @p SoundProxy class is used to control a @ref player_interface_sound
device, which allows you to play pre-recorded sound files on a robot.
*/
class SoundProxy : public ClientProxy
{

  public:
    
    /** The client calls this method to make a new proxy.  Leave access empty 
         to start unconnected. */
    SoundProxy(PlayerClient* pc, unsigned short index, 
                 unsigned char access='c') :
            ClientProxy(pc,PLAYER_SOUND_CODE,index,access) {}

    // these methods are the user's interface to this device
    
    /** Play the sound indicated by the index.  Returns 0 on success, -1 on
        error.
     */
    int Play(int index);

    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer) {};
    
    /// Does nothing.
    void Print() {};
};


class FiducialItem
{
  public:
    int id;
    double pose[3];
    double upose[3];
};


/** 

The @p FiducialProxy class is used to control @ref
player_interface_fiducial devices.  The latest set of detected beacons
is stored in the @p beacons array.
*/
class FiducialProxy : public ClientProxy
{

  public:
  // the latest laser beacon data

  /** The number of beacons detected 
   */
  unsigned short count;

  /** The pose of the sensor [x,y,theta] in [m,m,rad]
   */
  double pose[3];

  /** The size of the sensor [x,y] in [m,m]
   */
  double size[2];

  /** The size of the most recently detected fiducial
  */
  double fiducial_size[2];

  /** the minimum range of the sensor in meters (partially defines the FOV)
   */
  double min_range;
  
  /** the maximum range of the sensor in meters (partially defines the FOV)
   */
  double max_range;

  /** the receptive angle of the sensor in degrees (partially defines the FOV)
   */
  double view_angle;

  /** The latest laser beacon data. */
  FiducialItem beacons[PLAYER_FIDUCIAL_MAX_SAMPLES];
     
  /** Constructor.  Leave the access field empty to start
      unconnected. */
  FiducialProxy(PlayerClient* pc, unsigned short index,
                unsigned char access='c') :
    ClientProxy(pc,PLAYER_FIDUCIAL_CODE,index,access) { count=0; }
    
  // interface that all proxies must provide
  void FillData(player_msghdr_t hdr, const char* buffer);
    
  /// Print out latest beacon data.
  void Print();

  /// Print the latest FOV configuration
  int PrintFOV();

  /// Print the latest geometry configuration
  int PrintGeometry();

  /// Get the sensor's geometry configuration
  int GetConfigure();
 
  /// Get the field of view 
  int GetFOV();

  /** Set the fiducial identification value displayed by this device
      (if supported) returns the value actually used by the device,
      which may differ from the one requested, or -1 on error. Also
      stores the returned id in the proxy's data member 'id'.
  */
  int SetId( int id );

  /** Get the fiducial identification value displayed by this device
      (if supported). returns the ID or -1 on error. Also stores the
      returned id in the proxy's data member 'id'.
  */
  int GetId( void );

  /** Set the field of view, updating the proxy with the actual values
      achieved. Params are: minimum range in meters, maximum range in
      meters, view angle in radians 
  */
  
  int SetFOV( double min_range, 
	      double max_range, 
	      double view_angle);
  
  /**  Attempt to send a message to a fiducial. See the Player manual
       for details of the message packet. Use a target_id of -1 to
       broadcast. If consume is true, the message is sent only
       once. If false, the message may be sent multiple times, but
       this is device dependent. Note: these message functions use
       configs that are probably only supported by Stage-1.4 (or
       later) fiducial driver.
  */

  int SendMessage( player_fiducial_msg_t* msg, bool consume );
  
  /** Read a message received by the device. If a message is available,
     the recv_msg packet is filled in and 0 is returned.  no message can
     be retrieved from the device, returns -1. If consume is true, the
     message is deleted from the device on reading. If false, the
     message is kept and can be read again. Note: these message
     functions use configs that are probably only supported by Stage-1.4
     (or later) fiducial driver.
  */

  int RecvMessage( player_fiducial_msg_t* msg, bool consume );

};



/** 

The @p LaserProxy class is used to control a @ref player_interface_laser
device.  The latest scan data is held in two arrays: @p ranges and @p
intensity.  The laser scan range, resolution and so on can be configured
using the Configure() method.  */
class LaserProxy : public ClientProxy
{

  public:

    /// Number of points in scan
    int scan_count;

    /** Angular resolution of scan (radians)
     */
    double scan_res;

    /** Scan range for the latest set of data (radians)
     */
    double min_angle, max_angle;
    
    /// Range resolution of scan (mm)
    double range_res;

    /// Whether or not reflectance (i.e., intensity) values are being returned.
    bool intensity;

    /// Scan data (polar): range (m) and bearing (radians)
    double scan[PLAYER_LASER_MAX_SAMPLES][2];

    /// Scan data (Cartesian): x,y (m)
    double point[PLAYER_LASER_MAX_SAMPLES][2];

    // TODO: haven't verified that intensities work yet:
    /// The reflected intensity values (arbitrary units in range 0-7).
    unsigned char intensities[PLAYER_LASER_MAX_SAMPLES];

    double min_right,min_left;
   
    /** Constructor.  Leave the access field empty to start
        unconnected. */
    LaserProxy(PlayerClient* pc, unsigned short index, 
               unsigned char access='c') :
        ClientProxy(pc,PLAYER_LASER_CODE,index,access) {}

    // these methods are the user's interface to this device

    /** Enable/disable the laser.
      Set @p state to 1 to enable, 0 to disable.
      Note that when laser is disabled the client will still receive laser
      data, but the ranges will always be the last value read from the
      laser before it was disabled.
      Returns 0 on success, -1 if there is a problem.
      @b Note: The @ref player_driver_sicklms200 driver currently does
      not implement this feature.
     */
    int SetLaserState(const unsigned char state);

    /** Configure the laser scan pattern.  Angles @p min_angle and
        @p max_angle are measured in radians.
        @p scan_res is measured in units of @f$0.01^{\circ}@f$;
        valid values are: 25 (@f$0.25^{\circ}@f$), 50 (@f$0.5^{\circ}@f$) and
        @f$100 (1^{\circ}@f$).  @p range_res is measured in mm; valid values
        are: 1, 10, 100.  Set @p intensity to @p true to
        enable intensity measurements, or @p false to disable.
        Returns the 0 on success, or -1 of there is a problem.  
     */
    int Configure(double min_angle, 
                  double max_angle, 
                  unsigned int scan_res,
                  unsigned int range_res, 
                  bool intensity);

    /** Get the current laser configuration; it is read into the
        relevant class attributes. Returns the 0 on success, or -1
        of there is a problem.  */
    int GetConfigure();

    /// Get the number of range/intensity readings.
    int RangeCount() { return scan_count; }

    /// An alternate way to access the range data.
    double Ranges (int index)
    {
      if (index < scan_count)
        return scan[index][0];
      else
        return 0;
    }
    double MinLeft () { return min_left; }
    double MinRight () { return min_right; }

    /** Range access operator.  This operator provides an alternate
        way of access the range data.  For example, given an @p
        LaserProxy named @p lp, the following expressions are
        equivalent: @p lp.ranges[0], @p lp.Ranges(0), 
        and @p lp[0].  */
    double operator [] (unsigned int index)
    {
      return Ranges(index);
    }
    
    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    /// Print out the current configuration and laser range/intensity data.
    void Print();

    /// Print out the current configuration
    void PrintConfig();
};


 
class localize_hypoth
{
  public:
    // Pose estimate (m, m, radians)
    double mean[3];
    // Covariance (m^2, radians^2)
    double cov[3][3];
    // Weight associated with this hypothesis
    double weight;
};

/** 

The @p LocalizeProxy class is used to control a @ref
player_interface_localize device, which can provide multiple pose
hypotheses for a robot.
*/
class LocalizeProxy : public ClientProxy
{

  public:
    /// Map dimensions (cells)
    unsigned int map_size_x, map_size_y;

    /// Map scale (m/cell)
    double map_scale;

    /// Map data (empty = -1, unknown = 0, occupied = +1)
    int8_t *map_cells;

    /// Number of pending (unprocessed) sensor readings
    int pending_count;
    
    /// Number of possible poses
    int hypoth_count;

    /** Array of possible poses. */
    localize_hypoth hypoths[PLAYER_LOCALIZE_MAX_HYPOTHS];


    /** Constructor.
        Leave the access field empty to start unconnected.
     */
    LocalizeProxy(PlayerClient* pc, unsigned short index, 
                  unsigned char access = 'c') :
            ClientProxy(pc,PLAYER_LOCALIZE_CODE,index,access)
    { map_cells=NULL; bzero(&hypoths,sizeof(hypoths)); }

    ~LocalizeProxy();

    // these methods are the user's interface to this device
    
    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);

    /** Set the current pose hypothesis (m, m, radians).  Returns 0 on 
        success, -1 on error.
     */
    int SetPose(double pose[3], double cov[3][3]);

    /** Get the number of particles (for particle filter-based localization
        systems).  Returns the number of particles, or -1 on error.
     */
    int GetNumParticles();

    // deprecated: get map from map interface instead
#if 0
    int GetMap();
#endif
    
    /// Print out current hypotheses.
    void Print();
};


/** 

The @p MotorProxy class is used to control a @ref player_interface_motor
device.  The latest motor data is contained in the attributes @p theta,
@p thetaspeed, etc. */
class MotorProxy : public ClientProxy
{
  private:
  /// Motor angle (according to odometry) in radians.
  double theta;

  /// Angular speeds in radians/sec.
  double thetaspeed;;

  /// Stall flag: 1 if the robot is stalled and 0 otherwise.
  unsigned char stall;

  public:
  /** Constructor.
      Leave the access field empty to start unconnected. */
  MotorProxy(PlayerClient* pc, unsigned short index,
	     unsigned char access ='c') :
    ClientProxy(pc,PLAYER_MOTOR_CODE,index,access) {}

  // these methods are the user's interface to this device

  /** Send a motor command for velocity control mode.
      Specify the angular speeds in rad/s.
      Returns: 0 if everything's ok, -1 otherwise.
  */
  int SetSpeed(double speed);

  /** Send a motor command for position control mode.  Specify the
      desired angle of the motor in radians. Returns: 0 if
      everything's ok, -1 otherwise.
  */
  int GoTo(double angle);


  /** Enable/disable the motors.
      Set @p state to 0 to disable or 1 to enable.
      Be VERY careful with this method!  Your robot is likely to run across the
      room with the charger still attached.
      Returns: 0 if everything's ok, -1 otherwise.
  */
  int SetMotorState(unsigned char state);

  /** Select velocity control mode.
      The motor state is typically device independent.  This is where you
      can potentially choose between different types of control methods
      (PD, PID, etc)
      Returns: 0 if everything's ok, -1 otherwise.
  */
  int SelectVelocityControl(unsigned char mode);

  /** Reset odometry to (0,0,0).
      Returns: 0 if everything's ok, -1 otherwise.
  */
  int ResetOdometry();

  /** Sets the odometry to the pose @p (theta).
      Note that @p theta is in radians.
      Returns: 0 if OK, -1 else
  */
  int SetOdometry(double angle);

  /** Select position mode on the motor driver.
      Set @p mode for 0 for velocity mode, 1 for position mode.
      Returns: 0 if OK, -1 else
  */

  int SelectPositionMode(unsigned char mode);

  /** Set the PID parameters of the motor for use in velocity control mode
      Returns: 0 if OK, -1 else
  */

  int SetSpeedPID(double kp, double ki, double kd);

  /** Set the PID parameters of the motor for use in position control mode
      Returns: 0 if OK, -1 else
  */

  int SetPositionPID(double kp, double ki, double kd);

  /** Set the ramp profile of the motor when in position control mode
      spd is in rad/s and acc is in rad/s/s
      Returns: 0 if OK, -1 else
  */

  int SetPositionSpeedProfile(double spd, double acc);

  /// Accessor method
  double  Theta () const { return theta; }

  /// Accessor method
  double  ThetaSpeed () const { return thetaspeed; }

  /// Accessor method
  unsigned char Stall () const { return stall; }

  // interface that all proxies must provide
  void FillData(player_msghdr_t hdr, const char* buffer);

  /// Print current motor device state.
  void Print();
};

/** 

The @p PositionProxy class is used to control a @ref
player_interface_position device.  The latest position data is contained
in the attributes xpos, ypos, etc.  */
class PositionProxy : public ClientProxy
{

  public:
  /// Robot pose (according to odometry) in m, m, radians.
  double xpos,ypos,theta;

  /// Robot speeds in m/sec, m/sec, radians/sec.
  double speed, sidespeed, turnrate;

  /// Stall flag: 1 if the robot is stalled and 0 otherwise.
  unsigned char stall;
   
  /** Constructor.
      Leave the access field empty to start unconnected. */
  PositionProxy(PlayerClient* pc, unsigned short index,
                unsigned char access ='c') :
    ClientProxy(pc,PLAYER_POSITION_CODE,index,access) {}

  // these methods are the user's interface to this device

  /** Send a motor command for velocity control mode.
      Specify the forward, sideways, and angular speeds in m/sec, m/sec,
      and radians/sec, respectively.  Returns: 0 if everything's ok, 
      -1 otherwise.
  */
  int SetSpeed(double speed, double sidespeed, double turnrate);

  /** Same as the previous SetSpeed(), but doesn't take the sideways speed 
      (so use this one for non-holonomic robots). */
  int SetSpeed(double speed, double turnrate)
      { return(SetSpeed(speed,0.0,turnrate));}

  /** Send a motor command for position control mode.  Specify the
      desired pose of the robot in m, m, radians. Returns: 0 if
      everything's ok, -1 otherwise.  
  */
  int GoTo(double x, double y, double t);


  /** Enable/disable the motors.
      Set @p state to 0 to disable or 1 to enable.
      Be VERY careful with this method!  Your robot is likely to run across the
      room with the charger still attached.
      Returns: 0 if everything's ok, -1 otherwise.
  */
  int SetMotorState(unsigned char state);
    
  /** Select velocity control mode.
  
      For the the p2os_position driver, set @p mode to 0 for direct wheel 
      velocity control (default), or 1 for separate translational and 
      rotational control.

      For the reb_position driver: 0 is direct velocity control, 1 is for 
      velocity-based heading PD controller (uses DoDesiredHeading()).

      Returns: 0 if everything's ok, -1 otherwise.
  */
  int SelectVelocityControl(unsigned char mode);
   
  /** Reset odometry to (0,0,0).
      Returns: 0 if everything's ok, -1 otherwise.
  */
  int ResetOdometry();

  // the following ioctls are currently only supported by reb_position
  //

  /** Select position mode on the reb_position driver.
      Set @p mode for 0 for velocity mode, 1 for position mode.
      Returns: 0 if OK, -1 else
  */
  int SelectPositionMode(unsigned char mode);

  /** Sets the odometry to the pose @p (x, y, theta).
      Note that @p x and @p y are in m and @p theta is in radians.
      Returns: 0 if OK, -1 else
  */
  int SetOdometry(double x, double y, double t);

  /// Only supported by the reb_position driver.
  int SetSpeedPID(int kp, int ki, int kd);

  /// Only supported by the reb_position driver.
  int SetPositionPID(short kp, short ki, short kd);

  /// Only supported by the reb_position driver.
  int SetPositionSpeedProfile(short spd, short acc);

  /// Only supported by the reb_position driver.
  int DoStraightLine(int mm);

  /// Only supported by the reb_position driver.
  int DoRotation(int deg);

  /// Only supported by the reb_position driver.
  int DoDesiredHeading(int theta, int xspeed, int yawspeed);

  /// Only supported by the segwayrmp driver
  int SetStatus(uint8_t cmd, uint16_t value);

  /// Only supported by the segwayrmp driver
  int PlatformShutdown();

  /// Accessor method
  double  Xpos () const { return xpos; }
  
  /// Accessor method
  double  Ypos () const { return ypos; }
  
  /// Accessor method
  double Theta () const { return theta; }
  
  /// Accessor method
  double  Speed () const { return speed; }

  /// Accessor method
  double  SideSpeed () const { return sidespeed; }

  /// Accessor method
  double  TurnRate () const { return turnrate; }
  
  /// Accessor method
  unsigned char Stall () const { return stall; }

  // interface that all proxies must provide
  void FillData(player_msghdr_t hdr, const char* buffer);
    
  /// Print current position device state.
  void Print();
};


/** 

The @p Position2DProxy class is used to control a @ref
player_interface_position2d device.  The latest position data is contained
in the attributes xpos, ypos, etc.  */
class Position2DProxy : public ClientProxy
{
  private:
  /// Robot pose (according to odometry) in m, m, radians.
  double xpos,ypos,yaw;

  /// Robot speeds in m/sec, m/sec, radians/sec.
  double xspeed, yspeed, yawspeed;

  /// Stall flag: 1 if the robot is stalled and 0 otherwise.
  unsigned char stall;

  public:
  /** Constructor.
      Leave the access field empty to start unconnected. */
  Position2DProxy(PlayerClient* pc, unsigned short index,
                unsigned char access ='c') :
    ClientProxy(pc,PLAYER_POSITION_CODE,index,access) {}

  // these methods are the user's interface to this device

  /** Send a motor command for velocity control mode.
      Specify the forward, sideways, and angular speeds in m/sec, m/sec,
      and radians/sec, respectively.  Returns: 0 if everything's ok,
      -1 otherwise.
  */
  int SetSpeed(double xspeed, double yspeed, double yawspeed);

  /** Same as the previous SetSpeed(), but doesn't take the yspeed speed
      (so use this one for non-holonomic robots). */
  int SetSpeed(double speed, double turnrate)
      { return(SetSpeed(speed,0.0,turnrate));}

  /** Send a motor command for position control mode.  Specify the
      desired pose of the robot in m, m, radians. Returns: 0 if
      everything's ok, -1 otherwise.
  */
  int GoTo(double x, double y, double yaw);


  /** Enable/disable the motors.
      Set @p state to 0 to disable or 1 to enable.
      Be VERY careful with this method!  Your robot is likely to run across the
      room with the charger still attached.
      Returns: 0 if everything's ok, -1 otherwise.
  */
  int SetMotorState(unsigned char state);

  /** Select velocity control mode.

      For the the p2os_position driver, set @p mode to 0 for direct wheel
      velocity control (default), or 1 for separate translational and
      rotational control.

      For the reb_position driver: 0 is direct velocity control, 1 is for
      velocity-based heading PD controller (uses DoDesiredHeading()).

      Returns: 0 if everything's ok, -1 otherwise.
  */
  int SelectVelocityControl(unsigned char mode);

  /** Reset odometry to (0,0,0).
      Returns: 0 if everything's ok, -1 otherwise.
  */
  int ResetOdometry();

  // the following ioctls are currently only supported by reb_position
  //

  /** Select position mode on the reb_position driver.
      Set @p mode for 0 for velocity mode, 1 for position mode.
      Returns: 0 if OK, -1 else
  */
  int SelectPositionMode(unsigned char mode);

  /** Sets the odometry to the pose @p (x, y, yaw).
      Note that @p x and @p y are in m and @p yaw is in radians.
      Returns: 0 if OK, -1 else
  */
  int SetOdometry(double x, double y, double yaw);

  /// Only supported by the reb_position driver.
  int SetSpeedPID(double kp, double ki, double kd);

  /// Only supported by the reb_position driver.
  int SetPositionPID(double kp, double ki, double kd);

  /// Only supported by the reb_position driver.
  /// spd rad/s, acc rad/s/s
  int SetPositionSpeedProfile(double spd, double acc);

  /// Only supported by the reb_position driver.
  int DoStraightLine(double m);

  /// Only supported by the reb_position driver.
  int DoRotation(double yawspeed);

  /// Only supported by the reb_position driver.
  int DoDesiredHeading(double yaw, double xspeed, double yawspeed);

  /// Only supported by the segwayrmp driver
  int SetStatus(uint8_t cmd, uint16_t value);

  /// Only supported by the segwayrmp driver
  int PlatformShutdown();

  /// Accessor method
  double  Xpos () const { return xpos; }

  /// Accessor method
  double  Ypos () const { return ypos; }

  /// Accessor method
  double Yaw () const { return yaw; }

  // Speed, SideSpeed, and TurnRate are left for backwards compatibility, but
  // could probably be dropped.

  /// Accessor method
  double  Speed () const { return xspeed; }
  /// Accessor method
  double  SideSpeed () const { return yspeed; }
  /// Accessor method
  double  TurnRate () const { return yawspeed; }


  /// Accessor method
  double  XSpeed () const { return xspeed; }

  /// Accessor method
  double  YSpeed () const { return yspeed; }

  /// Accessor method
  double  YawSpeed () const { return yawspeed; }


  /// Accessor method
  unsigned char Stall () const { return stall; }

  // interface that all proxies must provide
  void FillData(player_msghdr_t hdr, const char* buffer);

  /// Print current position device state.
  void Print();
};

/** 

The @p Position3DProxy class is used to control
a player_interface_position3d device.  The latest position data is
contained in the attributes xpos, ypos, etc.
*/
class Position3DProxy : public ClientProxy
{
  private:

  /// Robot pose (according to odometry) in m, rad.
  double xpos,ypos,zpos;
  double roll,pitch,yaw;

  /// Robot speeds in m/sec, rad/sec
  double xspeed, yspeed, zspeed;
  double rollspeed, pitchspeed, yawspeed;

  /// Stall flag: 1 if the robot is stalled and 0 otherwise.
  unsigned char stall;

  public:

  /** Constructor.
      Leave the access field empty to start unconnected. */
  Position3DProxy(PlayerClient* pc, unsigned short index,
                  unsigned char access ='c') :
    ClientProxy(pc,PLAYER_POSITION3D_CODE,index,access) {}

  // these methods are the user's interface to this device

  /** Send a motor command for a planar robot.
      Specify the forward, sideways, and angular speeds in m/s, m/s, m/s,
      rad/s, rad/s, and rad/s, respectively.
      Returns: 0 if everything's ok,
      -1 otherwise.
  */
  int SetSpeed(double xspeed, double yspeed, double zspeed,
	       double rollspeed, double pitchspeed, double yawspeed);

  /** Send a motor command for a planar robot.
      Specify the forward, sideways, and angular speeds in m/s, m/s,
      and rad/s, respectively.  Returns: 0 if everything's ok,
      -1 otherwise.
  */
  int Position3DProxy::SetSpeed(double xspeed,double yspeed,
				double zspeed,double yawspeed)
  { return(SetSpeed(xspeed,yspeed,zspeed,0,0,yawspeed)); }

  int SetSpeed(double xspeed, double yspeed, double yawspeed)
    {  return SetSpeed(xspeed, yspeed, 0, 0, 0, yawspeed); }

  /** Same as the previous SetSpeed(), but doesn't take the sideways speed
      (so use this one for non-holonomic robots). */
  int SetSpeed(double xspeed, double yawspeed)
      { return(SetSpeed(xspeed,0,0,0,0,yawspeed));}


  /** Send a motor command for position control mode.  Specify the
      desired pose of the robot in m, m, m, rad, rad, rad.
      Returns: 0 if everything's ok, -1 otherwise.
  */
  int GoTo(double x, double y, double z,
	   double roll, double pitch, double yaw);

  /** Enable/disable the motors.
      Set @p state to 0 to disable or 1 to enable.
      Be VERY careful with this method!  Your robot is likely to run across the
      room with the charger still attached.
      Returns: 0 if everything's ok, -1 otherwise.
  */
  int SetMotorState(unsigned char state);

  /** Select velocity control mode.

      This is driver dependent.
      Returns: 0 if everything's ok, -1 otherwise.
  */
  int SelectVelocityControl(unsigned char mode);

  /** Reset odometry to (0,0,0).
      Returns: 0 if everything's ok, -1 otherwise.
  */
  int ResetOdometry();

  /** Sets the odometry to the pose @p (x, y, z, roll, pitch, yaw).
      Note that @p x, @p y, and @p z are in m and @p roll,
      @p pitch, and @p yaw are in radians.

      Returns: 0 if OK, -1 else
  */
  int SetOdometry(double x, double y, double z,
		  double roll, double pitch, double yaw);

  /** Select position mode
      Set @p mode for 0 for velocity mode, 1 for position mode.
      Returns: 0 if OK, -1 else
  */
  int SelectPositionMode(unsigned char mode);

  /// Only supported by the reb_position driver.
  int SetSpeedPID(double kp, double ki, double kd);

  /// Only supported by the reb_position driver.
  int SetPositionPID(double kp, double ki, double kd);

  /// Sets the ramp profile for position based control
  /// spd rad/s, acc rad/s/s
  int SetPositionSpeedProfile(double spd, double acc);

  // interface that all proxies must provide
  void FillData(player_msghdr_t hdr, const char* buffer);

  /// Print current position device state.
  void Print();

  /// Accessor method
  double  Xpos() const { return xpos; }

  /// Accessor method
  double  Ypos() const { return ypos; }

  /// Accessor method
  double  Zpos() const { return zpos; }

  /// Accessor method
  double  Roll() const { return roll; }

  /// Accessor method
  double  Pitch() const { return pitch; }

  /// Accessor method
  double  Yaw() const { return yaw; }

  /// Accessor method
  double  XSpeed() const { return xspeed; }

  /// Accessor method
  double  YSpeed() const { return yspeed; }

  /// Accessor method
  double  ZSpeed() const { return zspeed; }

  /// Accessor method
  double  RollSpeed() const { return rollspeed; }

  /// Accessor method
  double  PitchSpeed() const { return pitchspeed; }

  /// Accessor method
  double  YawSpeed() const { return yawspeed; }

  /// Accessor method
  unsigned char Stall () const { return stall; }
};

/** 

The @p PtzProxy class is used to control a @ref player_interface_ptz
device.  The state of the camera can be read from the pan, tilt, zoom
attributes and changed using the SetCam() method.
*/
class PtzProxy : public ClientProxy
{
  public:
    
    /// Pan, tilt, and field of view values (all radians).
    double pan, tilt, zoom;
    /// Pan and tilt speeds (rad/sec)
    double panspeed, tiltspeed;

    /** Constructor.
        Leave the access field empty to start unconnected.
    */
    PtzProxy(PlayerClient* pc, unsigned short index, 
             unsigned char access='c') :
            ClientProxy(pc,PLAYER_PTZ_CODE,index,access) {}

    // these methods are the user's interface to this device

    /** Change the camera state.
        Specify the new @p pan, @p tilt, and @p zoom values 
        (all degrees).
        Returns: 0 if everything's ok, -1 otherwise.
    */
    int SetCam(double pan, double tilt, double zoom);

    /** Specify new target velocities */
    int SetSpeed(double panspeed, double tiltspeed);

    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    /// Send a camera-specific config
    int SendConfig(uint8_t *bytes, size_t len, uint8_t *reply = NULL, 
                   size_t reply_len = 0);
    
    /** Select new control mode.  Use either PLAYER_PTZ_POSITION_CONTROL
        or PLAYER_PTZ_VELOCITY_CONTROL. */
    int SelectControlMode(uint8_t mode);

    /// Print out current ptz state.
    void Print();
};


/** 

The @p SonarProxy class is used to control a @ref player_interface_sonar
device.  The most recent sonar range measuremts can be read from the
range attribute, or using the the [] operator.
*/
class SonarProxy : public ClientProxy
{

  public:
    /** The number of sonar readings received.
     */
    unsigned short range_count;

    /** The latest sonar scan data.
        Range is measured in m.
     */
    double ranges[PLAYER_SONAR_MAX_SAMPLES];

    /** Number of valid sonar poses
     */
    int pose_count;

    /** Sonar poses (m,m,radians)
     */
    double poses[PLAYER_SONAR_MAX_SAMPLES][3];
   
    /** Constructor.
        Leave the access field empty to start unconnected.
    */
    SonarProxy(PlayerClient* pc, unsigned short index, 
               unsigned char access = 'c') :
            ClientProxy(pc,PLAYER_SONAR_CODE,index,access)
    { 
      memset(&poses,0,sizeof(poses)); 
      range_count=pose_count=0;
    }

    // these methods are the user's interface to this device
    
    /** Enable/disable the sonars.
        Set @p state to 1 to enable, 0 to disable.
        Note that when sonars are disabled the client will still receive sonar
        data, but the ranges will always be the last value read from the sonars
        before they were disabled.

        Returns 0 on success, -1 if there is a problem.
     */
    int SetSonarState(unsigned char state);

    /// Request the sonar geometry.
    int GetSonarGeom();

    /** Range access operator.
        This operator provides an alternate way of access the range data.
        For example, given a @p SonarProxy named @p sp, the following
        expressions are equivalent: @p sp.ranges[0] and @p sp[0].
     */
    double operator [](unsigned int index) 
    { 
      if(index < sizeof(ranges))
        return(ranges[index]);
      else 
        return(0);
    }

    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    /// Print out current sonar range data.
    void Print();
};

/** 

The @p SpeechProxy class is used to control a @ref player_interface_speech
device.  Use the say method to send things to say.
*/
class SpeechProxy : public ClientProxy
{

  public:
   
    /** Constructor.
        Leave the access field empty to start unconnected.
    */
    SpeechProxy(PlayerClient* pc, unsigned short index, 
                unsigned char access='c') :
            ClientProxy(pc,PLAYER_SPEECH_CODE,index,access) {}

    // these methods are the user's interface to this device

    /** Send a phrase to say.
        The phrase is an ASCII string.
        Returns the 0 on success, or -1 of there is a problem.
    */
    int Say(char* str);
};

/** 

The @p TruthProxy gets and sets the @e true pose of a @ref
player_interface_truth device [worldfile tag: truth()]. This may be
different from the pose returned by a device such as GPS or Position. If
you want to log what happened in an experiment, this is the device to use.

Setting the position of a truth device moves its parent, so you
can put a truth device on robot and teleport it around the place. 
*/
class TruthProxy : public ClientProxy
{
  
  public:

  /** These vars store the current device position (x,y,z) in m and
      orientation (roll, pitch, yaw) in radians. The values are
      updated at regular intervals as data arrives. You can read these
      values directly but setting them does NOT change the device's
      pose!. Use TruthProxy::SetPose() for that.  */
  double px, py, pz, rx, ry, rz; 

  /** Constructor.
      Leave the access field empty to start unconnected.  */
  TruthProxy(PlayerClient* pc, unsigned short index, 
             unsigned char access = 'c') :
    ClientProxy(pc,PLAYER_TRUTH_CODE,index,access) {};

    
  // interface that all proxies must provide
  void FillData(player_msghdr_t hdr, const char* buffer);
    
  /// Print out current pose info in a format suitable for data logging.
  void Print();

  /** Query Player about the current pose - requests the pose from the
      server, then fills in values for the arguments. Usually you'll
      just read the class attributes but this function allows you
      to get pose direct from the server if you need too. Returns 0 on
      success, -1 if there is a problem.
  */
  int GetPose( double *px, double *py, double *pz,
               double *rx, double *ry, double *rz );

  /** Request a change in pose. Returns 0 on success, -1
      if there is a problem.  
  */
  int SetPose( double px, double py, double pz,
               double rx, double ry, double rz);

  /** ???
   */
  int SetPoseOnRoot(  double px, double py, double pz,
                      double rx, double ry, double rz);

  /** Request the value returned by a fiducialfinder (and possibly a
      foofinser, depending on its mode), when detecting this
      object. */
  int GetFiducialID( int16_t* id );

  /** Set the value returned by a fiducialfinder (and possibly a
      foofinser, depending on its mode), when detecting this
      object. */
  int SetFiducialID( int16_t id );

};

class Blob
{
  public:
    unsigned int id;
    unsigned int color;
    unsigned int area;
    unsigned short x;
    unsigned short y;
    unsigned short left;
    unsigned short right;
    unsigned short top;
    unsigned short bottom;
    double range;
};

/** 

The @p BlobfinderProxy class is used to control a  @ref
player_interface_blobfinder device.  It contains no methods.  The latest
color blob data is stored in @p blobs, a dynamically allocated 2-D array,
indexed by color channel.
*/
class BlobfinderProxy : public ClientProxy
{

  public:
    
    /// Dimensions of the camera image, in pixels
    unsigned short width, height;

    /** number of blobs */
    int blob_count;
    /** Array containing arrays of the latest blob data. */
    Blob blobs[PLAYER_BLOBFINDER_MAX_BLOBS];
   
    /** Constructor.
        Leave the access field empty to start unconnected.
    */
    BlobfinderProxy(PlayerClient* pc, unsigned short index, 
                unsigned char access='c');
    ~BlobfinderProxy();

    // these methods are the user's interface to this device

    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    /// Print out current blob information.
    void Print();

    int SetTrackingColor();
    int SetTrackingColor(int rmin, int rmax, int gmin,
                         int gmax, int bmin, int bmax);
    int SetImagerParams(int contrast, int brightness,
                         int autogain, int colormode);
    int SetContrast(int c);
    int SetColorMode(int m);
    int SetBrightness(int b);
    int SetAutoGain(int g);

};


// these define default coefficients for our 
// range and standard deviation estimates
#define IRPROXY_DEFAULT_DIST_M_VALUE -0.661685227
#define IRPROXY_DEFAULT_DIST_B_VALUE  10.477102515

#define IRPROXY_DEFAULT_STD_M_VALUE  1.913005560938
#define IRPROXY_DEFAULT_STD_B_VALUE -7.728130591833

#define IRPROXY_M_PARAM 0
#define IRPROXY_B_PARAM 1

//this is the effective range of the sensor in mm
#define IRPROXY_MAX_RANGE 700

/** 

The @p IRProxy class is used to control an @ref player_interface_ir
device.
*/
class IRProxy : public ClientProxy
{
public:

  /// Latest range readings
  unsigned short ranges[PLAYER_IR_MAX_SAMPLES];
  
  /// Latest voltage readings
  unsigned short voltages[PLAYER_IR_MAX_SAMPLES];

  /// Standard deviations
  double stddev[PLAYER_IR_MAX_SAMPLES];

  /// Distance regression params
  double params[PLAYER_IR_MAX_SAMPLES][2]; 

  /// Standard deviation regression params
  double sparams[PLAYER_IR_MAX_SAMPLES][2]; 

  /** Poses of the IRs. */
  player_ir_pose_t ir_pose;

  /** Constructor.  Leave the access field empty to start unconnected.  */
  IRProxy(PlayerClient *pc, unsigned short index,
          unsigned char access = 'c');

  /// Enable/disable the IRs.
  int SetIRState(unsigned char state);

  /// Request the poses of the IRs.
  int GetIRPose();

  /// Set range parameters.
  void SetRangeParams(int which, double m, double );

  /// Set standard deviation parameters.
  void SetStdDevParams(int which, double m, double b);

  /// Calculate standard deviations.
  double CalcStdDev(int w, unsigned short range);

    /** Range access operator.
        This operator provides an alternate way of access the range data.
        For example, given a @p IRProxy named @p ip, the following
        expressions are equivalent: @p ip.ranges[0] and @p ip[0].
     */
  unsigned short operator [](unsigned int index) 
  {
    if (index < PLAYER_IR_MAX_SAMPLES) {
      return ranges[index];
    } 

    return 0;
  }

  // required methods
  void FillData(player_msghdr_t hdr, const char *buffer);

  /// Print out current IR data.
  void Print();
};

/** 

The @p WiFiProxy class controls a @ref player_interface_wifi device.  */
class WiFiProxy: public ClientProxy
{
public:


    /** Constructor.
        Leave the access field empty to start unconnected.
    */
  WiFiProxy(PlayerClient *pc, unsigned short index, 
	       unsigned char access = 'c') :
    ClientProxy(pc, PLAYER_WIFI_CODE, index, access), link_count(0) {}


  int GetLinkQuality(char * ip = NULL);
  int GetLevel(char * ip = NULL);
  int GetLeveldBm(char * ip = NULL) { return GetLevel(ip) - 0x100; }
  int GetNoise(char * ip = NULL);
  int GetNoisedBm(char * ip = NULL) { return GetNoise(ip) - 0x100; }

  uint16_t GetMaxLinkQuality() { return maxqual; }
  uint8_t GetMode() { return op_mode; }

  int GetBitrate();

  char * GetMAC(char *buf, int len);

  char * GetIP(char *buf, int len);
  char * GetAP(char *buf, int len);

  int AddSpyHost(char *address);
  int RemoveSpyHost(char *address);

  void FillData(player_msghdr_t hdr, const char *buffer);

  /// Print out current data.
  void Print();

protected:
  int GetLinkIndex(char *ip);

  /// The current wifi data.
  int link_count;
  player_wifi_link_t links[PLAYER_WIFI_MAX_LINKS];
  uint32_t throughput;
  uint8_t op_mode;
  int32_t bitrate;
  uint16_t qual_type, maxqual, maxlevel, maxnoise;
  
  char access_point[32];

};

/** 

The @p PowerProxy class controls a @ref player_interface_power device. */
class PowerProxy : public ClientProxy 
{

  public:
    /** Constructor.
      Leave the access field empty to start unconnected.
     */
    PowerProxy (PlayerClient* pc, unsigned short index,
                unsigned char access ='c')
            : ClientProxy(pc,PLAYER_POWER_CODE,index,access) {}

    /// Returns the current charge.
    double Charge () const { return charge; }

    // interface that all proxies must provide
    void FillData (player_msghdr_t hdr, const char* buffer);

    /// Print the current data.
    void Print ();

  private:
    // Remaining power in volts
    double charge;
};


/** 

The @p AudioProxy class controls an @ref player_interface_audio device.
*/
class AudioProxy : public ClientProxy 
{

  public:
    /** Hz, db ? */
    uint16_t frequency0, amplitude0;
    /** Hz, db ? */
    uint16_t frequency1, amplitude1;
    /** Hz, db ? */
    uint16_t frequency2, amplitude2;
    /** Hz, db ? */
    uint16_t frequency3, amplitude3;
    /** Hz, db ? */
    uint16_t frequency4, amplitude4;

    /** Constructor.
      Leave the access field empty to start unconnected.
     */
    AudioProxy (PlayerClient* pc, unsigned short index,
                unsigned char access ='c')
            : ClientProxy(pc,PLAYER_AUDIO_CODE,index,access) {}

    // interface that all proxies must provide
    void FillData (player_msghdr_t hdr, const char* buffer);

    /// Play a fixed-frequency tone
    int PlayTone(unsigned short freq, unsigned short amp, unsigned short dur);

    /// Print the current data.
    void Print ();
};

/** 

The @p AudioDSPProxy class controls an @ref player_interface_audiodsp device.
*/
class AudioDSPProxy : public ClientProxy 
{

  public:
    /** Format code of each sample */
    int16_t sampleFormat;

    /** Rate at which to sample (Hz) */
    uint16_t sampleRate;

    /** Number of channels */
    uint8_t channels;

    uint16_t freq[5];
    uint16_t amp[5];

    /** Constructor.
      Leave the access field empty to start unconnected.
     */
    AudioDSPProxy (PlayerClient* pc, unsigned short index,
                unsigned char access ='c')
            : ClientProxy(pc,PLAYER_AUDIODSP_CODE,index,access) {}

    int Configure(uint8_t channels, uint16_t sampleRate, 
        int16_t sampleFormat=0xFFFFFFFF );

    int GetConfigure();

    // interface that all proxies must provide
    void FillData (player_msghdr_t hdr, const char* buffer);

    /// Play a fixed-frequency tone
    int PlayTone(unsigned short freq, unsigned short amp, unsigned int dur);
    int PlayChirp(unsigned short freq, unsigned short amp, unsigned int dur,
        const unsigned char bitString[], unsigned short bitStringLen);
    int Replay();

    /// Print the current data.
    void Print ();
};

/** 

The @p AudioMixerProxy class controls an @ref player_interface_audiomixer device.
*/
class AudioMixerProxy : public ClientProxy 
{

  public:
    unsigned short masterLeft, masterRight;
    unsigned short pcmLeft, pcmRight;
    unsigned short lineLeft, lineRight;
    unsigned short micLeft, micRight;
    unsigned short iGain, oGain;

    /** Constructor.
      Leave the access field empty to start unconnected.
     */
    AudioMixerProxy (PlayerClient* pc, unsigned short index,
                unsigned char access ='c')
            : ClientProxy(pc,PLAYER_AUDIOMIXER_CODE,index,access) {}

    int GetConfigure();

    // interface that all proxies must provide
    void FillData (player_msghdr_t hdr, const char* buffer);

    int SetMaster(unsigned short left, unsigned short right);
    int SetPCM(unsigned short left, unsigned short right);
    int SetLine(unsigned short left, unsigned short right);
    int SetMic(unsigned short left, unsigned short right);
    int SetIGain(unsigned short gain);
    int SetOGain(unsigned short gain);

    // Print the current data.
    void Print ();
};

/** 

The @p BumperProxy class is used to read from a @ref
player_interface_bumper device.
*/
class BumperProxy : public ClientProxy 
{

public:
    /** Constructor.
        Leave the access field empty to start unconnected.
    */
    BumperProxy (PlayerClient* pc, unsigned short index,
                   unsigned char access = 'c')
            : ClientProxy(pc,PLAYER_BUMPER_CODE,index,access) 
      {}

    ~BumperProxy()
      {}

    // these methods are the user's interface to this device

    /** Returns 1 if the specified bumper has been bumped, 0 otherwise.
      */
    bool Bumped (const unsigned int i);

    /** Returns 1 if any bumper has been bumped, 0 otherwise.
      */
    bool BumpedAny ();

    /** Requests the geometries of the bumpers.
        Returns -1 if anything went wrong, 0 if OK
     */ 
    int GetBumperGeom( player_bumper_geom_t* bumper_defs );

    /// Returns the number of bumper readings.
    uint8_t BumperCount () const { return bumper_count; }

    // interface that all proxies must provide
    void FillData (player_msghdr_t hdr, const char* buffer);

    /// Print out the current bumper state.
    void Print ();

private:
    /** array representing bumped state.
     */
    uint8_t bumper_count;
    uint8_t bumpers[PLAYER_BUMPER_MAX_SAMPLES];
};

/** 

The @p DIOProxy class is used to read from a @ref player_interface_dio
(digital I/O) device.
*/
class DIOProxy : public ClientProxy 
{

public:
    /** Constructor.
        Leave the access field empty to start unconnected.
    */
    DIOProxy (PlayerClient* pc, unsigned short index,
                   unsigned char access = 'c')
            : ClientProxy(pc,PLAYER_DIO_CODE,index,access) 
      {}

    ~DIOProxy()
      {}

    /// The number of valid digital inputs.
    uint8_t count;

    /// A bitfield of the current digital inputs.
    uint32_t digin;

    /// Set the output
    int SetOutput(uint8_t output_count, uint32_t digout);
    
    // interface that all proxies must provide
    void FillData (player_msghdr_t hdr, const char* buffer);

    /// Print out the current digital input state.
    void Print ();
};
 
/** 

The @p WaveformProxy class is used to read raw digital waveforms from
a @ref player_interface_waveform device.  */
class WaveformProxy : public ClientProxy 
{

public:
    /** Constructor.
        Leave the access field empty to start unconnected.
    */
  WaveformProxy (PlayerClient* pc, unsigned short index,
		 unsigned char access = 'c')
    : ClientProxy(pc,PLAYER_WAVEFORM_CODE,index,access) 
    {
      this->ConfigureDSP(); // use latest settings	
    }
  
  ~WaveformProxy();
  
    /// sample rate in bits per second
    unsigned int bitrate;

    /// sample depth in bits
    unsigned short depth;

    //// the number of samples in the most recent packet
    unsigned int last_samples;

    /// the data is buffered here for playback
    unsigned char buffer[PLAYER_WAVEFORM_DATA_MAX];
  
    /// dsp file descriptor
    int fd; 
  
    // interface that all proxies must provide
    void FillData (player_msghdr_t hdr, const char* buffer);

    /// Print out the current status
    void Print ();

    // set up the DSP to the current bitrate and depth
    int ConfigureDSP() ;

    // open the sound device 
    void OpenDSPforWrite();

    // Play the waveform through the DSP
    void Play();

};

/** 

The @p MComProxy class is used to exchange data with other clients
connected with the same server, through a set of named "channels" in
a @ref player_interface_mcom device.  For some useful (but optional)
type and constant definitions that you can use in your clients, see
playermcomtypes.h.
*/
class MComProxy : public ClientProxy 
{

public:
    /** These members contain the results of the last command.
        Note: It's better to use the LastData() method. */   
    player_mcom_data_t data;
    int type;
    char channel[MCOM_CHANNEL_LEN];

public:
    MComProxy(PlayerClient* pc, unsigned short index, 
              unsigned char access = 'c') :
            ClientProxy(pc,PLAYER_MCOM_CODE,index,access){}

    /** Read and remove the most recent buffer in 'channel' with type 'type'.
        The result can be read with LastData() after the next call to 
        PlayerClient::Read().
        @return 0 if no error
        @return -1 on error, the channel does not exist, or the channel is empty.
    */
    int Pop(int type, char channel[MCOM_CHANNEL_LEN]);

    /** Read the most recent buffer in 'channel' with type 'type'.
        The result can be read with LastData() after the next call to
        PlayerClient::Read().
        @return 0 if no error
        @return -1 on error, the channel does not exist, or the channel is empty.
    */
    int Read(int type, char channel[MCOM_CHANNEL_LEN]);

    /** Push a message 'dat' into channel 'channel' with message type 'type'. */
    int Push(int type, char channel[MCOM_CHANNEL_LEN], char dat[MCOM_DATA_LEN]);

    /** Clear all messages of type 'type' on channel 'channel' */
    int Clear(int type, char channel[MCOM_CHANNEL_LEN]);

  /** Set the capacity of the buffer using 'type' and 'channel' to 'cap'.  
      Note that 'cap' is an unsigned char and must be < MCOM_N_BUFS */
  int SetCapacity(int type, char channel[MCOM_CHANNEL_LEN], unsigned char cap);

    /** Get the results of the last command (Pop or Read). Call
        PlayerClient::Read() before using.  */
    char* LastData() { return data.data; }

    /** Get the results of the last command (Pop or Read). Call
        PlayerClient::Read() before using.  */
    int LastMsgType() { return type; }

    /** Get the channel of the last command (Pop or Read). Call
        PlayerClient::Read() before using.  */
    char* LastChannel() { return channel; }

    void FillData(player_msghdr_t hdr, const char* buffer);
    void Print();
};

/** 

The @p BlinkenlightProxy class is used to enable and disable
a flashing indicator light, and to set its period, via a @ref
player_interface_blinkenlight device */
class BlinkenlightProxy : public ClientProxy 
{

public:
    /** Constructor.
        Leave the access field empty to start unconnected.
    */
  BlinkenlightProxy (PlayerClient* pc, unsigned short index,
		     unsigned char access = 'c')
    : ClientProxy(pc,PLAYER_BLINKENLIGHT_CODE,index,access) 
    {
      // assume the light is off by default
      this->period_ms = 0;
      this->enable = false;
    }
  
  virtual ~BlinkenlightProxy()
    { };
  
  /// true: indicator light enabled, false: disabled.
  bool enable;
  
  /** The current period (one whole on/off cycle) of the blinking
      light. If the period is zero and the light is enabled, the light
      is on. 
  */
  int period_ms;
  
    // interface that all proxies must provide
    void FillData (player_msghdr_t hdr, const char* buffer);

    /// Print out the current status
    void Print ();

    /** Set the state of the indicator light. A period of zero means
        the light will be unblinkingly on or off. Returns 0 on
        success, else -1.
    */
    int SetLight( bool enable, int period_ms );
};

/** 

The @p CameraProxy class can be used to get images from a @ref
player_interface_camera device. */
class CameraProxy : public ClientProxy 
{

public:
    // Constructor.  Leave the access field empty to start unconnected.
   CameraProxy (PlayerClient *pc, unsigned short index,
       unsigned char access='c');

   virtual ~CameraProxy();

   // Image color depth
   uint8_t depth;

   // Image dimensions (pixels)
   uint16_t width, height;

   // Sime of the image (bytes)
   uint32_t imageSize;

   // Image data
   uint8_t image[PLAYER_CAMERA_IMAGE_SIZE];

   // interface that all proxies must provide
   void FillData(player_msghdr_t hdr, const char* buffer);

   // prints out basic statistics of the camera
   void Print();

   void SaveFrame(const char *prefix);

private:
   char filename[256];
   int frameNo;
};


/** 

The @p PlannerProxy proxy provides an interface to a 2D motion @ref
player_interface_planner. */
class PlannerProxy : public ClientProxy
{

  // Constructor.  Leave the access field empty to start unconnected.
  public: PlannerProxy (PlayerClient *pc, unsigned short index,
              unsigned char access='c');

  // Destructor
  public: virtual ~PlannerProxy();

  /** Set the goal pose (gx, gy, ga) */
  public: int SetCmdPose( double gx, double gy, double ga);

  /** Get the list of waypoints. Writes the result into the proxy
      rather than returning it to the caller. */
  public: int GetWaypoints();

  /** Enable/disable the robot's motion.  Set state to 1 to enable, 0 to
      disable. */
  public: int Enable(int state);

  // interface that all proxies must provide
  public: void FillData( player_msghdr_t hdr, const char *buffer);

  /** Did the planner find a valid path? */
  public: bool pathValid;

  /** Have we arrived at the goal? */
  public: bool pathDone;

  /** Current pose (m, m, radians). */
  public: double px, py, pa;

  /** Goal location (m, m, radians) */
  public: double gx, gy, ga;

  /** Current waypoint location (m, m, radians) */
  public: double wx, wy, wa;

  /** Current waypoint index (handy if you already have the list
      of waypoints). May be negative if there's no plan, or if 
      the plan is done */
  public: short currWaypoint;

  /** Number of waypoints in the plan */
  public: short waypointCount;

  /** List of waypoints in the current plan (m,m,radians).*/
  public: double waypoints[PLAYER_PLANNER_MAX_WAYPOINTS][3];

};

/** 

The @p EnergyProxy class is used to read from an @ref
player_interface_energy device.
*/
class EnergyProxy : public ClientProxy 
{

public:
    /** Constructor.
        Leave the access field empty to start unconnected.
    */
    EnergyProxy (PlayerClient* pc, unsigned short index,
                   unsigned char access = 'c')
            : ClientProxy(pc,PLAYER_ENERGY_CODE,index,access) 
      {}

    ~EnergyProxy()
      {}

    // these methods are the user's interface to this device

    // interface that all proxies must provide
    void FillData (player_msghdr_t hdr, const char* buffer);

    /// Print out the current bumper state.
    void Print ();

    /** These members give the current amount of energy stored
	(Joules) and the amount of energy current being consumed
	(Watts). The charging flag is TRUE if we are currently
	charging, else FALSE.
     */

    double joules;
    double watts;
    bool charging;
};

/** 

The @p map proxy provides access to a @ref player_interface_map device.
*/
class MapProxy : public ClientProxy
{
  // Constructor
  public: MapProxy (PlayerClient *pc, unsigned short indes,
              unsigned char access='c');

  // Destructor
  public: ~MapProxy();

  /** Get the map, which is stored in the proxy */
  public: int GetMap();

  /** Return the index of the (x,y) item in the cell array */
  public: int GetCellIndex( int x, int y );

  /** Get the (x,y) cell; returns 0 on success, -1 on failure (i.e., indexes
      out of bounds) */
  public: int GetCell( char* cell, int x, int y );

  // interface that all proxies must provide
  public: void FillData (player_msghdr_t hde, const char *buffer);

  /** Map resolution, m/cell */
  public: double resolution;

  /* Map size, in cells */
  public: int width, height;

  /** Occupancy for each cell (empty = -1, unknown = 0, occupied = +1) */
  public: char *cells;
};


/** 
The @p SpeechRecognition proxy provides access to a @ref player_interface_speechrecognition device.
*/
class SpeechRecognitionProxy : public ClientProxy
{
  // Constructor
  public: SpeechRecognitionProxy (PlayerClient *pc, unsigned short indes,
              unsigned char access='c');

  // Destructor
  public: ~SpeechRecognitionProxy();

  // interface that all proxies must provide
  public: void FillData (player_msghdr_t hde, const char *buffer);

  public: void Clear();

  public: char rawText[SPEECH_RECOGNITION_TEXT_LEN];

  // Just estimating that no more than 20 words will be spoken between updates.
  // Assuming that the longest word is <= 30 characters.
  public: char words[20][30]; 
  public: int wordCount;
};


/** @} */
/** @} */
/** @} */


#endif
