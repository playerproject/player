#ifndef _SEGWAYIO_H_
#define _SEGWAYIO_H_

#include "canio.h"

#include <player.h>
#include <pthread.h>
#include <queue>

using namespace std;

#define RMP_CAN_ID_SHUTDOWN	0x0412
#define RMP_CAN_ID_COMMAND	0x0413
#define RMP_CAN_ID_MSG1		0x0400
#define RMP_CAN_ID_MSG2		0x0401
#define RMP_CAN_ID_MSG3		0x0402
#define RMP_CAN_ID_MSG4		0x0403
#define RMP_CAN_ID_MSG5		0x0404

#define RMP_CAN_CMD_NONE		0
#define RMP_CAN_CMD_MAX_VEL		10
#define RMP_CAN_CMD_MAX_ACCL		11
#define RMP_CAN_CMD_MAX_TURN		12
#define RMP_CAN_CMD_GAIN_SCHED		13
#define RMP_CAN_CMD_CURR_LIMIT		14
#define RMP_CAN_CMD_RST_INT		50

#define RMP_CAN_RST_RIGHT		0x01
#define RMP_CAN_RST_LEFT		0x02
#define RMP_CAN_RST_YAW			0x04
#define RMP_CAN_RST_FOREAFT		0x08
#define RMP_CAN_RST_ALL			(RMP_CAN_RST_RIGHT | \
					 RMP_CAN_RST_LEFT | \
					 RMP_CAN_RST_YAW | \
					 RMP_CAN_RST_FOREAFT)

#define RMP_COUNT_PER_M			33215
#define RMP_COUNT_PER_DEG		7.8
#define RMP_COUNT_PER_M_PER_S		332
#define RMP_COUNT_PER_DEG_PER_S		7.8
#define RMP_COUNT_PER_MM_PER_S		0.32882963
#define RMP_COUNT_PER_DEG_PER_SS	72.437229
#define RMP_COUNT_PER_REV               112644

#define RMP_MAX_TRANS_VEL_MM_S		3576
#define RMP_MAX_ROT_VEL_DEG_S		18	// from rmi_demo: 1300*0.013805056
#define RMP_MAX_TRANS_VEL_COUNT		1176
#define RMP_MAX_ROT_VEL_COUNT		1024

#define RMP_READ_WRITE_PERIOD		500	// 10 ms period = 100 Hz

// this holds all the RMP data it gives us
struct rmp_frame_t
{
  int16_t     	pitch;
  int16_t	pitch_dot;
  int16_t     	roll;
  int16_t     	roll_dot;
  int32_t      	yaw;
  int16_t	yaw_dot;
  int32_t      	left;
  int16_t      	left_dot;
  int32_t      	right;
  int16_t      	right_dot;
  int32_t      	foreaft;

  uint16_t     	frames;
  uint16_t	battery;
  uint8_t	ready;

  /*  rmp_frame_t() : pitch(0.0), pitch_dot(0.0),
      roll(0.0), roll_dot(0.0),
      yaw_dot(0.0), left(0),
      left_dot(0), right(0),
      right_dot(0), travel(0),
      turn(0), frames(0),
      battery(0), ready(0) {}
  */
  rmp_frame_t() : ready(0) {}

  void AddPacket(const can_packet_t &pkt);
  bool IsReady() { return ready == 0x1F; }
};

/* This class deals with Segway specific I/O.  So it will handle the timing
   of reading/writing to the CAN bus, and it will also interpret
   the CAN packets into sensible values.  It takes care of marshalling
   the RMP's status packets into a sensible data structure, and will
   also take position commands from player and turn them into 
   commands and send them on the CAN bus.
*/
class SegwayIO 
{
public:
  // use this instead of constructor to force one instance
  static SegwayIO *Instance();

  ~SegwayIO();

  int Init();

  int Shutdown();

  // return RMP data into data
  void GetData(player_position_data_t *data);
  void GetData(player_power_data_t *data);

  // use cmd as new position command
  int VelocityCommand(const player_position_cmd_t &cmd);

  int StatusCommand(const uint16_t &cmd, const uint16_t &val);
  
  int ShutdownCommand();

  // implement all the configs as methods here... they just create
  // a CAN packet and put it on the queue
protected:
  SegwayIO();
  static void * DummyReadLoop(void *);
  static void * DummyWriteLoop(void *);

  void ReadLoop();
  void WriteLoop();

protected:

  DualCANIO *canio;

  bool canioInit;
  bool canioShutdown;
  int usageCount;

  pthread_t read_thread;
  pthread_t write_thread;

  queue<can_packet_t> command_queue;
  pthread_mutex_t command_queue_mutex;

  int16_t trans_command;
  pthread_mutex_t trans_command_mutex;
  int16_t rot_command;
  pthread_mutex_t rot_command_mutex;
  
  rmp_frame_t latestData;
  pthread_mutex_t latestData_mutex;

private:
  static SegwayIO *instance;
};



#endif
