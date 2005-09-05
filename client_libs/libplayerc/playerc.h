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
#include <libplayercore/player.h>
#include <libplayercore/playercommon.h>
#include <libplayerxdr/playerxdr.h>

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
#define PLAYERC_IR_MAX_SAMPLES          PLAYER_IR_MAX_SAMPLES
#define PLAYERC_BLOBFINDER_MAX_BLOBS    PLAYER_BLOBFINDER_MAX_BLOBS
#define PLAYERC_WIFI_MAX_LINKS          PLAYER_WIFI_MAX_LINKS



/** @addtogroup player_clientlib_libplayerc libplayerc */
/** @{ */

/***************************************************************************/
/** @defgroup playerc_utility Utility and error-handling functions
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


// forward declaration to avoid including <sys/poll.h>, which may not be
// available when people are building clients against this lib
struct pollfd;


/***************************************************************************/
/** @defgroup multiclient Multi-Client object

@todo Document mutliclient

@{
*/

// Items in incoming data queue.
typedef struct
{
  player_msghdr_t header;
  void *data;
} playerc_client_item_t;


// Multi-client data
typedef struct
{
  // List of clients being managed
  int client_count;
  struct _playerc_client_t *client[128];

  // Poll info
  struct pollfd* pollfd;

  // Latest time received from any server
  double time;

} playerc_mclient_t;

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

/** @} */
/***************************************************************************/


/***************************************************************************/
/** @defgroup playerc_client Client API

The client object manages the connection with the Player server; it is
responsible for reading new data, setting data transmission modes and
so on.  The client object must be created and connected before device
proxies are initialized.

@{
*/

/** @brief Typedef for proxy callback function */
typedef void (*playerc_putmsg_fn_t) (void *device, char *header, char *data);

/** @brief Typedef for proxy callback function */
typedef void (*playerc_callback_fn_t) (void *data);


/** @brief Info about an available (but not necessarily subscribed)
    device.
 */
typedef struct
{
  /** Player id of the device. */
  player_devaddr_t addr;

  /** The driver name. */
  char drivername[PLAYER_MAX_DEVICE_STRING_LEN];

} playerc_device_info_t;


/** @brief Client object data. */
typedef struct _playerc_client_t
{
  /** A useful ID for identifying devices; mostly used by other
      language bindings. */
  void *id;

  /** Server address. */
  char *host;
  int port;

  /** @internal Socket descriptor */
  int sock;

  /** @internal Data delivery mode */
  int mode;

  /** List of available (but not necessarily subscribed) devices.
      This list is filled in by playerc_client_get_devlist(). */
  playerc_device_info_t devinfos[PLAYERC_MAX_DEVICES];
  int devinfo_count;

  /** List of subscribed devices */
  struct _playerc_device_t *device[32];
  int device_count;

  /** @internal A circular queue used to buffer incoming data packets. */
  playerc_client_item_t qitems[128];
  int qfirst, qlen, qsize;

  /** @internal Temp buffers for incoming / outgoing packets. */
  char *data;
  char *xdrdata;

  /** Server time stamp on the last packet. */
  double datatime;

} playerc_client_t;


/** @brief Create a client object.

@param mclient Multiclient object; set this NULL if this is a
stand-alone client.

@param host Player server host name (i.e., name of the machine
with the Player server).

@param port Player server port (typically 6665, but depends on the
server configuration).

@returns Returns a newly allocated pointer to the client object; use
playerc_client_destroy() to delete the object.

*/
playerc_client_t *playerc_client_create(playerc_mclient_t *mclient,
                                        const char *host, int port);

/** @brief Destroy a client object.

@param client Pointer to client object.

*/
void playerc_client_destroy(playerc_client_t *client);

/** @brief Connect to the server.

@param client Pointer to client object.

@returns Returns 0 on success, non-zero otherwise.  Use
playerc_error_str() to get a descriptive error message.

*/
int playerc_client_connect(playerc_client_t *client);

/** @brief Disconnect from the server.

@param client Pointer to client object.

@returns Returns 0 on success, non-zero otherwise.  Use
playerc_error_str() to get a descriptive error message.

*/
int playerc_client_disconnect(playerc_client_t *client);

/** @brief Change the server's data delivery mode.

@param client Pointer to client object.

@param mode Data delivery mode; must be one of
PLAYERC_DATAMODE_PUSH_ALL, PLAYERC_DATAMODE_PUSH_NEW,
PLAYERC_DATAMODE_PUSH_ASYNC; the defalt mode is
PLAYERC_DATAMODE_PUSH_ASYNC.

@returns Returns 0 on success, non-zero otherwise.  Use
playerc_error_str() to get a descriptive error message.

*/
int playerc_client_datamode(playerc_client_t *client, int mode);

/** @brief Request a round of data.

@param client Pointer to client object.

Request a round of data; only valid when in a request/reply (aka PULL)
data delivery mode.  But you don't need to call this function, because @ref
playerc_client_read will do it for you if the client is in a PULL mode.

Use @ref playerc_client_datamode to change modes.

*/
int playerc_client_requestdata(playerc_client_t* client);

/** @brief Change the server's data delivery frequency

@param client Pointer to client object.

@param freq Delivery frequency (in Hz).  Has no effect if the data
delivery mode is PLAYERC_DATAMODE_PUSH_ASYNC.

@returns Returns 0 on success, non-zero otherwise.  Use
playerc_error_str() to get a descriptive error message.

*/
int playerc_client_datafreq(playerc_client_t *client, int freq);

/** @brief Add a device proxy. @internal
 */
int playerc_client_adddevice(playerc_client_t *client, struct _playerc_device_t *device);


/** @brief Remove a device proxy. @internal
 */
int playerc_client_deldevice(playerc_client_t *client, struct _playerc_device_t *device);

/** @brief Add user callbacks (called when new data arrives). @internal
 */
int  playerc_client_addcallback(playerc_client_t *client, struct _playerc_device_t *device,
                                playerc_callback_fn_t callback, void *data);

/** @brief Remove user callbacks (called when new data arrives). @internal
 */
int  playerc_client_delcallback(playerc_client_t *client, struct _playerc_device_t *device,
                                playerc_callback_fn_t callback, void *data);

/** @brief Get the list of available device ids.

This function queries the server for the list of available devices,
and write result to the devinfos list in the client object.

@param client Pointer to client object.

@returns Returns 0 on success, non-zero otherwise.  Use
playerc_error_str() to get a descriptive error message.

*/
int playerc_client_get_devlist(playerc_client_t *client);

/** @brief Subscribe a device. @internal
 */
int playerc_client_subscribe(playerc_client_t *client, int code, int index,
                             int access, char *drivername, size_t len);

/** @brief Unsubscribe a device. @internal
 */
int playerc_client_unsubscribe(playerc_client_t *client, int code, int index);

/** @brief Issue a request to the server and await a reply (blocking). @internal

@returns Returns -1 on error and -2 on NACK.

*/
int playerc_client_request(playerc_client_t *client,
                           struct _playerc_device_t *device, uint8_t reqtype,
                           void *req_data, void *rep_data, int rep_len);

/** @brief Wait for response from server (blocking).

@param client Pointer to client object.
@param device
@param index
@param sequence

@returns Will return data size for ack, -1 for nack and -2 for failure

*/
/*int playerc_client_getresponse(playerc_client_t *client, uint16_t device,
    uint16_t index, uint16_t sequence, uint8_t * resptype, uint8_t * resp_data, int resp_len);
*/
/** @brief Test to see if there is pending data.

@param client Pointer to client object.

@param timeout Timeout value (ms).  Set timeout to 0 to check for
currently queued data.

@returns Returns -1 on error, 0 or 1 otherwise.

*/
int playerc_client_peek(playerc_client_t *client, int timeout);

/** @brief Read data from the server (blocking).

@param client Pointer to client object.

@returns For data packets, will return the ID of the proxy that got
the data; for synch packets, will return the ID of the client itself;
on error, will return NULL.

*/
void *playerc_client_read(playerc_client_t *client);



/** @brief Write data to the server.  @internal
*/
int playerc_client_write(playerc_client_t *client,
                         struct _playerc_device_t *device,
                         uint8_t subtype,
                         void *cmd, double* timestamp);


/** @} */
/**************************************************************************/


/***************************************************************************/
/** @defgroup playerc_device Device API

The device object provides a common interface to the functionality
that is shared by all device proxies (in OOP parlance, it is a base
class).  In general, this object should not be instantiated or
accessed directly: use the device proxies instead.

@{
*/

/** @brief Common device info. */
typedef struct _playerc_device_t
{
  /** A useful ID for identifying devices; mostly used by other
      language bindings.  For backwards-compatibility, this is passed
      as void pointer. */
  void *id;

  /** Pointer to the client proxy. */
  playerc_client_t *client;

  /** Device address */
  player_devaddr_t addr;

  /** The driver name. */
  char drivername[PLAYER_MAX_DEVICE_STRING_LEN];

  /** The subscribe flag is non-zero if the device has been
      successfully subscribed (read-only). */
  int subscribed;

  /** Data timestamp, i.e., the time at which the data was generated (s). */
  double datatime;

  /** Freshness flag.  Set to 1 whenever data is dispatched to this
      proxy.  Useful with the multi-client, but the user must manually
      set it to 0 after using the data. */
  int fresh;
  /** Freshness flag.  Set to 1 whenever data is dispatched to this
      proxy.  Useful with the multi-client, but the user must manually
      set it to 0 after using the data. */
  int freshgeom;
  /** Freshness flag.  Set to 1 whenever data is dispatched to this
      proxy.  Useful with the multi-client, but the user must manually
      set it to 0 after using the data. */
  int freshconfig;

  /**  Standard message callback for this device.  @internal */
  playerc_putmsg_fn_t putmsg;

  /** Extra user data for this device. @internal */
  void *user_data;

  /** Extra callbacks for this device. @internal */
  int callback_count;
  playerc_callback_fn_t callback[4];
  void *callback_data[4];

} playerc_device_t;


/** @brief Initialise the device. Additional callbacks for geom and config @internal */
void playerc_device_init(playerc_device_t *device, playerc_client_t *client,
                         int code, int index, playerc_putmsg_fn_t putmsg);

/** @brief Finalize the device. @internal */
void playerc_device_term(playerc_device_t *device);

/** @brief Subscribe the device. @internal */
int playerc_device_subscribe(playerc_device_t *device, int access);

/** @brief Unsubscribe the device. @internal */
int playerc_device_unsubscribe(playerc_device_t *device);

/** @} */
/**************************************************************************/


/***************************************************************************/
/** @defgroup playerc_proxies Proxies
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

/** @brief Description of a single blob. */
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


/** @brief Blobfinder device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Image dimensions (pixels). */
  int width, height;

  /** A list of detected blobs. */
  int blobs_count;
  playerc_blobfinder_blob_t blobs[PLAYERC_BLOBFINDER_MAX_BLOBS];

} playerc_blobfinder_t;


/** @brief Create a blobfinder proxy. */
playerc_blobfinder_t *playerc_blobfinder_create(playerc_client_t *client, int index);

/** @brief Destroy a blobfinder proxy. */
void playerc_blobfinder_destroy(playerc_blobfinder_t *device);

/** @brief Subscribe to the blobfinder device. */
int playerc_blobfinder_subscribe(playerc_blobfinder_t *device, int access);

/** @brief Un-subscribe from the blobfinder device. */
int playerc_blobfinder_unsubscribe(playerc_blobfinder_t *device);

/** @brief @internal Parse data from incoming packet */
void playerc_blobfinder_putmsg(playerc_blobfinder_t *device, player_msghdr_t *header,
                                player_blobfinder_data_t *data, size_t len);


/** @} */
/**************************************************************************/

#if 0

/**************************************************************************/
/** @defgroup playerc_proxy_bumper bumper

The bumper proxy provides an interface to the bumper sensors built
into robots such as the RWI B21R.

@{
*/

/** @brief Bumper proxy data. */
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


/** @brief Create a bumper proxy. */
playerc_bumper_t *playerc_bumper_create(playerc_client_t *client, int index);

/** @brief Destroy a bumper proxy. */
void playerc_bumper_destroy(playerc_bumper_t *device);

/** @brief Subscribe to the bumper device. */
int playerc_bumper_subscribe(playerc_bumper_t *device, int access);

/** @brief Un-subscribe from the bumper device. */
int playerc_bumper_unsubscribe(playerc_bumper_t *device);

/** @brief Get the bumper geometry.

The writes the result into the proxy rather than returning it to the
caller.

*/
int playerc_bumper_get_geom(playerc_bumper_t *device);


/** @} */
/***************************************************************************/

#endif
/***************************************************************************/
/** @defgroup playerc_proxy_camera camera

The camera proxy can be used to get images from a camera.

@{
*/

/** @brief Camera proxy data. */
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
  int image_count;

  /** Image data (byte aligned, row major order).  Multi-byte image
      formats (such as MONO16) are automatically converted to the
      correct host byte ordering.
  */
  uint8_t image[PLAYER_CAMERA_IMAGE_SIZE];

} playerc_camera_t;


/** @brief Create a camera proxy. */
playerc_camera_t *playerc_camera_create(playerc_client_t *client, int index);

/** @brief Destroy a camera proxy. */
void playerc_camera_destroy(playerc_camera_t *device);

/** @brief Subscribe to the camera device. */
int playerc_camera_subscribe(playerc_camera_t *device, int access);

/** @brief Un-subscribe from the camera device. */
int playerc_camera_unsubscribe(playerc_camera_t *device);

/** @brief Decompress the image (modifies the current proxy data). */
void playerc_camera_decompress(playerc_camera_t *device);

/** @brief Saves the image to disk as a .ppm */
void playerc_camera_save(playerc_camera_t *device, const char *filename);


/** @} */
/**************************************************************************/
#if 0
/***************************************************************************/
/** @defgroup playerc_proxy_fiducial fiducial

The fiducial proxy provides an interface to a fiducial detector.  This
device looks for fiducials (markers or beacons) in the laser scan, and
determines their identity, range, bearing and orientation.  See the
Player User Manual for a complete description of the various drivers
that support the fiducial interface.

@{
*/


/** @brief Description for a single fiducial. */
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


/** @brief Fiducial finder data. */
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


/** @brief Create a fiducial proxy. */
playerc_fiducial_t *playerc_fiducial_create(playerc_client_t *client, int index);

/** @brief Destroy a fiducial proxy. */
void playerc_fiducial_destroy(playerc_fiducial_t *device);

/** @brief Subscribe to the fiducial device. */
int playerc_fiducial_subscribe(playerc_fiducial_t *device, int access);

/** @brief Un-subscribe from the fiducial device. */
int playerc_fiducial_unsubscribe(playerc_fiducial_t *device);

/** @brief Get the fiducial geometry.

Ths writes the result into the proxy rather than returning it to the
caller.

*/
int playerc_fiducial_get_geom(playerc_fiducial_t *device);


/** @} */
/**************************************************************************/


/***************************************************************************/
/** @defgroup playerc_proxy_gps gps

The gps proxy provides an interface to a GPS-receiver.

@{
*/

/** @brief GPS proxy data. */
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

  /** Vertical dilution of precision. */
  double vdop;

  /** Horizontal and vertical error (meters). */
  double err_horz, err_vert;

  /** Quality of fix 0 = invalid, 1 = GPS fix, 2 = DGPS fix */
  int quality;

  /** Number of satellites in view. */
  int sat_count;

} playerc_gps_t;


/** @brief Create a gps proxy. */
playerc_gps_t *playerc_gps_create(playerc_client_t *client, int index);

/** @brief Destroy a gps proxy. */
void playerc_gps_destroy(playerc_gps_t *device);

/** @brief Subscribe to the gps device. */
int playerc_gps_subscribe(playerc_gps_t *device, int access);

/** @brief Un-subscribe from the gps device. */
int playerc_gps_unsubscribe(playerc_gps_t *device);


/** @} */
/**************************************************************************/

/***************************************************************************/
/** @defgroup playerc_proxy_ir ir

The ir proxy provides an interface to the ir sensors built into robots
such as the RWI B21R.

@{
*/

/** @brief Ir proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  // data
  player_ir_data_t ranges;

  // config
  player_ir_pose_t poses;

} playerc_ir_t;


/** @brief Create a ir proxy. */
playerc_ir_t *playerc_ir_create(playerc_client_t *client, int index);

/** @brief Destroy a ir proxy. */
void playerc_ir_destroy(playerc_ir_t *device);

/** @brief Subscribe to the ir device. */
int playerc_ir_subscribe(playerc_ir_t *device, int access);

/** @brief Un-subscribe from the ir device. */
int playerc_ir_unsubscribe(playerc_ir_t *device);

/** @brief Get the ir geometry.

This writes the result into the proxy rather than returning it to the
caller.

*/
int playerc_ir_get_geom(playerc_ir_t *device);


/** @} */
/***************************************************************************/


/***************************************************************************/
/** @defgroup playerc_proxy_joystick joystick

The joystick proxy provides an interface to joysticks.

@{
*/

/** @brief Joystick device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Scaled joystick position (0 = centered, 1 = hard over). */
  double px, py;

  /** Button states (bitmask) */
  uint16_t buttons;

} playerc_joystick_t;


/** @brief Create a joystick device proxy. */
playerc_joystick_t *playerc_joystick_create(playerc_client_t *client, int index);

/** @brief Destroy a joystick device proxy. */
void playerc_joystick_destroy(playerc_joystick_t *device);

/** @brief Subscribe to the joystick device */
int playerc_joystick_subscribe(playerc_joystick_t *device, int access);

/** @brief Un-subscribe from the joystick device */
int playerc_joystick_unsubscribe(playerc_joystick_t *device);

/** @brief @internal Parse data from incoming packet */
void playerc_joystick_putmsg(playerc_joystick_t *device, player_msghdr_t *header,
                              player_joystick_data_t *data, size_t len);


/** @} */
/**************************************************************************/

#endif

/***************************************************************************/
/** @defgroup playerc_proxy_laser laser

The laser proxy provides an interface to scanning laser range finders
such as the @ref player_driver_sicklms200.  Data is returned in the
playerc_laser_t structure.

This proxy wraps the low-level @ref player_interface_laser interface.

@{
*/

/** @brief Laser proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Laser geometry in the robot cs: pose gives the position and
      orientation, size gives the extent.  These values are filled in by
      playerc_laser_get_geom(). */
  double pose[3];
  double size[2];

  /** Robot pose (m,m,rad), filled in if the scan came with a pose attached */
  double robot_pose[3];

  /** Is intesity data returned. */
  int intensity_on;

  /** Number of points in the scan. */
  int scan_count;

  /** Start bearing of the scan (radians). */
  double scan_start;

  /** Angular resolution in radians. */
  unsigned char scan_res;

  /** Range resolution, in m. */
  unsigned char range_res;

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

  /** ID for this scan */
  int scan_id;

} playerc_laser_t;


/** @brief Create a laser proxy. */
playerc_laser_t *playerc_laser_create(playerc_client_t *client, int index);

/** @brief Destroy a laser proxy. */
void playerc_laser_destroy(playerc_laser_t *device);

/** @brief Subscribe to the laser device. */
int playerc_laser_subscribe(playerc_laser_t *device, int access);

/** @brief Un-subscribe from the laser device. */
int playerc_laser_unsubscribe(playerc_laser_t *device);

/** @brief Configure the laser.

@param device Pointer to proxy object.

@param min_angle, max_angle Start and end angles for the scan
(radians).

@param resolution Angular resolution in 0.01 degree
increments. Valid values are 25, 50, 100.

@param range_res Range resolution in mm.  Valid values are: 1, 10, 100.

@param intensity Intensity flag; set to 1 to enable reflection intensity data.

@returns Returns 0 on success, non-zero otherwise.  Use
playerc_error_str() to get a descriptive error message.

*/
int playerc_laser_set_config(playerc_laser_t *device,
                             double min_angle, double max_angle,
                             unsigned char resolution,
                             unsigned char range_res,
                             unsigned char intensity);

/** @brief Get the laser configuration.

@param device Pointer to proxy object.

@param min_angle, max_angle Start and end angles for the scan
(radians).

@param resolution Angular resolution in 0.01 degree
increments. Valid values are 25, 50, 100.

@param range_res Range resolution in mm.  Valid values are: 1, 10, 100.

@param intensity Intensity flag; set to 1 to enable reflection intensity data.

@returns Returns 0 on success, non-zero otherwise.  Use
playerc_error_str() to get a descriptive error message.

*/
int playerc_laser_get_config(playerc_laser_t *device,
                             double *min_angle,
                             double *max_angle,
                             unsigned char *resolution,
                             unsigned char *range_res,
                             unsigned char *intensity);

/** @brief Get the laser geometry.

This writes the result into the proxy rather than returning it to the
caller.

*/
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

/** @brief Hypothesis data. */
typedef struct
{
  /** Pose estimate (x, y, theta) in (m, m, radians). */
  double mean[3];

  /** Covariance. */
  double cov[3][3];

  /** Weight associated with this hypothesis. */
  double weight;

} playerc_localize_hypoth_t;

typedef struct playerc_localize_particle
{
  double pose[3];
  double weight;
} playerc_localize_particle_t;


/** @brief Localization device data. */
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

  double mean[3];
  double variance;
  int num_particles;
  playerc_localize_particle_t particles[PLAYER_LOCALIZE_PARTICLES_MAX];

} playerc_localize_t;


/** @brief Create a localize proxy. */
playerc_localize_t *playerc_localize_create(playerc_client_t *client, int index);

/** @brief Destroy a localize proxy. */
void playerc_localize_destroy(playerc_localize_t *device);

/** @brief Subscribe to the localize device. */
int playerc_localize_subscribe(playerc_localize_t *device, int access);

/** @brief Un-subscribe from the localize device. */
int playerc_localize_unsubscribe(playerc_localize_t *device);

/** @brief Set the the robot pose (mean and covariance). */
int playerc_localize_set_pose(playerc_localize_t *device, double pose[3], double cov[3]);

/* @brief Get the particle set.  Caller must supply sufficient storage for
   the result. */
int playerc_localize_get_particles(playerc_localize_t *device);

/** @} */
/**************************************************************************/


/***************************************************************************/
/** @defgroup playerc_proxy_log log

The log proxy provides start/stop control of data logging

@{
*/

/** @brief Log proxy data. */
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


/** @brief Create a log proxy. */
playerc_log_t *playerc_log_create(playerc_client_t *client, int index);

/** @brief Destroy a log proxy. */
void playerc_log_destroy(playerc_log_t *device);

/** @brief Subscribe to the log device. */
int playerc_log_subscribe(playerc_log_t *device, int access);

/** @brief Un-subscribe from the log device. */
int playerc_log_unsubscribe(playerc_log_t *device);

/** @brief Start/stop logging */
int playerc_log_set_write_state(playerc_log_t* device, int state);

/** @brief Start/stop playback */
int playerc_log_set_read_state(playerc_log_t* device, int state);

/** @brief Rewind playback */
int playerc_log_set_read_rewind(playerc_log_t* device);

/** @brief Get logging/playback state.

The result is written into the proxy.

*/
int playerc_log_get_state(playerc_log_t* device);

/** @brief Change name of log file to write to. */
int playerc_log_set_filename(playerc_log_t* device, const char* fname);


/** @} */


/***************************************************************************/
/** @defgroup playerc_proxy_map map

The map proxy provides an interface to a map.

@{
*/

/** @brief Map proxy data. */
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


/** @brief Convert from a cell position to a map index.
 */
#define PLAYERC_MAP_INDEX(dev, i, j) ((dev->width) * (j) + (i))

/** @brief Create a map proxy. */
playerc_map_t *playerc_map_create(playerc_client_t *client, int index);

/** @brief Destroy a map proxy. */
void playerc_map_destroy(playerc_map_t *device);

/** @brief Subscribe to the map device. */
int playerc_map_subscribe(playerc_map_t *device, int access);

/** @brief Un-subscribe from the map device. */
int playerc_map_unsubscribe(playerc_map_t *device);

/** @brief Get the map, which is stored in the proxy. */
int playerc_map_get_map(playerc_map_t* device);


/** @} */
/**************************************************************************/

#if 0

/***************************************************************************/
/** @defgroup playerc_proxy_motor motor

The motor proxy provides an interface a simple, single motor.

@{
*/

/** @brief Motor device data. */
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


/** @brief Create a motor device proxy. */
playerc_motor_t *playerc_motor_create(playerc_client_t *client, int index);

/** @brief Destroy a motor device proxy. */
void playerc_motor_destroy(playerc_motor_t *device);

/** @brief Subscribe to the motor device */
int playerc_motor_subscribe(playerc_motor_t *device, int access);

/** @brief Un-subscribe from the motor device */
int playerc_motor_unsubscribe(playerc_motor_t *device);

/** @brief Enable/disable the motors */
int playerc_motor_enable(playerc_motor_t *device, int enable);

/** @brief Change position control

@param device Pointer to proxy object.
@param type Control type: 0 = velocity, 1 = position.

*/
int playerc_motor_position_control(playerc_motor_t *device, int type);

/** @brief Set the target rotational velocity.

@param device Pointer to proxy object.
@param vt Velocity in in rad/s.
@param state @todo Document state

*/
int playerc_motor_set_cmd_vel(playerc_motor_t *device,
                              double vt, int state);

/** @brief Set the target pose.

@param device Pointer to proxy object.
@param gt Target pose in rad.
@param state @todo Document state

*/
int playerc_motor_set_cmd_pose(playerc_motor_t *device,
                               double gt, int state);

/** @} */
/**************************************************************************/

#endif

/***************************************************************************/
/** @defgroup playerc_proxy_planner planner

The planner proxy provides an interface to a 2D motion planner.

@{
*/

/** @brief Planner device data. */
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

/** @brief Create a planner device proxy. */
playerc_planner_t *playerc_planner_create(playerc_client_t *client, int index);

/** @brief Destroy a planner device proxy. */
void playerc_planner_destroy(playerc_planner_t *device);

/** @brief Subscribe to the planner device */
int playerc_planner_subscribe(playerc_planner_t *device, int access);

/** @brief Un-subscribe from the planner device */
int playerc_planner_unsubscribe(playerc_planner_t *device);

/** @brief Set the goal pose (gx, gy, ga) */
int playerc_planner_set_cmd_pose(playerc_planner_t *device,
                                  double gx, double gy, double ga);

/** @brief Get the list of waypoints.

Writes the result into the proxy rather than returning it to the
caller.

*/
int playerc_planner_get_waypoints(playerc_planner_t *device);

/** @brief Enable / disable the robot's motion

Set state to 1 to enable, 0 to disable.

*/
int playerc_planner_enable(playerc_planner_t *device, int state);

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
playerc_position2d_t *playerc_position2d_create(playerc_client_t *client,
                                                int index);

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

/** Set the odometry offset */
int playerc_position2d_set_odom(playerc_position2d_t *device,
                                double ox, double oy, double oa);

/** @} */
/**************************************************************************/

/***************************************************************************/
/** @defgroup playerc_proxy_position position

The position proxy provides backward compatibility for pre-Player 2.0 client
code.  New code should use the @ref playerc_proxy_position2d proxy instead.

@{
*/

/** Position device data. */
typedef playerc_position2d_t playerc_position_t;

/** Create a position device proxy. */
playerc_position_t *playerc_position_create(playerc_client_t *client,
                                            int index);
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
    rotational speed (rad/s).  All speeds are defined in the robot
    coordinate system. */
int playerc_position_set_cmd_vel(playerc_position_t *device,
                                   double vx, double vy, double va, int state);
/** Set the target pose (gx, gy, ga) is the target pose in the
    odometric coordinate system. */
int playerc_position_set_cmd_pose(playerc_position_t *device,
                                  double gx, double gy, double ga, int state);
/** Set the odometric offset */
int playerc_position_set_odom(playerc_position_t *device,
                              double ox, double oy, double oa);
/** @} */


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

/** @brief Power device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Battery charge (volts). */
  double charge;

  /** Battery charge (percent full). */
  double percent;

} playerc_power_t;


/** @brief Create a power device proxy. */
playerc_power_t *playerc_power_create(playerc_client_t *client, int index);

/** @brief Destroy a power device proxy. */
void playerc_power_destroy(playerc_power_t *device);

/** @brief Subscribe to the power device. */
int playerc_power_subscribe(playerc_power_t *device, int access);

/** @brief Un-subscribe from the power device. */
int playerc_power_unsubscribe(playerc_power_t *device);


/** @} */
/**************************************************************************/



/***************************************************************************/
/** @defgroup playerc_proxy_ptz ptz

The ptz proxy provides an interface to pan-tilt units such as the Sony
PTZ camera.

@{
*/

/** @brief PTZ device data. */
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


/** @brief Create a ptz proxy. */
playerc_ptz_t *playerc_ptz_create(playerc_client_t *client, int index);

/** @brief Destroy a ptz proxy. */
void playerc_ptz_destroy(playerc_ptz_t *device);

/** @brief Subscribe to the ptz device. */
int playerc_ptz_subscribe(playerc_ptz_t *device, int access);

/** @brief Un-subscribe from the ptz device. */
int playerc_ptz_unsubscribe(playerc_ptz_t *device);

/** @brief Set the pan, tilt and zoom values.

@param device Pointer to proxy object.
@param pan Pan value, in radians; 0 = centered.
@param tilt Tilt value, in radians; 0 = level.
@param zoom Zoom value, in radians (corresponds to camera field of view).

*/
int playerc_ptz_set(playerc_ptz_t *device, double pan, double tilt, double zoom);

/** @brief Set the pan, tilt and zoom values (and speed)

@param device Pointer to proxy object.
@param pan Pan value, in radians; 0 = centered.
@param tilt Tilt value, in radians; 0 = level.
@param zoom Zoom value, in radians (corresponds to camera field of view).
@param panspeed Pan speed, in radians/sec.
@param tiltspeed Tilt speed, in radians/sec.

*/
int playerc_ptz_set_ws(playerc_ptz_t *device, double pan, double tilt, double zoom,
                       double panspeed, double tiltspeed);


/** @} */
/**************************************************************************/

/***************************************************************************/
/** @defgroup playerc_proxy_sonar sonar

The sonar proxy provides an interface to the sonar range sensors built
into robots such as the ActiveMedia Pioneer series.

@{
*/

/** @brief Sonar proxy data. */
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


/** @brief Create a sonar proxy. */
playerc_sonar_t *playerc_sonar_create(playerc_client_t *client, int index);

/** @brief Destroy a sonar proxy. */
void playerc_sonar_destroy(playerc_sonar_t *device);

/** @brief Subscribe to the sonar device. */
int playerc_sonar_subscribe(playerc_sonar_t *device, int access);

/** @brief Un-subscribe from the sonar device. */
int playerc_sonar_unsubscribe(playerc_sonar_t *device);

/** @brief Get the sonar geometry.

This writes the result into the proxy
rather than returning it to the caller.
*/
int playerc_sonar_get_geom(playerc_sonar_t *device);

/** @} */
/**************************************************************************/

#if 0

/***************************************************************************/
/** @defgroup playerc_proxy_truth truth

The truth proxy can be used to get and set the pose of objects in a  simulator.

@{
*/


/** @brief Truth proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** The object position in the world cs (x, y, z). */
  double pos[3];

  /** The object orientation in the world cs (roll, pitch, yaw). */
  double rot[3];

} playerc_truth_t;


/** @brief Create a truth proxy. */
playerc_truth_t *playerc_truth_create(playerc_client_t *client, int index);

/** @brief Destroy a truth proxy. */
void playerc_truth_destroy(playerc_truth_t *device);

/** @brief Subscribe to the truth device. */
int playerc_truth_subscribe(playerc_truth_t *device, int access);

/** @brief Un-subscribe from the truth device. */
int playerc_truth_unsubscribe(playerc_truth_t *device);

/** @brief Get the object pose.
@param device Pointer to proxy object.
@param px, py, pz Object position in world cs.
@param rx, ry, rz Object orientation in world cs (roll, pitch, yaw).
*/
int playerc_truth_get_pose(playerc_truth_t *device,
                           double *px, double *py, double *pz,
                           double *rx, double *ry, double *rz);

/** @brief Set the object pose.
@param device Pointer to proxy object.
@param px, py, pz Object position in world cs.
@param rx, ry, rz Object orientation in world cs (roll, pitch, yaw).
*/
int playerc_truth_set_pose(playerc_truth_t *device,
                           double px, double py, double pz,
                           double rx, double ry, double rz);


/** @} */
/***************************************************************************/


/***************************************************************************/
/** @defgroup playerc_proxy_wifi wifi

The wifi proxy is used to query the state of a wireless network.  It
returns information such as the link quality and signal strength of
access points or of other wireless NIC's on an ad-hoc network.

@{
*/

/** @brief Individual link info. */
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


/** @brief Wifi device proxy. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** A list containing info for each link. */
  playerc_wifi_link_t links[PLAYERC_WIFI_MAX_LINKS];
  int link_count;

} playerc_wifi_t;


/** @brief Create a wifi proxy. */
playerc_wifi_t *playerc_wifi_create(playerc_client_t *client, int index);

/** @brief Destroy a wifi proxy. */
void playerc_wifi_destroy(playerc_wifi_t *device);

/** @brief Subscribe to the wifi device. */
int playerc_wifi_subscribe(playerc_wifi_t *device, int access);

/** @brief Un-subscribe from the wifi device. */
int playerc_wifi_unsubscribe(playerc_wifi_t *device);

/** @brief Get link state. */
playerc_wifi_link_t *playerc_wifi_get_link(playerc_wifi_t *device, int link);

#endif

/** @brief Simulation device proxy. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

} playerc_simulation_t;


// Create a new simulation proxy
playerc_simulation_t *playerc_simulation_create(playerc_client_t *client, int index);

// Destroy a simulation proxy
void playerc_simulation_destroy(playerc_simulation_t *device);

// Subscribe to the simulation device
int playerc_simulation_subscribe(playerc_simulation_t *device, int access);

// Un-subscribe from the simulation device
int playerc_simulation_unsubscribe(playerc_simulation_t *device);

int playerc_simulation_set_pose2d(playerc_simulation_t *device, char* name,
                                  double gx, double gy, double ga);

int playerc_simulation_get_pose2d(playerc_simulation_t *device, char* identifier,
          double *x, double *y, double *a);

/** @} */
/***************************************************************************/

#if 0

/***************************************************************************/
/** @defgroup playerc_proxy_gripper gripper

The gripper proxy provides an interface to the gripper

@{
*/

/** @brief Gripper device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  unsigned char state;
  unsigned char beams;
  int outer_break_beam;
  int inner_break_beam;
  int paddles_open;
  int paddles_closed;
  int paddles_moving;
  int gripper_error;
  int lift_up;
  int lift_down;
  int lift_moving;
  int lift_error;

} playerc_gripper_t;


/** @brief Create a gripper device proxy. */
playerc_gripper_t *playerc_gripper_create(playerc_client_t *client, int index);

/** @brief Destroy a gripper device proxy. */
void playerc_gripper_destroy(playerc_gripper_t *device);

/** @brief Subscribe to the gripper device */
int playerc_gripper_subscribe(playerc_gripper_t *device, int access);

/** @brief Un-subscribe from the gripper device */
int playerc_gripper_unsubscribe(playerc_gripper_t *device);

/** @brief Send the gripper a command */
int playerc_gripper_set_cmd(playerc_gripper_t *device, uint8_t cmd, uint8_t arg);

/**************************************************************************/
/** @defgroup playerc_proxy_dio dio

The dio proxy provides an interface to the digital input/output sensors.

@{
*/

/** Dio proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

    /// The number of valid digital inputs.
    uint8_t count;

    /// A bitfield of the current digital inputs.
    uint32_t digin;

} playerc_dio_t;


/** Create a dio proxy. */
playerc_dio_t *playerc_dio_create(playerc_client_t *client, int index);

/** Destroy a dio proxy. */
void playerc_dio_destroy(playerc_dio_t *device);

/** Subscribe to the dio device. */
int playerc_dio_subscribe(playerc_dio_t *device, int access);

/** Un-subscribe from the dio device. */
int playerc_dio_unsubscribe(playerc_dio_t *device);

/** Set the output for the dio device. */
int playerc_dio_set_output(playerc_dio_t *device, uint8_t output_count, uint32_t digout);


/** @} */
/***************************************************************************/


/**************************************************************************/
/** @defgroup playerc_proxy_speech speech

The speech proxy provides an interface to a speech synthesis system.

@{
*/

/** Speech proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;
} playerc_speech_t;


/** Create a speech proxy. */
playerc_speech_t *playerc_speech_create(playerc_client_t *client, int index);

/** Destroy a speech proxy. */
void playerc_speech_destroy(playerc_speech_t *device);

/** Subscribe to the speech device. */
int playerc_speech_subscribe(playerc_speech_t *device, int access);

/** Un-subscribe from the speech device. */
int playerc_speech_unsubscribe(playerc_speech_t *device);

/** Set the output for the speech device. */
int playerc_speech_say (playerc_speech_t *device, char *);


/** @} */
/***************************************************************************/

#endif

/**************************************************************************/
/** @} (proxies) */
/**************************************************************************/

/** @} (addtogroup) */

#ifdef __cplusplus
}
#endif

#endif
