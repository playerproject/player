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

// Get the message structures from Player
#include "player.h"


/***************************************************************************
 * Array sizes
 **************************************************************************/

#define PLAYERC_LASER_MAX_SAMPLES       PLAYER_LASER_MAX_SAMPLES
#define PLAYERC_FIDUCIAL_MAX_SAMPLES    PLAYER_FIDUCIAL_MAX_SAMPLES
#define PLAYERC_SONAR_MAX_SAMPLES       PLAYER_SONAR_MAX_SAMPLES
#define PLAYERC_BLOBFINDER_MAX_BLOBS    64
#define PLAYERC_WIFI_MAX_LINKS          PLAYER_WIFI_MAX_LINKS


/***************************************************************************
 * Declare all of the basic types
 **************************************************************************/

// Forward declare types
struct _playerc_client_t;
struct _playerc_device_t;

// Items in incoming data queue.
typedef struct
{
  player_msghdr_t header;
  int len;
  void *data;
} playerc_client_item_t;


// Typedefs for proxy callback functions
typedef void (*playerc_putdata_fn_t) (void *device, char *header, char *data, size_t len);
typedef void (*playerc_callback_fn_t) (void *data);

// forward declaration to avoid including <sys/poll.h>, which may not be
// available when people are building clients against this lib
struct pollfd;

// Multi-client data
typedef struct
{
  // List of clients being managed
  int client_count;
  struct _playerc_client_t *client[128];

  // Poll info 
  struct pollfd* pollfd;

} playerc_mclient_t;


// Single client data
typedef struct _playerc_client_t
{
  // Server address
  char *host;
  int port;
    
  // Socket descriptor
  int sock;

  // List of ids for available devices.  This list is filled in by
  // playerc_client_get_devlist().
  int id_count;
  player_device_id_t ids[PLAYER_MAX_DEVICES];
  char drivernames[PLAYER_MAX_DEVICES][PLAYER_MAX_DEVICE_STRING_LEN];
    
  // List of subscribed devices
  int device_count;
  struct _playerc_device_t *device[32];

  // A circular queue used to buffer incoming data packets.
  int qfirst, qlen, qsize;
  playerc_client_item_t qitems[128];
    
} playerc_client_t;


// Player device proxy info; basically a base class for devices.
typedef struct _playerc_device_t
{
  // Pointer to the client proxy.
  playerc_client_t *client;

  // Device code, index, etc.
  int robot, code, index; //REMOVE, access;

  // The driver name
  char drivername[PLAYER_MAX_DEVICE_STRING_LEN];
  
  // The subscribe flag is non-zero if the device has been
  // successfully subscribed.
  int subscribed;

  // Data timestamp, i.e., the time at which the data was generated (s).
  double datatime;

  // Standard callbacks for this device (private).
  playerc_putdata_fn_t putdata;

  // Extra user data for this device (private).
  void *user_data;
  
  // Extra callbacks for this device (private).
  int callback_count;
  playerc_callback_fn_t callback[4];
  void *callback_data[4];

} playerc_device_t;


/***************************************************************************
 * Error handling
 **************************************************************************/

// Use this function to read the error string
extern char *playerc_error_str(void);


/***************************************************************************
 * Utility functions
 **************************************************************************/

// Get the name for a given device code.
extern const char *playerc_lookup_name(int code);

// Get the device code for a give name.
extern int playerc_lookup_code(const char *name);


/***************************************************************************
 * Multi-client functions
 **************************************************************************/

// Create a multi-client object
playerc_mclient_t *playerc_mclient_create(void);

// Destroy a multi-client object
void playerc_mclient_destroy(playerc_mclient_t *mclient);

// Add a client to the multi-client (private).
int playerc_mclient_addclient(playerc_mclient_t *mclient, playerc_client_t *client);

// Test to see if there is pending data.
// Returns -1 on error, 0 or 1 otherwise.
int playerc_mclient_peek(playerc_mclient_t *mclient, int timeout);

// Read incoming data.  The timeout is in ms.  Set timeout to a
// negative value to wait indefinitely.
int playerc_mclient_read(playerc_mclient_t *mclient, int timeout);


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
int playerc_client_connect(playerc_client_t *client);
int playerc_client_disconnect(playerc_client_t *client);

// Change the server's data delivery mode
int playerc_client_datamode(playerc_client_t *client, int mode);

// Add/remove a device proxy (private)
int playerc_client_adddevice(playerc_client_t *client, playerc_device_t *device);
int playerc_client_deldevice(playerc_client_t *client, playerc_device_t *device);

// Add/remove user callbacks (called when new data arrives).
int  playerc_client_addcallback(playerc_client_t *client, playerc_device_t *device,
                                playerc_callback_fn_t callback, void *data);
int  playerc_client_delcallback(playerc_client_t *client, playerc_device_t *device,
                                playerc_callback_fn_t callback, void *data);

// Get the list of available device ids.  The data is written into the
// proxy structure rather than returned to the caller.
int playerc_client_get_devlist(playerc_client_t *client);

// Subscribe/unsubscribe a device from the sever (private)
int playerc_client_subscribe(playerc_client_t *client, int robot, int code, int index,
                             int access, char *drivername, size_t len);
int playerc_client_unsubscribe(playerc_client_t *client, int robot, int code, int index);

// Issue a request to the server and await a reply (blocking).
// Returns -1 on error and -2 on NACK.
int playerc_client_request(playerc_client_t *client, playerc_device_t *device,
                           void *req_data, int req_len, void *rep_data, int rep_len);

// Test to see if there is pending data.
// Returns -1 on error, 0 or 1 otherwise.
int playerc_client_peek(playerc_client_t *client, int timeout);

// Read data from the server (blocking).  For data packets, will
// return a pointer to the device proxy that got the data; for synch
// packets, will return a pointer to the client itself; on error, will
// return NULL.
void *playerc_client_read(playerc_client_t *client);

// Write data to the server (private).
int playerc_client_write(playerc_client_t *client, playerc_device_t *device,
                         void *cmd, int len);


/***************************************************************************
 * proxy : base : Base device interface
 **************************************************************************/

// Initialise the device 
void playerc_device_init(playerc_device_t *device, playerc_client_t *client,
                         int robot, int code, int index, playerc_putdata_fn_t putdata);

// Finalize the device
void playerc_device_term(playerc_device_t *device);

// Subscribe/unsubscribe the device
int playerc_device_subscribe(playerc_device_t *device, int access);
int playerc_device_unsubscribe(playerc_device_t *device);


/***************************************************************************
 * proxy : blobfinder (visual color blob detector)
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
  
} playerc_blobfinder_blob_t;


// Blobfinder device data
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;

  // Image dimensions
  int width, height;
  
  // A list of detected blobs
  int blob_count;
  playerc_blobfinder_blob_t blobs[PLAYERC_BLOBFINDER_MAX_BLOBS];
  
} playerc_blobfinder_t;


// Create a blobfinder proxy
playerc_blobfinder_t *playerc_blobfinder_create(playerc_client_t *client, int robot, int index);

// Destroy a blobfinder proxy
void playerc_blobfinder_destroy(playerc_blobfinder_t *device);

// Subscribe to the blobfinder device
int playerc_blobfinder_subscribe(playerc_blobfinder_t *device, int access);

// Un-subscribe from the blobfinder device
int playerc_blobfinder_unsubscribe(playerc_blobfinder_t *device);


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
playerc_bps_t *playerc_bps_create(playerc_client_t *client, int robot, int index);

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
 * proxy : comms (broadcast UDP nework communications)
 **************************************************************************/

// Comms proxy.
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;

  // The most recent incoming message
  size_t msg_len;
  uint8_t msg[PLAYER_MAX_MESSAGE_SIZE];
    
} playerc_comms_t;


// Create a comms proxy
playerc_comms_t *playerc_comms_create(playerc_client_t *client, int robot, int index);

// Destroy a comms proxy
void playerc_comms_destroy(playerc_comms_t *device);

// Subscribe to the comms device
int playerc_comms_subscribe(playerc_comms_t *device, int access);

// Un-subscribe from the comms device
int playerc_comms_unsubscribe(playerc_comms_t *device);

// Send a comms message.
int playerc_comms_send(playerc_comms_t *device, void *msg, int len);

// Read the next comms message.
//REMOVE int playerc_comms_recv(playerc_comms_t *device, void *msg, int len);


/***************************************************************************
 * proxy : gps (global positioning system)
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
playerc_gps_t *playerc_gps_create(playerc_client_t *client, int robot, int index);

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
 * proxy : laser (scanning range-finder)
 **************************************************************************/

// Laser proxy.
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;

  // laser geometry in robot cs: pose gives the position and
  // orientation, size gives the extent.  These values are filled in by
  // playerc_laser_get_geom().
  double pose[3];
  double size[2];
  
  // Number of points in the scan.
  int scan_count;

  // Angular resolution of the scan.
  double scan_res;

  // Scan data; range (m) and bearing (radians).
  double scan[PLAYERC_LASER_MAX_SAMPLES][2];

  // Scan data; x, y position (m).
  double point[PLAYERC_LASER_MAX_SAMPLES][2];

  // Scan reflection intensity values (0-3).
  int intensity[PLAYERC_LASER_MAX_SAMPLES];
  
} playerc_laser_t;


// Create a laser proxy
playerc_laser_t *playerc_laser_create(playerc_client_t *client, int robot, int index);

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
 * proxy : fiducial (fiducial detector)
 **************************************************************************/ 

// Description for a single fiducial
typedef struct
{
  // Id (0 if fiducial cannot be identified).
  int id;

  // Beacon range, bearing and orientation.
  double range, bearing, orient;
  
} playerc_fiducial_item_t;


// Laser beacon data
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;

  // Geometry in robot cs.  These values are filled in by
  // playerc_fiducial_get_geom().  [pose] is the detector pose in the
  // robot cs, [size] is the detector size, [fiducial_size] is the
  // fiducial size.
  double pose[3];
  double size[2];
  double fiducial_size[2];
  
  // List of detected beacons.
  int fiducial_count;
  playerc_fiducial_item_t fiducials[PLAYERC_FIDUCIAL_MAX_SAMPLES];
    
} playerc_fiducial_t;


// Create a fiducial proxy
playerc_fiducial_t *playerc_fiducial_create(playerc_client_t *client, int robot, int index);

// Destroy a fiducial proxy
void playerc_fiducial_destroy(playerc_fiducial_t *device);

// Subscribe to the fiducial device
int playerc_fiducial_subscribe(playerc_fiducial_t *device, int access);

// Un-subscribe from the fiducial device
int playerc_fiducial_unsubscribe(playerc_fiducial_t *device);

// Get the laser geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int playerc_fiducial_get_geom(playerc_fiducial_t *device);


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
playerc_position_t *playerc_position_create(playerc_client_t *client, int robot, int index);

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
 * proxy : power device
 **************************************************************************/

// Power device
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;

  // Battery charge (volts)
  double charge;
  
} playerc_power_t;


// Create a power device proxy.
playerc_power_t *playerc_power_create(playerc_client_t *client, int robot, int index);

// Destroy a power device proxy.
void playerc_power_destroy(playerc_power_t *device);

// Subscribe to the power device
int playerc_power_subscribe(playerc_power_t *device, int access);

// Un-subscribe from the power device
int playerc_power_unsubscribe(playerc_power_t *device);


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

  // The current zoom value (field of view angle).
  double zoom;
  
} playerc_ptz_t;


// Create a ptz proxy
playerc_ptz_t *playerc_ptz_create(playerc_client_t *client, int robot, int index);

// Destroy a ptz proxy
void playerc_ptz_destroy(playerc_ptz_t *device);

// Subscribe to the ptz device
int playerc_ptz_subscribe(playerc_ptz_t *device, int access);

// Un-subscribe from the ptz device
int playerc_ptz_unsubscribe(playerc_ptz_t *device);

// Set the pan, tilt and zoom values.
int playerc_ptz_set(playerc_ptz_t *device, double pan, double tilt, double zoom);


/***************************************************************************
 * proxy : sonar (fixed range-finder)
 **************************************************************************/

// Sonar proxy.
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;

  // Number of pose values.
  int pose_count;
  
  // Pose of each sonar relative to robot (m, m, radians).  This
  // structure is filled by calling playerc_sonar_get_geom().
  double poses[PLAYERC_SONAR_MAX_SAMPLES][3];
  
  // Number of points in the scan.
  int scan_count;

  // Scan data: range (m)
  double scan[PLAYERC_SONAR_MAX_SAMPLES];
  
} playerc_sonar_t;


// Create a sonar proxy
playerc_sonar_t *playerc_sonar_create(playerc_client_t *client, int robot, int index);

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
playerc_truth_t *playerc_truth_create(playerc_client_t *client, int robot, int index);

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
 * proxy : wifi (wireless info) device
 **************************************************************************/

// Individual link info
typedef struct
{
  // Destination IP address
  char ip[32];
 
  // Link properties
  int link, level, noise;
 
} playerc_wifi_link_t;


// Wifi device proxy
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;

  // A list containing info for each link
  int link_count;
  playerc_wifi_link_t links[PLAYERC_WIFI_MAX_LINKS];
  
} playerc_wifi_t;


// Create a wifi proxy.
playerc_wifi_t *playerc_wifi_create(playerc_client_t *client, int robot, int index);

// Destroy a wifi proxy.
void playerc_wifi_destroy(playerc_wifi_t *device);

// Subscribe to the wifi device
int playerc_wifi_subscribe(playerc_wifi_t *device, int access);

// Un-subscribe from the wifi device
int playerc_wifi_unsubscribe(playerc_wifi_t *device);


/***************************************************************************
 * proxy : localization device
 **************************************************************************/

// Hypothesis data
typedef struct
{
  // Pose estimate (m, m, radians)
  double mean[3];

  // Covariance
  double cov[3][3];

  // Weight associated with this hypothesis
  double weight;
  
} playerc_localize_hypoth_t;

// Localize device data
typedef struct
{
  // Device info; must be at the start of all device structures.
  playerc_device_t info;

  // Map dimensions (cells)
  int map_size_x, map_size_y;

  // Map scale (m/cell)
  double map_scale;

  // Map data (empty = -1, unknown = 0, occupied = +1)
  int8_t *map_cells;

  // List of possible poses
  int hypoth_count;
  playerc_localize_hypoth_t hypoths[PLAYER_LOCALIZE_MAX_HYPOTHS];

} playerc_localize_t;


// Create a localize proxy
playerc_localize_t *playerc_localize_create(playerc_client_t *client, int robot, int index);

// Destroy a localize proxy
void playerc_localize_destroy(playerc_localize_t *device);

// Subscribe to the localize device
int playerc_localize_subscribe(playerc_localize_t *device, int access);

// Un-subscribe from the localize device
int playerc_localize_unsubscribe(playerc_localize_t *device);

// Reset the localize device
int playerc_localize_reset(playerc_localize_t *device);

// Retrieve the occupancy map.  The map is written into the proxy
// structure.
int playerc_localize_get_map(playerc_localize_t *device);

// Get the current configuration.
int playerc_localize_get_config(playerc_localize_t *device, player_localize_config_t *config);

// Modify the current configuration.
int playerc_localize_set_config(playerc_localize_t *device, player_localize_config_t config);


/***************************************************************************
 * proxy : end (this is just here so the auto-documentation works.
 **************************************************************************/

#endif




