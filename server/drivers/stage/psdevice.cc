/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Richard Vaughan, & Andrew Howard
 *                      
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 *
 * Here is defined the PSDevice class, which inherits from Player's CDevice
 * class, but adds a couple of extra methods for those Player devices that 
 * should behave differently when Stage is being used.  In particular, methods
 * are provided here to allow such devices to send information back and forth
 * to Stage for visualization purposes.
 */

#include <string.h>

#include "psdevice.h"
#include "playertime.h"
extern PlayerTime* GlobalTime;

#if INCLUDE_STAGE
// call this to setup pointers to the stage buffer; args the same as 
// for the StageDevice constructor
void 
PSDevice::SetupStageBuffers(player_stage_info_t* info, 
                            int lockfd, int lockbyte )
{
#ifdef DEBUG
  printf( "P: Creating Stage device (%d,%d,%d) locking %d:%d\n", 
          info->player_id.port, 
          info->player_id.code, 
          info->player_id.index,
	  lockfd, lockbyte ); 
#endif

  void* data_buffer;
  size_t data_len;
  void* command_buffer;
  size_t command_len;

  m_info = info;
  m_info_len = sizeof(player_stage_info_t);
  
  data_buffer = (uint8_t*)((caddr_t)m_info + sizeof( player_stage_info_t) );
  data_len = m_info->data_len;
  
  command_buffer = (uint8_t*)((caddr_t)data_buffer + m_info->data_len );
  command_len = m_info->command_len;

  SetupBuffers((unsigned char*)data_buffer, data_len,
               (unsigned char*)command_buffer, command_len,
               (unsigned char*)NULL, 0,
               (unsigned char*)NULL, 0);
  
  InstallLock(lockfd, lockbyte);

#ifdef DEBUG
  PLAYER_TRACE4("creating device at addr: %p %p %p", 
                m_info, data_buffer, command_buffer);
  fflush( stdout );
#endif
}

void 
PSDevice::StageLock(void)
{
  // POSIX RECORD LOCKING METHOD
  struct flock cmd;

  cmd.l_type = F_WRLCK; // request write lock
  cmd.l_whence = SEEK_SET; // count bytes from start of file
  cmd.l_start = this->lock_byte; // lock my unique byte
  cmd.l_len = 1; // lock 1 byte

  fcntl(this->lock_fd, F_SETLKW, &cmd);
}

void 
PSDevice::StageUnlock(void)
{
  // POSIX RECORD LOCKING METHOD
  struct flock cmd;
  
  cmd.l_type = F_UNLCK; // request  unlock
  cmd.l_whence = SEEK_SET; // count bytes from start of file
  cmd.l_start = this->lock_byte; // unlock my unique byte
  cmd.l_len = 1; // unlock 1 byte
  
  fcntl(this->lock_fd, F_SETLKW, &cmd);
}

void 
PSDevice::PutStageCommand(void* client, unsigned char* command, size_t len)
{
  StageLock();

#ifdef DEBUG
  printf( "P: StageDevice::PutCommand (%d,%d,%d) info at %p,"
	  " command at %p\n", 
          m_info->player_id.port, 
          m_info->player_id.code, 
          m_info->player_id.index, 
          m_info, command);
  fflush( stdout );
#endif

  // Check for overflows
  if(len > stage_device_commandsize)
    PLAYER_ERROR("invalid command length; ignoring command");
    
  // Copy the command
  memcpy(stage_device_command, command, len);

  // Set flag to indicate command has been set
  m_info->command_avail = len;

  // set timestamp for this command
  struct timeval tv;
  GlobalTime->GetTime(&tv);

  m_info->command_timestamp_sec = tv.tv_sec;
  m_info->command_timestamp_usec = tv.tv_usec;

  StageUnlock();
}

size_t 
PSDevice::GetStageData(void* client, unsigned char* data, size_t size,
                       uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  StageLock();
#ifdef DEBUG
  printf( "P: getting (%d,%d,%d) info at %p, data at %p, buffer len %d, %d bytes available, size parameter %d\n", 
          m_info->player_id.port, 
          m_info->player_id.code, 
          m_info->player_id.index, 
          m_info, stage_device_data,
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
  if(data_avail > PLAYER_MAX_MESSAGE_SIZE)
  {
    printf( "Available data (%d bytes) is larger than Player's"
            " maximum message size (%d bytes)\n", 
            data_avail, PLAYER_MAX_MESSAGE_SIZE );
  }

  // Check for overflows 2
  //
  if(data_avail > stage_device_datasize)
  {
    printf("warning: available data (%d bytes) > buffer size (%d bytes); "
           "ignoring data\n", data_avail, stage_device_datasize );
    StageUnlock();
    return 0;
  }
    
  // Check for overflows 3
  //
  if(data_avail > size)
  {
    printf("warning data available (%d bytes) > space in Player packet "
           "(%d bytes); ignoring data\n", data_avail, size);
    StageUnlock();
    return 0;
  }
    
  // Copy the data
  memcpy(data, stage_device_data, data_avail);

  // store the timestamp in the device, because other devices may
  // wish to read it
  stage_data_timestamp_sec = m_info->data_timestamp_sec;
  stage_data_timestamp_usec = m_info->data_timestamp_usec;

  // also return the timestamp to the caller
  if(timestamp_sec)
    *timestamp_sec = stage_data_timestamp_sec;
  if(timestamp_usec)
    *timestamp_usec = stage_data_timestamp_usec;
    
  StageUnlock();

  return data_avail;
}

#endif
