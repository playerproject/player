#include <math.h>
#include <stdio.h>
#include <sys/time.h>

#include "segwayio.h"

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
  // start the CAN at 500 kpbs
  if (canio->Init(BAUD_500K) < 0) {
    fprintf(stderr, "SEGWAYIO: error on CAN Init\n");
    return -1;
  }
  
  // start the read/write thread...
  if (pthread_create(&read_write_thread, NULL, &DummyMain, this)) {
    fprintf(stderr, "SEGWAYIO: error creating read/write thread.\n");
    return -1;
  }

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

  pthread_cancel(read_write_thread);
  
  if (pthread_join(read_write_thread, &unused)) {
    return -1;
  }

  // shutdown the CAN
  canio->Shutdown();

  return 0;
}
		 
/* Calls our real readwrite loop
 *
 * returns: 
 */
void *
SegwayIO::DummyMain(void *p)
{
  ((SegwayIO *)p)->ReadWriteLoop();

}
/* Performs the main read/write loop using the CAN bus.  We read packets
 * off the bus (on both channels) til empty.  then we write the latest
 * command we have onto the bus.
 *
 * returns: 
 */
void 
SegwayIO::ReadWriteLoop()
{
  // store the last command packet so we can repeat it, even if we haven't gotten
  // new command from player
  static can_packet_t last_command; 
  can_packet_t pkt;
  int channel = 0;
  int ret;
  rmp_frame_t data_frame[2];

  data_frame[0].ready = 0;
  data_frame[1].ready = 0;

  struct timeval curr, last;

  gettimeofday(&last, NULL);

  while (1) {

    gettimeofday(&curr, NULL);

    if ( ((curr.tv_sec - last.tv_sec)*1000 +
	  (curr.tv_usec - last.tv_usec)/1000) < RMP_READ_WRITE_PERIOD) {
      continue;
    }
    
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

    // now we want to send the latest command...

    // DO COMMAND STUFF HERE
    // MAKE A COMMAND PACKET AND WRITE IT
    while (!command_queue.empty()) {
      pthread_mutex_lock(&command_queue_mutex);
      if (!command_queue.empty()) {
	pkt = command_queue.front();
	command_queue.pop();
      } else {
	pthread_mutex_unlock(&command_queue_mutex);
	continue;
      }

      pthread_mutex_unlock(&command_queue_mutex);
      // now we have a new command...  these are just going to be 
      // "status commands" ie, slots 2 & 3 in the message
      // the velocities are in trans_command and rot_command
      
      // this way we can send a command and keep the velocity the same

      pthread_mutex_lock(&trans_command_mutex);
      pkt.PutSlot(0, (uint16_t)trans_command);
      pthread_mutex_unlock(&trans_command_mutex);

      pthread_mutex_lock(&rot_command_mutex);
      pkt.PutSlot(1, (uint16_t)rot_command);
      pthread_mutex_unlock(&rot_command_mutex);

      // write it out!
      canio->WritePacket(pkt);
    }
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
  data->xpos = htonl( (int32_t) rint((double)rmp_data.foreaft / 
				     ((double)RMP_COUNT_PER_M/1000.0)) );
  
  
  // ypos is going to be pitch for now...
  data->ypos = htonl( (int32_t) rint((double)rmp_data.pitch / 
				     (double)RMP_COUNT_PER_DEG) );

  // yaw is integrated yaw
  data->yaw = htonl( rmp_data.yaw / 360 );
  
  // don't know the conversion yet...
  data->xspeed = htonl( (int32_t) rmp_data.left_dot / 
			RMP_COUNT_PER_M_PER_S );
    data->yspeed = htonl( (int32_t) rmp_data.right_dot / 
			  RMP_COUNT_PER_M_PER_S );

  data->yawspeed = htonl( (int32_t) rint(rmp_data.yaw_dot / 
					 (double)RMP_COUNT_PER_DEG_PER_S) );
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

  // we only care about cmd.xspeed and cmd.yawspeed
  // translational velocity is given to RMP in counts 
  // [-1176,1176] ([-8mph,8mph])

  // player is mm/s
  // 8mph is 3576.32 mm/s
  // so then mm/s -> counts = (1176/3576.32) = 0.32882963

  // this check should be done at a higher level???

  int16_t trans = (int16_t) rint((double)cmd.xspeed * RMP_COUNT_PER_MM_PER_S);

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
  int16_t rot = (int16_t) rint((double)cmd.yawspeed * RMP_COUNT_PER_DEG_PER_SS);

  if (rot > RMP_MAX_ROT_VEL_COUNT) {
    rot = RMP_MAX_ROT_VEL_COUNT;
  } else if (rot < -RMP_MAX_ROT_VEL_COUNT) {
    rot = -RMP_MAX_ROT_VEL_COUNT;
  }

  pthread_mutex_lock(&rot_command_mutex);
  rot_command = rot;
  pthread_mutex_unlock(&rot_command_mutex);

  //  printf("SEGWAYIO: trans: %d rot: %d\n", trans_command, rot_command);

  // now we have to add an empty packet onto the command queue, so that
  // our read/write loop will send this new velocity
  pkt.id = RMP_CAN_ID_COMMAND;
  pkt.PutSlot(2, (uint16_t)RMP_CAN_CMD_NONE);

  pthread_mutex_lock(&command_queue_mutex);
  command_queue.push(pkt);
  pthread_mutex_unlock(&command_queue_mutex);

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

  pthread_mutex_lock(&command_queue_mutex);
  command_queue.push(pkt);
  pthread_mutex_unlock(&command_queue_mutex);

  return 0;
}
