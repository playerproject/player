///////////////////////////////////////////////////////////////////////////
//
// Desc: Gazebo (simulator) camera driver
// Author: Pranav Srivastava
// Date: 16 Sep 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>       // for atoi(3)

#include "player.h"
#include "device.h"
#include "drivertable.h"

#include "gazebo.h"
#include "../../gazebo/gz_client.h"

// there must be a neater way of doing it.. but ftb
class CMGzCamera : public CDevice
{
  // Constructor
  public: CMGzCamera(char* interface, ConfigFile* cf, int section);

  // Destructor
  public: virtual ~CMGzCamera();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Data
  public: virtual size_t GetData(void* client, unsigned char* dest, size_t len,
                                 uint32_t* timestamp_sec, uint32_t* timestamp_usec);
  public: int width,height;

  // Gazebo device id
  private: const char *gz_id;

  // Gazebo client object
  public: gz_client_t *client;
  
  // Gazebo Interface
  public: gz_camera_t *iface;
};

