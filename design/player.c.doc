typedef struct player_devaddr
{
  /** The "host" on which the device resides.  Transport-dependent. */
  uint32_t host;
  /** The "robot" or device collection in which the device resides.
      Transport-dependent */
  uint32_t robot;
  /** The interface provided by the device; must be one of PLAYER_*_CODE */
  uint16_t interf;
  /** Which device of that interface */
  uint16_t index;
} player_devaddr_t;

// Generic message header.
typedef struct player_msghdr
{
  /** Origin address */
  player_devaddr_t from;
  /** Destination address */
  player_devaddr_t to;
  /** Message type; interface-specific */
  uint8_t type;
  /** Time associated with message contents (seconds since epoch) */
  double timestamp;
  /** For keeping track of associated messages.  Transport-specific. */
  uint32_t seq;
  /** Whether an acknowledgement is requeested */
  uint8_t ack;
  /** Size in bytes of the payload to follow */
  uint32_t size;
} player_msghdr_t;

// An acknowledgement message
typedef struct player_player_ack
{
  uint8_t value;
} player_player_ack_t;

typedef struct player_msgqueue
{
  // A linked list gets implemented here, using glib
} player_msgqueue_t;

typedef struct player_cache
{
  // A dictionary of messages, keyed by message signature, goes here
  // (use glib).  New messages overwrite old ones with the same signature.
} player_cache_t;

// Insert data, with signature hdr, into the cache
int player_cache_insert(player_cache_t* cache, 
                        player_msghdr_t* hdr,
                        void* data);
// Retrieve message with signature hdr, and put it in data.
int player_cache_retrieve(player_cache_t* cache, 
                          player_msghdr_t* hdr,
                          void* data);

// Similar to player_cache_retrieve, but deletes message from the cache 
// after retrieval
int player_cache_remove(player_cache_t* cache,
                        player_msghdr_t* hdr,
                        void* data);

// Forward declaration of TCP manager;
struct player_tcp_io;

// A player_io_t object contains all the state necessary for a single
// module.  A single module may have multiple addresses.
typedef struct player_io
{
  // A list of my addresses
  player_devaddr_t* addrs;
  // My input queue
  player_msgqueue_t* inqueue;
  // My output queue
  player_msgqueue_t* outqueue;
  // A cache of incoming message
  player_cache_t* message_cache;
  // Which transport are we using?
  uint8_t transport_id;
  // TCP manager object, in case we're using TCP
  player_tcp_io_t* tcp_io;
  // Other local state goes here
} player_io_t; 

typedef struct player_module_options
{
  // A dictionary/hash-table of module name-value options goes here, 
  // using glib
} player_module_options_t;

// initialize player. returns some local state.
player_io_t* player_init(int argc, char* argv);

// clean up
int player_fini(player_io_t* io);

// Parse a config file to build an option table
//
// If (*opt) is NULL, a new table is created.
int
player_parse_config_file(player_module_options_t** opt,
                         const char* fname);

// Declare an interface that I'll support
int player_add_interface(player_io_t* io, player_devaddr_t* addr);

// A message structure
typedef struct player_$(INTERFACE)_$(TYPE)
{
  // ...
} player_$(INTERFACE)_$(TYPE)_t;

// connect to a device somewhere out there. returns a handle to the device.
int
player_subscribe_$(INTERFACE)_$(TYPE)(player_io_t* io, 
                                      player_devaddr_t* addr)
{
  // Contact the device.  Now, this should be done in a transport-specific
  // way.
  if(io->transport == PLAYER_TRANSPORT_TCP)
  {
    // Do a TCP-specific subscription
  }
}

// disconnect from a device somewhere out there.
int
player_unsubscribe_$(INTERFACE)_$(TYPE)(player_io_t* io, 
                                        player_devaddr_t* addr);
{
  // Contact the device.  Now, this should be done in a transport-specific
  // way.
  if(io->transport == PLAYER_TRANSPORT_TCP)
  {
    // Do a TCP-specific unsubscription
  }
}

// Publish a message.
//
//   io: Local io handle
//   from: Address of originating device.  Needed to distinguish between
//         multiple interfaces.  If NULL, 
//   to: Address of destination device.  If NULL, then the message is sent
//       to all subscribers.
//   data: The message itself.
//   ack: If 1, then an acknowledgement from the destination device is
//        requested.  Cannot be 1 if 'to' is NULL (i.e., cannot request
//        acknowledgement for a multicast message).
//   ack_timeout: how long to wait for acknowledgement; 0 for no waiting,
//                -1 for indefinite timeout
//
// Returns: 0 on success, non-zero on error.  If ack is 0, then the return
// value only indicates whether the message was sent.  If ack is 1, then
// the return value is a response from the destination device (may need a
// special value to distinguish timeout from device-generated error).
int 
player_publish_$(INTERFACE)_$(TYPE)(player_io_t* io, 
                                     player_devaddr_t* from, 
                                     player_devaddr_t* to, 
                                     player_$(INTERFACE)_$(TYPE)_t* data,
                                     int ack,
                                     int ack_timeout);

// Short form of publishing, for convenience
int player_broadcast_$(INTERFACE)_$(TYPE)(player_io_t* io,
                                          player_devaddr_t* from,
                                          player_$(INTERFACE)_$(TYPE)_t* data)
{
  return(player_publish_$(INTERFACE)_$(TYPE)(io, from, NULL, data, 0, -1));
}

// A utility function to retrieve a certain type of message from the local 
// cache.  The 'remove' argument indicates whether the message should be
// removed from the cache.
int player_get_$(INTERFACE)_$(TYPE)(player_io_t* io,
                                    player_devaddr_t* from,
                                    player_devaddr_t* to,
                                    player_$(INTERFACE)_$(TYPE)_t* data,
                                    int remove)
{
  player_msghdr_t hdr;
  hdr.from = from;
  hdr.to = to;
  hdr.type = $(TYPE);
  if(remove)
    return(player_cache_remove(io->cache, &hdr, data));
  else
    return(player_cache_retrieve(io->cache, &hdr, data));
}

// A utility function to wait for a specific message to arrive on my
// inqueue.   The message is popped (?)
int player_waitfor_$(INTERFACE)_$(THING)(player_io_t* io, 
                                         player_devaddr_t* from, 
                                         player_devaddr_t* to, 
                                         player_$(INTERFACE)_$(TYPE)_t* data);

// A utility function to send an acknowledgement
int player_send_ack(player_io_t* io,
                    player_devaddr_t* from,
                    player_devaddr_t* to,
                    uint8_t value)
{
  player_player_ack_t ack;
  ack.value = value;
  return(player_publish_player_ack(io, from, to, &ack, 0, 0));
}

// A utility function to match message signature, using from address.  -1
// for any field means don't care
int player_match_message_from(player_io_t* io,
                              player_msghdr_t* hdr,
                              int host,
                              int robot,
                              int interf,
                              int index,
                              int type);

// A utility function to match message signature, using to address.  -1
// for any field means don't care
int player_match_message_to(player_io_t* io,
                            player_msghdr_t* hdr,
                            int host,
                            int robot,
                            int interf,
                            int index,
                            int type);

// Other message-match functions could be provided....

// Signature for a message-processing function
typedef int 
(*player_processmessage_fn_t)(player_io_t *io,
                              player_msghdr_t* hdr,
                              void* data);

// A default message processer, which simply caches the messages for later
// perusal
int
player_processmessage_default(player_io_t* io,
                              player_msghdr_t* hdr,
                              void* data)
{
  player_cache_insert(io->message_cache, hdr, data);

  // If an ack was requested, send a success code (might not be the right
  // behavior, but what other option is there if the module is not
  // personally processing each messagee?)
  if(hdr->ack)
    player_send_ack(io, hdr->to, hdr->from, 0);
}

// A utility function to pop and process messages, one by one
int player_processmessages(player_io_t* io, 
                           player_processmessage_fn_t process_fn)
{
  int retval;
  while(!player_queue_empty(io->inqueue))
  {
    // Pop a message off the queue
    player_msghdr_t* hdr;
    void* data;
    player_queue_pop(&hdr, &data, io->inqueue);

    // Process the message and catch the return value
    retval = (process_fn)(io,hdr,data);

    // If an ack was requested, send it
    if(hdr->ack)
      player_send_ack(io, hdr->to, hdr->from, retval);
  }
}


/////////////////////////////////////////////////////////////////////////
//
// TCP-specific functionality
//
typedef struct player_tcp_io
{
  // List of player_io objects that I'm managing (one per module)
  player_io_t** ios;
  // TCP state, like ports and sockets that I'm managing
} player_tcp_io_t;

// Add an io object (i.e., a module) to this TCP manager
int player_tcp_add_io(player_tcp_io_t* tcpio, player_io_t* io);

// Bind to one or more TCP ports, as indicated by the modules embedded
// in 'tcpio'.
int player_tcp_bind(player_tcp_io_t* io);

// Read incoming messages from bound TCP sockets and push them onto
// appropriate tcpio->io->inqueue.
int player_tcp_readmessages(player_tcp_io_t* tcpio, int timeout);

// Pop outgoing messages from all tcpio->io->outqueue and write them out 
// on TCP sockets
int player_tcp_writemessages(player_tcp_io_t* tcpio);
/////////////////////////////////////////////////////////////////////////





