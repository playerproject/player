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

#define ENABLE_TRACE 0

#include <string.h> // for memcpy()
#include "stagedevice.hh"
#include "offsets.h"


///////////////////////////////////////////////////////////////////////////
// Minimal constructor
// buffer points to a single buffer containing the data, command and configuration buffers.
//
CStageDevice::CStageDevice(void *buffer, size_t data_len, size_t command_len, size_t config_len)
{
    m_info = (PlayerStageInfo*) buffer;
    m_info_len = INFO_BUFFER_SIZE;
    
    m_data_buffer = (BYTE*) buffer + m_info_len;
    m_data_len = data_len;

    m_command_buffer = (BYTE*) m_data_buffer + data_len;
    m_command_len = command_len;

    m_config_buffer = (BYTE*) m_command_buffer + command_len;
    m_config_len = config_len;

    TRACE4("creating device at addr: %p %p %p %p", m_info, m_data_buffer,
        m_command_buffer, m_config_buffer);
}


///////////////////////////////////////////////////////////////////////////
// Initialise the device
//
int CStageDevice::Setup()
{
    /* *** TESTING -- this doesnt work right now
    // See if device is available
    //
    if (!m_info->available)
        return 1;
    */
    
    // Set the subscribed flag
    //
    m_info->subscribed = 1;
    return 0;
}


///////////////////////////////////////////////////////////////////////////
// Terminate the device
//
int CStageDevice::Shutdown()
{
    // Reset the subscribed flag
    //
    m_info->subscribed = 0;
    return 0;
};


///////////////////////////////////////////////////////////////////////////
// Read data from the device
//
size_t CStageDevice::GetData(unsigned char *data, size_t size)
{
    // See if there is any data
    //
    size_t data_len = m_info->data_len;
    ASSERT(data_len <= m_data_len);
    ASSERT(data_len <= size);
    
    // Copy the data
    //
    memcpy(data, m_data_buffer, data_len);
    return data_len;
}


///////////////////////////////////////////////////////////////////////////
// Write a command to the device
//
void CStageDevice::PutCommand(unsigned char *command, size_t len)
{
    ASSERT(len <= m_command_len);
    
    // Copy the command
    // 
    memcpy(m_command_buffer, command, len);

    // Set flag to indicate command has been set
    //
    m_info->command_len = len;
}


///////////////////////////////////////////////////////////////////////////
// Write configuration to the device
//
void CStageDevice::PutConfig(unsigned char *config, size_t len)
{
    ASSERT(len <= m_config_len);
    
    // Copy the data
    //
    memcpy(m_config_buffer, config, len);

    // Set flag to indicate config has been changed
    //
    m_info->config_len = len;
}

