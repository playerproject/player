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

#include "device.h"
#include "arenalock.h"



////////////////////////////////////////////////////////////////////////////////
// Error, msg, trace macros

#define ENABLE_TRACE 1

#include <assert.h>

#define ASSERT(m) assert(m)
#define VERIFY(m) assert(m)

#include <stdio.h>

#define ERROR(m)  printf("Error : %s : %s\n", __PRETTY_FUNCTION__, m)
#define MSG(m)       printf("Msg   : %s : "m"\n", __PRETTY_FUNCTION__)
#define MSG1(m, a)   printf("Msg   : %s : "m"\n", __PRETTY_FUNCTION__, a)
#define MSG2(m, a, b) printf("Msg   : %s : "m"\n", __PRETTY_FUNCTION__, a, b)
#define MSG3(m, a, b, c) printf("Msg   : %s : "m"\n", __PRETTY_FUNCTION__, a, b, c)
#define MSG4(m, a, b, c, d) printf("Msg   : %s : "m"\n", __PRETTY_FUNCTION__, a, b, c, d)

#if ENABLE_TRACE
    #define TRACE0(m)    printf("Debug : %s : "m"\n", __PRETTY_FUNCTION__)
    #define TRACE1(m, a) printf("Debug : %s : "m"\n", __PRETTY_FUNCTION__, a)
    #define TRACE2(m, a, b) printf("Debug : %s : "m"\n", __PRETTY_FUNCTION__, a, b)
    #define TRACE3(m, a, b, c) printf("Debug : %s : "m"\n", __PRETTY_FUNCTION__, a, b, c)
    #define TRACE4(m, a, b, c, d) printf("Debug : %s : "m"\n", __PRETTY_FUNCTION__, a, b, c, d)
#else
    #define TRACE0(m)
    #define TRACE1(m, a)
    #define TRACE2(m, a, b)
    #define TRACE3(m, a, b, c)
    #define TRACE4(m, a, b, c, d)
#endif


////////////////////////////////////////////////////////////////////////////////
// Define length-specific data types
//
#define BYTE unsigned char


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
    public: virtual int GetData( unsigned char * );

    // Write data to the device
    //
    public: virtual void PutData( unsigned char * ) {};

    // Read a command from  the device
    //
    public: virtual void GetCommand( unsigned char * ) {};
    
    // Write a command to the device
    //
    public: virtual void PutCommand( unsigned char * , int);

    // Read configuration from the device
    //
    public: virtual int GetConfig( unsigned char * ) {return 0;};

    // Write configuration to the device
    //
    public: virtual void PutConfig( unsigned char * , int);
    
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



