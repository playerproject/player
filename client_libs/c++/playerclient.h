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

#include <player.h>       /* from the server; gives message types */
#include <playercclient.h>  /* pure C networking building blocks */

#if HAVE_STRINGS_H
  #include <strings.h>
#endif

#include <sys/time.h>
#include <sys/poll.h>
#include <string.h>

// forward declaration for friending
class PlayerClient;


/* Base class for all proxy devices.
 */
class ClientProxy
{
  // GCC 3.0 requires this new syntax
  friend class PlayerClient; // ANSI C++ syntax?
  // friend PlayerClient; // syntax deprecated

 
  public:         
    PlayerClient* client;  // our controlling client object

    // if this generic proxy is not subclassed, the most recent 
    // data and header get copied in here. that way we can use this
    // base class on it's own as a generic proxy
    unsigned char last_data[4096];
    player_msghdr_t last_header;

    unsigned char access;   // 'r', 'w', or 'a' (others?)
    unsigned short device; // the name by which we identify this kind of device
    unsigned short index;  // which device we mean
    char driver_name[PLAYER_MAX_DEVICE_STRING_LEN]; // driver in use

    struct timeval timestamp;  // time at which this data was sensed
    struct timeval senttime;   // time at which this data was sent
    struct timeval receivedtime; // time at which this data was received

    /* Constructor.
        The constructor will try to get access to the device
        (unless \p req_device is 0 or \p req_access is 'c').
    */
    ClientProxy(PlayerClient* pc, 
		unsigned short req_device,
		unsigned short req_index,
		unsigned char req_access = 'c' );

    // destructor will try to close access to the device
    virtual ~ClientProxy();

    unsigned char GetAccess() { return(access); };  

    // methods for changin access mode
    int ChangeAccess(unsigned char req_access, 
                     unsigned char* grant_access=NULL );
    int Close() { return(ChangeAccess('c')); }

    // interface that all proxies must provide
    virtual void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    virtual void Print();
};

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
                            unsigned char* grant_access,
                            char* driver_name = NULL,
                            int driver_name_len = 0);

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
    // constructor
    PlayerMultiClient();
    
    // destructor
    ~PlayerMultiClient();

    // how many clients are we managing?
    int GetNumClients( void ){ return num_ufds; };

    // return a client pointer from a host and port, zero if not connected 
    // to that address
    PlayerClient* GetClient( char* host, int port );

    // return a client pointer from an address and port, zero if not connected 
    // to that address
    PlayerClient* GetClient( struct in_addr* addr, int port );
    
    // add a client to our watch list
    void AddClient(PlayerClient* client);
    
    // remove a client from the watch list
    void RemoveClient(PlayerClient* client);

    // read from one client (the first available)
    //
    // Returns:
    //    0 if everything went OK
    //   -1 if something went wrong (you should probably close the connection!)
    //
    int Read();

    // same as Read(), but reads everything off the socket so we end
    // up with the freshest data, subject to N maximum reads
    int ReadLatest( int max_reads );
};

/** The {\tt CommsProxy} class controls the {\tt broadcast} device.
    Data may be read one message at a time from the incoming broadcast
    queue using the {\tt Read} method.
    Data may be written one message at a time to the outgoing broadcast
    queue using the {\tt Write} method.
    Note that outgoing messages are not actually sent to the server
    until the {\tt Flush} method is called.
 */
class CommsProxy : public ClientProxy
{
  /** Proxy constructor.
      Leave the access field empty to start unconnected.
      You can change the access later using {\tt PlayerProxy::RequestDeviceAccess}.
  */
  public: CommsProxy(PlayerClient* pc, unsigned short index, unsigned char access ='c');

  /** Read a message from the incoming queue.
      Returns the number of bytes read, or -1 if the queue is empty.
  */
  public: int Read(void *msg, int len);

  /** Write a message to the outgoing queue.
      Returns the number of bytes written, or -1 if the queue is full.
  */
  public: int Write(void *msg, int len);

  // interface that all proxies must provide
  protected: void FillData(player_msghdr_t hdr, const char* buffer);
    
  // interface that all proxies SHOULD provide
  protected: void Print();
};


class DescartesProxy : public ClientProxy
{

  public:
    // the latest position data
    int xpos,ypos,theta;
    unsigned char bumpers[2];
   
    // the client calls this method to make a new proxy
    //   leave access empty to start unconnected
    DescartesProxy(PlayerClient* pc, unsigned short index, 
                  unsigned char access ='c'):
            ClientProxy(pc,PLAYER_DESCARTES_CODE,index,access) {}

    // these methods are the user's interface to this device

    // send a movement command
    //
    // Returns:
    //   0 if everything's ok
    //   -1 otherwise (that's bad)
    int Move(short speed, short heading, short distance );

    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    void Print();

    // fill in the arguments with the current position
    void GetPos( double* x, double* y, double* th );
};

/** The {\tt GpsProxy} class is used to control the {\tt gps} device.
    The latest pose data is stored in three class attributes.  The robot
    can be ``teleported'' with the {\tt Warp()} method
  */
class GpsProxy : public ClientProxy
{

  public:
    // the latest GPS data

    /** The latest global pose (in mm, mm, and degrees, respectively)
     */
    int xpos,ypos,heading;
   
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using
        {\tt PlayerProxy::RequestDeviceAccess()}.
    */
    GpsProxy(PlayerClient* pc, unsigned short index, 
              unsigned char access='c') :
            ClientProxy(pc,PLAYER_GPS_CODE,index,access) {}

    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    void Print();
};

/* gripper stuff */
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

/** The {\tt GripperProxy} class is used to control the {\tt gripper} device.
    The latest gripper data held in a handful of class attributes.
    A single method provides user control.
*/
class GripperProxy : public ClientProxy
{

  public:
    // the latest gripper data
    unsigned char state,beams;

    /** These boolean variables indicate the state of the gripper
     */
    bool outer_break_beam,inner_break_beam,
         paddles_open,paddles_closed,paddles_moving,
         gripper_error,lift_up,lift_down,lift_moving,
         lift_error;

    // the client calls this method to make a new proxy
    //   leave access empty to start unconnected
    GripperProxy(PlayerClient* pc, unsigned short index, 
                 unsigned char access='c') :
            ClientProxy(pc,PLAYER_GRIPPER_CODE,index,access) {}

    // these methods are the user's interface to this device

    /** Send a gripper command.  Look in the gripper manual for details
        on the command and argument.\\
        Returns 0 if everything's ok, and  -1 otherwise (that's bad).
    */
    int SetGrip(unsigned char cmd, unsigned char arg=0);

    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    void Print();
};

class IDARProxy : public ClientProxy
{
  public:
  
  // the client calls this method to make a new proxy
  //   leave access empty to start unconnected
  IDARProxy(PlayerClient* pc, unsigned short index, 
	    unsigned char access = 'c');
  
  // interface that all proxies must provide
  // reads the receive buffers from player
  //void FillData(player_msghdr_t hdr, const char* buffer);
  
  // interface that all proxies SHOULD provide
  void Print();
  
  // tx parameter is optional; defaults to 0
  int SendMessage( idartx_t* tx );
  
  // get message and transmission details 
  int GetMessage( idarrx_t* rx );  

  // get message and transmission details without flushing the buffer
  int GetMessageNoFlush( idarrx_t* rx );  

  // send and get a message
  int SendGetMessage( idartx_t* tx, idarrx_t* rx );  

  // pretty print a message
  void PrintMessage(idarrx_t* rx); 
};

class IDARTurretProxy : public ClientProxy
{
  public:
  
  // the client calls this method to make a new proxy
  //   leave access empty to start unconnected
  IDARTurretProxy(PlayerClient* pc, unsigned short index, 
	    unsigned char access = 'c');
  
  // interface that all proxies must provide
  // reads the receive buffers from player
  //void FillData(player_msghdr_t hdr, const char* buffer);
  
  // interface that all proxies SHOULD provide
  void Print();
  
  // tx parameter is optional; defaults to 0
  int SendMessages( player_idarturret_config_t* conf );
  
  // get message and transmission details 
  int GetMessages( player_idarturret_reply_t* reply );  

  // send and get in one operation for efficiency
  int SendGetMessages( player_idarturret_config_t* conf,
		       player_idarturret_reply_t* reply );

  // pretty print a message
  void PrintMessages( player_idarturret_reply_t* reply ); 

  void PrintMessage( idarrx_t* msg );
};


/** The {\tt FiducialProxy} class is used to control the
    {\tt fiducial} device.
    The latest set of detected beacons is stored in the
    {\tt beacons} array.
    The {\tt fiducial} device may be configured using
    the {\tt SetBits()} and {\tt SetThresh()} methods.
*/
class FiducialProxy : public ClientProxy
{

  public:
    // the latest laser beacon data

    /** The latest laser beacon data.
        Each beacon has the following information:
        \begin{verbatim}
        uint8_t id;
        uint16_t range;
        int16_t bearing;
        int16_t orient;
        \end{verbatim}
        where {\tt range} is measured in mm, and {\tt bearing} and
        {\tt orient} are measured in degrees.
     */
    player_fiducial_item_t beacons[PLAYER_FIDUCIAL_MAX_SAMPLES];

    /** The number of beacons detected
     */
    unsigned short count;

    /** The current bit count of the fiducial device.
        See the Player manual for information on this setting.
      */
    unsigned char bit_count;

    /** The current bit size (in mm) of the fiducial device.
        See the Player manual for information on this setting.
      */
    unsigned short bit_size;

    /** The current zero threshold of the fiducial device.
        See the Player manual for information on this setting.
      */
    unsigned short zero_thresh;

    /** The current one threshold of the fiducial device.
        See the Player manual for information on this setting.
      */
    unsigned short one_thresh;
   
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using
        {\tt PlayerProxy::RequestDeviceAccess()}.
    */
    FiducialProxy(PlayerClient* pc, unsigned short index,
                     unsigned char access='c'):
            ClientProxy(pc,PLAYER_FIDUCIAL_CODE,index,access) {}

    // these methods are the user's interface to this device

    /** Set the bit properties of the beacons.
        Set {\tt bit\_count} to the number of bits in your beacons
        (usually 5 or 8).
        Set {\tt bit\_size} to the width of each bit (in mm).\\
        Returns the 0 on success, or -1 of there is a problem.
     */
    int SetBits(unsigned char bit_count, unsigned short bit_size);

    /** Set the identification thresholds.
        Thresholds must be in the range 0--99.
        See the Player manual for a full discussion of these settings.\\
        Returns the 0 on success, or -1 of there is a problem.
     */
    int SetThresh(unsigned short zero_thresh, unsigned short one_thresh);

    /** Get the current configuration.
        Fills the current device configuration into the corresponding
        class attributes.\\
        Returns the 0 on success, or -1 of there is a problem.
     */
    int GetConfig();
    
    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    void Print();
};

/** The {\tt LaserProxy} class is used to control the {\tt laser} device.
    The latest scan data is held in two arrays: {\tt ranges} and {\tt
    intensity}.  The laser scan range, resolution and so on can be
    configured using the {\tt Configure()} method.
*/
class LaserProxy : public ClientProxy
{

  public:

    // the latest laser scan data
    
    /** Scan range for the latest set of data.
        Angles are measured in units of $0.01^{\circ}$,
        in the range -9000 ($-90^{\circ}$) to
        +9000 ($+90^{\circ}$).
    */
    short min_angle; short max_angle;

    /** Scan resolution for the latest set of data.
        Resolution is measured in units of $0.01^{\circ}$.
        Valid values are: 25, 50, and 100.
    */
    unsigned short resolution;

    /** Whether or not reflectance values are returned.
      */
    bool intensity;

    /// The number of range measurements in the latest set of data.
    unsigned short range_count;

    /// The range values (in mm).
    unsigned short ranges[PLAYER_LASER_MAX_SAMPLES];

    // TODO: haven't verified that intensities work yet:
    /// The reflected intensity values (arbitrary units in range 0-7).
    unsigned char intensities[PLAYER_LASER_MAX_SAMPLES];

    unsigned short min_right,min_left;
   
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using
        {\tt PlayerProxy::RequestDeviceAccess()}.
    */
    LaserProxy(PlayerClient* pc, unsigned short index, 
               unsigned char access='c'):
        ClientProxy(pc,PLAYER_LASER_CODE,index,access) {}

    // these methods are the user's interface to this device

    /** Enable/disable the laser.
      Set {\tt state} to 1 to enable, 0 to disable.
      Note that when laser is disabled the client will still receive laser
      data, but the ranges will always be the last value read from the
      laser before it was disabled.
      Returns 0 on success, -1 if there is a problem.
     */
    int SetLaserState (const unsigned char state);

    // returns the local rectangular coordinate of the i'th beam strike
    int CartesianCoordinate( int i, int *x, int *y );

    // configure the laser scan.
    /** Configure the laser scan pattern.
        Angles {\tt min\_angle} and {\tt max\_angle} are measured in
        units of $0.1^{\circ}$, in the range -9000
        ($-90^{\circ}$) to +9000 ($+90^{\circ}$).  {\tt
        resolution} is also measured in units of $0.1^{\circ}$; valid
        values are: 25 ($0.25^{\circ}$), 50 ($0.5^{\circ}$) and $100
        (1^{\circ}$).  Set {\tt intensity} to {\tt true} to enable
        intensity measurements, or {\tt false} to disable.\\
        Returns the 0 on success, or -1 of there is a problem.
    */
    int Configure(short min_angle, short max_angle, 
                  unsigned short resolution, bool intensity);

    /** Get the current laser configuration; it is read into the
        relevant class attributes.\\
        Returns the 0 on success, or -1 of there is a problem.
      */
    int GetConfigure();

    /** Accessors
     */
    int  RangeCount () { return range_count; }
    uint16_t Ranges (const unsigned int index)
    {
    	if (index < range_count)
    		return ranges[index];
    	else
    		return 0;
    }
    uint16_t MinLeft () { return min_left; }
    uint16_t MinRight () { return min_right; }
    /** Range access operator.
        This operator provides an alternate way of access the range data.
        For example, given an {\tt LaserProxy} named {\tt lp}, the following
        expressions are equivalent: \verb+lp.Ranges(0)+ and \verb+lp[0]+.
    */
    uint16_t operator [] (unsigned int index)
    {
      return Ranges(index);
    }
    
    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    void Print();
};


#define MAX_RX_BUF_SIZE 1024

class MoteProxy : public ClientProxy
{
    public: MoteProxy(PlayerClient* pc, unsigned short index, 
		      unsigned char access ='c');

    public: int Sendto(int src, int dest, char* msg, int len);

    public: int TransmitRaw(char *msg, uint16_t len);

    public: int RecieveRaw(char* msg, uint16_t len, double *rssi);

    public: int Sendto(int to, char* msg, int len);

    public: int SetStrength(uint8_t str);

    public: int GetStrength(void);

    public: int RecieveFrom(int from, char* msg, int len);

    public: inline float GetRSSI(void);

    // interface that all proxies must provide
    protected: void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    protected: void Print();
    
    private: player_mote_data_t *rx_data;

    private: player_mote_data_t *rx_queue;

    private: player_mote_config_t m_config;

    private: char r_flag;

    private: unsigned char msg_q_index;
};


/** The {\tt PositionProxy} class is used to control the {\tt position} device.
    The latest position data is contained in the attributes {\tt xpos, ypos}, etc.
 */
class PositionProxy : public ClientProxy
{

  public:
    /// Robot pose (according to odometry) in mm, mm, degrees.
    int xpos,ypos; unsigned short theta;

    /// Robot speed in mm/sec, degrees/sec.
    short speed, turnrate;

    /// Compass value (only valid if the compass is installed).
    //unsigned short compass;

    /// Stall flag: 1 if the robot is stalled and 0 otherwise.
    unsigned char stall;
   
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using
        {\tt PlayerProxy::RequestDeviceAccess()}.
    */
    PositionProxy(PlayerClient* pc, unsigned short index,
                  unsigned char access ='c') :
        ClientProxy(pc,PLAYER_POSITION_CODE,index,access) {}

    // these methods are the user's interface to this device

    /** Send a motor command.
        Specify the linear and angular speed in mm/s and degrees/sec,
        respectively.\\
        Returns: 0 if everything's ok, -1 otherwise.
    */
    int SetSpeed(int speed, int turnrate);

    /** Enable/disable the motors.
        Set {\tt state} to 0 to disable (default) or 1 to enable.
        Be VERY careful with this method!  Your robot is likely to run across the
        room with the charger still attached.
        
        Returns: 0 if everything's ok, -1 otherwise.
    */
    int SetMotorState(unsigned char state);
    
    /** Select velocity control mode for the Pioneer 2.
        Set {\tt mode} to 0 for direct wheel velocity control (default),
        or 1 for separate translational and rotational control.
        
	For "reb_position": 0 is direct velocity control, 1 is for velocity-based
	heading PD controller (uses DoDesiredHeading())

        Returns: 0 if everything's ok, -1 otherwise.
    */
    int SelectVelocityControl(unsigned char mode);
   
    /** Reset odometry to (0,0,0).
        Returns: 0 if everything's ok, -1 otherwise.
    */
    int ResetOdometry();

    // the following ioctls are currently only supported by reb_position
    //

    /** Select position mode on the REB.
	Set {\tt mode} for 0 for velocity mode, 1 for position mode.

	Returns: 0 if OK, -1 else
    */
    int SelectPositionMode(unsigned char mode);

    /** Sets the odometry to the pose {\tt (x, y, theta)}.
	Note that {\tt x} and {\tt y} are in mm and {\tt theta} is in degrees.
	
	Returns: 0 if OK, -1 else
    */
    int SetOdometry(long x,long y,int t);
    int SetSpeedPID(int kp, int ki, int kd);
    int SetPositionPID(short kp, short ki, short kd);
    int SetPositionSpeedProfile(short spd, short acc);
    int DoStraightLine(int mm);
    int DoRotation(int deg);
    int DoDesiredHeading(short theta, int xspeed, int yawspeed);

    /** Accessors
     */
    int32_t  Xpos () const { return xpos; }
    int32_t  Ypos () const { return ypos; }
    uint16_t Theta () const { return theta; }
    int16_t  Speed () const { return speed; }
    int16_t  TurnRate () const { return turnrate; }
    //unsigned short Compass () const { return compass; }
    unsigned char Stall () const { return stall; }

    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    void Print();
};


/** The {\tt PtzProxy} class is used to control the {\tt ptz} device.
    The state of the camera can be read from the {\tt pan, tilt, zoom}
    attributes and changed using the {\tt SetCam()} method.
 */
class PtzProxy : public ClientProxy
{

  public:
    /// Pan and tilt values (degrees).
    short pan, tilt;

    /// Zoom value (0 -- 1024, where 0 is wide angle and 1024 is telephoto).
    unsigned short zoom;
   
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using
        {\tt PlayerProxy::RequestDeviceAccess()}.
    */
    PtzProxy(PlayerClient* pc, unsigned short index, unsigned char access='c'):
            ClientProxy(pc,PLAYER_PTZ_CODE,index,access) {}

    // these methods are the user's interface to this device

    /** Change the camera state.
        Specify the new {\tt pan} and {\tt tilt} values (degrees)
        and the new {\tt zoom} value (0 -- 1024).
        Returns: 0 if everything's ok, -1 otherwise.
    */
    int SetCam(short pan, short tilt, unsigned short zoom);

    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    void Print();
};

/** The {\tt SonarProxy} class is used to control the {\tt sonar} device.
    The most recent sonar range measuremts can be read from the {\tt range}
    attribute, or using the the {\tt []} operator.
 */
class SonarProxy : public ClientProxy
{

  public:
    /** The number of sonar readings received.
     */
    unsigned short range_count;

    /** The latest sonar scan data.
        Range is measured in mm.
     */
    unsigned short ranges[PLAYER_SONAR_MAX_SAMPLES];

    /** Positions of sonars
     */
    player_sonar_geom_t sonar_pose;
   
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using {\tt PlayerProxy::RequestDeviceAccess}.
    */
    SonarProxy(PlayerClient* pc, unsigned short index, 
               unsigned char access = 'c') :
            ClientProxy(pc,PLAYER_SONAR_CODE,index,access)
    { bzero(&sonar_pose,sizeof(sonar_pose)); }

    // these methods are the user's interface to this device
    
    /** Enable/disable the sonars.
        Set {\tt state} to 1 to enable, 0 to disable.
        Note that when sonars are disabled the client will still receive sonar
        data, but the ranges will always be the last value read from the sonars
        before they were disabled.\\
        Returns 0 on success, -1 if there is a problem.
     */
    int SetSonarState(unsigned char state);

    int GetSonarGeom();

    /** Range access operator.
        This operator provides an alternate way of access the range data.
        For example, given a {\tt SonarProxy} named {\tt sp}, the following
        expressions are equivalent: \verb+sp.ranges[0]+ and \verb+sp[0]+.
     */
    unsigned short operator [](unsigned int index) 
    { 
      if(index < sizeof(ranges))
        return(ranges[index]);
      else 
        return(0);
    }

    /** Get the pose of a particular sonar.
        This is a convenience function that returns the pose of any
        sonar on a Pioneer2DX robot.  It will {\em not} return valid
        poses for other configurations.
    */
    void GetSonarPose(int s, double* px, double* py, double* pth);
    
    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    void Print();
};

/** The {\tt SpeechProxy} class is used to control the
    {\tt speech} device.
    Use the {\tt say} method to send things to say.
*/
class SpeechProxy : public ClientProxy
{

  public:
   
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using
        {\tt PlayerProxy::RequestDeviceAccess()}.
    */
    SpeechProxy(PlayerClient* pc, unsigned short index, 
                unsigned char access='c'):
            ClientProxy(pc,PLAYER_SPEECH_CODE,index,access) {}

    // these methods are the user's interface to this device

    /** Send a phrase to say.
        The phrase is an ASCII string.
        Returns the 0 on success, or -1 of there is a problem.
    */
    int Say(char* str);
};

// Convert radians to degrees
//
#ifndef RTOD
#define RTOD(r) ((r) * 180 / M_PI)
#endif

// Convert degrees to radians
//
#ifndef DTOR
#define DTOR(d) ((d) * M_PI / 180)
#endif

// Normalize angle to domain -pi, pi
//
#ifndef NORMALIZE
#define NORMALIZE(z) atan2(sin(z), cos(z))
#endif

/** The {\tt TruthProxy} gets and sets the {\em true} pose of a truth
    device [worldfile tag: truth()]. This may be different from the
    pose returned by a device such as GPS or Position. If you want to
    log what happened in an experiment, this is the device to
    use. 

    Setting the position of a truth device moves its parent, so you
    can put a truth device on robot and teleport it around the place. 
 */
class TruthProxy : public ClientProxy
{
  
  public:

  /** These vars store the current device pose (x,y,a) as
      (m,m,radians). The values are updated at regular intervals as
      data arrives. You can read these values directly but setting
      them does NOT change the device's pose!. Use {\tt
      TruthProxy::SetPose()} for that.  
*/
  double x, y, a; 


  /** Constructor.
      Leave the access field empty to start unconnected.
      You can change the access later using 
      {\tt PlayerProxy::RequestDeviceAccess}.
  */
  TruthProxy(PlayerClient* pc, unsigned short index, 
             unsigned char access = 'c') :
    ClientProxy(pc,PLAYER_TRUTH_CODE,index,access) {};

    
  // interface that all proxies must provide
  void FillData(player_msghdr_t hdr, const char* buffer);
    
  // interface that all proxies SHOULD provide
  void Print();

  /** Query Player about the current pose - requests the pose from the
      server, then fills in values for the arguments
      (m,m,radians). Usually you'll just read the {\tt x,y,a}
      attributes but this function allows you to get pose direct from
      the server if you need too. Returns 0 on success, -1 if there is
      a problem.  
  */
  int GetPose( double *px, double *py, double *pa );

  /** Request a change in pose (m,m,radians). Returns 0 on success, -1
      if there is a problem.  
  */
  int SetPose( double px, double py, double pa );
};


class Blob
{
  public:
    unsigned int area;
    unsigned char x;
    unsigned char y;
    unsigned char left;
    unsigned char right;
    unsigned char top;
    unsigned char bottom;
};


/** The {\tt BlobfinderProxy} class is used to control the {\tt vision} device.
    It contains no methods.  The latest color blob data is stored in
    {\tt blobs}, a dynamically allocated 2-D array, indexed by color
    channel.
*/
class BlobfinderProxy : public ClientProxy
{

  public:
    // the latest vision data

    /** Array containing the number of blobs detected on each channel 
     */ 
    char num_blobs[PLAYER_BLOBFINDER_MAX_CHANNELS];

    /** Array containing the latest blob data.
        Each blob contains the following information:
        \begin{verbatim}
        unsigned int area;
        unsigned char x;
        unsigned char y;
        unsigned char left;
        unsigned char right;
        unsigned char top;
        unsigned char bottom;
        \end{verbatim}
     */
    Blob* blobs[PLAYER_BLOBFINDER_MAX_CHANNELS];
   
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using
        {\tt PlayerProxy::RequestDeviceAccess()}.
    */
    BlobfinderProxy(PlayerClient* pc, unsigned short index, 
                unsigned char access='c');
    ~BlobfinderProxy();

    // these methods are the user's interface to this device

    // interface that all proxies must provide
    void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    void Print();
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

class IRProxy : public ClientProxy
{
public:

  unsigned short voltages[PLAYER_IR_MAX_SAMPLES];
  unsigned short ranges[PLAYER_IR_MAX_SAMPLES];
  double stddev[PLAYER_IR_MAX_SAMPLES];

  double params[PLAYER_IR_MAX_SAMPLES][2]; // distance regression params
  double sparams[PLAYER_IR_MAX_SAMPLES][2]; //std dev regression params

  player_ir_pose_t ir_pose;

  IRProxy(PlayerClient *pc, unsigned short index,
          unsigned char access = 'c');

  // these are config methods
  int SetIRState(unsigned char state);

  int GetIRPose();

  void SetRangeParams(int which, double m, double );

  void SetStdDevParams(int which, double m, double b);

  double CalcStdDev(int w, unsigned short range);

  unsigned short operator [](unsigned int index) 
  {
    if (index < PLAYER_IR_MAX_SAMPLES) {
      return ranges[index];
    } 

    return 0;
  }

  // required methods
  void FillData(player_msghdr_t hdr, const char *buffer);

  void Print();
};

class WiFiProxy: public ClientProxy
{
public:
  unsigned short link, level, noise;

  WiFiProxy(PlayerClient *pc, unsigned short index, 
	       unsigned char access = 'c') : 
    ClientProxy(pc, PLAYER_WIFI_CODE, index, access) {}

  void FillData(player_msghdr_t hdr, const char *buffer);

  void Print();
};

class PowerProxy : public ClientProxy 
{

  public:
    /** Constructor.
      Leave the access field empty to start unconnected.
      You can change the access later using
      {\tt PlayerProxy::RequestDeviceAccess()}.
     */
    PowerProxy (PlayerClient* pc, unsigned short index,
                unsigned char access ='c')
            : ClientProxy(pc,PLAYER_POWER_CODE,index,access) {}

    uint16_t Charge () const { return charge; }

    // interface that all proxies must provide
    void FillData (player_msghdr_t hdr, const char* buffer);

    // interface that all proxies SHOULD provide
    void Print ();

  private:
    // Remaining power in centivolts
    uint16_t charge;
};

/** The {\tt BumperProxy} class is used to read from the {\tt rwi_bumper}
	device.
 */
class BumperProxy : public ClientProxy {

public:
    /** Constructor.
        Leave the access field empty to start unconnected.
        You can change the access later using
        {\tt PlayerProxy::RequestDeviceAccess}.
    */
    BumperProxy (PlayerClient* pc, unsigned short index,
                   unsigned char access = 'c')
            : ClientProxy(pc,PLAYER_BUMPER_CODE,index,access) {}

    // these methods are the user's interface to this device

    /** These methods return 1 if the specified bumper(s)
        have been bumped, 0 otherwise.
      */
    bool Bumped (const unsigned int i);
    bool BumpedAny ();

    uint8_t BumperCount () const { return bumper_count; }
    //uint32_t Bumpfield () const { return bumpfield; }

    // interface that all proxies must provide
    void FillData (player_msghdr_t hdr, const char* buffer);

    // interface that all proxies SHOULD provide
    void Print ();

private:
    /** array representing bumped state.
     */
    uint8_t  bumper_count;
    uint8_t bumpers[PLAYER_BUMPER_MAX_SAMPLES];
};

#endif
