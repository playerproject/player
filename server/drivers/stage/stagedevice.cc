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

#define PLAYER_ENABLE_TRACE 0


#include <string.h> // for memcpy()
#include <stagedevice.h>
#include <stage.h>
#include <sys/file.h> //for flock

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
{
#ifdef DEBUG
  printf( "P: Creating Stage device (%d,%d,%d) locking %d:%d\n", 
          info->player_id.port, 
          info->player_id.type, 
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

  SetupBuffers((unsigned char*)data_buffer, data_len,
               (unsigned char*)command_buffer, command_len,
               (unsigned char*)config_buffer, m_info->config_len,
               (unsigned char*)reply_buffer, m_info->reply_len);
  
  next = 0; // initialize linked list pointer

  InstallLock( lockfd, lockbyte );

#ifdef DEBUG
  PLAYER_TRACE4("creating device at addr: %p %p %p %p %p %p", 
                m_info, data_buffer, command_buffer, 
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
size_t StageDevice::GetData(void* client,unsigned char *data, size_t size,
                        uint32_t* timestamp_sec, 
                        uint32_t* timestamp_usec)
{
  Lock();
#ifdef DEBUG
  printf( "P: getting (%d,%d,%d) info at %p, data at %p, buffer len %d, %d bytes available, size parameter %d\n", 
          m_info->player_id.port, 
          m_info->player_id.type, 
          m_info->player_id.index, 
          m_info, device_data,
          m_info->data_len,
          m_info->data_avail,
          size );
  fflush( stdout );
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
            data_avail, PLAYER_MAX_MESSAGE_SIZE );
  }

  // Check for overflows 2
  //
  if (data_avail > device_datasize )
  {
    printf("warning: available data (%d bytes) > buffer size (%d bytes); ignoring data\n", data_avail, device_datasize );
    Unlock();
    return 0;
    //data_avail = m_data_len;
  }
    
  // Check for overflows 3
  //
  if( data_avail > size)
  {
    printf("warning data available (%d bytes) > space in Player packet (%d bytes); ignoring data\n", data_avail, size );
    Unlock();
    return 0;

    //data_avail = size;
  }
    

  // Copy the data
  memcpy(data, device_data, data_avail);

  // TODO: should this still be here?
  // no! and it wasted a good hour of my time.... - RTV
  //m_info->data_avail = 0;// consume this data for testing purposes

  // store the timestamp in the device, because other devices may
  // wish to read it
  data_timestamp_sec = m_info->data_timestamp_sec;
  data_timestamp_usec = m_info->data_timestamp_usec;

  // also return the timestamp to the caller
  if(timestamp_sec)
    *timestamp_sec = data_timestamp_sec;
  if(timestamp_usec)
    *timestamp_usec = data_timestamp_usec;
    
  Unlock();

  return data_avail;
}


///////////////////////////////////////////////////////////////////////////
// Write a command to the device
//
void StageDevice::PutCommand(void* client,unsigned char *command, size_t len)
{
  Lock();

#ifdef DEBUG
  printf( "P: StageDevice::PutCommand (%d,%d,%d) info at %p,"
	  " command at %p\n", 
          m_info->player_id.port, 
          m_info->player_id.type, 
          m_info->player_id.index, 
          m_info, command);
  fflush( stdout );
#endif

  // Check for overflows
  if (len > device_commandsize)
    PLAYER_ERROR("invalid command length; ignoring command");
    
  // Copy the command
  memcpy(device_command, command, len);

  // Set flag to indicate command has been set
  m_info->command_avail = len;

  // set timestamp for this command
  struct timeval tv;
  GlobalTime->GetTime(&tv);

  m_info->command_timestamp_sec = tv.tv_sec;
  m_info->command_timestamp_usec = tv.tv_usec;

  Unlock();
}


///////////////////////////////////////////////////////////////////////////
// Write configuration to the device
//
/*
int StageDevice::PutConfig(CClientData* client, unsigned char *config, 
                            size_t len)
{
  Lock();

  // Check for overflows
  //
  if (len > m_config_len)
    PLAYER_ERROR("invalid config length; ignoring config");
    
  // Copy the data
  memcpy(m_config_buffer, config, len);

  // Set flag to indicate config has been changed
  m_info->config_avail = len;

  // set timestamp for this config
  struct timeval tv;
  GlobalTime->GetTime(&tv);

  m_info->config_timestamp_sec = tv.tv_sec;
  m_info->config_timestamp_usec = tv.tv_usec;

  Unlock();

  return(0);
}
*/

void StageDevice::Lock( void )
{
  //printf( "P: LOCK %p\n", m_lock );

//  #ifdef POSIX_SEM

//      if( sem_wait( m_lock ) < 0 )
//        {
//          perror( "sem_wait failed" );
//          return false;
//        }

//  #else

//    // BSD file locking style
//    flock( lock_fd, LOCK_EX );

//  #endif

 // POSIX RECORD LOCKING METHOD
  struct flock cmd;

  cmd.l_type = F_WRLCK; // request write lock
  cmd.l_whence = SEEK_SET; // count bytes from start of file
  cmd.l_start = this->lock_byte; // lock my unique byte
  cmd.l_len = 1; // lock 1 byte


  //printf( "WAITING for byte %d\n", this->lock_byte );



  fcntl( this->lock_fd, F_SETLKW, &cmd );
  //printf( "DONE WAITING\n" );
}

void StageDevice::Unlock( void )
{
  //printf( "P: UNLOCK %p\n", m_lock );

//  #ifdef POSIX_SEM

//    if( sem_post( m_lock ) < 0 )
//    {
//    perror( "sem_post failed" );
//    return false;
//    }

//  #else

//    // BSD file locking style
//    flock( lock_fd, LOCK_UN );

//  #endif

 // POSIX RECORD LOCKING METHOD
  struct flock cmd;

  cmd.l_type = F_UNLCK; // request  unlock
  cmd.l_whence = SEEK_SET; // count bytes from start of file
  cmd.l_start = this->lock_byte; // unlock my unique byte
  cmd.l_len = 1; // unlock 1 byte

  fcntl( this->lock_fd, F_SETLKW, &cmd );
}

