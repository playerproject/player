///////////////////////////////////////////////////////////////////////////
//
// File: stagedevice.hh
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

#ifndef STAGEDEVICE_HH
#define STAGEDEVICE_HH

// For length-specific types, error macros, etc
//
#include "rtk-types.hh"

#include "device.h"
#include "arenalock.h"


class CStageDevice : public CDevice
{
    // Minimal constructor
    // buffer points to a single buffer containing the data, command and configuration buffers.
    //
    public: CStageDevice(void *buffer, size_t data_len, size_t command_len, size_t config_len);

    // Initialise the device
    //
    public: virtual int Setup();

    // Terminate the device
    //
    public: virtual int Shutdown();
    
    // Read data from the device
    //
    public: virtual size_t GetData( unsigned char *, size_t maxsize);

    // Write data to the device
    //
    public: virtual void PutData( unsigned char *, size_t maxsize) {};

    // Read a command from  the device
    //
    public: virtual void GetCommand( unsigned char *, size_t maxsize) {};
    
    // Write a command to the device
    //
    public: virtual void PutCommand( unsigned char * , size_t maxsize);

    // Read configuration from the device
    //
    public: virtual size_t GetConfig( unsigned char *, size_t maxsize) {return 0;};

    // Write configuration to the device
    //
    public: virtual void PutConfig( unsigned char *, size_t maxsize);
    
    // Get a lockable object for synchronizing data exchange
    //
    public: virtual CLock* GetLock( void ) {return &m_lock;};

    // Simulator lock
    //
    private: CArenaLock m_lock;

    // Pointer to shared info buffers
    //
    private: BYTE *m_info_buffer;
    private: size_t m_info_len;

    // Pointer to shared data buffers
    //
    private: void *m_data_buffer;
    private: size_t m_data_len;

    // Pointer to shared command buffers
    //
    private: void *m_command_buffer;
    private: size_t m_command_len;

    // Pointer to shared config buffers
    //
    private: void *m_config_buffer;
    private: size_t m_config_len;
};

class CStageSonarDevice : public CStageDevice
{
public: 
  CStageSonarDevice(void *buffer, size_t data_len, 
		    size_t command_len, size_t config_len);
  
  
  // override the CStageDevice GetData to stuff this data
  // into the P2OS buffer
  //
  virtual int GetData( unsigned char * );
};

#endif



