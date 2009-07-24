// Implementation of mymodule, a module that does laser-based obstacle
// avoidance

// My message-processing function
int mymodule_processmessage(player_io_t *io, player_msghdr_t* hdr, void* data);

// My configuration function
int mymodule_configure(player_io_t* io, player_module_options_t* opt);

// My run function
void mymodule_run(player_io_t* io);

int
main(int argc, char** argv)
{
  player_io_t* io;
  player_tcp_io_t tcpio;
  player_devaddr_t addr;
  player_module_options_t* opt = NULL;

  // Initialize player
  io = player_init(argc, argv);

  // Parse config file to create option table
  player_parse_config_file(&opt, argv[1]);

  // Configure module
  mymodule_configure(opt);

  // Construct my address
  addr.host = 0; // 0 means localhost
  addr.robot = 6665;
  addr.interf = PLAYER_POSITION2D_CODE;
  addr.index = 0;

  // Add my interface (could add more in the same way)
  player_add_interface(io,&addr);

  // In this case we'll use TCP, so do the necessary transport-specific
  // binding
  player_tcp_add_io(&tcpio, io);
  player_tcp_bind(&tcpio);

  // Start up my module
  mymodule_run(io);

  return(0);
}

int 
mymodule_configure(player_io_t* io, player_module_options_t* opt)
{
  // Step through table, storing options to where?
}

int 
mymodule_processmessage(player_io_t *io, player_msghdr_t* hdr, void* data)
{
  // If no ack is requested, just cache it.  New messages overwrite old
  // ones, so if I want to see *every* message, I must process them here,
  // instead of caching them like this.
  if(!hdr->ack)
  {
    player_cache_insert(io->message_cache, hdr, data);
    return(0);
  }
  else
  {
    // An ack was requested.  What kind of message is it?
    
    // Is it a message to me to set odometry?
    if(player_match_message_to(io, hdr, 
                               io->addrs[0].host,
                               io->addrs[0].robot,
                               io->addrs[0].interf,
                               io->addrs[0].index,
                               PLAYER_POSITION2D_REQ_SET_ODOM))
    {
      // Try to set odometry, return 0 on success, -1 on failure
      if(set_odometry() == 0)
        return(0);
      else
        return(-1);
    }
    // Is it a message to me to enable motor power?
    else if(player_match_message_to(io, hdr, 
                                    io->addrs[0].host,
                                    io->addrs[0].robot,
                                    io->addrs[0].interf,
                                    io->addrs[0].index,
                                    PLAYER_POSITION2D_REQ_MOTOR_POWER))
    {
      // Forward it to the underlying device, somehow...
    }
    else
    {
      // don't know this message
      return(-1);
    }
  }
}

// Run, with me in control of the event loop
void
mymodule_run(player_io_t* io)
{
  for(;;)
  {
    // Read incoming messages, with short timeout
    player_tcp_readmessages(io->tcp_io,10);
   
    // Call my update method
    mymodule_update(io);

    // Write outgoing messages
    player_tcp_writemessages(io->tcp_io);
  }
}

// Execute one loop of my module
void
mymodule_update(player_io_t* io)
{
  // Read from my inqueue, processing each message:
  //
  player_processmessages(io, mymodule_processmessage);

  // Alternatively, I could just use the default processor, which puts
  // incoming messages into a cache:
  //
  //player_processmessages(io, player_processmessage_default);

  // Do some module-specific stuff here

  // Get latest laser scan, from anybody, to anybody, and delete it from 
  // the cache
  player_laser_data_t scan;
  if(player_get_laser_scan(io, NULL, NULL, PLAYER_LASER_DATA_SCAN,
                           &scan, 1))
  {
    // handle data in 'scan'
  }

  // Construct an outgoing message (fill it in)
  player_position2d_data_t msg;
  // Publish the message, to all interested parties, from my first address
  player_broadcast_position2d_pose(io, 
                                   io->addrs[0],
                                   &msg);
}

