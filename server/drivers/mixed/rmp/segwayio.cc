#include <errno.h>
#include <math.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/time.h>

#include "segwayio.h"
SegwayIO * SegwayIO::instance(NULL);

/* Takes a CAN packet from the RMP and parses it into a
 * rmp_frame_t struct.  sets the ready bitfield 
 * depending on which CAN packet we have.  when
 * ready == 0x1F, then we have gotten 5 packets, so everything
 * is filled in.
 *
 * returns: 
 */
inline void
rmp_frame_t::AddPacket(const can_packet_t &pkt)
{
  bool known = true;

  switch(pkt.id) {
  case RMP_CAN_ID_MSG1:
    battery = pkt.GetSlot(2);
    break;
    
  case RMP_CAN_ID_MSG2:
    pitch = pkt.GetSlot(0);
    pitch_dot = pkt.GetSlot(1);
    roll =  pkt.GetSlot(2);
    roll_dot =  pkt.GetSlot(3);
    break;
    
  case RMP_CAN_ID_MSG3:
    left_dot = (int16_t) pkt.GetSlot(0);
    right_dot = (int16_t) pkt.GetSlot(1);
    yaw_dot = pkt.GetSlot(2);
    frames = pkt.GetSlot(3);
    break;

  case RMP_CAN_ID_MSG4:
    left = (int32_t)(((uint32_t)pkt.GetSlot(1) << 16) | 
		     (uint32_t)pkt.GetSlot(0));
    right = (int32_t)(((uint32_t)pkt.GetSlot(3) << 16) | 
		     (uint32_t)pkt.GetSlot(2));
    break;
    
  case RMP_CAN_ID_MSG5:
    foreaft = (int32_t) (((uint32_t)pkt.GetSlot(1) << 16) | 
			(uint32_t)pkt.GetSlot(0));
    yaw = (int32_t) (((uint32_t)pkt.GetSlot(3) << 16) | 
		      (uint32_t)pkt.GetSlot(2));
    break;
  default:
    known = false;
    break;
  }

  // now set the ready flags
  if (known) {
    ready |= (1 << (pkt.id & 0xF));
  }
}

/* create an instance of SegwayIO, enforces only one
 * instantiation (since only one instance may access bus)
 *
 * returns: pointer to SegwayIO object.
 */
SegwayIO *
SegwayIO::Instance()
{
  if (!instance) {
    instance = new SegwayIO();
  }

  return instance;
}

SegwayIO::SegwayIO() 
{
  canio = new DualCANIO();
  canioInit = false;
  canioShutdown = true;

  usageCount = 0;

  pthread_mutex_init(&latestData_mutex, NULL);
  pthread_mutex_init(&command_queue_mutex, NULL);
  pthread_mutex_init(&trans_command_mutex, NULL);
  pthread_mutex_init(&rot_command_mutex, NULL);

  latestData.ready = 0x1F;
}

SegwayIO::~SegwayIO()
{
  delete canio;
}

/* Initializes the CAN bus and starts the read/write thread
 *
 * returns: 0 on success, negative otherwise
 */
int
SegwayIO::Init()
{
  if (!canioInit) {
    // start the CAN at 500 kpbs
    if (canio->Init(BAUD_500K) < 0) {
      fprintf(stderr, "SEGWAYIO: error on CAN Init\n");
      return -1;
    }
    
    // start the read/write thread...
    if (pthread_create(&read_thread, NULL, &DummyReadLoop, this)) {
      fprintf(stderr, "SEGWAYIO: error creating read/write thread.\n");
      return -1;
    }

    // start the read/write thread...
    if (pthread_create(&write_thread, NULL, &DummyWriteLoop, this)) {
      fprintf(stderr, "SEGWAYIO: error creating read/write thread.\n");
      return -1;
    }

    canioInit = true;
    canioShutdown = false;
  }

  usageCount++;

  return 0;
}

/* Stops the read/write thread
 *
 * returns: 0 on success, negative else
 */
int
SegwayIO::Shutdown()
{
  void *unused;

  usageCount--;

  // shutdown if started and no one else is using...
  if (!canioShutdown && !usageCount) {
    pthread_cancel(read_thread);
    pthread_cancel(write_thread);

    if (pthread_join(read_thread, &unused)) {
      perror("SegwayIO: Shutdown:pthread_join(read)");
      return -1;
    }

    if (pthread_join(write_thread, &unused)) {
      perror("SegwayIO: Shutdown:pthread_join(write)");
      return -1;
    }

    // shutdown the CAN
    canio->Shutdown();

    canioShutdown = true;
    canioInit = false;
  }

  return 0;
}
		 
/* Calls our real readwrite loop
 *
 * returns: 
 */
void *
SegwayIO::DummyReadLoop(void *p)
{
  ((SegwayIO *)p)->ReadLoop();

  pthread_exit(NULL);
}

void *
SegwayIO::DummyWriteLoop(void *p)
{
  ((SegwayIO *)p)->WriteLoop();

  pthread_exit(NULL);
}

/* performs the read loop.  reads packets from the CAN bus and
 * creates player data structures
 * 
 *
 * returns: 
 */
void 
SegwayIO::ReadLoop()
{
  can_packet_t pkt;
  int channel = 0;
  int ret;
  rmp_frame_t data_frame[2];
  int loopmillis;

  data_frame[0].ready = 0;
  data_frame[1].ready = 0;

  struct timeval curr, last;

  gettimeofday(&last, NULL);

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

  while (1) {

    gettimeofday(&curr, NULL);

    loopmillis = ((curr.tv_sec - last.tv_sec)*1000 +
		  (curr.tv_usec - last.tv_usec)/1000);

    if (loopmillis < RMP_READ_WRITE_PERIOD) {
      continue;
    }
    
    last = curr;

    // look for cancellation requests
    pthread_testcancel();

    // read both channels til they have no packets on them
    for (channel = 0; channel < 2; channel++) {
      // read from channel 0 while we have a packet
      while ((ret = canio->ReadPacket(&pkt, channel)) > 0) {
	// then we got a new packet...

	//	printf("SEGWAYIO: pkt: %s\n", pkt.toString());
	// only take data from channel 0
	if (channel == 0) {
	  data_frame[channel].AddPacket(pkt);
	  
	  // if data has been filled in, then let's make it the latest data 
	  // available to player...
	  if (data_frame[channel].IsReady()) {
	    pthread_mutex_lock(&latestData_mutex);
	    latestData = data_frame[channel];
	    pthread_mutex_unlock(&latestData_mutex);
	    
	    data_frame[channel].ready = 0;
	  }
	}
      }
      if (ret < 0) {
	fprintf(stderr, "SEGWAYIO: error (%d) reading packet on channel %d\n",
		ret, channel);
      }
    }
  }
}



void
SegwayIO::WriteLoop()
{
  can_packet_t pkt;
  rmp_frame_t data_frame[2];
  int loopmillis;

  data_frame[0].ready = 0;
  data_frame[1].ready = 0;

  struct timeval curr, last;

  gettimeofday(&last, NULL);

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

  while (1) {

    pthread_testcancel();

    gettimeofday(&curr, NULL);

    loopmillis = ((curr.tv_sec - last.tv_sec)*1000 +
		  (curr.tv_usec - last.tv_usec)/1000);


    if (loopmillis < RMP_READ_WRITE_PERIOD) {
      continue;
    }

    last = curr;

    // now we want to send the latest command...

    // want to write a packet at no more than 100 Hz, so 
    // just do one each time through the loop
    if (!command_queue.empty()) {
      if (pthread_mutex_trylock(&command_queue_mutex) == EBUSY) {
	// lock is busy, so make a blank pkt to send this time
	pkt = can_packet_t();
	pkt.id = RMP_CAN_ID_COMMAND;
	pkt.PutSlot(2, (uint16_t)RMP_CAN_CMD_NONE);
      } else {
	if (!command_queue.empty()) {
	  pkt = command_queue.front();
	  command_queue.pop();
	} else {
	  pkt = can_packet_t();
	  pkt.id = RMP_CAN_ID_COMMAND;
	  pkt.PutSlot(2, (uint16_t)RMP_CAN_CMD_NONE);
	}
	
	pthread_mutex_unlock(&command_queue_mutex);
	// now we have a new command...  these are just going to be 
	// "status commands" ie, slots 2 & 3 in the message
	// the velocities are in trans_command and rot_command
	
      // this way we can send a command and keep the velocity the same
      }
    } else {
      // no status command, so make a blank one
      pkt = can_packet_t();
      pkt.id = RMP_CAN_ID_COMMAND;
      pkt.PutSlot(2, (uint16_t)RMP_CAN_CMD_NONE);
    }

    // now pkt is filled with either a status command
    // or blank, awaiting velocity commands

    // fill in the latest rot & trans velocities
    pthread_mutex_lock(&trans_command_mutex);
    pkt.PutSlot(0, (uint16_t)trans_command);
    pthread_mutex_unlock(&trans_command_mutex);
    
    pthread_mutex_lock(&rot_command_mutex);
    pkt.PutSlot(1, (uint16_t)rot_command);
    pthread_mutex_unlock(&rot_command_mutex);

    /*
    if (pkt.GetSlot(0) != 0 ||
	pkt.GetSlot(1) != 0 ||
	pkt.GetSlot(2) != 0) {
      printf("SEGWAYIO: %d ms WRITE: pkt: %s\n", loopmillis,
	     pkt.toString());
    }
    */
    //    printf("SEGWAYIO: %d ms WRITE: %s\n", loopmillis, pkt.toString());

    // write it out!
    canio->WritePacket(pkt);
  }
}

  

/* Marshals the feedback info from the RMP into a player
 * position data format, in a network ready format too
 *
 * returns: 
 */
void
SegwayIO::GetData(player_position_data_t *data)
{
  rmp_frame_t rmp_data;

  // copy the RMP data from the shared struct
  pthread_mutex_lock(&latestData_mutex);
  rmp_data = latestData;
  pthread_mutex_unlock(&latestData_mutex);

  // should alway be totally filled out by the time
  // we get it...
  assert(rmp_data.IsReady());

  // xpos is fore/aft integrated position?
  // change from counts to mm
  data->xpos = htonl( (uint32_t) rint((double)rmp_data.foreaft / 
  				     ((double)RMP_COUNT_PER_M/1000.0)) );

  
  // ypos is going to be pitch for now...
  // change from counts to milli-degrees
  data->ypos = htonl( (uint32_t) rint(((double)rmp_data.pitch / 
				       (double)RMP_COUNT_PER_DEG) * 1000.0));
  
  // yaw is integrated yaw
  // not sure about this one..
  // from counts/rev to degrees.  one rev is 360 degree?
  data->yaw = htonl((uint32_t) rint( ((double)rmp_data.yaw /
				      (double)RMP_COUNT_PER_REV) * 360.0));

  // don't know the conversion yet...
  // change from counts/m/s into mm/s
  data->xspeed = htonl( (uint32_t) rint( ((double)rmp_data.left_dot / 
					 (double)RMP_COUNT_PER_M_PER_S)*1000.0 ));
  
  data->yspeed = htonl( (uint32_t) rint( ((double)rmp_data.right_dot / 
					 (double)RMP_COUNT_PER_M_PER_S)*1000.0) );
  
  // from counts/deg/sec into mill-deg/sec
  data->yawspeed = htonl( (uint32_t) (rint(((double)rmp_data.yaw_dot / 
					 (double)RMP_COUNT_PER_DEG_PER_S))*1000.0) );

}
  
/* marshals power related data into player format
 *
 * returns: 
 */
void
SegwayIO::GetData(player_power_data_t *data)
{
  rmp_frame_t rmp_data;

  pthread_mutex_lock(&latestData_mutex);
  rmp_data = latestData;
  pthread_mutex_unlock(&latestData_mutex);

  assert(rmp_data.IsReady());

  data->charge = htons(rmp_data.battery);
}
  
  
/* takes a player command and turns it into CAN packets for the RMP
 *
 * returns: 0 on success, negative else
 */
int
SegwayIO::VelocityCommand(const player_position_cmd_t &cmd)
{
  can_packet_t pkt;
  static int16_t last_trans=0, last_rot = 0;

  // we only care about cmd.xspeed and cmd.yawspeed
  // translational velocity is given to RMP in counts 
  // [-1176,1176] ([-8mph,8mph])

  // player is mm/s
  // 8mph is 3576.32 mm/s
  // so then mm/s -> counts = (1176/3576.32) = 0.32882963

  // this check should be done at a higher level???

  //  int16_t trans = (int16_t) rint((double)cmd.xspeed * RMP_COUNT_PER_MM_PER_S);
  // for now dont do any conversion...
  int16_t trans = cmd.xspeed;

  if (trans > RMP_MAX_TRANS_VEL_COUNT) {
    trans = RMP_MAX_TRANS_VEL_COUNT;
  } else if (trans < -RMP_MAX_TRANS_VEL_COUNT) {
    trans = -RMP_MAX_TRANS_VEL_COUNT;
  }

  // these may not be necessary since the read/write loop will just
  // read these values, not write to them... 
  pthread_mutex_lock(&trans_command_mutex);
  trans_command = trans;
  pthread_mutex_unlock(&trans_command_mutex);

  // rotational RMP command \in [-1024, 1024]
  // this is ripped from rmi_demo... to go from deg/s^2 to counts
  // deg/s^2 -> count = 1/0.013805056
  //  int16_t rot = (int16_t) rint((double)cmd.yawspeed * RMP_COUNT_PER_DEG_PER_SS);
  int16_t rot = cmd.yawspeed;

  if (rot > RMP_MAX_ROT_VEL_COUNT) {
    rot = RMP_MAX_ROT_VEL_COUNT;
  } else if (rot < -RMP_MAX_ROT_VEL_COUNT) {
    rot = -RMP_MAX_ROT_VEL_COUNT;
  }

  pthread_mutex_lock(&rot_command_mutex);
  rot_command = rot;
  pthread_mutex_unlock(&rot_command_mutex);
  
  /*
  if (rot || trans) {
    printf("SEGWAYIO: trans: %d rot: %d\n", trans_command, rot_command);
  }
  */

  // now we have to add an empty packet onto the command queue, so that
  // our read/write loop will send this new velocity

  if (rot_command != last_rot ||
      trans_command != last_trans) {
    pthread_mutex_lock(&command_queue_mutex);
    command_queue.push(pkt);
    pthread_mutex_unlock(&command_queue_mutex);

    last_rot = rot_command;
    last_trans = trans_command;
  }

  return 0;
}
  
  
/* Creates a status CAN packet from the given arguments and puts
 * it into the write queue
 *
 * returns: 0 on success, negative else
 */  
int
SegwayIO::StatusCommand(const uint16_t &cmd, const uint16_t &val)
{
  can_packet_t pkt;

  pkt.id = RMP_CAN_ID_COMMAND;
  pkt.PutSlot(2, cmd);
  pkt.PutSlot(3, val);

  if (cmd) {
    printf("SEGWAYIO: STATUS: cmd: %02x val: %02x pkt: %s\n", 
	   cmd, val, pkt.toString());
  }

  pthread_mutex_lock(&command_queue_mutex);
  command_queue.push(pkt);
  pthread_mutex_unlock(&command_queue_mutex);

  return 0;
}
