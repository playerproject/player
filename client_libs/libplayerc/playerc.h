/* 
 *  libplayerc : a Player client library
 *  Copyright (C) Andrew Howard 2002-2003
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
/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) Andrew Howard 2003
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

#ifndef MIN
  #define MIN(a,b) ((a < b) ? a : b)
#endif
#ifndef MAX
  #define MAX(a,b) ((a > b) ? a : b)
#endif

#ifdef __cplusplus
extern "C" {
#endif

  
/***************************************************************************
 * Useful constants (re-defined here so SWIG can pick them up easily)
 **************************************************************************/

/** The device access modes */
#define PLAYERC_READ_MODE PLAYER_READ_MODE
#define PLAYERC_WRITE_MODE PLAYER_WRITE_MODE
#define PLAYERC_ALL_MODE PLAYER_ALL_MODE
#define PLAYERC_CLOSE_MODE PLAYER_CLOSE_MODE
#define PLAYERC_ERROR_MODE PLAYER_ERROR_MODE
  
/** The valid data delivery modes */
#define PLAYERC_DATAMODE_PUSH_ALL PLAYER_DATAMODE_PUSH_ALL 
#define PLAYERC_DATAMODE_PULL_ALL PLAYER_DATAMODE_PULL_ALL 
#define PLAYERC_DATAMODE_PUSH_NEW PLAYER_DATAMODE_PUSH_NEW 
#define PLAYERC_DATAMODE_PULL_NEW PLAYER_DATAMODE_PULL_NEW 
#define PLAYERC_DATAMODE_PUSH_ASYNC PLAYER_DATAMODE_PUSH_ASYNC


/***************************************************************************
 * Array sizes
 **************************************************************************/

#define PLAYERC_MAX_DEVICES             PLAYER_MAX_DEVICES
#define PLAYERC_LASER_MAX_SAMPLES       PLAYER_LASER_MAX_SAMPLES
#define PLAYERC_FIDUCIAL_MAX_SAMPLES    PLAYER_FIDUCIAL_MAX_SAMPLES
#define PLAYERC_SONAR_MAX_SAMPLES       PLAYER_SONAR_MAX_SAMPLES
#define PLAYERC_BUMPER_MAX_SAMPLES      PLAYER_BUMPER_MAX_SAMPLES
#define PLAYERC_IR_MAX_SAMPLES		      PLAYER_IR_MAX_SAMPLES
#define PLAYERC_BLOBFINDER_MAX_BLOBS    PLAYER_BLOBFINDER_MAX_BLOBS
#define PLAYERC_WIFI_MAX_LINKS          PLAYER_WIFI_MAX_LINKS

  

/** @addtogroup player_clientlib_libplayerc libplayerc */
/** @{ */
/** @defgroup player_clientlib_libplayerc_core Core functionality */
/** @{ */


/***************************************************************************/
/** @defgroup utility Utility and error-handling functions
@{
*/
/***************************************************************************/

/** Retrieve the last error (as a descriptive string).  Most functions
    in will return 0 on success and non-zero value on error; a
    descriptive error message can be obtained by calling this
    function. */
const char *playerc_error_str(void);

/** Get the name for a given interface code. */
const char *playerc_lookup_name(int code);

/** Get the interface code for a given name. */
int playerc_lookup_code(const char *name);

/** @}*/
/***************************************************************************/

  
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


/***************************************************************************
 * Multi-client functions
 **************************************************************************/

// Create a multi-client object
playerc_mclient_t *playerc_mclient_create(void);

// Destroy a multi-client object
void playerc_mclient_destroy(playerc_mclient_t *mclient);

// Add a client to the multi-client (private).
int playerc_mclient_addclient(playerc_mclient_t *mclient, struct _playerc_client_t *client);

// Test to see if there is pending data.
// Returns -1 on error, 0 or 1 otherwise.
int playerc_mclient_peek(playerc_mclient_t *mclient, int timeout);

// Read incoming data.  The timeout is in ms.  Set timeout to a
// negative value to wait indefinitely.
int playerc_mclient_read(playerc_mclient_t *mclient, int timeout);


/***************************************************************************/
/** @defgroup client Client proxy

The client proxy provides an interface to the Player server itself,
and therefore has somewhat different functionality from the regular
device proxies. The create function returns a pointer to an
playerc_client_t structure to be used in other subsequent calls.

@{
*/

/** Info about an available (but not necessarily subscribed) device. */
typedef struct
{  
  /** Player id of the device. */
  int port, code, index;

  /** The driver name. */
  char drivername[PLAYER_MAX_DEVICE_STRING_LEN];
  
} playerc_device_info_t;


/** Client data. */
typedef struct _playerc_client_t
{
  /** A useful ID for identifying devices; mostly used by other
      language bindings. */
  void *id;

  /** Server address. */
  char *host;
  int port;
    
  /** Socket descriptor */
  int sock;

  /** @{ */
  /** List of available (but not necessarily subscribed) devices.
      This list is filled in by playerc_client_get_devlist(). */
  playerc_device_info_t devinfos[PLAYERC_MAX_DEVICES];
  int devinfo_count;
  /** @} */

  /** @{ */
  /** List of subscribed devices */
  struct _playerc_device_t *device[32];
  int device_count;
  /** @} */

  /* @{ */
  // A circular queue used to buffer incoming data packets.
  playerc_client_item_t qitems[128];
  int qfirst, qlen, qsize;
  /* @} */

  /** Temp buffer for incoming packets. */
  char *data;

  /** Data time stamp on the last SYNC packet */
  double datatime;

} playerc_client_t;


/** Create a single-client object.
    Set mclient to NULL if this is a stand-alone client.*/
playerc_client_t *playerc_client_create(playerc_mclient_t *mclient,
                                        const char *host, int port);

/** Destroy a single-client object. */
void playerc_client_destroy(playerc_client_t *client);

/** Connect to the server. */
int playerc_client_connect(playerc_client_t *client);

/** Disconnect from the server. */
int playerc_client_disconnect(playerc_client_t *client);

/** Change the server's data delivery mode. */
int playerc_client_datamode(playerc_client_t *client, int mode);

/** Change the server's data delivery frequency (freq is in Hz) */
int playerc_client_datafreq(playerc_client_t *client, int freq);

// Add/remove a device proxy (private)
int playerc_client_adddevice(playerc_client_t *client, struct _playerc_device_t *device);
int playerc_client_deldevice(playerc_client_t *client, struct _playerc_device_t *device);

// Add/remove user callbacks (called when new data arrives).
int  playerc_client_addcallback(playerc_client_t *client, struct _playerc_device_t *device,
                                playerc_callback_fn_t callback, void *data);
int  playerc_client_delcallback(playerc_client_t *client, struct _playerc_device_t *device,
                                playerc_callback_fn_t callback, void *data);

/** Get the list of available device ids.  The data is written into the
    proxy structure rather than returned to the caller. */
int playerc_client_get_devlist(playerc_client_t *client);

// Subscribe/unsubscribe a device from the sever (private)
int playerc_client_subscribe(playerc_client_t *client, int code, int index,
                             int access, char *drivername, size_t len);
int playerc_client_unsubscribe(playerc_client_t *client, int code, int index);

// Issue a request to the server and await a reply (blocking).
// Returns -1 on error and -2 on NACK. (private)
int playerc_client_request(playerc_client_t *client, struct _playerc_device_t *device,
                           void *req_data, int req_len, void *rep_data, int rep_len);
                                
/** Test to see if there is pending data.
    Returns -1 on error, 0 or 1 otherwise. */
int playerc_client_peek(playerc_client_t *client, int timeout);

/** Read data from the server (blocking).  For data packets, will
    return the ID of the proxy that got the data; for synch packets,
    will return the ID of the client itself; on error, will return
    NULL. */
void *playerc_client_read(playerc_client_t *client);

// Write data to the server (private).
int playerc_client_write(playerc_client_t *client, struct _playerc_device_t *device,
                         void *cmd, int len);


/** @} */
/**************************************************************************/


/***************************************************************************/
/** @defgroup device Generic device proxy

The generic device proxy provides a `generic' interface to the
functionality that is shared by all devices (i.e., a base class, in
OOP parlance).  This proxy can be accessed through the info element
present in each of the concrete device proxies.  In general, this
proxy should not be instantiated directly.

@{
*/

/** Generic device info. */
typedef struct _playerc_device_t
{
  /** A useful ID for identifying devices; mostly used by other
      language bindings.  For backwards-compatibility, this is passed
      as void pointer. */
  void *id;

  /** Pointer to the client proxy. */
  playerc_client_t *client;

  /** Device code, index. */
  int code, index;

  /** The driver name. */
  char drivername[PLAYER_MAX_DEVICE_STRING_LEN];
  
  /** The subscribe flag is non-zero if the device has been
      successfully subscribed (read-only). */
  int subscribed;

  /** Data timestamp, i.e., the time at which the data was generated (s). */
  double datatime;

  /** Freshness flag.  Set to 1 whenever data is dispatched to this proxy.
      Useful with the mclient, but the user must manually set it to 0 after 
      using the data */
  int fresh;

  // Standard callbacks for this device (private).
  playerc_putdata_fn_t putdata;

  // Extra user data for this device (private).
  void *user_data;
  
  // Extra callbacks for this device (private).
  int callback_count;
  playerc_callback_fn_t callback[4];
  void *callback_data[4];

} playerc_device_t;


/* Initialise the device (private). */
void playerc_device_init(playerc_device_t *device, playerc_client_t *client,
                         int code, int index, playerc_putdata_fn_t putdata);

/* Finalize the device (private). */
void playerc_device_term(playerc_device_t *device);

/* Subscribe the device (private). */
int playerc_device_subscribe(playerc_device_t *device, int access);

/* Unsubscribe the device (private). */
int playerc_device_unsubscribe(playerc_device_t *device);

/** @} */
/**************************************************************************/

/** @} (core) */

/***************************************************************************/
/** @defgroup player_clientlib_libplayer_proxies Proxies
    @{
*/
/***************************************************************************/


/***************************************************************************/
/** @defgroup playerc_proxy_blobfinder blobfinder

The blobfinder proxy provides an interface to color blob detectors
such as the ACTS vision system.  See the Player User Manual for a
complete description of the drivers that support this interface.

@{
*/

/** Description of a single blob. */
typedef struct
{  
  /** The blob id; e.g. the color class this blob belongs to. */
  int id;

  /** A descriptive color for the blob.  Stored as packed RGB 32, i.e.:
      0x00RRGGBB. */
  uint32_t color;

  /** Blob centroid (image coordinates). */
  int x, y;

  /** Bounding box for blob (image coordinates). */
  int left, top, right, bottom;

  /** Blob area (pixels). */
  int area;

  /** Blob range (m). */
  double range;
  
} playerc_blobfinder_blob_t;


/** Blobfinder device data. */
typedef struct 
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Image dimensions (pixels). */
  int width, height;
  
  /** A list of detected blobs. */
  int blob_count;
  playerc_blobfinder_blob_t blobs[PLAYERC_BLOBFINDER_MAX_BLOBS];
  
} playerc_blobfinder_t;


/** Create a blobfinder proxy. */
playerc_blobfinder_t *playerc_blobfinder_create(playerc_client_t *client, int index);

/** Destroy a blobfinder proxy. */
void playerc_blobfinder_destroy(playerc_blobfinder_t *device);

/** Subscribe to the blobfinder device. */
int playerc_blobfinder_subscribe(playerc_blobfinder_t *device, int access);

/** Un-subscribe from the blobfinder device. */
int playerc_blobfinder_unsubscribe(playerc_blobfinder_t *device);

/** @internal Parse data from incoming packet */
void playerc_blobfinder_putdata(playerc_blobfinder_t *device, player_msghdr_t *header,
                                player_blobfinder_data_t *data, size_t len);


/** @} */
/**************************************************************************/


/**************************************************************************/
/** @defgroup playerc_proxy_bumper bumper

The bumper proxy provides an interface to the bumper sensors built
into robots such as the RWI B21R.

@{
*/

/** Bumper proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Number of pose values. */
  int pose_count;
  
  /** Pose of each bumper relative to robot (mm, mm, deg, mm, mm).
      This structure is filled by calling playerc_bumper_get_geom().
      values are x,y (of center) ,normal,length,curvature */
  double poses[PLAYERC_BUMPER_MAX_SAMPLES][5];
  
  /** Number of points in the scan. */
  int bumper_count;

  /** Bump data: unsigned char, either boolean or code indicating corner. */
  double bumpers[PLAYERC_BUMPER_MAX_SAMPLES];
  
} playerc_bumper_t;


/** Create a bumper proxy. */
playerc_bumper_t *playerc_bumper_create(playerc_client_t *client, int index);

/** Destroy a bumper proxy. */
void playerc_bumper_destroy(playerc_bumper_t *device);

/** Subscribe to the bumper device. */
int playerc_bumper_subscribe(playerc_bumper_t *device, int access);

/** Un-subscribe from the bumper device. */
int playerc_bumper_unsubscribe(playerc_bumper_t *device);

/** Get the bumper geometry.  The writes the result into the proxy
    rather than returning it to the caller. */
int playerc_bumper_get_geom(playerc_bumper_t *device);


/** @} */
/***************************************************************************/


/***************************************************************************/
/** @defgroup playerc_proxy_camera camera

The camera proxy can be used to get images from a camera.  

@{
*/

/** Camera proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Image dimensions (pixels). */
  int width, height;

  /** Image bits-per-pixel (8, 16, 24). */
  int bpp;

  /** Image format (e.g., RGB888). */
  int format;

  /** Some images (such as disparity maps) use scaled pixel values;
      for these images, fdiv specifies the scale divisor (i.e., divide
      the integer pixel value by fdiv to recover the real pixel value). */
  int fdiv;

  /** Image compression method. */
  int compression;

  /** Size of image data (bytes) */
  int image_size;
  
  /** Image data (packed format). */
  uint8_t image[PLAYER_CAMERA_IMAGE_SIZE];
    
} playerc_camera_t;


/** Create a camera proxy. */
playerc_camera_t *playerc_camera_create(playerc_client_t *client, int index);

/** Destroy a camera proxy. */
void playerc_camera_destroy(playerc_camera_t *device);

/** Subscribe to the camera device. */
int playerc_camera_subscribe(playerc_camera_t *device, int access);

/** Un-subscribe from the camera device. */
int playerc_camera_unsubscribe(playerc_camera_t *device);

/** Decompress the image (modifies the current proxy data). */
void playerc_camera_decompress(playerc_camera_t *device);


/** @} */
/**************************************************************************/

/***************************************************************************/
/** @defgroup playerc_proxy_fiducial fiducial

The fiducial proxy provides an interface to a fiducial detector.  This
device looks for fiducials (markers or beacons) in the laser scan, and
determines their identity, range, bearing and orientation.  See the
Player User Manual for a complete description of the various drivers
that support the fiducial interface.

@{
*/


/** Description for a single fiducial. */
typedef struct
{
  /** Id (0 if fiducial cannot be identified). */
  int id;

  /** @deprecated Beacon range, bearing and orientation. */
  double range, bearing, orient;

  /** Relative beacon position (x, y, z). */
  double pos[3];

  /** Relative beacon orientation (r, p, y). */
  double rot[3];

  /** Position uncertainty (x, y, z). */
  double upos[3];

  /** Orientation uncertainty (r, p, y). */
  double urot[3];

} playerc_fiducial_item_t;


/** Fiducial finder data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Geometry in robot cs.  These values are filled in by
      playerc_fiducial_get_geom().  [pose] is the detector pose in the
      robot cs, [size] is the detector size, [fiducial_size] is the
      fiducial size. */
  double pose[3];
  double size[2];
  double fiducial_size[2];
  
  /** List of detected beacons. */
  int fiducial_count;
  playerc_fiducial_item_t fiducials[PLAYERC_FIDUCIAL_MAX_SAMPLES];
    
} playerc_fiducial_t;


/** Create a fiducial proxy. */
playerc_fiducial_t *playerc_fiducial_create(playerc_client_t *client, int index);

/** Destroy a fiducial proxy. */
void playerc_fiducial_destroy(playerc_fiducial_t *device);

/** Subscribe to the fiducial device. */
int playerc_fiducial_subscribe(playerc_fiducial_t *device, int access);

/** Un-subscribe from the fiducial device. */
int playerc_fiducial_unsubscribe(playerc_fiducial_t *device);

/** Get the fiducial geometry.  The writes the result into the proxy
    rather than returning it to the caller. */
int playerc_fiducial_get_geom(playerc_fiducial_t *device);


/** @} */
/**************************************************************************/


/***************************************************************************/
/** @defgroup playerc_proxy_gps gps

The gps proxy provides an interface to a GPS-receiver.

@{
*/

/** GPS proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** UTC time (seconds since the epoch) */
  double utc_time;
  
  /** Latitude and logitude (degrees).  Latitudes are positive for
      north, negative for south.  Logitudes are positive for east,
      negative for west. */
  double lat, lon;

  /** Altitude (meters).  Positive is above sea-level, negative is
      below. */
  double alt;

  /** UTM easting and northing (meters). */
  double utm_e, utm_n;

  /** Horizontal dilution of precision. */
  double hdop;

  /** Horizontal and vertical error (meters). */
  double err_horz, err_vert;

  /** Quality of fix 0 = invalid, 1 = GPS fix, 2 = DGPS fix */
  int quality;
     
  /** Number of satellites in view. */
  int sat_count;

} playerc_gps_t;


/** Create a gps proxy. */
playerc_gps_t *playerc_gps_create(playerc_client_t *client, int index);

/** Destroy a gps proxy. */
void playerc_gps_destroy(playerc_gps_t *device);

/** Subscribe to the gps device. */
int playerc_gps_subscribe(playerc_gps_t *device, int access);

/** Un-subscribe from the gps device. */
int playerc_gps_unsubscribe(playerc_gps_t *device);


/** @} */
/**************************************************************************/

/***************************************************************************/
/** @defgroup playerc_proxy_ir ir

The ir proxy provides an interface to the ir sensors built into robots
such as the RWI B21R.

@{
*/

/** Ir proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  // data
  player_ir_data_t ranges;
  
  // config
  player_ir_pose_t poses;

} playerc_ir_t;


/** Create a ir proxy. */
playerc_ir_t *playerc_ir_create(playerc_client_t *client, int index);

/** Destroy a ir proxy. */
void playerc_ir_destroy(playerc_ir_t *device);

/** Subscribe to the ir device. */
int playerc_ir_subscribe(playerc_ir_t *device, int access);

/** Un-subscribe from the ir device. */
int playerc_ir_unsubscribe(playerc_ir_t *device);

/** Get the ir geometry.  The writes the result into the proxy
    rather than returning it to the caller. */
int playerc_ir_get_geom(playerc_ir_t *device);


/** @} */
/***************************************************************************/


/***************************************************************************/
/** @defgroup playerc_proxy_joystick joystick

The joystick proxy provides an interface to joysticks.

@{
*/

/** Joystick device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;
  
  /** Scaled joystick position (0 = centered, 1 = hard over). */
  double px, py;

  /** Button states (bitmask) */
  uint16_t buttons;
  
} playerc_joystick_t;


/** Create a joystick device proxy. */
playerc_joystick_t *playerc_joystick_create(playerc_client_t *client, int index);

/** Destroy a joystick device proxy. */
void playerc_joystick_destroy(playerc_joystick_t *device);

/** Subscribe to the joystick device */
int playerc_joystick_subscribe(playerc_joystick_t *device, int access);

/** Un-subscribe from the joystick device */
int playerc_joystick_unsubscribe(playerc_joystick_t *device);

/** @internal Parse data from incoming packet */
void playerc_joystick_putdata(playerc_joystick_t *device, player_msghdr_t *header,
                              player_joystick_data_t *data, size_t len);


/** @} */
/**************************************************************************/


/***************************************************************************/
/** @defgroup playerc_proxy_laser laser
    
The laser proxy provides an interface to scanning laser range finders
such as the @ref player_driver_sicklms200.  Data is returned in the
playerc_laser_t structure.

This proxy wraps the low-level @ref player_interface_laser interface.

@{
*/

/** Laser proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** @{ */
  /** Laser geometry in the robot cs: pose gives the position and
      orientation, size gives the extent.  These values are filled in by
      playerc_laser_get_geom(). */
  double pose[3];
  double size[2];
  /** @} */
  
  /** Number of points in the scan. */
  int scan_count;

  /** Start bearing of the scan (radians). */
  double scan_start;

  /** Angular resolution of the scan (radians). */
  double scan_res;

  /** Range resolution multiplier */
  int range_res;

  /** Raw range data; range (m). */
  double ranges[PLAYERC_LASER_MAX_SAMPLES];

  /** Scan data; range (m) and bearing (radians). */
  double scan[PLAYERC_LASER_MAX_SAMPLES][2];
  
  /** Scan data; x, y position (m). */
  double point[PLAYERC_LASER_MAX_SAMPLES][2];

  /** Scan reflection intensity values (0-3).  Note that the intensity
      values will only be filled if intensity information is enabled
      (using the set_config function). */
  int intensity[PLAYERC_LASER_MAX_SAMPLES];
  
} playerc_laser_t;


/** Create a laser proxy. */
playerc_laser_t *playerc_laser_create(playerc_client_t *client, int index);

/** Destroy a laser proxy. */
void playerc_laser_destroy(playerc_laser_t *device);

/** Subscribe to the laser device. */
int playerc_laser_subscribe(playerc_laser_t *device, int access);

/** Un-subscribe from the laser device. */
int playerc_laser_unsubscribe(playerc_laser_t *device);

/** @internal Parse data from incoming packet */
void playerc_laser_putdata(playerc_laser_t *device, player_msghdr_t *header,
                           player_laser_data_t *data, size_t len);

/** Configure the laser.
    min_angle, max_angle : Start and end angles for the scan.
    resolution : Resolution in 0.01 degree increments.  Valid values are 25, 50, 100.
    range_res : Range resolution.  Valid: 1, 10, 100.
    intensity : Intensity flag; set to 1 to enable reflection intensity data. */
int  playerc_laser_set_config(playerc_laser_t *device, double min_angle,
			      double max_angle, int resolution, 
			      int range_res, int intensity);

/** Get the laser configuration
    min_angle, max_angle : Start and end angles for the scan.
    resolution : Resolution is in 0.01 degree increments.
    range_res : Range Resolution.  Valid: 1, 10, 100.
    intensity : Intensity flag; set to 1 to enable reflection intensity data. */
int  playerc_laser_get_config(playerc_laser_t *device, double *min_angle,
			      double *max_angle, int *resolution, 
			      int *range_res, int *intensity);

/** Get the laser geometry.  The writes the result into the proxy
    rather than returning it to the caller. */
int playerc_laser_get_geom(playerc_laser_t *device);

/** @} */
/**************************************************************************/

/***************************************************************************/
/** @defgroup playerc_proxy_localize localize

The localize proxy provides an interface to localization drivers.
Generally speaking, these are abstract drivers that attempt to
localize the robot by matching sensor observations (odometry, laser
and/or sonar) against a prior map of the environment (such as an
occupancy grid).  Since the pose may be ambiguous, multiple hypotheses
may returned; each hypothesis specifies one possible pose estimate for
the robot.  See the Player manual for details of the localize
interface, and and drivers that support it (such as the amcl driver).

@{
*/

/** Hypothesis data. */
typedef struct
{
  /** Pose estimate (x, y, theta) in (m, m, radians). */
  double mean[3];

  /** Covariance. */
  double cov[3][3];

  /** Weight associated with this hypothesis. */
  double weight;
  
} playerc_localize_hypoth_t;


/** Localization device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Map dimensions (cells). */
  int map_size_x, map_size_y;
  
  /** Map scale (m/cell). */
  double map_scale;

  /** Next map tile to read. */
  int map_tile_x, map_tile_y;
  
  /** Map data (empty = -1, unknown = 0, occupied = +1). */
  int8_t *map_cells;

  /** The number of pending (unprocessed) sensor readings. */
  int pending_count;

  /** The timestamp on the last reading processed. */
  double pending_time;

  /** List of possible poses. */
  int hypoth_count;
  playerc_localize_hypoth_t hypoths[PLAYER_LOCALIZE_MAX_HYPOTHS];

} playerc_localize_t;


/** Create a localize proxy. */
playerc_localize_t *playerc_localize_create(playerc_client_t *client, int index);

/** Destroy a localize proxy. */
void playerc_localize_destroy(playerc_localize_t *device);

/** Subscribe to the localize device. */
int playerc_localize_subscribe(playerc_localize_t *device, int access);

/** Un-subscribe from the localize device. */
int playerc_localize_unsubscribe(playerc_localize_t *device);

/** Set the the robot pose (mean and covariance). */
int playerc_localize_set_pose(playerc_localize_t *device, double pose[3], double cov[3][3]);

/* REMOVE
// deprecated: get the map from the map interface now
int playerc_localize_get_map_info(playerc_localize_t *device);
int playerc_localize_get_map_tile(playerc_localize_t *device);
int playerc_localize_get_map(playerc_localize_t *device);
*/

/** Get the current configuration. */
int playerc_localize_get_config(playerc_localize_t *device, player_localize_config_t *config);

/** Modify the current configuration. */
int playerc_localize_set_config(playerc_localize_t *device, player_localize_config_t config);


/** @} */
/**************************************************************************/


/***************************************************************************/
/** @defgroup playerc_proxy_log log

The log proxy provides start/stop control of data logging

@{
*/

/** Log proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** What kind of log device is this? Either PLAYER_LOG_TYPE_READ or
      PLAYER_LOG_TYPE_WRITE. Call playerc_log_get_state() to fill it. */
  int type;

  /** Is logging/playback enabled? Call playerc_log_get_state() to 
      fill it. */
  int state;
} playerc_log_t;


/** Create a log proxy. */
playerc_log_t *playerc_log_create(playerc_client_t *client, int index);

/** Destroy a log proxy. */
void playerc_log_destroy(playerc_log_t *device);

/** Subscribe to the log device. */
int playerc_log_subscribe(playerc_log_t *device, int access);

/** Un-subscribe from the log device. */
int playerc_log_unsubscribe(playerc_log_t *device);

/** Start/stop logging */
int playerc_log_set_write_state(playerc_log_t* device, int state);

/** Start/stop playback */
int playerc_log_set_read_state(playerc_log_t* device, int state);

/** Rewind playback */
int playerc_log_set_read_rewind(playerc_log_t* device);

/** Get logging/playback state; the result is written into the proxy */
int playerc_log_get_state(playerc_log_t* device);


/** @} */



/***************************************************************************/
/** @defgroup playerc_proxy_map map

The map proxy provides an interface to a map.

@{
*/

/** Map proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Map resolution, m/cell */
  double resolution;

  /** Map size, in cells */
  int width, height;

  /** Occupancy for each cell (empty = -1, unknown = 0, occupied = +1) */
  char* cells;
} playerc_map_t;


#define PLAYERC_MAP_INDEX(dev, i, j) ((dev->width) * (j) + (i))

/** Create a map proxy. */
playerc_map_t *playerc_map_create(playerc_client_t *client, int index);

/** Destroy a map proxy. */
void playerc_map_destroy(playerc_map_t *device);

/** Subscribe to the map device. */
int playerc_map_subscribe(playerc_map_t *device, int access);

/** Un-subscribe from the map device. */
int playerc_map_unsubscribe(playerc_map_t *device);

/** Get the map, which is stored in the proxy. */
int playerc_map_get_map(playerc_map_t* device);


/** @} */
/**************************************************************************/


/***************************************************************************/
/** @defgroup playerc_proxy_motor motor

The motor proxy provides an interface a simple, single motor.

@{
*/

/** Motor device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Odometric pose (radians). */
  double pt;

  /** Odometric velocity (radians). */
  double vt;

  /** Stall flag [0, 1]. */
  int stall;

} playerc_motor_t;

/** [Methods] */

/** Create a motor device proxy. */
playerc_motor_t *playerc_motor_create(playerc_client_t *client, int index);

/** Destroy a motor device proxy. */
void playerc_motor_destroy(playerc_motor_t *device);

/** Subscribe to the motor device */
int playerc_motor_subscribe(playerc_motor_t *device, int access);

/** Un-subscribe from the motor device */
int playerc_motor_unsubscribe(playerc_motor_t *device);

/** Enable/disable the motors */
int playerc_motor_enable(playerc_motor_t *device, int enable);

/** Change position control 0=velocity;1=position */
int playerc_motor_position_control(playerc_motor_t *device, int type);

/** Set the target rotational velocity (vt) in rad/s.  */
int playerc_motor_set_cmd_vel(playerc_motor_t *device,
                                 double vt, int state);

/** Set the target pose (pt) is the target pose in rad/s. */
int playerc_motor_set_cmd_pose(playerc_motor_t *device,
                                  double gt, int state);

/** @} */
/**************************************************************************/

/***************************************************************************/
/** @defgroup playerc_proxy_planner planner

The planner proxy provides an interface to a 2D motion planner.

@{
*/

/** Planner device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Did the planner find a valid path? */
  int path_valid;

  /** Have we arrived at the goal? */
  int path_done;

  /** Current pose (m, m, radians). */
  double px, py, pa;

  /** Goal location (m, m, radians) */
  double gx, gy, ga;

  /** Current waypoint location (m, m, radians) */
  double wx, wy, wa;

  /** Current waypoint index (handy if you already have the list
      of waypoints). May be negative if there's no plan, or if 
      the plan is done */
  int curr_waypoint;

  /** Number of waypoints in the plan */
  int waypoint_count;

  /** List of waypoints in the current plan (m,m,radians).  Call
      playerc_planner_get_waypoints() to fill this in. */
  double waypoints[PLAYER_PLANNER_MAX_WAYPOINTS][3];
  
} playerc_planner_t;

/** [Methods] */

/** Create a planner device proxy. */
playerc_planner_t *playerc_planner_create(playerc_client_t *client, int index);

/** Destroy a planner device proxy. */
void playerc_planner_destroy(playerc_planner_t *device);

/** Subscribe to the planner device */
int playerc_planner_subscribe(playerc_planner_t *device, int access);

/** Un-subscribe from the planner device */
int playerc_planner_unsubscribe(playerc_planner_t *device);

/** Set the goal pose (gx, gy, ga) */
int playerc_planner_set_cmd_pose(playerc_planner_t *device,
                                  double gx, double gy, double ga, int state);

/** Get the list of waypoints. Writes the result into the proxy rather
    than returning it to the caller. */
int playerc_planner_get_waypoints(playerc_planner_t *device);

/** @} */
/**************************************************************************/


/***************************************************************************/
/** @defgroup playerc_proxy_position position 

The position proxy provides an interface to a mobile robot base, such
as the ActiveMedia Pioneer series.  The proxy supports both
differential drive robots (which are capable of forward motion and
rotation) and omni-drive robots (which are capable of forward,
sideways and rotational motion).

@{
*/


/** Position device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Robot geometry in robot cs: pose gives the position and
      orientation, size gives the extent.  These values are filled in
      by playerc_position_get_geom(). */
  double pose[3];
  double size[2];
  
  /** Odometric pose (m, m, radians). */
  double px, py, pa;

  /** Odometric velocity (m, m, radians). */
  double vx, vy, va;
  
  /** Stall flag [0, 1]. */
  int stall;
} playerc_position_t;


/** Create a position device proxy. */
playerc_position_t *playerc_position_create(playerc_client_t *client, int index);

/** Destroy a position device proxy. */
void playerc_position_destroy(playerc_position_t *device);

/** Subscribe to the position device */
int playerc_position_subscribe(playerc_position_t *device, int access);

/** Un-subscribe from the position device */
int playerc_position_unsubscribe(playerc_position_t *device);

/** Enable/disable the motors */
int playerc_position_enable(playerc_position_t *device, int enable);

/** Get the position geometry.  The writes the result into the proxy
    rather than returning it to the caller. */
int playerc_position_get_geom(playerc_position_t *device);

/** Set the target speed.  vx : forward speed (m/s).  vy : sideways
    speed (m/s); this field is used by omni-drive robots only.  va :
    rotational speed (radians/s).  All speeds are defined in the robot
    coordinate system. */
int playerc_position_set_cmd_vel(playerc_position_t *device,
                                 double vx, double vy, double va, int state);

/** Set the target pose (gx, gy, ga) is the target pose in the
    odometric coordinate system. */
int playerc_position_set_cmd_pose(playerc_position_t *device,
                                  double gx, double gy, double ga, int state);


/** @} */
/**************************************************************************/

 
/***************************************************************************/
/** @defgroup playerc_proxy_position2d position2d

The position2d proxy provides an interface to a mobile robot base,
such as the ActiveMedia Pioneer series.  The proxy supports both
differential drive robots (which are capable of forward motion and
rotation) and omni-drive robots (which are capable of forward,
sideways and rotational motion).

@{
*/

/** Position2d device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Robot geometry in robot cs: pose gives the position2d and
      orientation, size gives the extent.  These values are filled in
      by playerc_position2d_get_geom(). */
  double pose[3];
  double size[2];

  /** Odometric pose (m, m, rad). */
  double px, py, pa;

  /** Odometric velocity (m/s, m/s, rad/s). */
  double vx, vy, va;

  /** Stall flag [0, 1]. */
  int stall;

} playerc_position2d_t;


/** Create a position2d device proxy. */
playerc_position2d_t *playerc_position2d_create(playerc_client_t *client, int index);

/** Destroy a position2d device proxy. */
void playerc_position2d_destroy(playerc_position2d_t *device);

/** Subscribe to the position2d device */
int playerc_position2d_subscribe(playerc_position2d_t *device, int access);

/** Un-subscribe from the position2d device */
int playerc_position2d_unsubscribe(playerc_position2d_t *device);

/** Enable/disable the motors */
int playerc_position2d_enable(playerc_position2d_t *device, int enable);

/** Get the position2d geometry.  The writes the result into the proxy
    rather than returning it to the caller. */
int playerc_position2d_get_geom(playerc_position2d_t *device);

/** Set the target speed.  vx : forward speed (m/s).  vy : sideways
    speed (m/s); this field is used by omni-drive robots only.  va :
    rotational speed (rad/s).  All speeds are defined in the robot
    coordinate system. */
int playerc_position2d_set_cmd_vel(playerc_position2d_t *device,
                                   double vx, double vy, double va, int state);

/** Set the target pose (gx, gy, ga) is the target pose in the
    odometric coordinate system. */
int playerc_position2d_set_cmd_pose(playerc_position2d_t *device,
                                    double gx, double gy, double ga, int state);

/** @} */
/**************************************************************************/


/***************************************************************************/
/** @defgroup playerc_proxy_position3d position3d

The position3d proxy provides an interface to a mobile robot base,
such as the Segway RMP series.  The proxy supports both differential
drive robots (which are capable of forward motion and rotation) and
omni-drive robots (which are capable of forward, sideways and
rotational motion).

@{
*/


/** Position3d device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Robot geometry in robot cs: pose gives the position3d and
      orientation, size gives the extent.  These values are filled in
      by playerc_position3d_get_geom(). */
  double pose[3];
  double size[2];

  /** Device position (m). */
  double pos_x, pos_y, pos_z;

  /** Device orientation (radians). */
  double pos_roll, pos_pitch, pos_yaw;

  /** Linear velocity (m/s). */
  double vel_x, vel_y, vel_z;

  /** Angular velocity (radians/sec). */
  double vel_roll, vel_pitch, vel_yaw;

  /** Stall flag [0, 1]. */
  int stall;

} playerc_position3d_t;


/** Create a position3d device proxy. */
playerc_position3d_t *playerc_position3d_create(playerc_client_t *client,
						int index);

/** Destroy a position3d device proxy. */
void playerc_position3d_destroy(playerc_position3d_t *device);

/** Subscribe to the position3d device */
int playerc_position3d_subscribe(playerc_position3d_t *device, int access);

/** Un-subscribe from the position3d device */
int playerc_position3d_unsubscribe(playerc_position3d_t *device);

/** @internal Parse data from incoming packet */
void playerc_position3d_putdata(playerc_position3d_t *device, player_msghdr_t *header,
                                player_position3d_data_t *data, size_t len);

/** Enable/disable the motors */
int playerc_position3d_enable(playerc_position3d_t *device, int enable);

/** Get the position3d geometry.  The writes the result into the proxy
    rather than returning it to the caller. */
int playerc_position3d_get_geom(playerc_position3d_t *device);

/** Set the target speed.  vx : forward speed (m/s).  vy : sideways
    speed (m/s); vz : vertical speed (m/s). vr : roll speed (rad/s) .
    vp : pitch speed (rad/s) . vt : theta speed (rad/s).
    All speeds are defined in the robot coordinate system. */
int playerc_position3d_set_velocity(playerc_position3d_t *device,
                                    double vx, double vy, double vz,
                                    double vr, double vp, double vt, int state);

/** For compatibility with old position3d interface */
int playerc_position3d_set_speed(playerc_position3d_t *device,
                                 double vx, double vy, double vz, int state);

/** Set the target pose (gx, gy, ga, gr, gp, gt) is the target pose in the
    odometric coordinate system. */
int playerc_position3d_set_pose(playerc_position3d_t *device,
                                double gx, double gy, double gz,
                                double gr, double gp, double gt);

/** For compatibility with old position3d interface */
int playerc_position3d_set_cmd_pose(playerc_position3d_t *device,
                                    double gx, double gy, double gz);

/** @} */
/**************************************************************************/

/***************************************************************************/
/** @defgroup playerc_proxy_power power

The power proxy provides an interface through which battery levels can
    be monitored.

@{
*/

/** Power device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Battery charge (volts). */
  double charge;
  
} playerc_power_t;

/** [Methods] */

/** Create a power device proxy. */
playerc_power_t *playerc_power_create(playerc_client_t *client, int index);

/** Destroy a power device proxy. */
void playerc_power_destroy(playerc_power_t *device);

/** Subscribe to the power device. */
int playerc_power_subscribe(playerc_power_t *device, int access);

/** Un-subscribe from the power device. */
int playerc_power_unsubscribe(playerc_power_t *device);


/** @} */
/**************************************************************************/ 


/***************************************************************************/
/** @defgroup playerc_proxy_ptz ptz

The ptz proxy provides an interface to pan-tilt units such as the Sony
PTZ camera.

@{
*/

/** PTZ device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** The current ptz pan and tilt angles.  pan : pan angle (+ve to
      the left, -ve to the right).  tilt : tilt angle (+ve upwrds, -ve
      downwards). */
  double pan, tilt;

  /** The current zoom value (field of view angle). */
  double zoom;
  
} playerc_ptz_t;


/** Create a ptz proxy. */
playerc_ptz_t *playerc_ptz_create(playerc_client_t *client, int index);

/** Destroy a ptz proxy. */
void playerc_ptz_destroy(playerc_ptz_t *device);

/** Subscribe to the ptz device. */
int playerc_ptz_subscribe(playerc_ptz_t *device, int access);

/** Un-subscribe from the ptz device. */
int playerc_ptz_unsubscribe(playerc_ptz_t *device);

/** Set the pan, tilt and zoom values. */
int playerc_ptz_set(playerc_ptz_t *device, double pan, double tilt, double zoom);

/** Set the pan, tilt and zoom values. (and speed) */
int playerc_ptz_set_ws(playerc_ptz_t *device, double pan, double tilt, double zoom, double panspeed, double tiltspeed);


/** @} */
/**************************************************************************/ 


/***************************************************************************/
/** @defgroup playerc_proxy_sonar sonar

The sonar proxy provides an interface to the sonar range sensors built
into robots such as the ActiveMedia Pioneer series.

@{
*/

/** Sonar proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Number of pose values. */
  int pose_count;
  
  /** Pose of each sonar relative to robot (m, m, radians).  This
      structure is filled by calling playerc_sonar_get_geom(). */
  double poses[PLAYERC_SONAR_MAX_SAMPLES][3];
  
  /** Number of points in the scan. */
  int scan_count;

  /** Scan data: range (m). */
  double scan[PLAYERC_SONAR_MAX_SAMPLES];
  
} playerc_sonar_t;


/** Create a sonar proxy. */
playerc_sonar_t *playerc_sonar_create(playerc_client_t *client, int index);

/** Destroy a sonar proxy. */
void playerc_sonar_destroy(playerc_sonar_t *device);

/** Subscribe to the sonar device. */
int playerc_sonar_subscribe(playerc_sonar_t *device, int access);

/** Un-subscribe from the sonar device. */
int playerc_sonar_unsubscribe(playerc_sonar_t *device);

/** Get the sonar geometry.  The writes the result into the proxy
    rather than returning it to the caller. */
int playerc_sonar_get_geom(playerc_sonar_t *device);

/** @} */
/**************************************************************************/


/***************************************************************************/
/** @defgroup playerc_proxy_truth truth
 
The truth proxy can be used to get and set the pose of objects in a  simulator.

@{
*/


/** Truth proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** The object pose (world cs). */
  double px, py, pa;
    
} playerc_truth_t;


/** Create a truth proxy. */
playerc_truth_t *playerc_truth_create(playerc_client_t *client, int index);

/** Destroy a truth proxy. */
void playerc_truth_destroy(playerc_truth_t *device);

/** Subscribe to the truth device. */
int playerc_truth_subscribe(playerc_truth_t *device, int access);

/** Un-subscribe from the truth device. */
int playerc_truth_unsubscribe(playerc_truth_t *device);

/** Get the object pose.
    px, py, pa : the pose (global coordinates). */
int playerc_truth_get_pose(playerc_truth_t *device, double *px, double *py, double *pa);

/** Set the object pose.  px, py, pa : the new pose (global
    coordinates). */
int playerc_truth_set_pose(playerc_truth_t *device, double px, double py, double pa);


/** @} */
/***************************************************************************/


/***************************************************************************/
/** @defgroup playerc_proxy_wifi wifi

The wifi proxy is used to query the state of a wireless network.  It
returns information such as the link quality and signal strength of
access points or of other wireless NIC's on an ad-hoc network.

@{
*/

/** Individual link info. */
typedef struct
{
  /** Mac accress. */
  char mac[32];

  /** IP address. */
  char ip[32];

  /** ESSID id */
  char essid[32];

  /** Mode (master, ad-hoc, etc). */
  int mode;

  /** Encrypted? */
  int encrypt;

  /** Frequency (MHz). */
  double freq;

  /** Link properties. */
  int qual, level, noise;
 
} playerc_wifi_link_t;


/** Wifi device proxy. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** @{ */
  /** A list containing info for each link. */
  playerc_wifi_link_t links[PLAYERC_WIFI_MAX_LINKS];
  int link_count;
  /** @} */
  
} playerc_wifi_t;


/** [Methods] */

/** Create a wifi proxy. */
playerc_wifi_t *playerc_wifi_create(playerc_client_t *client, int index);

/** Destroy a wifi proxy. */
void playerc_wifi_destroy(playerc_wifi_t *device);

/** Subscribe to the wifi device. */
int playerc_wifi_subscribe(playerc_wifi_t *device, int access);

/** Un-subscribe from the wifi device. */
int playerc_wifi_unsubscribe(playerc_wifi_t *device);

/** Get link state. */
playerc_wifi_link_t *playerc_wifi_get_link(playerc_wifi_t *device, int link);


/** @} */
/***************************************************************************/


/**************************************************************************/
/** @} (proxies) */
/**************************************************************************/

/** @} (addtogroup) */

#ifdef __cplusplus
}
#endif

#endif




