/***************************************************************************
 * Desc: A Player client
 * Author: Andrew Howard
 * Date: 24 Aug 2001
 # CVS: $Id$
 **************************************************************************/

#ifndef PLAYERC_H
#define PLAYERC_H

#include <stdio.h>
#include <sys/poll.h>

// Get the message structures from Player
#include "messages.h"


/***************************************************************************
 * Declare all of the basic types
 **************************************************************************/

// Forward declare types
struct _playerc_client_t;
struct _playerc_device_t;


// Typedefs for proxy callback functions
typedef void (*playerc_putdata_fn_t) (void *device, char *header, char *data, size_t len);
typedef void (*playerc_callback_fn_t) (void *data);


// Multi-client data
typedef struct
{
  // List of clients being managed
  int client_count;
  struct _playerc_client_t *client[128];

  // Poll info 
  struct pollfd pollfd[128];

} playerc_mclient_t;


// Single client data
typedef struct _playerc_client_t
{
  // Server address
  char *host;
  int port;
    
  // Socket descriptor
  int sock;
    
  // List of subscribed devices
  int device_count;
  struct _playerc_device_t *device[32];
    
} playerc_client_t;


// Player device proxy info
// Basically a base class for devices.
typedef struct _playerc_device_t
{
  playerc_client_t *client;

  // Device code, etc
  int code;
  int index;
  int access;

  // Data timestamp
  double datatime;

  // Standard callbacks for this device
  playerc_putdata_fn_t putdata;

  // Extra callbacks for this device
  int callback_count;
  playerc_callback_fn_t callback[4];
  void *callback_data[4];

} playerc_device_t;


/***************************************************************************
 * Error handling
 **************************************************************************/

// Errors get written here
extern char playerc_errorstr[];


/***************************************************************************
 * Multi-client functions
 **************************************************************************/

// Create a multi-client object
playerc_mclient_t *playerc_mclient_create();

// Destroy a multi-client object
void playerc_mclient_destroy(playerc_mclient_t *mclient);

// Connect to all the servers at once
int playerc_mclient_connect(playerc_mclient_t *mclient);

// Disconnect from all the servers at once
int playerc_mclient_disconnect(playerc_mclient_t *mclient);

// Read incoming data.
// The timeout is in ms.  Set timeout to a negative value to wait
// indefinitely.
int  playerc_mclient_read(playerc_mclient_t *mclient, int timeout);

// Add a client to the multi-client (private).
int playerc_mclient_addclient(playerc_mclient_t *mclient, playerc_client_t *client);


/***************************************************************************
 * Single-client functions
 **************************************************************************/

// Create a single-client object.
// Set mclient to NULL if this is a stand-alone client.
playerc_client_t *playerc_client_create(playerc_mclient_t *mclient, const char *host, int port);

// Destroy a single-client object.
void playerc_client_destroy(playerc_client_t *client);

// Connect/disconnect to the server.
int  playerc_client_connect(playerc_client_t *client);
int  playerc_client_disconnect(playerc_client_t *client);

// Add/remove a device proxy (private)
int playerc_client_adddevice(playerc_client_t *client, playerc_device_t *device);
int playerc_client_deldevice(playerc_client_t *client, playerc_device_t *device);

// Add/remove user callbacks (called when new data arrives).
int  playerc_client_addcallback(playerc_client_t *client, playerc_device_t *device,
                                playerc_callback_fn_t callback, void *data);
int  playerc_client_delcallback(playerc_client_t *client, playerc_device_t *device,
                                playerc_callback_fn_t callback, void *data);

// Subscribe/unsubscribe a device from the sever (private)
int playerc_client_subscribe(playerc_client_t *client, int code, int index, int access);
int playerc_client_unsubscribe(playerc_client_t *client, int code, int index);


// Issue a request to the server and await a reply (blocking).
int playerc_client_request(playerc_client_t *client, playerc_device_t *device,
                           char *req_data, int req_len, char *rep_data, int rep_len);

// Read data from the server (blocking).
int  playerc_client_read(playerc_client_t *client);

// Write data to the server.
int  playerc_client_write(playerc_client_t *client, playerc_device_t *device, char *cmd, int len);


/***************************************************************************
 * Base device
 **************************************************************************/

// Initialise the device 
void playerc_device_init(playerc_device_t *device, playerc_client_t *client,
                         int code, int index, playerc_putdata_fn_t putdata);

// Finalize the device
void playerc_device_term(playerc_device_t *device);

// Subscribe/unsubscribe the device
int playerc_device_subscribe(playerc_device_t *device, int access);
int playerc_device_unsubscribe(playerc_device_t *device);


/***************************************************************************
 * Position device
 **************************************************************************/

// Position device
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;
    
  // Odometric pose (m, m, radians)
  double px, py, pa;

  // Odometric velocity (m, m, radians)
  double vx, vy, va;
  
  // Stall flag [0, 1]
  int stall;

} playerc_position_t;


// Create a position device proxy.
playerc_position_t *playerc_position_create(playerc_client_t *client, int index);

// Destroy a position device proxy.
void playerc_position_destroy(playerc_position_t *device);

// Subscribe to the position device
int playerc_position_subscribe(playerc_position_t *device, int access);

// Un-subscribe from the position device
int playerc_position_unsubscribe(playerc_position_t *device);

// Enable/disable the motors
int playerc_position_enable(playerc_position_t *device, int enable);

// Set the robot speed
int  playerc_position_setspeed(playerc_position_t *device, double vx, double vy, double va);


/***************************************************************************
 * Laser device
 **************************************************************************/

// Laser device data
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;

  // Number of points in scan
  int scan_count;

  // Laser scan data (range and bearing)
  double scan[401][2];

  // Laser scan data (position)
  double point[402][2];

  // Laser scan intensity values (0-3)
  int intensity[402];
  
} playerc_laser_t;


// Create a laser proxy
playerc_laser_t *playerc_laser_create(playerc_client_t *client, int index);

// Destroy a laser proxy
void playerc_laser_destroy(playerc_laser_t *device);

// Subscribe to the laser device
int playerc_laser_subscribe(playerc_laser_t *device, int access);

// Un-subscribe from the laser device
int playerc_laser_unsubscribe(playerc_laser_t *device);

// Configure the laser.
int  playerc_laser_configure(playerc_laser_t *device, double min_angle, double max_angle,
                             double resolution, int intensity);


/***************************************************************************
 * Laser beacon device
 **************************************************************************/ 

// Description for a single beacon
typedef struct
{
  // Beacon id (0 is beacon cannot be identified).
  int id;

  // Beacon range, bearing and orientation.
  double range, bearing, orient;
  
} playerc_laserbeacon_beacon_t;


// Laser beacon data
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;

  // List of detected beacons.
  int beacon_count;
  playerc_laserbeacon_beacon_t beacons[64];
    
} playerc_laserbeacon_t;


// Create a laserbeacon proxy
playerc_laserbeacon_t *playerc_laserbeacon_create(playerc_client_t *client, int index);

// Destroy a laserbeacon proxy
void playerc_laserbeacon_destroy(playerc_laserbeacon_t *device);

// Subscribe to the laserbeacon device
int playerc_laserbeacon_subscribe(playerc_laserbeacon_t *device, int access);

// Un-subscribe from the laserbeacon device
int playerc_laserbeacon_unsubscribe(playerc_laserbeacon_t *device);

// Configure the laserbeacon device
int playerc_laserbeacon_configure(playerc_laserbeacon_t *device, int bit_count, double bit_width);


/***************************************************************************
 * GPS device
 **************************************************************************/

/* GPS device data */
typedef struct
{
  /* Device info; must be at the start of all device structures. */
  playerc_device_t info;
    
  /* GPS pose (m, m, radians) */
  double px, py, pa;
    
} playerc_gps_t;


/* GPS methods */
playerc_gps_t *playerc_gps_create(playerc_client_t *client, int index, int access);
void playerc_gps_destroy(playerc_gps_t *device);
int playerc_gps_teleport(playerc_gps_t *device, double px, double py, double pa);


/***************************************************************************
 * BPS device
 **************************************************************************/

/* BPS device data */
typedef struct
{
  /* Device info; must be at the start of all device structures. */
  playerc_device_t info;
    
  /* BPS pose (m, m, radians) */
  double px, py, pa;

  /* Residual error in pose */
  double err;
    
} playerc_bps_t;


/* BPS methods */
playerc_bps_t *playerc_bps_create(playerc_client_t *client, int index, int access);
void playerc_bps_destroy(playerc_bps_t *device);
int  playerc_bps_setgain(playerc_bps_t *device, double gain);
int  playerc_bps_setlaser(playerc_bps_t *device, double px, double py, double pa);
int  playerc_bps_setbeacon(playerc_bps_t *device, int id, double px, double py, double pa,
                           double ux, double uy, double ua);


/***************************************************************************
 * Broadcast device
 **************************************************************************/

/* Broadcast device data */
typedef struct
{
  /* Device info; must be at the start of all device structures. */
  playerc_device_t info;
    
  /* Incoming message queue */
  char *in_data;
  int in_len, in_size;

} playerc_broadcast_t;


/* Broadcast methods */
playerc_broadcast_t *playerc_broadcast_create(playerc_client_t *client,
                                              int index, int access);
void playerc_broadcast_destroy(playerc_broadcast_t *device);
int  playerc_broadcast_read(playerc_broadcast_t *device,
                            char *msg, int len);
int  playerc_broadcast_write(playerc_broadcast_t *device,
                             const char *msg, int len);

#endif




