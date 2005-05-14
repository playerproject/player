//////////////////////////////////////////////////////////////////////////
//
// File: stagedevice.cc
// Author: Andrew Howard, Richard Vaughan
// Date: 28 Nov 2000
// Desc: Class for stage (simulated) devices
//
// CVS info:
//  $Source$
//  $Author$
//  $Revision$
//
// Usage:
//  (empty)
//
// Theory of operation:
//  (empty)
//
// Known bugs:
//  (empty)
//
// Possible enhancements:
//  (empty)
//
///////////////////////////////////////////////////////////////////////////

//#define DEBUG 1
#define PLAYER_ENABLE_TRACE 0

#include <string.h> // for memcpy()
#include <stagedevice.h>
#include <stage1p3.h>
#include <sys/file.h> //for flock

#include <error.h>
#include <devicetable.h>
extern DeviceTable* deviceTable;

#include <playertime.h>
extern PlayerTime* GlobalTime;

//#define DEBUG
//#define VERBOSE

///////////////////////////////////////////////////////////////////////////
// Minimal constructor
// buffer points to a single buffer containing the data, command and configuration buffers.
//
StageDevice::StageDevice(player_stage_info_t* info, 
                         int lockfd, int lockbyte )
: Driver(NULL, 0)

{
#ifdef DEBUG
  printf( "P: Creating Stage device (%d,%d,%d) locking %d:%d\n", 
          info->player_id.robot, 
          info->player_id.code, 
          info->player_id.index,
	  lockfd, lockbyte ); 
#endif

  void* data_buffer;
  size_t data_len;
  void* command_buffer;
  size_t command_len;
  void* config_buffer;
  void* reply_buffer;

  m_info = info;
  m_info_len = sizeof( player_stage_info_t );
  
  data_buffer = (uint8_t*)((caddr_t)m_info + sizeof( player_stage_info_t) );
  data_len = m_info->data_len;
  
  command_buffer = (uint8_t*)((caddr_t)data_buffer + m_info->data_len );
  command_len = m_info->command_len;

  config_buffer = (uint8_t*)((caddr_t)command_buffer + m_info->command_len);
  reply_buffer = (uint8_t*)((caddr_t)config_buffer + 
                            (m_info->config_len * sizeof(playerqueue_elt_t)));

  this->AddInterface(m_info->player_id,PLAYER_ALL_MODE);

  // cache a pointer to my device, so that I can get at these buffers later
  assert(this->device = deviceTable->GetDevice(m_info->player_id));
  
  next = 0; // initialize linked list pointer

  InstallLock( lockfd, lockbyte );

#ifdef DEBUG
  PLAYER_TRACE4("creating device at addr: %p %p %p %p %p", 
                data_buffer, command_buffer, 
                config_buffer, reply_buffer);
  fflush( stdout );
#endif
}


///////////////////////////////////////////////////////////////////////////
// Initialise the device
//
int StageDevice::Setup()
{
  // Set the subscribed flag
  m_info->subscribed++;
  return 0;
}


///////////////////////////////////////////////////////////////////////////
// Terminate the device
//
int StageDevice::Shutdown()
{
  // Reset the subscribed flag
  m_info->subscribed--;
  return 0;
};


///////////////////////////////////////////////////////////////////////////
// Read data from the device
//
size_t StageDevice::GetData(player_device_id_t id,
                            void* dest, size_t size,
                            struct timeval* timestamp)
{
  Lock();
  
#ifdef DEBUG
  /*
  printf( "P: getting (%d,%d,%d) info at %p, data at %p, buffer len %d, %d bytes available, size parameter %d\n", 
          m_info->player_id.robot, 
          m_info->player_id.code, 
          m_info->player_id.index, 
          m_info, device_data,
          m_info->data_len,
          m_info->data_avail,
          size );
  fflush( stdout );
  */
#endif

  // See if there is any data
  //
  size_t data_avail = m_info->data_avail;

  // Check for overflows 1
  //
  if (data_avail > PLAYER_MAX_MESSAGE_SIZE )
  {
    printf( "Available data (%d bytes) is larger than Player's"
            " maximum message size (%d bytes)\n", 
            (int)data_avail, PLAYER_MAX_MESSAGE_SIZE );
  }

  // Check for overflows 2
  //
  if (data_avail > this->device->data_size )
  {
    printf("warning: available data (%d bytes) > buffer size (%d bytes); ignoring data\n", (int)data_avail, (int)this->device->data_size );
    Unlock();
    return 0;
    //data_avail = m_data_len;
  }
    
  // Check for overflows 3
  //
  if( data_avail > size)
  {
    printf("warning data available (%d bytes) > space in Player packet (%d bytes); ignoring data\n", (int)data_avail, (int)size );
    Unlock();
    return 0;
  }
    
  // Copy the data
  memcpy(dest, this->device->data, data_avail);

  // store the timestamp in the device, because other devices may
  // wish to read it
  this->device->data_timestamp.tv_sec = m_info->data_timestamp_sec;
  this->device->data_timestamp.tv_usec = m_info->data_timestamp_usec;

  // also return the timestamp to the caller
  if(timestamp)
    *timestamp = this->device->data_timestamp;
    
  Unlock();

  return data_avail;
}


///////////////////////////////////////////////////////////////////////////
// Write a command to the device
//
void StageDevice::PutCommand(player_device_id_t id,
                             void* src, size_t len,
                             struct timeval* timestamp)
{

  Lock();

#ifdef DEBUG
  printf( "P: StageDevice::PutCommand (%d,%d,%d) info at %p,"
	  " command at %p\n", 
          m_info->player_id.robot, 
          m_info->player_id.code, 
          m_info->player_id.index, 
          m_info, src);
  fflush( stdout );
#endif

  // Check for overflows
  if (len > this->device->command_size)
    PLAYER_ERROR2("invalid command length (%d > %d); ignoring command", 
                  len, this->device->command_size);
    
  // Copy the command
  memcpy(this->device->command, src, len);

  // Set flag to indicate command has been set
  m_info->command_avail = len;

  // set timestamp for this command
  struct timeval tv;
  GlobalTime->GetTime(&tv);

  m_info->command_timestamp_sec = tv.tv_sec;
  m_info->command_timestamp_usec = tv.tv_usec;

  Unlock();
}

void StageDevice::Update(void)
{
  // testing
#if 0
  Lock();
  
  // is there new data available?
  if(m_info->data_avail && 
     (this->m_info->data_timestamp_sec > 
      this->device->data_timestamp.tv_sec) ||
     ((this->m_info->data_timestamp_sec ==
       this->device->data_timestamp.tv_sec) &&
      (this->m_info->data_timestamp_usec >
       this->device->data_timestamp.tv_usec)))
    this->DataAvailable();

  Unlock();
#endif
}

void StageDevice::Lock( void )
{
 // POSIX RECORD LOCKING METHOD
  struct flock cmd;

  cmd.l_type = F_WRLCK; // request write lock
  cmd.l_whence = SEEK_SET; // count bytes from start of file
  cmd.l_start = this->lock_byte; // lock my unique byte
  cmd.l_len = 1; // lock 1 byte

  fcntl( this->lock_fd, F_SETLKW, &cmd );
}

void StageDevice::Unlock( void )
{
  // POSIX RECORD LOCKING METHOD
  struct flock cmd;
  
  cmd.l_type = F_UNLCK; // request  unlock
  cmd.l_whence = SEEK_SET; // count bytes from start of file
  cmd.l_start = this->lock_byte; // unlock my unique byte
  cmd.l_len = 1; // unlock 1 byte
  
  fcntl( this->lock_fd, F_SETLKW, &cmd );
}

