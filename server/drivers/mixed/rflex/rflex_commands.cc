#include <netinet/in.h>
#include "rflex-info.h"
#include "rflex_commands.h"
#include "rflex-io.h"
#include "rflex_configs.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

//holds data until someone wants to read it
typedef struct {
  int distance;
  int bearing;
  int t_vel;
  int r_vel;
  int num_sonars;
  int *ranges;
  int *oldranges;
  int num_bumpers;
  char *bumpers;
} rflex_status_t;

static rflex_status_t status;

//finds the sign of a value
static int sgn( long val )
{
  if (val<0) {
    return(0);
  } else {
    return(1);
  }
}

/* COMPUTE CRC CODE */

static int computeCRC( unsigned char *buf, int nChars )
{ 
  int i, crc;
  if (nChars==0) {
    crc = 0;
  } else {
    crc = buf[0];
    for (i=1; i<nChars; i++) {
      crc ^= buf[i];
    }
  }
  return(crc);
}

/* CONVERSION BYTES -> NUM */

#if 0 

static unsigned int convertBytes2UInt8( unsigned char *bytes )
{
  unsigned int i;
  memcpy( &i, bytes, 1 );
  return(i);
}

#endif

static unsigned int convertBytes2UInt16( unsigned char *bytes )
{
  unsigned int i;
  memcpy( &i, bytes, 2 );
  return(htons(i));
}


static unsigned long convertBytes2UInt32( unsigned char *bytes )
{
  unsigned long i;
  memcpy( &i, bytes, 4 );
  return(htonl(i));
}

/* CONVERSION NUM -> BYTES */

static void convertUInt8( unsigned int i, unsigned char *bytes )
{
  memcpy( bytes, &i, 1 );
}

#if 0 

static void convertUInt16( unsigned int i, unsigned char *bytes )
{
  uint16_t conv;
  conv = htonl( i );
  memcpy( bytes, &conv, 2 );
}

#endif

static void convertUInt32( unsigned long l, unsigned char *bytes )
{
  uint32_t conv;
  conv = htonl( l );
  memcpy( bytes, &conv, 4 );
}

//sends a command to the rflex
static void cmdSend( int fd, int port, int id, int opcode, int len, unsigned char *data )
{
  int i;
  static unsigned char cmd[MAX_COMMAND_LENGTH];
  /* START CODE */
  cmd[0] = 0x1b;
  cmd[1] = 0x02;
  /* PORT */
  cmd[2] = (unsigned char) port;
  /* ID */
  cmd[3] = (unsigned char) id;
  /* OPCODE */
  cmd[4] = (unsigned char) opcode;
  /* LENGTH */
  cmd[5] = (unsigned char) len;
  /* DATA */
  for (i=0; i<len; i++) {
    cmd[6+i] = data[i];
  }
  /* CRC */
  cmd[6+len] = computeCRC( &(cmd[2]), len+4 );    /* END CODE */
  cmd[6+len+1] = 0x1b;
  cmd[6+len+2] = 0x03;

  pthread_testcancel();
  writeData( fd, cmd, 9+len );
  pthread_testcancel();
}

void rflex_sonars_on( int fd )
{
  unsigned char data[MAX_COMMAND_LENGTH];
  convertUInt32( (unsigned long) rflex_configs.sonar_echo_delay/*30000*/, &(data[0]) );
  convertUInt32( (unsigned long) rflex_configs.sonar_ping_delay/*0*/, &(data[4]) );
  convertUInt32( (unsigned long) rflex_configs.sonar_set_delay/*0*/, &(data[8]) );
  convertUInt8(  (unsigned int) 2, &(data[12]) );
  cmdSend( fd, SONAR_PORT, 4, SONAR_RUN, 13, data );
}

void rflex_sonars_off( int fd )
{
  unsigned char data[MAX_COMMAND_LENGTH];
  convertUInt32( (long) 0, &(data[0]) );
  convertUInt32( (long) 0, &(data[4]) );
  convertUInt32( (long) 0, &(data[8]) );
  convertUInt8(  (int) 0, &(data[12]) );
  cmdSend( fd, SONAR_PORT, 4, SONAR_RUN, 13, data );
}

void rflex_digital_io_on( int fd, int period )
{
  unsigned char data[MAX_COMMAND_LENGTH];
  convertUInt32( (long) period, &(data[0]) );
  cmdSend( fd, DIO_PORT, 0, DIO_REPORTS_REQ, 4, data );
}

void rflex_digital_io_off( int fd )
{
  unsigned char data[MAX_COMMAND_LENGTH];
  convertUInt32( (long) 0, &(data[0]) );
  cmdSend( fd, SONAR_PORT, 4, DIO_REPORTS_REQ, 4, data );
}

void rflex_brake_on( int fd )
{
  cmdSend( fd, MOT_PORT, 0, MOT_BRAKE_SET, 0, NULL );
}

void rflex_brake_off( int fd )
{
  cmdSend( fd, MOT_PORT, 0, MOT_BRAKE_RELEASE, 0, NULL );
}

void rflex_motion_set_defaults( int fd )
{
  cmdSend( fd, MOT_PORT, 0, MOT_SET_DEFAULTS, 0, NULL );
}

void rflex_odometry_on( int fd, long period )
{ 
  unsigned char data[MAX_COMMAND_LENGTH];
  convertUInt32( period, &(data[0]) );         /* period in ms */
  convertUInt32( (long) 3, &(data[4]) );       /* mask */
  cmdSend( fd, MOT_PORT, 0, MOT_SYSTEM_REPORT_REQ, 8, data );
}

void rflex_odometry_off( int fd )
{ 
  unsigned char data[MAX_COMMAND_LENGTH];
  convertUInt32( (long) 0, &(data[0]) );       /* period in ms */
  convertUInt32( (long) 0, &(data[4]) );       /* mask */
  cmdSend( fd, MOT_PORT, 0, MOT_SYSTEM_REPORT_REQ, 8, data );
}

//returns the last battery, timestamp and brake information
void rflex_update_system( int fd , int *battery, 
			    int *timestamp, int *brake)
{
  cmdSend( fd, SYS_PORT, 0, SYS_STATUS, 0, NULL );
  
  battery = battery;
  timestamp = timestamp;
  brake = brake;
}

void rflex_set_velocity( int fd, long tvel, long rvel,
			   long acceleration )
{
  unsigned char data[MAX_COMMAND_LENGTH];

  convertUInt8( (long) 0,                 &(data[0]) );       /* forward motion */
  convertUInt32( (long) labs(tvel),        &(data[1]) );       /* abs trans velocity */
  convertUInt32( (long) acceleration,    &(data[5]) );       /* trans acc */
  convertUInt32( (long) STD_TRANS_TORQUE, &(data[9]) );       /* trans torque */
  convertUInt8( (long) sgn(tvel),         &(data[13]) );      /* trans direction */

  cmdSend( fd, MOT_PORT, 0, MOT_AXIS_SET_DIR, 14, data );

  convertUInt8( (long) 1,                 &(data[0]) );       /* rotational motion */
  convertUInt32( (long) labs(rvel),        &(data[1]) );       /* abs rot velocity  */
  /* 0.275 rad/sec * 10000 */ 
  convertUInt32( (long) STD_ROT_ACC,      &(data[5]) );       /* rot acc */
  convertUInt32( (long) STD_ROT_TORQUE,   &(data[9]) );       /* rot torque */
  convertUInt8( (long) sgn(rvel),         &(data[13]) );      /* rot direction */

  cmdSend( fd, MOT_PORT, 0, MOT_AXIS_SET_DIR, 14, data );
}

void rflex_stop_robot( int fd, int deceleration)
{
  rflex_set_velocity( fd, 0, 0, deceleration);
}

int rflex_open_connection(char *device_name, int *fd)
{
  Device   rdev;

  strncpy( rdev.ttyport, device_name, MAX_NAME_LENGTH);
  rdev.baud           = 115200;
  rdev.databits       = 8;
  rdev.parity         = N;
  rdev.stopbits       = 1;
  rdev.hwf            = 0;
  rdev.swf            = 0;
  
  printf("trying port %s\n",rdev.ttyport); 
  if (DEVICE_connect_port( &rdev )<0) {
    fprintf(stderr,"Can't open device %s\n",rdev.ttyport);
    return -1;
  } 

  *fd = rdev.fd;

  rflex_odometry_on(*fd, 100000);
  rflex_digital_io_on(*fd, 100000);
  rflex_motion_set_defaults(*fd);

  return 0;
}

//processes a motor packet fromt he rflex - and saves the data in the
//struct for later use
static void parseMotReport( unsigned char *buffer )
{
  int rv, timeStamp, acc, trq;
  unsigned char axis, opcode;
    
  opcode = buffer[4];
  switch(opcode) {
  case MOT_SYSTEM_REPORT:
    rv        = convertBytes2UInt32(&(buffer[6]));
    timeStamp = convertBytes2UInt32(&(buffer[10]));
    axis      = buffer[14];
    if (axis == 0) {
      status.distance = convertBytes2UInt32(&(buffer[15]));
      status.t_vel = convertBytes2UInt32(&(buffer[19]));
    } else if (axis == 1) {
      status.bearing = convertBytes2UInt32(&(buffer[15]));
      status.r_vel = convertBytes2UInt32(&(buffer[19]));
    }
    acc       = convertBytes2UInt32(&(buffer[23]));
    trq       = convertBytes2UInt32(&(buffer[27]));
    break;
  default:
    break;
  }
}

//processes a sonar packet fromt the rflex, and saves the data in the
//struct for later use
//HACK - also buffers data and filters for smallest in last AGE readings
static void parseSonarReport( unsigned char *buffer )
{

  unsigned int sid;
 
  int x,smallest;

  int count, retval, timeStamp;
  unsigned char opcode, dlen;
    
  opcode = buffer[4];
  dlen   = buffer[5];

  status.num_sonars=rflex_configs.max_num_sonars;
  switch(opcode) {
  case SONAR_REPORT:
    retval    = convertBytes2UInt32(&(buffer[6]));
    timeStamp = convertBytes2UInt32(&(buffer[10]));
    count = 0;
    while ((8+count*3<dlen) && (count<256)) {
      sid = buffer[14+count*3];
      //shift buffer
      for(x=0;x<rflex_configs.sonar_age-1;x++)
	status.oldranges[x+1+sid*rflex_configs.sonar_age]=status.oldranges[x+sid*rflex_configs.sonar_age];
      //add value to buffer
      smallest=status.oldranges[0+sid*rflex_configs.sonar_age]=convertBytes2UInt16( &(buffer[14+count*3+1]));
      //find the smallest
      for(x=1;x<rflex_configs.sonar_age;x++)
	if(smallest>status.oldranges[x+sid*rflex_configs.sonar_age])
	  smallest=status.oldranges[x+sid*rflex_configs.sonar_age];
      //set the smallest in last sonar_age as our value
      status.ranges[sid] = smallest;
      count++;
    }
    break;
  default:
    break;
  }
}

//parses a packet from the rflex, and decides what to do with it
static int parseBuffer( unsigned char *buffer, unsigned int len )
{
  unsigned int port, dlen, crc;

  port   = buffer[2];
  dlen   = buffer[5];

  if (dlen+8>len) {
    return(0);
  } else {
    crc    = computeCRC( &(buffer[2]), dlen+4 );
    if (crc != buffer[len-3])
      return(0);
    switch(port) {
    case SYS_PORT:
      fprintf( stderr, "(sys)" );
      break;
    case MOT_PORT:
      parseMotReport( buffer );
      break;
    case JSTK_PORT:
      break;
    case SONAR_PORT:
      parseSonarReport( buffer );
      break;
    case DIO_PORT:
      fprintf( stderr, "(dio)" );
      break;
    case IR_PORT:
      fprintf( stderr, "(ir)" );
      break;
    default:
      break;
    }
  }
  return(1);
}

static void clear_incoming_data(int fd)
{
  unsigned char buffer[4096];
  int len;
  int bytes;

  // 32 bytes here because the motion packet is 34. No sense in waiting for a
  // complete packet -- we can starve because the last 2 bytes might always
  // arrive with the next packet also, and we never leave the loop. 

  while ((bytes = bytesWaiting(fd)) > 32) {
    pthread_testcancel();
    waitForAnswer(fd, buffer, &len);
    pthread_testcancel();
    parseBuffer(buffer, len);
  }
}

//returns the odometry data saved int he struct
void rflex_update_status(int fd, int *distance,  int *bearing, 
			   int *t_vel, int *r_vel)
{
  clear_incoming_data(fd);

  *distance = status.distance;
  *bearing = status.bearing;
  *t_vel = status.t_vel;
  *r_vel = status.r_vel;
}

/* gets actual data and returns it in ranges
 * NOTE - actual mappings are strange
 * each module's sonar are numbered 0-15
 * thus for 4 sonar modules id's are 0-64
 * even if only say 5 or 8 sonar are connected to a given module
 * thus, we record values as they come in 
 * (data comes in in sets of n modules, 3 if you have 3 modules for example)
 *
 * note the remmapping done using the config parameters into 0-n, this mapping
 * should come out the same as the mapping adverstised on your robot
 * this if you put the poses in that order in the config file - everything
 * will line up nicely
 *
 * -1 is returned if we get back fewer sonar than requested 
 * (meaning your parameters are probobly incorrect) otherwise we return 0 
 */
int rflex_update_sonar(int fd,int num_sonars, int * ranges){
  int x;
  int y;
  int i;

  clear_incoming_data(fd);

  //copy all data
  i=0;
  y=0;
  for(x=0;x<rflex_configs.num_sonar_banks;x++)
    for(y=0;y<rflex_configs.num_sonars_in_bank[x];y++)
      ranges[i++]=status.ranges[x*rflex_configs.num_sonars_possible_per_bank+y];
  if (i<num_sonars){
    fprintf(stderr,"Requested %d sonar only %d supported\n",num_sonars,y);
    num_sonars = y;
    return -1;
  }
 return 0;
}

//haven't bothered using this yet - as I have no bumpers, who wants it?
void rflex_update_bumpers(int fd, int num_bumpers, 
			    char *values)
{
  clear_incoming_data(fd);

  if (num_bumpers > status.num_bumpers) {
    fprintf(stderr,"Requested more bumpers than available.\n");
    num_bumpers = status.num_bumpers;
  }

  memcpy(values, status.bumpers, num_bumpers*sizeof(char));
}


/* TODO - trans_pos rot_pos unused... AGAIN, fix this 
 * same effects are emulated at a higher level - is it possible to
 * do it here?
 */
void rflex_initialize(int fd, int trans_acceleration,
		      int rot_acceleration,
		      int trans_pos,
		      int rot_pos)
{
  unsigned char data[MAX_COMMAND_LENGTH];
  int x;

  data[0] = 0;
  convertUInt32( (long) 0, &(data[1]) );          /* velocity */ 
  convertUInt32( (long) trans_acceleration, &(data[5]) );       
                                                  /* acceleration */ 
  convertUInt32( (long) 0, &(data[9]) );      /* torque */ 
  data[13] = 0;

  cmdSend( fd, MOT_PORT, 0, MOT_AXIS_SET_DIR, 14, data );

  data[0] = 1;
  convertUInt32( (long) 0, &(data[1]) );          /* velocity */ 
  convertUInt32( (long) rot_acceleration, &(data[5]) );       
                                                  /* acceleration */ 
  convertUInt32( (long) 0, &(data[9]) );      /* torque */ 
  data[13] = 0;

  cmdSend( fd, MOT_PORT, 0, MOT_AXIS_SET_DIR, 14, data );

  //mark all non-existant (or no-data) sonar as such
  //note - this varies from MAX_INT set when sonar fail to get a valid echo.
  status.ranges=(int*) malloc(rflex_configs.max_num_sonars*sizeof(int));
  status.oldranges=(int*) malloc(rflex_configs.max_num_sonars*rflex_configs.sonar_age*sizeof(int));
  for(x=0;x<rflex_configs.max_num_sonars;x++)
    status.ranges[x]=-1;
}












