//////////////////////////////////////////////////////////////////////////
//
// File: stagedevice.cc
// Author: Andrew Howard
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

#include <sys/time.h> // for gettimeofdaty()
#include <string.h> // for memcpy()
#include <stagedevice.h>
#include <stage.h>

//#define DEBUG
//#define VERBOSE

///////////////////////////////////////////////////////////////////////////
// Minimal constructor
// buffer points to a single buffer containing the data, command and configuration buffers.
//
CStageDevice::CStageDevice(player_stage_info_t* info )
{
#ifdef DEBUG
  printf( "P: Creating Stage device (%d,%d,%d)\n", 
	  info->player_id.port, 
	  info->player_id.type, 
	  info->player_id.index ); 
#endif

  m_info = info;
  m_info_len = sizeof( player_stage_info_t );
  
  m_data_buffer = (uint8_t*)((caddr_t)m_info + sizeof( player_stage_info_t) );
  m_data_len = m_info->data_len;
  
  m_command_buffer = (uint8_t*)((caddr_t)m_data_buffer + m_info->data_len );
  m_command_len = m_info->command_len;
  
  m_config_buffer = (uint8_t*)
    ((caddr_t)m_command_buffer + m_info->command_len );
  
  m_config_len = m_info->config_len;
  
  next = 0; // initialize linked list pointer
      
#ifdef DEBUG
  PLAYER_TRACE4("creating device at addr: %p %p %p %p %p", 
		m_info, m_data_buffer, m_command_buffer, m_config_buffer );
  fflush( stdout );
#endif
}


///////////////////////////////////////////////////////////////////////////
// Initialise the device
//
int CStageDevice::Setup()
{
    // Set the subscribed flag
    //
    m_info->subscribed++;
    return 0;
}


///////////////////////////////////////////////////////////////////////////
// Terminate the device
//
int CStageDevice::Shutdown()
{
  // Reset the subscribed flag
  //
   m_info->subscribed--;
    return 0;
};


///////////////////////////////////////////////////////////////////////////
// Read data from the device
//
size_t CStageDevice::GetData(unsigned char *data, size_t size)
{
#ifdef DEBUG
  printf( "P: getting (%d,%d,%d) info at %p, data at %p, buffer len %d, %d bytes available, size parameter %d\n", 
	  m_info->player_id.port, 
	  m_info->player_id.type, 
	  m_info->player_id.index, 
	  m_info, m_data_buffer,
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
    if (data_avail > m_data_len )
    {
        printf("warning: available data (%d bytes) > buffer size (%d bytes); ignoring data\n", data_avail, m_data_len );
	return 0;
	//data_avail = m_data_len;
    }
    
    // Check for overflows 3
    //
    if( data_avail > size)
    {
        printf("warning data available (%d bytes) > space in Player packet (%d bytes); ignoring data\n", data_avail, size );
        return 0;

	//data_avail = size;
    }
    

    // Copy the data
    //
    memcpy(data, m_data_buffer, data_avail);

    //m_info->data_avail = 0;// consume this data for testing purposes

    // Copy the timestamp
    //
    data_timestamp_sec = m_info->data_timestamp_sec;
    data_timestamp_usec = m_info->data_timestamp_usec;
    
    return data_avail;
}


///////////////////////////////////////////////////////////////////////////
// Write a command to the device
//
void CStageDevice::PutCommand(unsigned char *command, size_t len)
{
#ifdef DEBUG
  printf( "P: StageDevice::PutCommand (%d,%d,%d) info at %p, command at %p\n", 
	  m_info->player_id.port, 
	  m_info->player_id.type, 
	  m_info->player_id.index, 
	    m_info, command );
    fflush( stdout );
#endif

    // Check for overflows
    //
    if (len > m_command_len)
        PLAYER_ERROR("invalid command length; ignoring command");
    
    
    // Copy the command
    // 
    memcpy(m_command_buffer, command, len);

    // Set flag to indicate command has been set
    //
    m_info->command_avail = len;

    // set timestamp for this command
    struct timeval tv;
    gettimeofday( &tv, 0 );

    m_info->command_timestamp_sec = tv.tv_sec;
    m_info->command_timestamp_usec = tv.tv_usec;
}


///////////////////////////////////////////////////////////////////////////
// Write configuration to the device
//
void CStageDevice::PutConfig(unsigned char *config, size_t len)
{
    // Check for overflows
    //
    if (len > m_config_len)
        PLAYER_ERROR("invalid config length; ignoring config");
    
    // Copy the data
    //
    memcpy(m_config_buffer, config, len);

    // Set flag to indicate config has been changed
    //
    m_info->config_avail = len;

    // set timestamp for this config
    struct timeval tv;
    gettimeofday( &tv, 0 );
    
    m_info->config_timestamp_sec = tv.tv_sec;
    m_info->config_timestamp_usec = tv.tv_usec;
}













