/* 
 *  libplayerc : a Player client library
 *  Copyright (C) Andrew Howard 2002
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
/***************************************************************************
 * Desc: Player client
 * Author: Andrew Howard
 * Date: 24 Aug 2001
 # CVS: $Id$
 **************************************************************************/

#ifndef PLAYERC_H
#define PLAYERC_H

#include <stdio.h>
#include <sys/poll.h>

// Get the message structures from Player
#include "player.h"


/***************************************************************************
 * Array sizes
 **************************************************************************/

#define PLAYERC_LASER_MAX_SCAN PLAYER_NUM_LASER_SAMPLES
#define PLAYERC_LBD_MAX_BEACONS PLAYER_MAX_LASERBEACONS
#define PLAYERC_SONAR_MAX_SCAN PLAYER_NUM_SONAR_SAMPLES
#define PLAYERC_VISION_MAX_BLOBS 64


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


// Player device proxy info; basically a base class for devices.
typedef struct _playerc_device_t
{
  // Pointer to the client proxy.
  playerc_client_t *client;

  // Device code, index, etc.
  int code, index, access;

  // The subscribe flag is non-zero if the device has been
  // successfully subscribed.
  int subscribed;

  // Data timestamp, i.e., the time at which the data was generated (s).
  double datatime;

  // Standard callbacks for this device (private).
  playerc_putdata_fn_t putdata;

  // Extra callbacks for this device (private).
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
 * proxy : client : Single-client functions
 **************************************************************************/

// Create a single-client object.
// Set mclient to NULL if this is a stand-alone client.
playerc_client_t *playerc_client_create(playerc_mclient_t *mclient,
                                        const char *host, int port);

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
                           void *req_data, int req_len, void *rep_data, int rep_len);

// Read data from the server (blocking).
// Returns a pointer to the device that got the data, or NULL if there
// is an error.
void *playerc_client_read(playerc_client_t *client);

// Write data to the server (private).
int playerc_client_write(playerc_client_t *client, playerc_device_t *device, void *cmd, int len);


/***************************************************************************
 * proxy : base : Base device interface
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
 * proxy : bps (beacon positioning system) device
 **************************************************************************/

// BPS device data
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;
    
  // Robot pose estimate (global coordinates).
  double px, py, pa;

  // Robot pose uncertainty (global coordinates).
  double ux, uy, ua;

  // Residual error in estimates pose.
  double err;
    
} playerc_bps_t;


// Create a bps proxy
playerc_bps_t *playerc_bps_create(playerc_client_t *client, int index);

// Destroy a bps proxy
void playerc_bps_destroy(playerc_bps_t *device);

// Subscribe to the bps device
int playerc_bps_subscribe(playerc_bps_t *device, int access);

// Un-subscribe from the bps device
int playerc_bps_unsubscribe(playerc_bps_t *device);

// Set the pose of a beacon in global coordinates.
// id : the beacon id.
// px, py, pa : the beacon pose (global coordinates).
// ux, uy, ua : the uncertainty in the beacon pose.
int  playerc_bps_set_beacon(playerc_bps_t *device, int id,
                            double px, double py, double pa,
                            double ux, double uy, double ua);

// Get the pose of a beacon in global coordinates.
// id : the beacon id.
// px, py, pa : the beacon pose (global coordinates).
// ux, uy, ua : the uncertainty in the beacon pose.
int  playerc_bps_get_beacon(playerc_bps_t *device, int id,
                            double *px, double *py, double *pa,
                            double *ux, double *uy, double *ua);


/***************************************************************************
 * proxy : broadcast device
 **************************************************************************/

// Broadcast device data
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;
    
} playerc_broadcast_t;


// Create a broadcast proxy
playerc_broadcast_t *playerc_broadcast_create(playerc_client_t *client, int index);

// Destroy a broadcast proxy
void playerc_broadcast_destroy(playerc_broadcast_t *device);

// Subscribe to the broadcast device
int playerc_broadcast_subscribe(playerc_broadcast_t *device, int access);

// Un-subscribe from the broadcast device
int playerc_broadcast_unsubscribe(playerc_broadcast_t *device);

// Send a broadcast message.
int playerc_broadcast_send(playerc_broadcast_t *device, void *msg, int len);

// Read the next broadcast message.
int playerc_broadcast_recv(playerc_broadcast_t *device, void *msg, int len);


/***************************************************************************
 * proxy : gps (global positioning system) device
 **************************************************************************/

// GPS device data
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;
    
  // GPS pose estimate (in global coordinates).
  double px, py, pa;
    
} playerc_gps_t;


// Create a gps proxy.
playerc_gps_t *playerc_gps_create(playerc_client_t *client, int index);

// Destroy a gps proxy.
void playerc_gps_destroy(playerc_gps_t *device);

// Subscribe to the gps device
int playerc_gps_subscribe(playerc_gps_t *device, int access);

// Un-subscribe from the gps device
int playerc_gps_unsubscribe(playerc_gps_t *device);

// Teleport the robot to a new pose.  Works in simulation only.
// px, py, pa : the new pose (global coordinates).
int playerc_gps_teleport(playerc_gps_t *device, double px, double py, double pa);


/***************************************************************************
 * proxy : laser device
 **************************************************************************/

// Laser device data
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;

  // Laser geometry in robot cs: pose gives the position and
  // orientation, size gives the extent.  These values are filled in by
  // playerc_laser_get_geom().
  double pose[3];
  double size[2];
  
  // Number of points in the scan.
  int scan_count;

  // Scan data; range (m) and bearing (radians).
  double scan[PLAYERC_LASER_MAX_SCAN][2];

  // Scan data; x, y position (m).
  double point[PLAYERC_LASER_MAX_SCAN][2];

  // Scan reflection intensity values (0-3).
  int intensity[PLAYERC_LASER_MAX_SCAN];
  
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
// min_angle, max_angle : Start and end angles for the scan.
// resolution : Resolution in 0.01 degree increments.  Valid values
//              are 25, 50, 100.
// intensity : Intensity flag; set to 1 to enable reflection intensity data.
int  playerc_laser_set_config(playerc_laser_t *device, double min_angle,
                              double max_angle, int resolution, int intensity);

// Get the laser configuration
// min_angle, max_angle : Start and end angles for the scan.
// resolution : Resolution is in 0.01 degree increments.
// intensity : Intensity flag; set to 1 to enable reflection intensity data.
int  playerc_laser_get_config(playerc_laser_t *device, double *min_angle,
                              double *max_angle, int *resolution, int *intensity);

// Get the laser geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int playerc_laser_get_geom(playerc_laser_t *device);


/***************************************************************************
 * proxy : lbd (laser beacon detector) device
 **************************************************************************/ 

// Description for a single beacon
typedef struct
{
  // Beacon id (0 is beacon cannot be identified).
  int id;

  // Beacon range, bearing and orientation.
  double range, bearing, orient;
  
} playerc_lbd_beacon_t;


// Laser beacon data
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;

  // List of detected beacons.
  int beacon_count;
  playerc_lbd_beacon_t beacons[PLAYERC_LBD_MAX_BEACONS];
    
} playerc_lbd_t;


// Create a lbd proxy
playerc_lbd_t *playerc_lbd_create(playerc_client_t *client, int index);

// Destroy a lbd proxy
void playerc_lbd_destroy(playerc_lbd_t *device);

// Subscribe to the lbd device
int playerc_lbd_subscribe(playerc_lbd_t *device, int access);

// Un-subscribe from the lbd device
int playerc_lbd_unsubscribe(playerc_lbd_t *device);

// Set the device configuration.
// bit_count : the number of bits in the barcode.
// bit_width : the width of each bit in the barcode.
int playerc_lbd_set_config(playerc_lbd_t *device,
                           int bit_count, double bit_width);

// Get the device configuration.
// bit_count : the number of bits in the barcode.
// bit_width : the width of each bit in the barcode.
int playerc_lbd_get_config(playerc_lbd_t *device,
                           int *bit_count, double *bit_width);


/***************************************************************************
 * proxy : position device
 **************************************************************************/

// Position device
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;

  // Robot geometry in robot cs: pose gives the position and
  // orientation, size gives the extent.  These values are filled in by
  // playerc_position_get_geom().
  double pose[3];
  double size[2];
  
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

// Get the position geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int playerc_position_get_geom(playerc_position_t *device);

// Set the robot speed.
// vx : forward speed (m/s).
// vy : sideways speed (m/s); this field is used by omni-drive robots only.
// va : rotational speed (radians/s).
// All speeds are defined in the robot coordinate system.
int  playerc_position_set_speed(playerc_position_t *device,
                                double vx, double vy, double va);


/***************************************************************************
 * proxy : ptz (pan-tilt-zoom camera) device
 **************************************************************************/

// PTZ device data
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;

  // The current ptz pan and tilt angles.
  // pan : pan angle (+ve to the left, -ve to the right).
  // tilt : tilt angle (+ve upwrds, -ve downwards).
  double pan, tilt;

  // The current zoom value.
  // 0 is wide angle; > 0 is telephoto.
  int zoom;
  
} playerc_ptz_t;


// Create a ptz proxy
playerc_ptz_t *playerc_ptz_create(playerc_client_t *client, int index);

// Destroy a ptz proxy
void playerc_ptz_destroy(playerc_ptz_t *device);

// Subscribe to the ptz device
int playerc_ptz_subscribe(playerc_ptz_t *device, int access);

// Un-subscribe from the ptz device
int playerc_ptz_unsubscribe(playerc_ptz_t *device);

// Set the pan, tilt and zoom values.
int playerc_ptz_set(playerc_ptz_t *device, double pan, double tilt, int zoom);


/***************************************************************************
 * proxy : sonar device
 **************************************************************************/

// Sonar device data
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;

  // Number of pose values.
  int pose_count;
  
  // Pose of each sonar relative to robot (m, m, radians).  This
  // structure is filled by calling playerc_sonar_get_geom().
  double poses[PLAYERC_SONAR_MAX_SCAN][3];
  
  // Number of points in the scan.
  int scan_count;

  // Scan data: range (m)
  double scan[PLAYERC_SONAR_MAX_SCAN];
  
} playerc_sonar_t;


// Create a sonar proxy
playerc_sonar_t *playerc_sonar_create(playerc_client_t *client, int index);

// Destroy a sonar proxy
void playerc_sonar_destroy(playerc_sonar_t *device);

// Subscribe to the sonar device
int playerc_sonar_subscribe(playerc_sonar_t *device, int access);

// Un-subscribe from the sonar device
int playerc_sonar_unsubscribe(playerc_sonar_t *device);

// Get the sonar geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int playerc_sonar_get_geom(playerc_sonar_t *device);


/***************************************************************************
 * proxy : truth (simulator ground truth) device
 **************************************************************************/

// Truth device proxy
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;

  // The object pose (world cs)
  double px, py, pa;
    
} playerc_truth_t;


// Create a truth proxy.
playerc_truth_t *playerc_truth_create(playerc_client_t *client, int index);

// Destroy a truth proxy.
void playerc_truth_destroy(playerc_truth_t *device);

// Subscribe to the truth device
int playerc_truth_subscribe(playerc_truth_t *device, int access);

// Un-subscribe from the truth device
int playerc_truth_unsubscribe(playerc_truth_t *device);

// Get the object pose.
// px, py, pa : the pose (global coordinates).
int playerc_truth_get_pose(playerc_truth_t *device, double *px, double *py, double *pa);

// Set the object pose.
// px, py, pa : the new pose (global coordinates).
int playerc_truth_set_pose(playerc_truth_t *device, double px, double py, double pa);


/***************************************************************************
 * proxy : vision device
 **************************************************************************/

// Description of a single blob.
typedef struct
{  
  // The blob "channel"; i.e. the color class this blob belongs to.
  int channel;

  // A descriptive color for the blob.  Stored as packed RGB 32, i.e.:
  // 0x00RRGGBB.
  uint32_t color;

  // Blob centroid (image coordinates).
  int x, y;

  // Blob area (pixels).
  int area;

  // Bounding box for blob (image coordinates).
  int left, top, right, bottom;
  
} playerc_vision_blob_t;


// Vision device data
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;

  // Image dimensions
  int width, height;
  
  // A list of detected blobs
  int blob_count;
  playerc_vision_blob_t blobs[PLAYERC_VISION_MAX_BLOBS];
  
} playerc_vision_t;


// Create a vision proxy
playerc_vision_t *playerc_vision_create(playerc_client_t *client, int index);

// Destroy a vision proxy
void playerc_vision_destroy(playerc_vision_t *device);

// Subscribe to the vision device
int playerc_vision_subscribe(playerc_vision_t *device, int access);

// Un-subscribe from the vision device
int playerc_vision_unsubscribe(playerc_vision_t *device);


/***************************************************************************
 * proxy : end (this is just here so the auto-documentation works.
 **************************************************************************/

#endif




