/*
 *  libplayerc : a Player client library
 *  Copyright (C) Andrew Howard and contributors 2002-2007
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


/** @ingroup clientlibs
@defgroup player_clientlib_libplayerc libplayerc
@brief A C client library for the @ref util_player

libplayerc is a client library for the @ref util_player.  It is
written in C to maximize portability, and in the expectation that
users will write bindings for other languages (such as Python and
Java) against this library; @ref player_clientlib_libplayerc_py
"Python bindings" are already available.

Be sure to check out the @ref libplayerc_example "example".

The @ref libplayerc_datamodes "data modes" section is important reading for all
client writers.

libplayerc was originally written by Andrew Howard and is maintained
by the Player Project. Subsequent contributors include Brian Gerkey,
Geoffrey Biggs, Richard Vaughan.
*/
/** @{ */


#ifndef PLAYERC_H
#define PLAYERC_H

#if !defined (WIN32)
  #include <netinet/in.h> /* need this for struct sockaddr_in */
#endif
#include <stddef.h> /* size_t */
#include <stdio.h>

#include <playerconfig.h>

/* Get the message structures from Player*/
#include <libplayercommon/playercommon.h>
#include <libplayerinterface/player.h>
#include <libplayercommon/playercommon.h>
#include <libplayerinterface/interface_util.h>
#include <libplayerinterface/playerxdr.h>
#include <libplayerinterface/functiontable.h>
#include <libplayerwkb/playerwkb.h>
#if defined (WIN32)
  #include <winsock2.h>
#endif

#ifndef MIN
  #define MIN(a,b) ((a < b) ? a : b)
#endif
#ifndef MAX
  #define MAX(a,b) ((a > b) ? a : b)
#endif

#if defined (WIN32)
  #if defined (PLAYER_STATIC)
    #define PLAYERC_EXPORT
  #elif defined (playerc_EXPORTS)
    #define PLAYERC_EXPORT    __declspec (dllexport)
  #else
    #define PLAYERC_EXPORT    __declspec (dllimport)
  #endif
#else
  #define PLAYERC_EXPORT
#endif


#ifdef __cplusplus
extern "C" {
#endif


/***************************************************************************
 * Useful constants (re-defined here so SWIG can pick them up easily)
 **************************************************************************/

/** The device access modes */
#define PLAYERC_OPEN_MODE     PLAYER_OPEN_MODE
#define PLAYERC_CLOSE_MODE    PLAYER_CLOSE_MODE
#define PLAYERC_ERROR_MODE    PLAYER_ERROR_MODE


/** The valid data delivery modes */
#define PLAYERC_DATAMODE_PUSH PLAYER_DATAMODE_PUSH
#define PLAYERC_DATAMODE_PULL PLAYER_DATAMODE_PULL

/** The valid transports */
#define PLAYERC_TRANSPORT_TCP 1
#define PLAYERC_TRANSPORT_UDP 2

#define PLAYERC_QUEUE_RING_SIZE 512

/** @} */

/**
@ingroup player_clientlib_libplayerc
@defgroup libplayerc_example libplayerc example
@brief An example

libplayerc is based on a device <i>proxy</i> model, in which the client
maintains a local proxy for each of the devices on the remote server.
Thus, for example, one can create local proxies for the
@ref interface_position2d
and @ref interface_laser devices.  There is also a special <tt>client</tt> proxy,
used to control the Player server itself.

Programs using libplayerc will generally have the following structure:

<PRE>
\#include <stdio.h>
\#include <libplayerc/playerc.h>

int
main(int argc, const char **argv)
{
  int i;
  playerc_client_t *client;
  playerc_position2d_t *position2d;

  // Create a client and connect it to the server.
  client = playerc_client_create(NULL, "localhost", 6665);
  if (0 != playerc_client_connect(client))
    return -1;

  // Create and subscribe to a position2d device.
  position2d = playerc_position2d_create(client, 0);
  if (playerc_position2d_subscribe(position2d, PLAYER_OPEN_MODE))
    return -1;

  // Make the robot spin
  playerc_position2d_enable(position2d, 1);
  playerc_position2d_set_speed(position2d, 0, 0, 0.1);

  for (i = 0; i < 200; i++)
  {
    // Wait for new data from server
    playerc_client_read(client);

    // Print current robot pose
    printf("position2d : %f %f %f\n",
           position2d->px, position2d->py, position2d->pa);
  }

  // Shutdown
  playerc_position2d_unsubscribe(position2d);
  playerc_position2d_destroy(position2d);
  playerc_client_disconnect(client);
  playerc_client_destroy(client);

  return 0;
}
</PRE>

This example can be built using the command:
<PRE>
$ gcc -o simpleclient `pkg-config --cflags playerc` simpleclient.c `pkg-config --libs playerc`
</PRE>

Make sure that <tt>libplayerc</tt> is installed somewhere that pkg-config
can find it.

<P>
The above program can be broken into six steps, as follows.

<ul>

<li> Create and connect a client proxy.

<P>
<PRE>
client = playerc_client_create(NULL, "localhost", 6665);
playerc_client_connect(client);
</PRE>
The <TT>create</TT> function creates a new client proxy and returns a
pointer to be used in future function calls (<TT>localhost</TT> should be
replaced with the network host name of the robot).  The <TT>connect</TT>
function notifies the Player server that a new client wishes to
recieve data.


<li> Create and subscribe a device proxy.

<P>
<PRE>
position2d = playerc_position2d_create(client, 0);
playerc_position2d_subscribe(position2d, PLAYERC_OPEN_MODE);
</PRE>
The <TT>create</TT> function creates a new position2d device proxy and
returns a pointer to be used in future function calls.  The <TT>subscribe</TT> function notifies the Player server that the client is
using the position2d device, and that the client expects to both send
commands and recieve data (<TT>PLAYERC_OPEN_MODE</TT>).

<li> Configure the device, send commands.

<P>
<PRE>
playerc_position2d_enable(position2d, 1);
playerc_position2d_set_speed(position2d, 0, 0, 0.1);
</PRE>
The <TT>enable</TT> function sends a configuration request to the server,
changing the robot's motor state from <TT>off</TT> to <TT>on</TT>, thereby
allowing the robot to move.  The <TT>setspeed</TT> function sends a new
motor speed, in this case commanding the robot to turn on the spot.


<P>
Note that most Player devices will accept both asynchronous
<EM>command</EM> and synchronous <EM>configuration</EM> requests.
Sending commands is analogous using the standard Unix <TT>write</TT>
device interface, while sending configuration requests is analogous to
using the <TT>ioctl</TT> interface.  For the most part,
<TT>libplayerc</TT> hides the distinction between these two
interfaces.  Users should be aware, however, that while commands are
always handled promptly by the server, configuration requests may take
significant time to complete.  If possible, configuration requests
should therefore be restricted to the initialization phase of the
program.


<li> Read data from the device.

<P>
<PRE>
playerc_client_read(client);
printf("position : %f %f %f\n", position2d-&gt;px, ... );
</PRE>
The <TT>read</TT> function blocks until new data arrives from the Player
server.  This data may be from one of the subscribed devices, or it
may be from the server itself (which sends regular synchronization
messages to all of its clients).  The <TT>read</TT> function inspects the
incoming data and automatically updates the elements in the
appropriate device proxy.  This function also returns a pointer to the
proxy that was updated, so that user programs may, if desired, trigger
appropriate events on the arrival of different kinds of data.

In pull mode read will process a full set of messages from the server,
in push mode read will process a single message.

<li> Unsubscribe and destroy the device proxy.

<P>
<PRE>
playerc_position2d_unsubscribe(position2d);
playerc_position2d_destroy(position2d);
</PRE>
The <TT>unsubscribe</TT> function tells the Player server that the client
is no longer using this device.  The <TT>destroy</TT> function then frees
the memory associated with the device proxy; the <TT>device</TT> pointer
is now invalid and should be not be re-used.


<li> Disconnect and destroy the client proxy.

<P>
<PRE>
playerc_client_disconnect(client);
playerc_client_destroy(client);
</PRE>
The <TT>disconnect</TT> function tells the server that the client is
shutting down.  The <TT>destroy</TT> function then frees the memory
associated with the client proxy; the <TT>client</TT> pointer is now
invalid and should be not be re-used.

</ul>
*/

/**
@ingroup player_clientlib_libplayerc
@defgroup libplayerc_datamodes Client Data Modes
@brief Describes the data modes used for delivering data from the server to the client.

The server may send data to the client in one of two modes: PUSH and PULL. The
current data mode is changed using the @ref playerc_client_datamode function.

The default mode for clients is PULL.

@section pushmode PUSH Mode

PUSH mode is more reliable for ensuring that all data generated by the server
will reach the client, although there is still a risk of message queues
overflowing in this mode.

When in PUSH mode the server will send all messages directed at the client as
soon as they become available. For example, if a message carrying range
readings from a laser scanner is generated by a laser data, it will be added
to the client's queue and sent out as soon as possible by the server. When
a client read is performed by calling @ref playerc_client_read the next packet
in the client's queue is read.

When using PUSH mode be aware that the client must keep up with the speed of
generation of DATA messages by the server. If the client cannot read messages
as fast as the server is producing them, the data received by the client will
become increasingly out of date. This may be avoidable by setting appropriate
add-replace rules, but read the next paragraph first.

If you are using the TCP protocol for the connection between the client and the
server, and are in PUSH mode, then add-replace rules for the client's queue
may not function correctly. This is because messages will leave the client's
queue on the server as soon as there is space for them in the operating
system's TCP buffer for the connection. As a result, messages will not be in
the actual queue long enough for add-replace rules to function correctly until
this buffer becomes full (it is typically many megabytes large). To avoid this
problem, use PULL mode.

@section pullmode PULL Mode

PULL mode is the preferred mode for most clients. When used together with
suitable add-replace rules, it guarentees that data received by the client is
the most up-to-date data available from the server at the beginning of the read
function.

When in PULL mode the server will send all messages directed at the client, but
only when requested by the client. This request is performed at the beginning
of every call to @ref playerc_client_read, so client programs do not need to
perform it themselves. The client will then read all data that is waiting on
the server in one go. In this mode, client programs do not need to continually
call read until there are no messages waiting.

When using PULL mode, beware of the client's queue on the server overflowing,
particularly if using TCP. While PUSH mode can take advantage of the OS's TCP
buffer, in PULL mode this buffer is not used until the client requests data.
The standard length of the client's message queue on the server is 32 messages.
To avoid the queue overflowing, use add-replace rules to automatically replace
older messages with more up-to-date messages for the same message signature.

@section datamodeexample Data Mode Example

The following code will change a client's data mode to PULL mode and set an
add-replace rule to replace all DATA messages generated by the server,
preventing the message queue from overflowing when they are produced faster
than the client reads and ensuring that the client receives the most up-to-date
data.

<pre>
if (playerc_client_datamode (client, PLAYERC_DATAMODE_PULL) != 0)
{
  fprintf(stderr, "error: %s\n", playerc_error_str());
  return -1;
}
if (playerc_client_set_replace_rule (client, -1, -1, PLAYER_MSGTYPE_DATA, -1, 1) != 0)
{
  fprintf(stderr, "error: %s\n", playerc_error_str());
  return -1;
}
</pre>

The add-replace rule specifies that any message of type PLAYER_MSGTYPE_DATA
will be replaced by newer versions. So, if a driver produces a data packet and
there is already a data packet from that driver with the same signature on the
queue, it will be removed from the queue (and not sent to the client), with
the new packet pushed onto the end of the queue.
*/

/***************************************************************************/
/** @ingroup player_clientlib_libplayerc
 * @defgroup playerc_utility Utility and error-handling
 * @brief Some helper functions
@{
*/
/***************************************************************************/

/** Retrieve the last error (as a descriptive string).  Most functions
    in will return 0 on success and non-zero value on error; a
    descriptive error message can be obtained by calling this
    function. */
PLAYERC_EXPORT const char *playerc_error_str(void);

/** Get the name for a given interface code. */
/*const char *playerc_lookup_name(int code);*/

/** Get the interface code for a given name. */
/*int playerc_lookup_code(const char *name);*/

/** Add new entries to the XDR function table. */
PLAYERC_EXPORT int playerc_add_xdr_ftable(playerxdr_function_t *flist, int replace);

/** @}*/
/***************************************************************************/


/* Forward declare types*/
struct _playerc_client_t;
struct _playerc_device_t;


/* forward declaration to avoid including <sys/poll.h>, which may not be
   available when people are building clients against this lib*/
struct pollfd;


/***************************************************************************/
/** @ingroup player_clientlib_libplayerc
 * @defgroup multiclient Multi-Client object
 * @brief The multi-client object manages connections to multiple server in
 * parallel.

@todo Document mutliclient

@{
*/

/* Items in incoming data queue.*/
typedef struct
{
  player_msghdr_t header;
  void *data;
} playerc_client_item_t;


/* Multi-client data*/
typedef struct
{
  /* List of clients being managed*/
  int client_count;
  struct _playerc_client_t *client[128];

  /* Poll info*/
  struct pollfd* pollfd;

  /* Latest time received from any server*/
  double time;

} playerc_mclient_t;

/* Create a multi-client object*/
PLAYERC_EXPORT playerc_mclient_t *playerc_mclient_create(void);

/* Destroy a multi-client object*/
PLAYERC_EXPORT void playerc_mclient_destroy(playerc_mclient_t *mclient);

/* Add a client to the multi-client (private).*/
PLAYERC_EXPORT int playerc_mclient_addclient(playerc_mclient_t *mclient, struct _playerc_client_t *client);

/* Test to see if there is pending data.
   Returns -1 on error, 0 or 1 otherwise.*/
PLAYERC_EXPORT int playerc_mclient_peek(playerc_mclient_t *mclient, int timeout);

/* Read incoming data.  The timeout is in ms.  Set timeout to a
   negative value to wait indefinitely.*/
PLAYERC_EXPORT int playerc_mclient_read(playerc_mclient_t *mclient, int timeout);

/** @} */
/***************************************************************************/


/***************************************************************************/
/** @ingroup player_clientlib_libplayerc
 * @defgroup playerc_client Client API

The client object manages the connection with the Player server; it is
responsible for reading new data, setting data transmission modes and
so on.  The client object must be created and connected before device
proxies are initialized.

@{
*/

/** @brief Typedef for proxy callback function */
PLAYERC_EXPORT typedef void (*playerc_putmsg_fn_t) (void *device, char *header, char *data);

/** @brief Typedef for proxy callback function */
PLAYERC_EXPORT typedef void (*playerc_callback_fn_t) (void *data);


/** @brief Info about an available (but not necessarily subscribed)
    device.
 */
typedef struct
{
  /** Player id of the device. */
  player_devaddr_t addr;

  /** The driver name. */
  char drivername[PLAYER_MAX_DRIVER_STRING_LEN];

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
  int transport;
  struct sockaddr_in server;

  /** Whether or not we're currently connected. Set by
   * playerc_client_connect() and playerc_client_disconnect().  Read-only. */
  int connected;

  /** How many times we'll try to reconnect after a socket error.  Use @ref
   * playerc_client_set_retry_limit() to set this value. Set to -1 for
   * infinite retry. */
  int retry_limit;

  /** How long to sleep, in seconds, to sleep between reconnect attempts.
   * Use @ref playerc_client_set_retry_time() to set this value. */
  double retry_time;

  /** How many messages were lost on the server due to overflows, incremented by player, cleared by user. */
  uint32_t overflow_count;


  /** @internal Socket descriptor */
  int sock;

  /** @internal Data delivery mode */
  uint8_t mode;

  /** @internal Data request flag; if mode == PLAYER_DATAMODE_PULL, have we
   * requested and not yet received a round of data? */
  int data_requested;

  /** @internal Data request flag; if mode == PLAYER_DATAMODE_PULL, have we
   * received any data in this round? */
  int data_received;


  /** List of available (but not necessarily subscribed) devices.
      This list is filled in by playerc_client_get_devlist(). */
  playerc_device_info_t devinfos[PLAYER_MAX_DEVICES];
  int devinfo_count;

  /** List of subscribed devices */
  struct _playerc_device_t *device[PLAYER_MAX_DEVICES];
  int device_count;

  /** @internal A circular queue used to buffer incoming data packets. */
  playerc_client_item_t qitems[PLAYERC_QUEUE_RING_SIZE];
  int qfirst, qlen, qsize;

  /** @internal Temp buffers for incoming / outgoing packets. */
  char *data;
  char *write_xdrdata;
  char *read_xdrdata;
  size_t read_xdrdata_len;


  /** Server time stamp on the last packet. */
  double datatime;
  /** Server time stamp on the previous packet. */
  double lasttime;

  double request_timeout;

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
PLAYERC_EXPORT playerc_client_t *playerc_client_create(playerc_mclient_t *mclient,
                                        const char *host, int port);

/** @brief Destroy a client object.

@param client Pointer to client object.

*/
PLAYERC_EXPORT void playerc_client_destroy(playerc_client_t *client);

/** @brief Set the transport type.

@param client Pointer to client object.
@param transport Either PLAYERC_TRANSPORT_UDP or PLAYERC_TRANSPORT_TCP
*/
PLAYERC_EXPORT void playerc_client_set_transport(playerc_client_t* client,
                                  unsigned int transport);

/** @brief Connect to the server.

@param client Pointer to client object.

@returns Returns 0 on success, non-zero otherwise.  Use
playerc_error_str() to get a descriptive error message.

*/
PLAYERC_EXPORT int playerc_client_connect(playerc_client_t *client);

/** @brief Disconnect from the server.

@param client Pointer to client object.

@returns Returns 0 on success, non-zero otherwise.  Use
playerc_error_str() to get a descriptive error message.

*/
PLAYERC_EXPORT int playerc_client_disconnect(playerc_client_t *client);

/** @brief Disconnect from the server, with potential retry. @internal

@param client Pointer to client object.

@returns Returns 0 on success, non-zero otherwise.
*/
PLAYERC_EXPORT int playerc_client_disconnect_retry(playerc_client_t *client);

/** @brief Change the server's data delivery mode.

Be sure to @ref libplayerc_datamodes "read about data modes" before using this function.

@param client Pointer to client object.

@param mode Data delivery mode; must be one of
PLAYERC_DATAMODE_PUSH, PLAYERC_DATAMODE_PULL; the defalt mode is
PLAYERC_DATAMODE_PUSH.

@returns Returns 0 on success, non-zero otherwise.  Use
playerc_error_str() to get a descriptive error message.

*/
PLAYERC_EXPORT int playerc_client_datamode(playerc_client_t *client, uint8_t mode);

/** @brief Request a round of data.

@param client Pointer to client object.

Request a round of data; only valid when in a request/reply (aka PULL)
data delivery mode.  But you don't need to call this function, because @ref
playerc_client_read will do it for you if the client is in a PULL mode.

Use @ref playerc_client_datamode to change modes.

*/
PLAYERC_EXPORT int playerc_client_requestdata(playerc_client_t* client);

/** @brief Set a replace rule for the client queue on the server

If a rule with the same pattern already exists, it will be replaced with
the new rule (i.e., its setting to replace will be updated).

@param client Pointer to client object.

@param interf Interface to set replace rule for (-1 for wildcard)

@param index Index to set replace rule for (-1 for wildcard)

@param type Type to set replace rule for (-1 for wildcard),
i.e. PLAYER_MSGTYPE_DATA

@param subtype Message subtype to set replace rule for (-1 for wildcard)

@param replace Should we replace these messages

@returns Returns 0 on success, non-zero otherwise.  Use
playerc_error_str() to get a descriptive error message.

*/
PLAYERC_EXPORT int playerc_client_set_replace_rule(playerc_client_t *client, int interf, int index, int type, int subtype, int replace);


/** @brief Add a device proxy. @internal
 */
PLAYERC_EXPORT int playerc_client_adddevice(playerc_client_t *client, struct _playerc_device_t *device);


/** @brief Remove a device proxy. @internal
 */
PLAYERC_EXPORT int playerc_client_deldevice(playerc_client_t *client, struct _playerc_device_t *device);

/** @brief Add user callbacks (called when new data arrives). @internal
 */
PLAYERC_EXPORT int  playerc_client_addcallback(playerc_client_t *client, struct _playerc_device_t *device,
                                playerc_callback_fn_t callback, void *data);

/** @brief Remove user callbacks (called when new data arrives). @internal
 */
PLAYERC_EXPORT int  playerc_client_delcallback(playerc_client_t *client, struct _playerc_device_t *device,
                                playerc_callback_fn_t callback, void *data);

/** @brief Get the list of available device ids.

This function queries the server for the list of available devices,
and write result to the devinfos list in the client object.

@param client Pointer to client object.

@returns Returns 0 on success, non-zero otherwise.  Use
playerc_error_str() to get a descriptive error message.

*/
PLAYERC_EXPORT int playerc_client_get_devlist(playerc_client_t *client);

/** @brief Subscribe a device. @internal
 */
PLAYERC_EXPORT int playerc_client_subscribe(playerc_client_t *client, int code, int index,
                             int access, char *drivername, size_t len);

/** @brief Unsubscribe a device. @internal
 */
PLAYERC_EXPORT int playerc_client_unsubscribe(playerc_client_t *client, int code, int index);

/** @brief Issue a request to the server and await a reply (blocking). @internal

The rep_data pointer is filled with a pointer to the response data received. It is
the callers responisbility to free this memory with the approriate player _free method.

If an error is returned then no data will have been stored in rep_data.

@returns Returns -1 on error and -2 on NACK.

*/
PLAYERC_EXPORT int playerc_client_request(playerc_client_t *client,
                           struct _playerc_device_t *device, uint8_t reqtype,
                           const void *req_data, void **rep_data);


/* @brief Wait for response from server (blocking).

@param client Pointer to client object.
@param device
@param index
@param sequence

@returns Will return data size for ack, -1 for nack and -2 for failure


int playerc_client_getresponse(playerc_client_t *client, uint16_t device,
    uint16_t index, uint16_t sequence, uint8_t * resptype, uint8_t * resp_data, int resp_len);
*/


/** @brief Test to see if there is pending data. Send a data request if one has not been sent already.
 * A data request is necessary to provoke a response from the server.

@param client Pointer to client object.

@param timeout Timeout value (ms).  Set timeout to 0 to check for
currently queued data.

@returns Returns -1 on error, 0 or 1 otherwise.

*/
PLAYERC_EXPORT int playerc_client_peek(playerc_client_t *client, int timeout);

/** @brief Test to see if there is pending data. Don't send a request for data.
 * This function is reliant on a call being made elsewhere to request data from
 * the server.

@param client Pointer to client object.

@param timeout Timeout value (ms).  Set timeout to 0 to check for
currently queued data.

@returns Returns -1 on error, 0 or 1 otherwise.

*/
PLAYERC_EXPORT int playerc_client_internal_peek(playerc_client_t *client, int timeout);

/** @brief Read data from the server (blocking).

In PUSH mode this will read and process a single message. In PULL mode this
will process a full batch of messages up to the sync from the server.

@param client Pointer to client object.

@returns PUSH mode: For data packets, will return the ID of the proxy that got
the data; for synch packets, will return the ID of the client itself;
on error, will return NULL.
PULL mode: Will return NULL on error, the ID of the client on success. Will
never return the ID of a proxy other than the client.

*/
PLAYERC_EXPORT void *playerc_client_read(playerc_client_t *client);

/** Read and process a packet (nonblocking)
   @returns 0 if no data recieved, 1 if data recieved and -1 on error*/
PLAYERC_EXPORT int playerc_client_read_nonblock(playerc_client_t *client);
/** Read and process a packet (nonblocking), fills in pointer to proxy that got data
   @returns 0 if no data recieved, 1 if data recieved and -1 on error*/
PLAYERC_EXPORT int playerc_client_read_nonblock_withproxy(playerc_client_t *client, void ** proxy);

/** @brief Set the timeout for client requests.

@param client Pointer to client object.
@param seconds Seconds to wait for a reply.

*/
PLAYERC_EXPORT void playerc_client_set_request_timeout(playerc_client_t* client, uint32_t seconds);

/** @brief Set the connection retry limit.

@param client Pointer to the client object
@param limit The number of times to attempt to reconnect to the server.  Give -1 for
       infinite retry.
*/
PLAYERC_EXPORT void playerc_client_set_retry_limit(playerc_client_t* client, int limit);

/** @brief Set the connection retry sleep time.

@param client Pointer to the client object
@param time The amount of time, in seconds, to sleep between reconnection attempts.
*/
PLAYERC_EXPORT void playerc_client_set_retry_time(playerc_client_t* client, double time);

/** @brief Write data to the server.  @internal
*/
PLAYERC_EXPORT int playerc_client_write(playerc_client_t *client,
                         struct _playerc_device_t *device,
                         uint8_t subtype,
                         void *cmd, double* timestamp);


/** @} */
/**************************************************************************/


/***************************************************************************/
/** @ingroup player_clientlib_libplayerc
 * @defgroup playerc_device Device API

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
  char drivername[PLAYER_MAX_DRIVER_STRING_LEN];

  /** The subscribe flag is non-zero if the device has been
      successfully subscribed (read-only). */
  int subscribed;

  /** Data timestamp, i.e., the time at which the data was generated (s). */
  double datatime;

  /** Data timestamp from the previous data. */
  double lasttime;

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
PLAYERC_EXPORT void playerc_device_init(playerc_device_t *device, playerc_client_t *client,
                         int code, int index, playerc_putmsg_fn_t putmsg);

/** @brief Finalize the device. @internal */
PLAYERC_EXPORT void playerc_device_term(playerc_device_t *device);

/** @brief Subscribe the device. @internal */
PLAYERC_EXPORT int playerc_device_subscribe(playerc_device_t *device, int access);

/** @brief Unsubscribe the device. @internal */
PLAYERC_EXPORT int playerc_device_unsubscribe(playerc_device_t *device);

/** @brief Request capabilities of device */
PLAYERC_EXPORT int playerc_device_hascapability(playerc_device_t *device, uint32_t type, uint32_t subtype);

/** @brief Request a boolean property */
PLAYERC_EXPORT int playerc_device_get_boolprop(playerc_device_t *device, char *property, BOOL *value);

/** @brief Set a boolean property */
PLAYERC_EXPORT int playerc_device_set_boolprop(playerc_device_t *device, char *property, BOOL value);

/** @brief Request an integer property */
PLAYERC_EXPORT int playerc_device_get_intprop(playerc_device_t *device, char *property, int32_t *value);

/** @brief Set an integer property */
PLAYERC_EXPORT int playerc_device_set_intprop(playerc_device_t *device, char *property, int32_t value);

/** @brief Request a double property */
PLAYERC_EXPORT int playerc_device_get_dblprop(playerc_device_t *device, char *property, double *value);

/** @brief Set a double property */
PLAYERC_EXPORT int playerc_device_set_dblprop(playerc_device_t *device, char *property, double value);

/** @brief Request a string property */
PLAYERC_EXPORT int playerc_device_get_strprop(playerc_device_t *device, char *property, char **value);

/** @brief Set a string property */
PLAYERC_EXPORT int playerc_device_set_strprop(playerc_device_t *device, char *property, char *value);


/** @} */
/**************************************************************************/


/***************************************************************************/
/** @ingroup player_clientlib_libplayerc
 * @defgroup playerc_proxies Device proxies
 * @brief Each interface has a corresponding proxy
*/
/***************************************************************************/

/**************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_aio aio

The aio proxy provides an interface to the analog input/output sensors.

@{
*/

/** Aio proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /* The number of valid analog inputs.*/
  uint8_t voltages_count;

  /* A bitfield of the current digital inputs.*/
  float *voltages;

} playerc_aio_t;


/** Create a aio proxy. */
PLAYERC_EXPORT playerc_aio_t *playerc_aio_create(playerc_client_t *client, int index);

/** Destroy a aio proxy. */
PLAYERC_EXPORT void playerc_aio_destroy(playerc_aio_t *device);

/** Subscribe to the aio device. */
PLAYERC_EXPORT int playerc_aio_subscribe(playerc_aio_t *device, int access);

/** Un-subscribe from the aio device. */
PLAYERC_EXPORT int playerc_aio_unsubscribe(playerc_aio_t *device);

/** Set the output for the aio device. */
PLAYERC_EXPORT int playerc_aio_set_output(playerc_aio_t *device, uint8_t id, float volt);

/** get the aio data */
PLAYERC_EXPORT float playerc_aio_get_data(playerc_aio_t *device, uint32_t index);

/** @} */
/***************************************************************************/


/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_actarray actarray

The actarray proxy provides an interface to actuator arrays
such as the ActivMedia Pioneer Arm. See the Player User Manual for a
complete description of the drivers that support this interface.

@{
*/

#define PLAYERC_ACTARRAY_NUM_ACTUATORS PLAYER_ACTARRAY_NUM_ACTUATORS
#define PLAYERC_ACTARRAY_ACTSTATE_IDLE PLAYER_ACTARRAY_ACTSTATE_IDLE
#define PLAYERC_ACTARRAY_ACTSTATE_MOVING PLAYER_ACTARRAY_ACTSTATE_MOVING
#define PLAYERC_ACTARRAY_ACTSTATE_BRAKED PLAYER_ACTARRAY_ACTSTATE_BRAKED
#define PLAYERC_ACTARRAY_ACTSTATE_STALLED PLAYER_ACTARRAY_ACTSTATE_STALLED
#define PLAYERC_ACTARRAY_TYPE_LINEAR PLAYER_ACTARRAY_TYPE_LINEAR
#define PLAYERC_ACTARRAY_TYPE_ROTARY PLAYER_ACTARRAY_TYPE_ROTARY


/** @brief Actarray device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** The number of actuators in the array. */
  uint32_t actuators_count;
  /** The actuator data, geometry and motor state. */
  player_actarray_actuator_t *actuators_data;
  /** The number of actuators we have geometry for. */
  uint32_t actuators_geom_count;
  player_actarray_actuatorgeom_t *actuators_geom;
  /** Reports if the actuators are off (0) or on (1) */
  uint8_t motor_state;
  /** The position of the base of the actarray. */
  player_point_3d_t base_pos;
  /** The orientation of the base of the actarray. */
  player_orientation_3d_t base_orientation;
} playerc_actarray_t;

/** @brief Create an actarray proxy. */
PLAYERC_EXPORT playerc_actarray_t *playerc_actarray_create(playerc_client_t *client, int index);

/** @brief Destroy an actarray proxy. */
PLAYERC_EXPORT void playerc_actarray_destroy(playerc_actarray_t *device);

/** @brief Subscribe to the actarray device. */
PLAYERC_EXPORT int playerc_actarray_subscribe(playerc_actarray_t *device, int access);

/** @brief Un-subscribe from the actarray device. */
PLAYERC_EXPORT int playerc_actarray_unsubscribe(playerc_actarray_t *device);

/** Accessor method for the actuator data */
PLAYERC_EXPORT player_actarray_actuator_t playerc_actarray_get_actuator_data(playerc_actarray_t *device, uint32_t index);

/** Accessor method for the actuator geom */
PLAYERC_EXPORT player_actarray_actuatorgeom_t playerc_actarray_get_actuator_geom(playerc_actarray_t *device, uint32_t index);

/** @brief Get the actarray geometry.  The writes the result into the proxy
    rather than returning it to the caller. */
PLAYERC_EXPORT int playerc_actarray_get_geom(playerc_actarray_t *device);

/** @brief Command a joint in the array to move to a specified position. */
PLAYERC_EXPORT int playerc_actarray_position_cmd(playerc_actarray_t *device, int joint, float position);

/** @brief Command all joints in the array to move to specified positions. */
PLAYERC_EXPORT int playerc_actarray_multi_position_cmd(playerc_actarray_t *device, float *positions, int positions_count);

/** @brief Command a joint in the array to move at a specified speed. */
PLAYERC_EXPORT int playerc_actarray_speed_cmd(playerc_actarray_t *device, int joint, float speed);

/** @brief Command a joint in the array to move at a specified speed. */
PLAYERC_EXPORT int playerc_actarray_multi_speed_cmd(playerc_actarray_t *device, float *speeds, int speeds_count);

/** @brief Command a joint (or, if joint is -1, the whole array) to go to its home position. */
PLAYERC_EXPORT int playerc_actarray_home_cmd(playerc_actarray_t *device, int joint);

/** @brief Command a joint in the array to move with a specified current. */
PLAYERC_EXPORT int playerc_actarray_current_cmd(playerc_actarray_t *device, int joint, float current);

/** @brief Command all joints in the array to move with specified currents. */
PLAYERC_EXPORT int playerc_actarray_multi_current_cmd(playerc_actarray_t *device, float *currents, int currents_count);


/** @brief Turn the power to the array on or off. Be careful
when turning power on that the array is not obstructed from its home
position in case it moves to it (common behaviour). */
PLAYERC_EXPORT int playerc_actarray_power(playerc_actarray_t *device, uint8_t enable);

/** @brief Turn the brakes of all actuators in the array that have them on or off. */
PLAYERC_EXPORT int playerc_actarray_brakes(playerc_actarray_t *device, uint8_t enable);

/** @brief Set the speed of a joint (-1 for all joints) for all subsequent movement commands. */
PLAYERC_EXPORT int playerc_actarray_speed_config(playerc_actarray_t *device, int joint, float speed);

/* Set the accelration of a joint (-1 for all joints) for all subsequent movement commands*/
PLAYERC_EXPORT int playerc_actarray_accel_config(playerc_actarray_t *device, int joint, float accel);


/** @} */
/**************************************************************************/

/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_audio audio

The audio proxy provides access to drivers supporting the @ref interface_audio .
See the Player User Manual for a complete description of the drivers that support
this interface.

@{
*/

/** @brief Audio device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Details of the channels from the mixer. */
  player_audio_mixer_channel_list_detail_t channel_details_list;

  /** last block of recorded data */
  player_audio_wav_t wav_data;

  /** last block of seq data */
  player_audio_seq_t seq_data;

  /** current channel data */
  player_audio_mixer_channel_list_t mixer_data;

  /** current driver state */
  uint32_t state;

  int last_index;

} playerc_audio_t;

/** @brief Create an audio proxy. */
PLAYERC_EXPORT playerc_audio_t *playerc_audio_create(playerc_client_t *client, int index);

/** @brief Destroy an audio proxy. */
PLAYERC_EXPORT void playerc_audio_destroy(playerc_audio_t *device);

/** @brief Subscribe to the audio device. */
PLAYERC_EXPORT int playerc_audio_subscribe(playerc_audio_t *device, int access);

/** @brief Un-subscribe from the audio device. */
PLAYERC_EXPORT int playerc_audio_unsubscribe(playerc_audio_t *device);

/** @brief Command to play an audio block */
PLAYERC_EXPORT int playerc_audio_wav_play_cmd(playerc_audio_t *device, uint32_t data_count, uint8_t data[], uint32_t format);

/** @brief Command to set recording state */
PLAYERC_EXPORT int playerc_audio_wav_stream_rec_cmd(playerc_audio_t *device, uint8_t state);

/** @brief Command to play prestored sample */
PLAYERC_EXPORT int playerc_audio_sample_play_cmd(playerc_audio_t *device, int index);

/** @brief Command to play sequence of tones */
PLAYERC_EXPORT int playerc_audio_seq_play_cmd(playerc_audio_t *device, player_audio_seq_t * tones);

/** @brief Command to set mixer levels for multiple channels */
PLAYERC_EXPORT int playerc_audio_mixer_multchannels_cmd(playerc_audio_t *device, player_audio_mixer_channel_list_t * levels);

/** @brief Command to set mixer levels for a single channel */
PLAYERC_EXPORT int playerc_audio_mixer_channel_cmd(playerc_audio_t *device, uint32_t index, float amplitude, uint8_t active);

/** @brief Request to record a single audio block
Value is returned into wav_data, block length is determined by device */
PLAYERC_EXPORT int playerc_audio_wav_rec(playerc_audio_t *device);

/** @brief Request to load an audio sample */
PLAYERC_EXPORT int playerc_audio_sample_load(playerc_audio_t *device, int index, uint32_t data_count, uint8_t data[], uint32_t format);

/** @brief Request to retrieve an audio sample
Data is stored in wav_data */
PLAYERC_EXPORT int playerc_audio_sample_retrieve(playerc_audio_t *device, int index);

/** @brief Request to record new sample */
PLAYERC_EXPORT int playerc_audio_sample_rec(playerc_audio_t *device, int index, uint32_t length);

/** @brief Request mixer channel data
result is stored in mixer_data*/
PLAYERC_EXPORT int playerc_audio_get_mixer_levels(playerc_audio_t *device);

/** @brief Request mixer channel details list
result is stored in channel_details_list*/
PLAYERC_EXPORT int playerc_audio_get_mixer_details(playerc_audio_t *device);

/** @} */
/**************************************************************************/

/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_blackboard blackboard

The blackboard proxy provides an interface to a simple data-store in a similar fashion to a hash-map.
Data is set and retrieved by using a label. Any player message structure can be stored in the blackboard.
At this time it is up to the user to pack and unpack the entry data. The xdr functions can be used to do
this.
@{ */

#define PLAYERC_BLACKBOARD_DATA_TYPE_NONE       0
#define PLAYERC_BLACKBOARD_DATA_TYPE_SIMPLE     1
#define PLAYERC_BLACKBOARD_DATA_TYPE_COMPLEX    2

#define PLAYERC_BLACKBOARD_DATA_SUBTYPE_NONE    0
#define PLAYERC_BLACKBOARD_DATA_SUBTYPE_STRING  1
#define PLAYERC_BLACKBOARD_DATA_SUBTYPE_INT     2
#define PLAYERC_BLACKBOARD_DATA_SUBTYPE_DOUBLE  3

/** @brief BlackBoard proxy. */
typedef struct playerc_blackboard
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;
  /** Function to be called when a key is updated. */
  void (*on_blackboard_event)(struct playerc_blackboard*, player_blackboard_entry_t);
  /** Kludge to get around python callback issues. */
  void *py_private;
} playerc_blackboard_t;

/** @brief Create a blackboard proxy. */
PLAYERC_EXPORT playerc_blackboard_t *playerc_blackboard_create(playerc_client_t *client, int index);

/** @brief Destroy a blackboard proxy. */
PLAYERC_EXPORT void playerc_blackboard_destroy(playerc_blackboard_t *device);

/** @brief Subscribe to the blackboard device. */
PLAYERC_EXPORT int playerc_blackboard_subscribe(playerc_blackboard_t *device, int access);

/** @brief Un-subscribe from the blackboard device. */
PLAYERC_EXPORT int playerc_blackboard_unsubscribe(playerc_blackboard_t *device);

/** @brief Subscribe to a key. If entry is none null it will be filled in with the response. The caller is
 * responsible for freeing it. */
PLAYERC_EXPORT int playerc_blackboard_subscribe_to_key(playerc_blackboard_t *device, const char* key, const char* group, player_blackboard_entry_t** entry);

/** @brief Get the current value of a key, without subscribing. If entry is none null it will be filled in with the response. The caller is
 * responsible for freeing it. */
PLAYERC_EXPORT int playerc_blackboard_get_entry(playerc_blackboard_t *device, const char* key, const char* group, player_blackboard_entry_t** entry);

/** @brief Unsubscribe from a key. */
PLAYERC_EXPORT int playerc_blackboard_unsubscribe_from_key(playerc_blackboard_t *device, const char* key, const char* group);

/** @brief Subscribe to a group. The current entries are sent as data messages. */
PLAYERC_EXPORT int playerc_blackboard_subscribe_to_group(playerc_blackboard_t *device, const char* group);

/** @brief Unsubscribe from a group. */
PLAYERC_EXPORT int playerc_blackboard_unsubscribe_from_group(playerc_blackboard_t *device, const char* group);

/** @brief Set an entry value. */
PLAYERC_EXPORT int playerc_blackboard_set_entry(playerc_blackboard_t *device, player_blackboard_entry_t* entry);

PLAYERC_EXPORT int playerc_blackboard_set_string(playerc_blackboard_t *device, const char* key, const char* group, const char* value);

PLAYERC_EXPORT int playerc_blackboard_set_int(playerc_blackboard_t *device, const char* key, const char* group, const int value);

PLAYERC_EXPORT int playerc_blackboard_set_double(playerc_blackboard_t *device, const char* key, const char* group, const double value);

/** @} */

/**************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_blinkenlight blinkenlight

The blinkenlight proxy provides an interface to a (possibly colored
and/or blinking) indicator light.

@{
*/

/** Blinklight proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  uint32_t enabled;
  double duty_cycle;
  double period;
  uint8_t red, green, blue;
} playerc_blinkenlight_t;


/** Create a blinkenlight proxy. */
PLAYERC_EXPORT playerc_blinkenlight_t *playerc_blinkenlight_create(playerc_client_t *client, int index);

/** Destroy a blinkenlight proxy. */
PLAYERC_EXPORT void playerc_blinkenlight_destroy(playerc_blinkenlight_t *device);

/** Subscribe to the blinkenlight device. */
PLAYERC_EXPORT int playerc_blinkenlight_subscribe(playerc_blinkenlight_t *device, int access);

/** Un-subscribe from the blinkenlight device. */
PLAYERC_EXPORT int playerc_blinkenlight_unsubscribe(playerc_blinkenlight_t *device);

/** Enable/disable power to the blinkenlight device. */
PLAYERC_EXPORT int playerc_blinkenlight_enable( playerc_blinkenlight_t *device,
				 uint32_t enable );

/** Set the output color for the blinkenlight device. */
PLAYERC_EXPORT int playerc_blinkenlight_color( playerc_blinkenlight_t *device,
				uint32_t id,
				uint8_t red,
				uint8_t green,
				uint8_t blue );
/** Make the light blink, setting the period in seconds and the
    mark/space ratiom (0.0 to 1.0). */
PLAYERC_EXPORT int playerc_blinkenlight_blink( playerc_blinkenlight_t *device,
				uint32_t id,
				float period,
				float duty_cycle );
/** @} */

/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_blobfinder blobfinder

The blobfinder proxy provides an interface to color blob detectors
such as the ACTS vision system.  See the Player User Manual for a
complete description of the drivers that support this interface.

@{
*/

typedef player_blobfinder_blob_t playerc_blobfinder_blob_t;

/** @brief Blobfinder device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Image dimensions (pixels). */
  unsigned int width, height;

  /** A list of detected blobs. */
  unsigned int blobs_count;
  playerc_blobfinder_blob_t *blobs;

} playerc_blobfinder_t;


/** @brief Create a blobfinder proxy. */
PLAYERC_EXPORT playerc_blobfinder_t *playerc_blobfinder_create(playerc_client_t *client, int index);

/** @brief Destroy a blobfinder proxy. */
PLAYERC_EXPORT void playerc_blobfinder_destroy(playerc_blobfinder_t *device);

/** @brief Subscribe to the blobfinder device. */
PLAYERC_EXPORT int playerc_blobfinder_subscribe(playerc_blobfinder_t *device, int access);

/** @brief Un-subscribe from the blobfinder device. */
PLAYERC_EXPORT int playerc_blobfinder_unsubscribe(playerc_blobfinder_t *device);


/** @} */
/**************************************************************************/


/**************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_bumper bumper

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
  player_bumper_define_t *poses;

  /** Number of points in the scan. */
  int bumper_count;

  /** Bump data: unsigned char, either boolean or code indicating corner. */
  uint8_t *bumpers;

} playerc_bumper_t;


/** @brief Create a bumper proxy. */
PLAYERC_EXPORT playerc_bumper_t *playerc_bumper_create(playerc_client_t *client, int index);

/** @brief Destroy a bumper proxy. */
PLAYERC_EXPORT void playerc_bumper_destroy(playerc_bumper_t *device);

/** @brief Subscribe to the bumper device. */
PLAYERC_EXPORT int playerc_bumper_subscribe(playerc_bumper_t *device, int access);

/** @brief Un-subscribe from the bumper device. */
PLAYERC_EXPORT int playerc_bumper_unsubscribe(playerc_bumper_t *device);

/** @brief Get the bumper geometry.

The writes the result into the proxy rather than returning it to the
caller.

*/
PLAYERC_EXPORT int playerc_bumper_get_geom(playerc_bumper_t *device);


/** @} */
/***************************************************************************/


/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_camera camera

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
  uint8_t *image;

  /** Norm and source channel, filled by playerc_camera_get_source(). */
  char norm[16];
  int source;

} playerc_camera_t;


/** @brief Create a camera proxy. */
PLAYERC_EXPORT playerc_camera_t *playerc_camera_create(playerc_client_t *client, int index);

/** @brief Destroy a camera proxy. */
PLAYERC_EXPORT void playerc_camera_destroy(playerc_camera_t *device);

/** @brief Subscribe to the camera device. */
PLAYERC_EXPORT int playerc_camera_subscribe(playerc_camera_t *device, int access);

/** @brief Un-subscribe from the camera device. */
PLAYERC_EXPORT int playerc_camera_unsubscribe(playerc_camera_t *device);

/** @brief Decompress the image (modifies the current proxy data). */
PLAYERC_EXPORT void playerc_camera_decompress(playerc_camera_t *device);

/** @brief Saves the image to disk as a .ppm */
PLAYERC_EXPORT void playerc_camera_save(playerc_camera_t *device, const char *filename);

/** @brief Set source channel. */
PLAYERC_EXPORT int playerc_camera_set_source(playerc_camera_t *device, int source, const char *norm);

/** @brief Get source channel (sets norm and source fields in the current proxy data). */
PLAYERC_EXPORT int playerc_camera_get_source(playerc_camera_t *device);

/** @brief Force to get current image. */
PLAYERC_EXPORT int playerc_camera_get_image(playerc_camera_t *device);

/** @brief Copy image to some pre-allocated place. */
PLAYERC_EXPORT void playerc_camera_copy_image(playerc_camera_t * device, void * dst, size_t dst_size);

/** @brief Get given component of given pixel. */
PLAYERC_EXPORT unsigned playerc_camera_get_pixel_component(playerc_camera_t * device, unsigned int x, unsigned int y, int component);

/** @} */
/**************************************************************************/


/**************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_dio dio

The dio proxy provides an interface to the digital input/output sensors.

@{
*/

/** Dio proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

    /*/ The number of valid digital inputs.*/
    uint8_t count;

    /*/ A bitfield of the current digital inputs.*/
    uint32_t digin;

} playerc_dio_t;


/** Create a dio proxy. */
PLAYERC_EXPORT playerc_dio_t *playerc_dio_create(playerc_client_t *client, int index);

/** Destroy a dio proxy. */
PLAYERC_EXPORT void playerc_dio_destroy(playerc_dio_t *device);

/** Subscribe to the dio device. */
PLAYERC_EXPORT int playerc_dio_subscribe(playerc_dio_t *device, int access);

/** Un-subscribe from the dio device. */
PLAYERC_EXPORT int playerc_dio_unsubscribe(playerc_dio_t *device);

/** Set the output for the dio device. */
PLAYERC_EXPORT int playerc_dio_set_output(playerc_dio_t *device, uint8_t output_count, uint32_t digout);


/** @} */
/***************************************************************************/


/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_fiducial fiducial

The fiducial proxy provides an interface to a fiducial detector.  This
device looks for fiducials (markers or beacons) in the laser scan, and
determines their identity, range, bearing and orientation.  See the
Player User Manual for a complete description of the various drivers
that support the fiducial interface.

@{
*/


/** @brief Fiducial finder data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Geometry in robot cs.  These values are filled in by
      playerc_fiducial_get_geom().  [pose] is the detector pose in the
      robot cs, [size] is the detector size, [fiducial_size] is the
      fiducial size. */
  player_fiducial_geom_t fiducial_geom;

  /** List of detected beacons. */
  int fiducials_count;
  player_fiducial_item_t *fiducials;

} playerc_fiducial_t;


/** @brief Create a fiducial proxy. */
PLAYERC_EXPORT playerc_fiducial_t *playerc_fiducial_create(playerc_client_t *client, int index);

/** @brief Destroy a fiducial proxy. */
PLAYERC_EXPORT void playerc_fiducial_destroy(playerc_fiducial_t *device);

/** @brief Subscribe to the fiducial device. */
PLAYERC_EXPORT int playerc_fiducial_subscribe(playerc_fiducial_t *device, int access);

/** @brief Un-subscribe from the fiducial device. */
PLAYERC_EXPORT int playerc_fiducial_unsubscribe(playerc_fiducial_t *device);

/** @brief Get the fiducial geometry.

Ths writes the result into the proxy rather than returning it to the
caller.

*/
PLAYERC_EXPORT int playerc_fiducial_get_geom(playerc_fiducial_t *device);


/** @} */
/**************************************************************************/

/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_gps gps
 *

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
PLAYERC_EXPORT playerc_gps_t *playerc_gps_create(playerc_client_t *client, int index);

/** @brief Destroy a gps proxy. */
PLAYERC_EXPORT void playerc_gps_destroy(playerc_gps_t *device);

/** @brief Subscribe to the gps device. */
PLAYERC_EXPORT int playerc_gps_subscribe(playerc_gps_t *device, int access);

/** @brief Un-subscribe from the gps device. */
PLAYERC_EXPORT int playerc_gps_unsubscribe(playerc_gps_t *device);


/** @} */
/**************************************************************************/

/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_graphics2d graphics2d

The graphics2d proxy provides an interface to the graphics2d

@{
*/

/** @brief Graphics2d device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** current drawing color */
  player_color_t color;

} playerc_graphics2d_t;


/** @brief Create a graphics2d device proxy. */
PLAYERC_EXPORT playerc_graphics2d_t *playerc_graphics2d_create(playerc_client_t *client, int index);

/** @brief Destroy a graphics2d device proxy. */
PLAYERC_EXPORT void playerc_graphics2d_destroy(playerc_graphics2d_t *device);

/** @brief Subscribe to the graphics2d device */
PLAYERC_EXPORT int playerc_graphics2d_subscribe(playerc_graphics2d_t *device, int access);

/** @brief Un-subscribe from the graphics2d device */
PLAYERC_EXPORT int playerc_graphics2d_unsubscribe(playerc_graphics2d_t *device);

/** @brief Set the current drawing color */
PLAYERC_EXPORT int playerc_graphics2d_setcolor(playerc_graphics2d_t *device,
                                player_color_t col );

/** @brief Draw some points */
PLAYERC_EXPORT int playerc_graphics2d_draw_points(playerc_graphics2d_t *device,
           player_point_2d_t pts[],
           int count );

/** @brief Draw a polyline that connects an array of points */
PLAYERC_EXPORT int playerc_graphics2d_draw_polyline(playerc_graphics2d_t *device,
             player_point_2d_t pts[],
             int count );

/** @brief Draw a set of lines whose end points are at pts[2n] and pts[2n+1] */
PLAYERC_EXPORT int playerc_graphics2d_draw_multiline(playerc_graphics2d_t *device,
             player_point_2d_t pts[],
             int count );



/** @brief Draw a polygon */
PLAYERC_EXPORT int playerc_graphics2d_draw_polygon(playerc_graphics2d_t *device,
            player_point_2d_t pts[],
            int count,
            int filled,
            player_color_t fill_color );

/** @brief Clear the canvas */
PLAYERC_EXPORT int playerc_graphics2d_clear(playerc_graphics2d_t *device );


/** @} */

/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_graphics3d graphics3d

The graphics3d proxy provides an interface to the graphics3d

@{
*/

/** @brief Graphics3d device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** current drawing color */
  player_color_t color;

} playerc_graphics3d_t;


/** @brief Create a graphics3d device proxy. */
PLAYERC_EXPORT playerc_graphics3d_t *playerc_graphics3d_create(playerc_client_t *client, int index);

/** @brief Destroy a graphics3d device proxy. */
PLAYERC_EXPORT void playerc_graphics3d_destroy(playerc_graphics3d_t *device);

/** @brief Subscribe to the graphics3d device */
PLAYERC_EXPORT int playerc_graphics3d_subscribe(playerc_graphics3d_t *device, int access);

/** @brief Un-subscribe from the graphics3d device */
PLAYERC_EXPORT int playerc_graphics3d_unsubscribe(playerc_graphics3d_t *device);

/** @brief Set the current drawing color */
PLAYERC_EXPORT int playerc_graphics3d_setcolor(playerc_graphics3d_t *device,
                                player_color_t col );

/** @brief Draw some points in the given mode */
PLAYERC_EXPORT int playerc_graphics3d_draw(playerc_graphics3d_t *device,
           player_graphics3d_draw_mode_t mode,
           player_point_3d_t pts[],
           int count );

/** @brief Clear the canvas */
PLAYERC_EXPORT int playerc_graphics3d_clear(playerc_graphics3d_t *device );

/** @brief Translate the drawing coordinate system in 3d */
PLAYERC_EXPORT int playerc_graphics3d_translate(playerc_graphics3d_t *device,
				 double x, double y, double z );


/** @brief Rotate the drawing coordinate system by [a] radians about the vector described by [x,y,z] */
PLAYERC_EXPORT int playerc_graphics3d_rotate( playerc_graphics3d_t *device,
			       double a, double x, double y, double z );
/** @} */

/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_gripper gripper

The gripper proxy provides an interface to the gripper

@{
*/

/** @brief Gripper device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Gripper geometry in the robot cs:
      pose gives the position and orientation,
      outer_size gives the extent when open
      inner_size gives the size of the space between the open fingers
      These values are initially zero, but can be filled in by calling
      playerc_gripper_get_geom(). */
  player_pose3d_t pose;
  player_bbox3d_t outer_size;
  player_bbox3d_t inner_size;
  /** The number of breakbeams the gripper has */
  uint8_t num_beams;
  /** The capacity of the gripper's store - if 0, the gripper cannot store */
  uint8_t capacity;

  /** The gripper's state: may be one of PLAYER_GRIPPER_STATE_OPEN,
    PLAYER_GRIPPER_STATE_CLOSED, PLAYER_GRIPPER_STATE_MOVING
    or PLAYER_GRIPPER_STATE_ERROR. */
  uint8_t state;
  /** The position of the object in the gripper */
  uint32_t beams;
  /** The number of currently-stored objects */
  uint8_t stored;
} playerc_gripper_t;

/** @brief Create a gripper device proxy. */
PLAYERC_EXPORT playerc_gripper_t *playerc_gripper_create(playerc_client_t *client, int index);

/** @brief Destroy a gripper device proxy. */
PLAYERC_EXPORT void playerc_gripper_destroy(playerc_gripper_t *device);

/** @brief Subscribe to the gripper device */
PLAYERC_EXPORT int playerc_gripper_subscribe(playerc_gripper_t *device, int access);

/** @brief Un-subscribe from the gripper device */
PLAYERC_EXPORT int playerc_gripper_unsubscribe(playerc_gripper_t *device);

/** @brief Command the gripper to open */
PLAYERC_EXPORT int playerc_gripper_open_cmd(playerc_gripper_t *device);

/** @brief Command the gripper to close */
PLAYERC_EXPORT int playerc_gripper_close_cmd(playerc_gripper_t *device);

/** @brief Command the gripper to stop */
PLAYERC_EXPORT int playerc_gripper_stop_cmd(playerc_gripper_t *device);

/** @brief Command the gripper to store */
PLAYERC_EXPORT int playerc_gripper_store_cmd(playerc_gripper_t *device);

/** @brief Command the gripper to retrieve */
PLAYERC_EXPORT int playerc_gripper_retrieve_cmd(playerc_gripper_t *device);

/** @brief Print a human-readable version of the gripper state. If
    set, the string &lt;prefix&gt; is printed before the state string. */
PLAYERC_EXPORT void playerc_gripper_printout(playerc_gripper_t *device, const char* prefix);

/** @brief Get the gripper geometry.

This writes the result into the proxy rather than returning it to the
caller. */
PLAYERC_EXPORT int playerc_gripper_get_geom(playerc_gripper_t *device);

/** @} */
/**************************************************************************/

/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_health health

The health proxy provides an interface to the HEALTH Module.
@{
*/


/** Note: the structure describing the HEALTH's data packet is declared in
          Player. */


/** @brief HEALTH proxy data. */
typedef struct
{
    /** Device info; must be at the start of all device structures.         */
    playerc_device_t info;
    /** The current cpu usage						*/
    player_health_cpu_t cpu_usage;
    /** The memory stats						*/
    player_health_memory_t mem;
    /** The swap stats							*/
    player_health_memory_t swap;
} playerc_health_t;


/** @brief Create a health proxy. */
PLAYERC_EXPORT playerc_health_t *playerc_health_create(playerc_client_t *client, int index);

/** @brief Destroy a health proxy. */
PLAYERC_EXPORT void playerc_health_destroy(playerc_health_t *device);

/** @brief Subscribe to the health device. */
PLAYERC_EXPORT int playerc_health_subscribe(playerc_health_t *device, int access);

/** @brief Un-subscribe from the health device. */
PLAYERC_EXPORT int playerc_health_unsubscribe(playerc_health_t *device);


/** @} */
/***************************************************************************/


/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_ir ir

The ir proxy provides an interface to the ir sensors built into robots
such as the RWI B21R.

@{
*/

/** @brief Ir proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /* data*/
  player_ir_data_t data;

  /* config*/
  player_ir_pose_t poses;

} playerc_ir_t;


/** @brief Create a ir proxy. */
PLAYERC_EXPORT playerc_ir_t *playerc_ir_create(playerc_client_t *client, int index);

/** @brief Destroy a ir proxy. */
PLAYERC_EXPORT void playerc_ir_destroy(playerc_ir_t *device);

/** @brief Subscribe to the ir device. */
PLAYERC_EXPORT int playerc_ir_subscribe(playerc_ir_t *device, int access);

/** @brief Un-subscribe from the ir device. */
PLAYERC_EXPORT int playerc_ir_unsubscribe(playerc_ir_t *device);

/** @brief Get the ir geometry.

This writes the result into the proxy rather than returning it to the
caller.

*/
PLAYERC_EXPORT int playerc_ir_get_geom(playerc_ir_t *device);


/** @} */
/***************************************************************************/


/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_joystick joystick

The joystick proxy can be used to get images from a joystick.

@{
*/

/** @brief joystick proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;
  int32_t axes_count;
  int32_t pos[8];
  uint32_t buttons;
  int * axes_max;
  int * axes_min;
  double * scale_pos;


} playerc_joystick_t;


/** @brief Create a joystick proxy. */
PLAYERC_EXPORT playerc_joystick_t *playerc_joystick_create(playerc_client_t *client, int index);

/** @brief Destroy a joystick proxy. */
PLAYERC_EXPORT void playerc_joystick_destroy(playerc_joystick_t *device);

/** @brief Subscribe to the joystick device. */
PLAYERC_EXPORT int playerc_joystick_subscribe(playerc_joystick_t *device, int access);

/** @brief Un-subscribe from the joystick device. */
PLAYERC_EXPORT int playerc_joystick_unsubscribe(playerc_joystick_t *device);

/** @} */
/**************************************************************************/


/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_laser laser

The laser proxy provides an interface to scanning laser range finders
such as the @ref driver_sicklms200.  Data is returned in the
playerc_laser_t structure.

This proxy wraps the low-level @ref interface_laser interface.

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
  double scan_res;

  /** Range resolution, in m. */
  double range_res;

  /** Maximum range of sensor, in m. */
  double max_range;

  /** Scanning frequency in Hz. */
  double scanning_frequency;

  /** Raw range data; range (m). */
  double *ranges;

  /** Scan data; range (m) and bearing (radians). */
  double (*scan)[2];

  /** Scan data; x, y position (m). */
  player_point_2d_t *point;

  /** Scan reflection intensity values (0-3).  Note that the intensity
      values will only be filled if intensity information is enabled
      (using the set_config function). */
  int *intensity;

  /** ID for this scan */
  int scan_id;

  /** Laser IDentification information */
  int laser_id;

  /** Minimum range, in meters, in the right half of the scan (those ranges
   * from the first beam, counterclockwise, up to the middle of the scan,
   * including the middle beam, if one exists). */
  double min_right;

  /** Minimum range, in meters, in the left half of the scan (those ranges
   * from the first beam after the middle of the scan, counterclockwise, to
   * the last beam). */
  double min_left;
} playerc_laser_t;


/** @brief Create a laser proxy. */
PLAYERC_EXPORT playerc_laser_t *playerc_laser_create(playerc_client_t *client, int index);

/** @brief Destroy a laser proxy. */
PLAYERC_EXPORT void playerc_laser_destroy(playerc_laser_t *device);

/** @brief Subscribe to the laser device. */
PLAYERC_EXPORT int playerc_laser_subscribe(playerc_laser_t *device, int access);

/** @brief Un-subscribe from the laser device. */
PLAYERC_EXPORT int playerc_laser_unsubscribe(playerc_laser_t *device);

/** @brief Configure the laser.

@param device Pointer to proxy object.

@param min_angle, max_angle Start and end angles for the scan
(radians).

@param resolution Angular resolution in radians. Valid values depend on the
underlyling driver.

@param range_res Range resolution in m.  Valid values depend on the
underlyling driver.

@param intensity Intensity flag; set to 1 to enable reflection intensity data.

@param scanning_frequency Scanning frequency in Hz. Valid values depend on the
underlyling driver.

@returns Returns 0 on success, non-zero otherwise.  Use
playerc_error_str() to get a descriptive error message.

*/
PLAYERC_EXPORT int playerc_laser_set_config(playerc_laser_t *device,
                             double min_angle, double max_angle,
                             double resolution,
                             double range_res,
                             unsigned char intensity,
                             double scanning_frequency);

/** @brief Get the laser configuration.

@param device Pointer to proxy object.

@param min_angle, max_angle Start and end angles for the scan
(radians).

@param resolution Angular resolution in radians. Valid values depend on the
underlyling driver.

@param range_res Range resolution in m.  Valid values depend on the
underlyling driver.

@param intensity Intensity flag; set to 1 to enable reflection intensity data.

@param scanning_frequency Scanning frequency in Hz. Valid values depend on the
underlyling driver.

@returns Returns 0 on success, non-zero otherwise.  Use
playerc_error_str() to get a descriptive error message.

*/
PLAYERC_EXPORT int playerc_laser_get_config(playerc_laser_t *device,
                             double *min_angle,
                             double *max_angle,
                             double *resolution,
                             double *range_res,
                             unsigned char *intensity,
                             double *scanning_frequency);

/** @brief Get the laser geometry.

This writes the result into the proxy rather than returning it to the
caller.

*/
PLAYERC_EXPORT int playerc_laser_get_geom(playerc_laser_t *device);

/** @brief Get the laser IDentification information.

This writes the result into the proxy rather than returning it to the
caller. */
PLAYERC_EXPORT int playerc_laser_get_id (playerc_laser_t *device);

/** @brief Print a human-readable summary of the laser state on
    stdout. */
PLAYERC_EXPORT void playerc_laser_printout( playerc_laser_t * device,
        const char* prefix );

/** @} */
/**************************************************************************/


/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_limb limb

The limb proxy provides an interface to limbs using forward/inverse
kinematics, such as the ActivMedia Pioneer Arm. See the Player User Manual for a
complete description of the drivers that support this interface.

@{
*/

/** @brief Limb device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  player_limb_data_t data;
  player_limb_geom_req_t geom;
} playerc_limb_t;

/** @brief Create a limb proxy. */
PLAYERC_EXPORT playerc_limb_t *playerc_limb_create(playerc_client_t *client, int index);

/** @brief Destroy a limb proxy. */
PLAYERC_EXPORT void playerc_limb_destroy(playerc_limb_t *device);

/** @brief Subscribe to the limb device. */
PLAYERC_EXPORT int playerc_limb_subscribe(playerc_limb_t *device, int access);

/** @brief Un-subscribe from the limb device. */
PLAYERC_EXPORT int playerc_limb_unsubscribe(playerc_limb_t *device);

/** @brief Get the limb geometry.  The writes the result into the proxy
    rather than returning it to the caller. */
PLAYERC_EXPORT int playerc_limb_get_geom(playerc_limb_t *device);

/** @brief Command the end effector to move home. */
PLAYERC_EXPORT int playerc_limb_home_cmd(playerc_limb_t *device);

/** @brief Command the end effector to stop immediatly. */
PLAYERC_EXPORT int playerc_limb_stop_cmd(playerc_limb_t *device);

/** @brief Command the end effector to move to a specified pose. */
PLAYERC_EXPORT int playerc_limb_setpose_cmd(playerc_limb_t *device, float pX, float pY, float pZ, float aX, float aY, float aZ, float oX, float oY, float oZ);

/** @brief Command the end effector to move to a specified position
(ignoring approach and orientation vectors). */
PLAYERC_EXPORT int playerc_limb_setposition_cmd(playerc_limb_t *device, float pX, float pY, float pZ);

/** @brief Command the end effector to move along the provided vector from
its current position for the provided distance. */
PLAYERC_EXPORT int playerc_limb_vecmove_cmd(playerc_limb_t *device, float x, float y, float z, float length);

/** @brief Turn the power to the limb on or off. Be careful
when turning power on that the limb is not obstructed from its home
position in case it moves to it (common behaviour). */
PLAYERC_EXPORT int playerc_limb_power(playerc_limb_t *device, uint32_t enable);

/** @brief Turn the brakes of all actuators in the limb that have them on or off. */
PLAYERC_EXPORT int playerc_limb_brakes(playerc_limb_t *device, uint32_t enable);

/** @brief Set the speed of the end effector (m/s) for all subsequent movement commands. */
PLAYERC_EXPORT int playerc_limb_speed_config(playerc_limb_t *device, float speed);

/** @} */
/**************************************************************************/


/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_localize localize

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
  player_localize_hypoth_t *hypoths;

  double mean[3];
  double variance;
  int num_particles;
  playerc_localize_particle_t *particles;

} playerc_localize_t;


/** @brief Create a localize proxy. */
PLAYERC_EXPORT playerc_localize_t *playerc_localize_create(playerc_client_t *client, int index);

/** @brief Destroy a localize proxy. */
PLAYERC_EXPORT void playerc_localize_destroy(playerc_localize_t *device);

/** @brief Subscribe to the localize device. */
PLAYERC_EXPORT int playerc_localize_subscribe(playerc_localize_t *device, int access);

/** @brief Un-subscribe from the localize device. */
PLAYERC_EXPORT int playerc_localize_unsubscribe(playerc_localize_t *device);

/** @brief Set the the robot pose (mean and covariance). */
PLAYERC_EXPORT int playerc_localize_set_pose(playerc_localize_t *device, double pose[3], double cov[6]);

/** @brief Request the particle set.  Caller must supply sufficient storage for
   the result. */
PLAYERC_EXPORT int playerc_localize_get_particles(playerc_localize_t *device);

/** @} */
/**************************************************************************/


/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_log log

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
PLAYERC_EXPORT playerc_log_t *playerc_log_create(playerc_client_t *client, int index);

/** @brief Destroy a log proxy. */
PLAYERC_EXPORT void playerc_log_destroy(playerc_log_t *device);

/** @brief Subscribe to the log device. */
PLAYERC_EXPORT int playerc_log_subscribe(playerc_log_t *device, int access);

/** @brief Un-subscribe from the log device. */
PLAYERC_EXPORT int playerc_log_unsubscribe(playerc_log_t *device);

/** @brief Start/stop logging */
PLAYERC_EXPORT int playerc_log_set_write_state(playerc_log_t* device, int state);

/** @brief Start/stop playback */
PLAYERC_EXPORT int playerc_log_set_read_state(playerc_log_t* device, int state);

/** @brief Rewind playback */
PLAYERC_EXPORT int playerc_log_set_read_rewind(playerc_log_t* device);

/** @brief Get logging/playback state.

The result is written into the proxy.

*/
PLAYERC_EXPORT int playerc_log_get_state(playerc_log_t* device);

/** @brief Change name of log file to write to. */
PLAYERC_EXPORT int playerc_log_set_filename(playerc_log_t* device, const char* fname);


/** @} */


/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_map map

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

  /** Map origin, in meters (i.e., the real-world coordinates of cell 0,0)*/
  double origin[2];

  /** Value for each cell (-range <= EMPTY < 0, unknown = 0, 0 < OCCUPIED <= range) */
  int8_t data_range;

  /** Occupancy for each cell */
  char* cells;
 
  /** Vector-based version of the map (call playerc_map_get_vector() to
   * fill this in). */
  double vminx, vminy, vmaxx, vmaxy;
  int num_segments;
  player_segment_t* segments;
} playerc_map_t;

/** @brief Convert from a cell position to a map index.
 */
#define PLAYERC_MAP_INDEX(dev, i, j) ((dev->width) * (j) + (i))

/** @brief Create a map proxy. */
PLAYERC_EXPORT playerc_map_t *playerc_map_create(playerc_client_t *client, int index);

/** @brief Destroy a map proxy. */
PLAYERC_EXPORT void playerc_map_destroy(playerc_map_t *device);

/** @brief Subscribe to the map device. */
PLAYERC_EXPORT int playerc_map_subscribe(playerc_map_t *device, int access);

/** @brief Un-subscribe from the map device. */
PLAYERC_EXPORT int playerc_map_unsubscribe(playerc_map_t *device);

/** @brief Get the map, which is stored in the proxy. */
PLAYERC_EXPORT int playerc_map_get_map(playerc_map_t* device);

/** @brief Get the vector map, which is stored in the proxy. */
PLAYERC_EXPORT int playerc_map_get_vector(playerc_map_t* device);

/** @} */
/**************************************************************************/

/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_vectormap vectormap

The vectormap proxy provides an interface to a map of geometric features.

@{ */

/** @brief Vectormap proxy. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;
  /** Spatial reference identifier. Use '0' if you are not using spatial references. */
  uint32_t srid;
  /** Boundary area. */
  player_extent2d_t extent;
  /** The number of layers. */
  uint32_t layers_count;
  /** Layer data. */
  player_vectormap_layer_data_t** layers_data;
  /** Layer info. */
  player_vectormap_layer_info_t** layers_info;
  /** WKB processor instance if you want to deal with WKB data */
  playerwkbprocessor_t wkbprocessor;

} playerc_vectormap_t;

/** @brief Create a vectormap proxy. */
PLAYERC_EXPORT playerc_vectormap_t *playerc_vectormap_create(playerc_client_t *client, int index);

/** @brief Destroy a vectormap proxy. */
PLAYERC_EXPORT void playerc_vectormap_destroy(playerc_vectormap_t *device);

/** @brief Subscribe to the vectormap device. */
PLAYERC_EXPORT int playerc_vectormap_subscribe(playerc_vectormap_t *device, int access);

/** @brief Un-subscribe from the vectormap device. */
PLAYERC_EXPORT int playerc_vectormap_unsubscribe(playerc_vectormap_t *device);

/** @brief Get the vectormap metadata, which is stored in the proxy. */
PLAYERC_EXPORT int playerc_vectormap_get_map_info(playerc_vectormap_t* device);

/** @brief Get the layer data by index. Must only be used after a successfull call to playerc_vectormap_get_map_info. */
PLAYERC_EXPORT int playerc_vectormap_get_layer_data(playerc_vectormap_t *device, unsigned layer_index);

/** @brief Write layer data. */
PLAYERC_EXPORT int playerc_vectormap_write_layer(playerc_vectormap_t *device, const player_vectormap_layer_data_t * data);

/** @brief Clean up the dynamically allocated memory for the vectormap. */
PLAYERC_EXPORT void playerc_vectormap_cleanup(playerc_vectormap_t *device);

/** @brief Get an individual feature as a WKB geometry. Must only be used after a successful call to playerc_vectormap_get_layer_data.
 *  The WKB geometry is owned by the proxy, duplicate it if it is needed after the next call to get_feature_data. */
PLAYERC_EXPORT uint8_t * playerc_vectormap_get_feature_data(playerc_vectormap_t *device, unsigned layer_index, unsigned feature_index);
PLAYERC_EXPORT size_t playerc_vectormap_get_feature_data_count(playerc_vectormap_t *device, unsigned layer_index, unsigned feature_index);

/** @} */

/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_opaque opaque

The opaque proxy provides an interface for generic messages to drivers.
See examples/plugins/opaquedriver for an example of using this interface in
combination with a custom plugin.
@{
*/

/** @brief Opaque device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Size of data (bytes) */
  int data_count;

  /** Data  */
  uint8_t *data;
} playerc_opaque_t;

/** @brief Create an opaque device proxy. */
PLAYERC_EXPORT playerc_opaque_t *playerc_opaque_create(playerc_client_t *client, int index);

/** @brief Destroy an opaque device proxy. */
PLAYERC_EXPORT void playerc_opaque_destroy(playerc_opaque_t *device);

/** @brief Subscribe to the opaque device */
PLAYERC_EXPORT int playerc_opaque_subscribe(playerc_opaque_t *device, int access);

/** @brief Un-subscribe from the opaque device */
PLAYERC_EXPORT int playerc_opaque_unsubscribe(playerc_opaque_t *device);

/** @brief Send a generic command */
PLAYERC_EXPORT int playerc_opaque_cmd(playerc_opaque_t *device, player_opaque_data_t *data);

/** @brief Send a generic request
 *
 * If a non null value is passed for reply memory for the response will be allocated
 * and its pointer stored in reply. The caller is responsible for freeing this memory
 *
 * If an error is returned no memory will have been allocated*/
PLAYERC_EXPORT int playerc_opaque_req(playerc_opaque_t *device, player_opaque_data_t *request, player_opaque_data_t **reply);

/** @} */
/**************************************************************************/


/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_planner planner

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
  double (*waypoints)[3];

} playerc_planner_t;

/** @brief Create a planner device proxy. */
PLAYERC_EXPORT playerc_planner_t *playerc_planner_create(playerc_client_t *client, int index);

/** @brief Destroy a planner device proxy. */
PLAYERC_EXPORT void playerc_planner_destroy(playerc_planner_t *device);

/** @brief Subscribe to the planner device */
PLAYERC_EXPORT int playerc_planner_subscribe(playerc_planner_t *device, int access);

/** @brief Un-subscribe from the planner device */
PLAYERC_EXPORT int playerc_planner_unsubscribe(playerc_planner_t *device);

/** @brief Set the goal pose (gx, gy, ga) */
PLAYERC_EXPORT int playerc_planner_set_cmd_pose(playerc_planner_t *device,
                                  double gx, double gy, double ga);

/** @brief Get the list of waypoints.

Writes the result into the proxy rather than returning it to the
caller.

*/
PLAYERC_EXPORT int playerc_planner_get_waypoints(playerc_planner_t *device);

/** @brief Enable / disable the robot's motion

Set state to 1 to enable, 0 to disable.

*/
PLAYERC_EXPORT int playerc_planner_enable(playerc_planner_t *device, int state);

/** @} */
/**************************************************************************/

/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_position1d position1d

The position1d proxy provides an interface to 1 DOF actuator such as a
linear or rotational actuator.

@{
*/

/** Position1d device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Robot geometry in robot cs: pose gives the position1d and
      orientation, size gives the extent.  These values are filled in
      by playerc_position1d_get_geom(). */
  double pose[3];
  double size[2];

  /** Odometric pose [m] or [rad]. */
  double pos;

  /** Odometric velocity [m/s] or [rad/s]. */
  double vel;

  /** Stall flag [0, 1]. */
  int stall;

  /** Status bitfield of extra data in the following order:
      - status (unsigned byte)
        - bit 0: limit min
        - bit 1: limit center
        - bit 2: limit max
        - bit 3: over current
        - bit 4: trajectory complete
        - bit 5: is enabled
        - bit 6:
        - bit 7:
    */
  int status;

} playerc_position1d_t;

/** Create a position1d device proxy. */
PLAYERC_EXPORT playerc_position1d_t *playerc_position1d_create(playerc_client_t *client,
                                                int index);

/** Destroy a position1d device proxy. */
PLAYERC_EXPORT void playerc_position1d_destroy(playerc_position1d_t *device);

/** Subscribe to the position1d device */
PLAYERC_EXPORT int playerc_position1d_subscribe(playerc_position1d_t *device, int access);

/** Un-subscribe from the position1d device */
PLAYERC_EXPORT int playerc_position1d_unsubscribe(playerc_position1d_t *device);

/** Enable/disable the motors */
PLAYERC_EXPORT int playerc_position1d_enable(playerc_position1d_t *device, int enable);

/** Get the position1d geometry.  The writes the result into the proxy
    rather than returning it to the caller. */
PLAYERC_EXPORT int playerc_position1d_get_geom(playerc_position1d_t *device);

/** Set the target speed. */
PLAYERC_EXPORT int playerc_position1d_set_cmd_vel(playerc_position1d_t *device,
                                   double vel, int state);

/** @brief Set the target position.
    -@arg pos The position to move to
 */
PLAYERC_EXPORT int playerc_position1d_set_cmd_pos(playerc_position1d_t *device,
                                   double pos, int state);

/** @brief Set the target position with movement velocity
    -@arg pos The position to move to
    -@arg vel The speed at which to move to the position
 */
PLAYERC_EXPORT int playerc_position1d_set_cmd_pos_with_vel(playerc_position1d_t *device,
                                            double pos, double vel, int state);

/** Set the odometry offset */
PLAYERC_EXPORT int playerc_position1d_set_odom(playerc_position1d_t *device,
                                double odom);

/** @} */
/**************************************************************************/


/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_position2d position2d

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
PLAYERC_EXPORT playerc_position2d_t *playerc_position2d_create(playerc_client_t *client,
                                                int index);

/** Destroy a position2d device proxy. */
PLAYERC_EXPORT void playerc_position2d_destroy(playerc_position2d_t *device);

/** Subscribe to the position2d device */
PLAYERC_EXPORT int playerc_position2d_subscribe(playerc_position2d_t *device, int access);

/** Un-subscribe from the position2d device */
PLAYERC_EXPORT int playerc_position2d_unsubscribe(playerc_position2d_t *device);

/** Enable/disable the motors */
PLAYERC_EXPORT int playerc_position2d_enable(playerc_position2d_t *device, int enable);

/** Get the position2d geometry.  The writes the result into the proxy
    rather than returning it to the caller. */
PLAYERC_EXPORT int playerc_position2d_get_geom(playerc_position2d_t *device);

/** Set the target speed.  vx : forward speed (m/s).  vy : sideways
    speed (m/s); this field is used by omni-drive robots only.  va :
    rotational speed (rad/s).  All speeds are defined in the robot
    coordinate system. */
PLAYERC_EXPORT int playerc_position2d_set_cmd_vel(playerc_position2d_t *device,
                                   double vx, double vy, double va, int state);

/** Set the target pose with given motion vel */
PLAYERC_EXPORT int playerc_position2d_set_cmd_pose_with_vel(playerc_position2d_t *device,
                                             player_pose2d_t pos,
                                             player_pose2d_t vel,
                                             int state);

/** Set the target speed and heading.  vx : forward speed (m/s).  vy : sideways
    speed (m/s); this field is used by omni-drive robots only.  pa :
    rotational heading (rad).  All speeds and angles are defined in the robot
    coordinate system. */
PLAYERC_EXPORT int playerc_position2d_set_cmd_vel_head(playerc_position2d_t *device,
                                   double vx, double vy, double pa, int state);



/** Set the target pose (gx, gy, ga) is the target pose in the
    odometric coordinate system. */
PLAYERC_EXPORT int playerc_position2d_set_cmd_pose(playerc_position2d_t *device,
                                    double gx, double gy, double ga, int state);

/** Set the target cmd for car like position */
PLAYERC_EXPORT int playerc_position2d_set_cmd_car(playerc_position2d_t *device,
                                    double vx, double a);

/** Set the odometry offset */
PLAYERC_EXPORT int playerc_position2d_set_odom(playerc_position2d_t *device,
                                double ox, double oy, double oa);

/** @} */
/**************************************************************************/

/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_position3d position3d

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
PLAYERC_EXPORT playerc_position3d_t *playerc_position3d_create(playerc_client_t *client,
                                                int index);

/** Destroy a position3d device proxy. */
PLAYERC_EXPORT void playerc_position3d_destroy(playerc_position3d_t *device);

/** Subscribe to the position3d device */
PLAYERC_EXPORT int playerc_position3d_subscribe(playerc_position3d_t *device, int access);

/** Un-subscribe from the position3d device */
PLAYERC_EXPORT int playerc_position3d_unsubscribe(playerc_position3d_t *device);

/** Enable/disable the motors */
PLAYERC_EXPORT int playerc_position3d_enable(playerc_position3d_t *device, int enable);

/** Get the position3d geometry.  The writes the result into the proxy
    rather than returning it to the caller. */
PLAYERC_EXPORT int playerc_position3d_get_geom(playerc_position3d_t *device);

/** Set the target speed.  vx : forward speed (m/s).  vy : sideways
    speed (m/s); vz : vertical speed (m/s). vr : roll speed (rad/s) .
    vp : pitch speed (rad/s) . vt : theta speed (rad/s).
    All speeds are defined in the robot coordinate system. */
PLAYERC_EXPORT int playerc_position3d_set_velocity(playerc_position3d_t *device,
                                    double vx, double vy, double vz,
                                    double vr, double vp, double vt,
                                    int state);

/** For compatibility with old position3d interface */
PLAYERC_EXPORT int playerc_position3d_set_speed(playerc_position3d_t *device,
                                 double vx, double vy, double vz, int state);

/** Set the target pose (gx, gy, ga, gr, gp, gt) is the target pose in the
    odometric coordinate system. */
PLAYERC_EXPORT int playerc_position3d_set_pose(playerc_position3d_t *device,
                                double gx, double gy, double gz,
                                double gr, double gp, double gt);


/** Set the target pose (pos,vel) define desired position and motion speed */
PLAYERC_EXPORT int playerc_position3d_set_pose_with_vel(playerc_position3d_t *device,
                                         player_pose3d_t pos,
                                         player_pose3d_t vel);

/** For compatibility with old position3d interface */
PLAYERC_EXPORT int playerc_position3d_set_cmd_pose(playerc_position3d_t *device,
                                    double gx, double gy, double gz);

/** Set the velocity mode. This is driver dependent. */
PLAYERC_EXPORT int playerc_position3d_set_vel_mode(playerc_position3d_t *device, int mode);

/** Set the odometry offset */
PLAYERC_EXPORT int playerc_position3d_set_odom(playerc_position3d_t *device,
                                double ox, double oy, double oz,
                                double oroll, double opitch, double oyaw);

/** Reset the odometry offset */
PLAYERC_EXPORT int playerc_position3d_reset_odom(playerc_position3d_t *device);

/** @} */
/**************************************************************************/


/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_power power

The power proxy provides an interface through which battery levels can
be monitored.

@{
*/

/** @brief Power device data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** status bits. Bitwise-and with PLAYER_POWER_MASK_ values to see
      which fields are being set by the driver. */
  int valid;

  /** Battery charge (Volts). */
  double charge;

  /** Battery charge (percent full). */
  double percent;

  /** energy stored (Joules) */
  double joules;

  /** power currently being used (Watts). Negative numbers indicate
      charging. */
  double watts;

  /** charging flag. TRUE if charging, else FALSE */
  int charging;

} playerc_power_t;


/** @brief Create a power device proxy. */
PLAYERC_EXPORT playerc_power_t *playerc_power_create(playerc_client_t *client, int index);

/** @brief Destroy a power device proxy. */
PLAYERC_EXPORT void playerc_power_destroy(playerc_power_t *device);

/** @brief Subscribe to the power device. */
PLAYERC_EXPORT int playerc_power_subscribe(playerc_power_t *device, int access);

/** @brief Un-subscribe from the power device. */
PLAYERC_EXPORT int playerc_power_unsubscribe(playerc_power_t *device);


/** @} */
/**************************************************************************/



/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_ptz ptz

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

  /** The current pan and tilt status */
  int status;
} playerc_ptz_t;


/** @brief Create a ptz proxy. */
PLAYERC_EXPORT playerc_ptz_t *playerc_ptz_create(playerc_client_t *client, int index);

/** @brief Destroy a ptz proxy. */
PLAYERC_EXPORT void playerc_ptz_destroy(playerc_ptz_t *device);

/** @brief Subscribe to the ptz device. */
PLAYERC_EXPORT int playerc_ptz_subscribe(playerc_ptz_t *device, int access);

/** @brief Un-subscribe from the ptz device. */
PLAYERC_EXPORT int playerc_ptz_unsubscribe(playerc_ptz_t *device);

/** @brief Set the pan, tilt and zoom values.

@param device Pointer to proxy object.
@param pan Pan value, in radians; 0 = centered.
@param tilt Tilt value, in radians; 0 = level.
@param zoom Zoom value, in radians (corresponds to camera field of view).

*/
PLAYERC_EXPORT int playerc_ptz_set(playerc_ptz_t *device, double pan, double tilt, double zoom);

/** @brief Query the pan and tilt status.

@param device Pointer to proxy object.

*/
PLAYERC_EXPORT int playerc_ptz_query_status(playerc_ptz_t *device);

/** @brief Set the pan, tilt and zoom values (and speed)

@param device Pointer to proxy object.
@param pan Pan value, in radians; 0 = centered.
@param tilt Tilt value, in radians; 0 = level.
@param zoom Zoom value, in radians (corresponds to camera field of view).
@param panspeed Pan speed, in radians/sec.
@param tiltspeed Tilt speed, in radians/sec.

*/
PLAYERC_EXPORT int playerc_ptz_set_ws(playerc_ptz_t *device, double pan, double tilt, double zoom,
                       double panspeed, double tiltspeed);

/** @brief Change control mode (select velocity or position control)

  @param device Pointer to proxy object.
  @param mode Desired mode (@ref PLAYER_PTZ_VELOCITY_CONTROL or @ref PLAYER_PTZ_POSITION_CONTROL)

  @returns 0 on success, -1 on error, -2 on NACK.
*/
PLAYERC_EXPORT int playerc_ptz_set_control_mode(playerc_ptz_t *device, int mode);

/** @} */
/**************************************************************************/

/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_ranger ranger

The ranger proxy provides an interface to ranger sensor devices.

@{
*/

/** @brief Ranger proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** Number of individual elements in the device. */
  uint32_t element_count;

  /** Start angle of scans [rad]. May be unfilled. */
  double min_angle;
  /** End angle of scans [rad]. May be unfilled. */
  double max_angle;
  /** Scan resolution [rad]. May be unfilled. */
  double angular_res;
  /** Minimum range [m]. Values below this are typically errors and should
  usually not be treated as valid data. May be unfilled. */
  double min_range;
  /** Maximum range [m]. May be unfilled. */
  double max_range;
  /** Range resolution [m]. May be unfilled. */
  double range_res;
  /** Scanning frequency [Hz]. May be unfilled. */
  double frequency;

  /** Device geometry in the robot CS: pose gives the position and orientation,
      size gives the extent. These values are filled in by calling
      playerc_ranger_get_geom(), or from pose-stamped data. */
  player_pose3d_t device_pose;
  player_bbox3d_t device_size;
  /** Geometry of each individual element in the device (e.g. a single
      sonar sensor in a sonar array). These values are filled in by calling
      playerc_ranger_get_geom(), or from pose-stamped data. */
  player_pose3d_t *element_poses;
  player_bbox3d_t *element_sizes;

  /** Number of ranges in a scan. */
  uint32_t ranges_count;
  /** Range data [m]. */
  double *ranges;

  /** Number of intensities in a scan. */
  uint32_t intensities_count;
  /** Intensity data [m]. Note that this data may require setting of the
  suitable property on the driver to before it will be filled. Possible
  properties include intns_on for laser devices and volt_on for IR devices. */
  double *intensities;

  /** Number of scan bearings. */
  uint32_t bearings_count;
  /** Scan bearings in the XY plane [radians]. Note that for multi-element
  devices, these bearings will be from the pose of each element. */
  double *bearings;

  /** Number of scan points. */
  uint32_t points_count;
  /** Scan points (x, y, z). */
  player_point_3d_t *points;

} playerc_ranger_t;

/** @brief Create a ranger proxy. */
PLAYERC_EXPORT playerc_ranger_t *playerc_ranger_create(playerc_client_t *client, int index);

/** @brief Destroy a ranger proxy. */
PLAYERC_EXPORT void playerc_ranger_destroy(playerc_ranger_t *device);

/** @brief Subscribe to the ranger device. */
PLAYERC_EXPORT int playerc_ranger_subscribe(playerc_ranger_t *device, int access);

/** @brief Un-subscribe from the ranger device. */
PLAYERC_EXPORT int playerc_ranger_unsubscribe(playerc_ranger_t *device);

/** @brief Get the ranger geometry.

This writes the result into the proxy rather than returning it to the caller.
*/
PLAYERC_EXPORT int playerc_ranger_get_geom(playerc_ranger_t *device);

/** @brief Turn device power on or off.

@param device Pointer to ranger device.
@param value Set to TRUE to turn power on, FALSE to turn power off. */
PLAYERC_EXPORT int playerc_ranger_power_config(playerc_ranger_t *device, uint8_t value);

/** @brief Turn intensity data on or off.

@param device Pointer to ranger device.
@param value Set to TRUE to turn the data on, FALSE to turn the data off. */
PLAYERC_EXPORT int playerc_ranger_intns_config(playerc_ranger_t *device, uint8_t value);

/** @brief Set the ranger device's configuration. Not all values may be used.

@param device Pointer to ranger device.
@param min_angle Start angle of scans [rad].
@param max_angle End angle of scans [rad].
@param angular_res Scan resolution [rad].
@param min_range Minimum range[m].
@param max_range Maximum range [m].
@param range_res Range resolution [m].
@param frequency Scanning frequency [Hz]. */
PLAYERC_EXPORT int playerc_ranger_set_config(playerc_ranger_t *device, double min_angle,
                              double max_angle, double angular_res,
                              double min_range, double max_range,
                              double range_res, double frequency);

/** @brief Get the ranger device's configuration. Not all values may be filled.

@param device Pointer to ranger device.
@param min_angle Start angle of scans [rad].
@param max_angle End angle of scans [rad].
@param angular_res Scan resolution [rad].
@param min_range Minimum range [m].
@param max_range Maximum range [m].
@param range_res Range resolution [m].
@param frequency Scanning frequency [Hz]. */
PLAYERC_EXPORT int playerc_ranger_get_config(playerc_ranger_t *device, double *min_angle,
                              double *max_angle, double *angular_res,
                              double *min_range, double *max_range,
                              double *range_res, double *frequency);

/** @} */
/**************************************************************************/

/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_sonar sonar

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
  player_pose3d_t *poses;

  /** Number of points in the scan. */
  int scan_count;

  /** Scan data: range (m). */
  double *scan;

} playerc_sonar_t;


/** @brief Create a sonar proxy. */
PLAYERC_EXPORT playerc_sonar_t *playerc_sonar_create(playerc_client_t *client, int index);

/** @brief Destroy a sonar proxy. */
PLAYERC_EXPORT void playerc_sonar_destroy(playerc_sonar_t *device);

/** @brief Subscribe to the sonar device. */
PLAYERC_EXPORT int playerc_sonar_subscribe(playerc_sonar_t *device, int access);

/** @brief Un-subscribe from the sonar device. */
PLAYERC_EXPORT int playerc_sonar_unsubscribe(playerc_sonar_t *device);

/** @brief Get the sonar geometry.

This writes the result into the proxy
rather than returning it to the caller.
*/
PLAYERC_EXPORT int playerc_sonar_get_geom(playerc_sonar_t *device);

/** @} */
/**************************************************************************/

/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_wifi wifi

The wifi proxy is used to query the state of a wireless network.  It
returns information such as the link quality and signal strength of
access points or of other wireless NIC's on an ad-hoc network.

@{
*/

/** @brief Individual link info. */
typedef struct
{
  /** Mac accress. */
  uint8_t mac[32];

  /** IP address. */
  uint8_t ip[32];

  /** ESSID id */
  uint8_t essid[32];

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
  playerc_wifi_link_t *links;
  int link_count;
  char ip[32];
} playerc_wifi_t;


/** @brief Create a wifi proxy. */
PLAYERC_EXPORT playerc_wifi_t *playerc_wifi_create(playerc_client_t *client, int index);

/** @brief Destroy a wifi proxy. */
PLAYERC_EXPORT void playerc_wifi_destroy(playerc_wifi_t *device);

/** @brief Subscribe to the wifi device. */
PLAYERC_EXPORT int playerc_wifi_subscribe(playerc_wifi_t *device, int access);

/** @brief Un-subscribe from the wifi device. */
PLAYERC_EXPORT int playerc_wifi_unsubscribe(playerc_wifi_t *device);

/** @brief Get link state. */
PLAYERC_EXPORT playerc_wifi_link_t *playerc_wifi_get_link(playerc_wifi_t *device, int link);


/** @} */
/**************************************************************************/

/***************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_simulation simulation

The simulation proxy is used to interact with objects in a simulation.

@todo write playerc_proxy_simulation description

@{
*/

/** @brief Simulation device proxy. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

} playerc_simulation_t;


/** @brief Create a new simulation proxy */
PLAYERC_EXPORT playerc_simulation_t *playerc_simulation_create(playerc_client_t *client, int index);

/** @brief Destroy a simulation proxy */
PLAYERC_EXPORT void playerc_simulation_destroy(playerc_simulation_t *device);

/** @brief Subscribe to the simulation device */
PLAYERC_EXPORT int playerc_simulation_subscribe(playerc_simulation_t *device, int access);

/** @brief Un-subscribe from the simulation device */
PLAYERC_EXPORT int playerc_simulation_unsubscribe(playerc_simulation_t *device);

/** @brief Set the 2D pose of a named simulation object */
PLAYERC_EXPORT int playerc_simulation_set_pose2d(playerc_simulation_t *device, char* name,
                                  double gx, double gy, double ga);

/** @brief Get the 2D pose of a named simulation object */
PLAYERC_EXPORT int playerc_simulation_get_pose2d(playerc_simulation_t *device, char* identifier,
				  double *x, double *y, double *a);

/** @brief Set the 3D pose of a named simulation object */
PLAYERC_EXPORT int playerc_simulation_set_pose3d(playerc_simulation_t *device, char* name,
				  double gx, double gy, double gz,
				  double groll, double gpitch, double gyaw);

/** @brief Get the 3D pose of a named simulation object */
PLAYERC_EXPORT int playerc_simulation_get_pose3d(playerc_simulation_t *device, char* identifier,
				  double *x, double *y, double *z,
				  double *roll, double *pitch, double *yaw, double *time);

/** @brief Set a property value */
PLAYERC_EXPORT int playerc_simulation_set_property(playerc_simulation_t *device,
                                    char* name,
                                    char* property,
                                    void* value,
				    size_t value_len);

/** @brief Get a property value */
PLAYERC_EXPORT int playerc_simulation_get_property(playerc_simulation_t *device,
                                    char* name,
                                    char* property,
                                    void* value,
                                    size_t value_len);

/** @brief  pause / unpause the simulation */
PLAYERC_EXPORT int playerc_simulation_pause(playerc_simulation_t *device );

/** @brief  reset the simulation state  */
PLAYERC_EXPORT int playerc_simulation_reset(playerc_simulation_t *device );

/** @brief make the simulation save the status/world */
PLAYERC_EXPORT int playerc_simulation_save(playerc_simulation_t *device );


/** @} */
/***************************************************************************/


/**************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_speech speech

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
PLAYERC_EXPORT playerc_speech_t *playerc_speech_create(playerc_client_t *client, int index);

/** Destroy a speech proxy. */
PLAYERC_EXPORT void playerc_speech_destroy(playerc_speech_t *device);

/** Subscribe to the speech device. */
PLAYERC_EXPORT int playerc_speech_subscribe(playerc_speech_t *device, int access);

/** Un-subscribe from the speech device. */
PLAYERC_EXPORT int playerc_speech_unsubscribe(playerc_speech_t *device);

/** Set the output for the speech device. */
PLAYERC_EXPORT int playerc_speech_say (playerc_speech_t *device, char *);


/** @} */
/***************************************************************************/

/**************************************************************************/
/** @ingroup playerc_proxies
 * @defgroup playerc_proxy_speech_recognition speech recognition

The speech recognition proxy provides an interface to a speech recognition system.

@{
*/

/** Speech recognition proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  char *rawText;
  /* Just estimating that no more than 20 words will be spoken between updates.
   Assuming that the longest word is <= 30 characters.*/
  char **words;
  int wordCount;
} playerc_speechrecognition_t;


/** Create a speech recognition proxy. */
PLAYERC_EXPORT playerc_speechrecognition_t *playerc_speechrecognition_create(playerc_client_t *client, int index);

/** Destroy a speech recognition proxy. */
PLAYERC_EXPORT void playerc_speechrecognition_destroy(playerc_speechrecognition_t *device);

/** Subscribe to the speech recognition device. */
PLAYERC_EXPORT int playerc_speechrecognition_subscribe(playerc_speechrecognition_t *device, int access);

/** Un-subscribe from the speech recognition device */
PLAYERC_EXPORT int playerc_speechrecognition_unsubscribe(playerc_speechrecognition_t *device);

/** @} */
/***************************************************************************/

/**************************************************************************/
/** @ingroup playerc_proxies
    @defgroup playerc_proxy_rfid rfid

The rfid proxy provides an interface to a RFID reader.

@{
*/

/** @brief Structure describing a single RFID tag. */
typedef struct
{
    /** Tag type. */
    uint32_t type;
    /** GUID count. */
    uint32_t guid_count;
    /** The Globally Unique IDentifier (GUID) of the tag. */
    uint8_t *guid;
}  playerc_rfidtag_t;

/** @brief RFID proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** The number of RFID tags found. */
  uint16_t tags_count;

  /** The list of RFID tags. */
  playerc_rfidtag_t *tags;
} playerc_rfid_t;


/** @brief Create a rfid proxy. */
PLAYERC_EXPORT playerc_rfid_t *playerc_rfid_create(playerc_client_t *client, int index);

/** @brief Destroy a rfid proxy. */
PLAYERC_EXPORT void playerc_rfid_destroy(playerc_rfid_t *device);

/** @brief Subscribe to the rfid device. */
PLAYERC_EXPORT int playerc_rfid_subscribe(playerc_rfid_t *device, int access);

/** @brief Un-subscribe from the rfid device. */
PLAYERC_EXPORT int playerc_rfid_unsubscribe(playerc_rfid_t *device);

/** @} */
/***************************************************************************/

/**************************************************************************/
/** @ingroup playerc_proxies
    @defgroup playerc_proxy_pointcloud3d pointcloud3d

The pointcloud3d proxy provides an interface to a pointcloud3d device.

@{
*/

/** @brief Structure describing a single 3D pointcloud element. */
typedef player_pointcloud3d_element_t playerc_pointcloud3d_element_t;

/** @brief pointcloud3d proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /** The number of 3D pointcloud elementS found. */
  uint16_t points_count;

  /** The list of 3D pointcloud elements. */
  playerc_pointcloud3d_element_t *points;
} playerc_pointcloud3d_t;


/** @brief Create a pointcloud3d proxy. */
PLAYERC_EXPORT playerc_pointcloud3d_t *playerc_pointcloud3d_create (playerc_client_t *client, int index);

/** @brief Destroy a pointcloud3d proxy. */
PLAYERC_EXPORT void playerc_pointcloud3d_destroy (playerc_pointcloud3d_t *device);

/** @brief Subscribe to the pointcloud3d device. */
PLAYERC_EXPORT int playerc_pointcloud3d_subscribe (playerc_pointcloud3d_t *device, int access);

/** @brief Un-subscribe from the pointcloud3d device. */
PLAYERC_EXPORT int playerc_pointcloud3d_unsubscribe (playerc_pointcloud3d_t *device);

/** @} */
/***************************************************************************/

/**************************************************************************/
/** @ingroup playerc_proxies
    @defgroup playerc_proxy_stereo stereo

The stereo proxy provides an interface to a stereo device.

@{
*/
/** @brief Structure describing a single 3D pointcloud element. */
typedef player_pointcloud3d_stereo_element_t playerc_pointcloud3d_stereo_element_t;

/** @brief stereo proxy data. */
typedef struct
{
  /** Device info; must be at the start of all device structures. */
  playerc_device_t info;

  /* Left channel image */
  playerc_camera_t left_channel;
  /* Right channel image */
  playerc_camera_t right_channel;

  /* Disparity image */
  playerc_camera_t disparity;
  
  /* 3-D stereo point cloud */
  uint32_t points_count;
  playerc_pointcloud3d_stereo_element_t *points;
//  player_pointcloud3d_data_t pointcloud;
} playerc_stereo_t;


/** @brief Create a stereo proxy. */
PLAYERC_EXPORT playerc_stereo_t *playerc_stereo_create (playerc_client_t *client, int index);

/** @brief Destroy a stereo proxy. */
PLAYERC_EXPORT void playerc_stereo_destroy (playerc_stereo_t *device);

/** @brief Subscribe to the stereo device. */
PLAYERC_EXPORT int playerc_stereo_subscribe (playerc_stereo_t *device, int access);

/** @brief Un-subscribe from the stereo device. */
PLAYERC_EXPORT int playerc_stereo_unsubscribe (playerc_stereo_t *device);

/** @} */
/***************************************************************************/

/**************************************************************************/
/** @ingroup playerc_proxies
    @defgroup playerc_proxy_imu imu

The imu proxy provides an interface to an Inertial Measurement Unit.

@{
*/

/** @brief IMU proxy state data. */
typedef struct
{
    /** Device info; must be at the start of all device structures.         */
    playerc_device_t info;

    /** The complete pose of the IMU in 3D coordinates + angles             */
    player_pose3d_t pose;
	player_pose3d_t vel;
	player_pose3d_t acc;

    /** Calibrated IMU data (accel, gyro, magnetometer)                     */
    player_imu_data_calib_t calib_data;

    /** Orientation data as quaternions                                     */
    float q0, q1, q2, q3;
} playerc_imu_t;

/** @brief Create a imu proxy. */
PLAYERC_EXPORT playerc_imu_t *playerc_imu_create (playerc_client_t *client, int index);

/** @brief Destroy a imu proxy. */
PLAYERC_EXPORT void playerc_imu_destroy (playerc_imu_t *device);

/** @brief Subscribe to the imu device. */
PLAYERC_EXPORT int playerc_imu_subscribe (playerc_imu_t *device, int access);

/** @brief Un-subscribe from the imu device. */
PLAYERC_EXPORT int playerc_imu_unsubscribe (playerc_imu_t *device);

/** Change the data type to one of the predefined data structures. */
PLAYERC_EXPORT int playerc_imu_datatype (playerc_imu_t *device, int value);

/**  Reset orientation. */
PLAYERC_EXPORT int playerc_imu_reset_orientation (playerc_imu_t *device, int value);

/**  Reset euler orientation. */
PLAYERC_EXPORT int playerc_imu_reset_euler(playerc_imu_t *device, float roll, float pitch, float yaw);

/** @} */
/***************************************************************************/

/**************************************************************************/
/** @ingroup playerc_proxies
    @defgroup playerc_proxy_wsn wsn

The wsn proxy provides an interface to a Wireless Sensor Network.

@{
*/

/** Note: the structure describing the WSN node's data packet is declared in
          Player. */

/** @brief WSN proxy data. */
typedef struct
{
    /** Device info; must be at the start of all device structures.         */
    playerc_device_t info;

    /** The type of WSN node.                                               */
    uint32_t node_type;
    /** The ID of the WSN node.                                             */
    uint32_t node_id;
    /** The ID of the WSN node's parent (if existing).                      */
    uint32_t node_parent_id;
    /** The WSN node's data packet.                                         */
    player_wsn_node_data_t data_packet;
} playerc_wsn_t;


/** @brief Create a wsn proxy. */
PLAYERC_EXPORT playerc_wsn_t *playerc_wsn_create(playerc_client_t *client, int index);

/** @brief Destroy a wsn proxy. */
PLAYERC_EXPORT void playerc_wsn_destroy(playerc_wsn_t *device);

/** @brief Subscribe to the wsn device. */
PLAYERC_EXPORT int playerc_wsn_subscribe(playerc_wsn_t *device, int access);

/** @brief Un-subscribe from the wsn device. */
PLAYERC_EXPORT int playerc_wsn_unsubscribe(playerc_wsn_t *device);

/** Set the device state. */
PLAYERC_EXPORT int playerc_wsn_set_devstate(playerc_wsn_t *device, int node_id,
                             int group_id, int devnr, int state);

/** Put the node in sleep mode (0) or wake it up (1). */
PLAYERC_EXPORT int playerc_wsn_power(playerc_wsn_t *device, int node_id, int group_id,
                      int value);

/** Change the data type to RAW or converted engineering units. */
PLAYERC_EXPORT int playerc_wsn_datatype(playerc_wsn_t *device, int value);

/** Change data delivery frequency. */
PLAYERC_EXPORT int playerc_wsn_datafreq(playerc_wsn_t *device, int node_id, int group_id,
                         double frequency);

/** @} */
/***************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
