///////////////////////////////////////////////////////////////////////////
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

#include <string.h> // for memcpy()

#include "stagedevice.hh"
#include "offsets.h"


///////////////////////////////////////////////////////////////////////////
// Minimal constructor
// buffer points to a single buffer containing the data, command and configuration buffers.
//
CStageDevice::CStageDevice(void *buffer, size_t data_len, size_t command_len, size_t config_len)
{
    m_info_buffer = (BYTE*) buffer;
    m_info_len = INFO_BUFFER_SIZE;
    
    m_data_buffer = (BYTE*) m_info_buffer + m_info_len;
    m_data_len = data_len;

    m_command_buffer = (BYTE*) m_data_buffer + data_len;
    m_command_len = command_len;

    m_config_buffer = (BYTE*) m_command_buffer + command_len;
    m_config_len = config_len;

    TRACE4("creating device at addr: %p %p %p %p", m_info_buffer, m_data_buffer,
        m_command_buffer, m_config_buffer);
}


///////////////////////////////////////////////////////////////////////////
// Initialise the device
//
int CStageDevice::Setup()
{
    // Set the subscribed flag
    //
    m_info_buffer[INFO_SUBSCRIBE_FLAG] = 1;
    return 0;
}


///////////////////////////////////////////////////////////////////////////
// Terminate the device
//
int CStageDevice::Shutdown()
{
    // Reset the subscribed flag
    //
    m_info_buffer[INFO_SUBSCRIBE_FLAG] = 0;
    return 0;
};


///////////////////////////////////////////////////////////////////////////
// Read data from the device
//
int CStageDevice::GetData(unsigned char *data)
{
    // *** WARNING There REALLY REALLY should be a length specifier here!!!
    int len; 

    // See if there is any data
    //
    if (m_info_buffer[INFO_DATA_FLAG] != 0)
    {
        len = m_data_len; //*** just assume length!!

        // Copy the data
        //
        memcpy(data, m_data_buffer, len);

        // Reset flag to indicate data has been read
        //
        m_info_buffer[INFO_DATA_FLAG] = 0;
    }
    else
    {
        // Reset length to indicate no data
        //
        len = m_data_len;  // *** player expects data anyway!!
    }

    return len;
}


///////////////////////////////////////////////////////////////////////////
// Write a command to the device
//
void CStageDevice::PutCommand(unsigned char *command, int len)
{
    ASSERT((size_t) len <= m_command_len);
    
    // Copy the command
    //
    memcpy(m_command_buffer, command, len);

    // Set flag to indicate command has been changed
    //
    m_info_buffer[INFO_COMMAND_FLAG] = 1;
}


///////////////////////////////////////////////////////////////////////////
// Write configuration to the device
//
void CStageDevice::PutConfig(unsigned char *config, int len)
{
    ASSERT((size_t) len <= m_config_len);
    
    // Copy the data
    //
    memcpy(m_config_buffer, config, len);

    // Set flag to indicate config has been changed
    //
    m_info_buffer[INFO_CONFIG_FLAG] = 1;
}
