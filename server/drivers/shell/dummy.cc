/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Andrew Howard
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
 * Desc: Driver generating dummy data
 * Author: Andrew Howard
 * Date: 15 Sep 2004
 * CVS: $Id$
 */

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_dummy Dummy driver

The dummy driver generates dummy data and consumes dummy commands for
any interface; useful for debugging client libraries and benchmarking
server performance


@par Interfaces
- This driver will support any interface.

@par Supported configuration requests

- This driver will consume any configuration requests.

@par Configuration file options

- rate
  - Data rate (Hz); e.g., rate 20 will generate data at 10Hz.
      
@par Example 

@verbatim
driver
(
  name "dummy"
  devices ["laser:0"]  # Generate dummy laser data
  rate 75              # Generate data at 75Hz
)
@endverbatim
*/
/** @} */
  
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "player.h"
#include "driver.h"
#include "drivertable.h"
#include "deviceregistry.h"


// The logfile driver
class Dummy: public Driver 
{
  // Constructor
  public: Dummy(ConfigFile* cf, int section);

  // Destructor
  public: ~Dummy();

  // Initialize the driver
  public: virtual int Setup();

  // Finalize the driver
  public: virtual int Shutdown();

  // Device thread
  private: virtual void Main(void);

  // Device id
  private: player_device_id_t local_id;

  // Data rate
  private: double rate;
  
  // Dummy data buffer
  private: size_t data_len, cmd_len;
  private: char *data_buffer;
};



////////////////////////////////////////////////////////////////////////////
// Create a driver for reading log files
Driver* ReadDummy_Init(ConfigFile* cf, int section)
{
  return ((Driver*) (new Dummy(cf, section)));
}


////////////////////////////////////////////////////////////////////////////
// Device factory registration
void Dummy_Register(DriverTable* table)
{
  table->AddDriver("dummy", ReadDummy_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////
// Constructor
Dummy::Dummy(ConfigFile* cf, int section)
    : Driver(cf, section)
{
  int num_ids;
  player_device_id_t *ids;
  
  // Parse out the device id's
  if ((num_ids = cf->ParseDeviceIds(section, &ids)) < 0)
  {
    this->SetError(-1);
    return;
  }

  // We can only support one interface at a time
  if (cf->ReadDeviceId(&this->local_id, ids, num_ids, -1, 0) < 0)
  {
    PLAYER_ERROR("no interfaces specified");
    this->SetError(-1);
    return;
  }
  if (cf->UnusedIds(section, ids, num_ids))
  {
    PLAYER_WARN("dummy driver support one interface at a time");
  }

  free(ids);
    
  // Figure out how big our buffers need to be
  switch (this->local_id.code)
  {
    case PLAYER_LASER_CODE:
      this->data_len = sizeof(player_laser_data_t);
      this->cmd_len = 0;
      break;
    case PLAYER_POSITION_CODE:
      this->data_len = sizeof(player_position_data_t);
      this->cmd_len = sizeof(player_position_cmd_t);
      break;
    default:
      PLAYER_ERROR1("unsupported interface [%s]",
                    lookup_interface_name(0, this->local_id.code));
      this->SetError(-1);
      return;
  }

  // Add our interface
  if (this->AddInterface(this->local_id, PLAYER_ALL_MODE, this->data_len,
                         this->cmd_len, 10, 10) != 0)
  {
    this->SetError(-1);
    return;
  }

  // Create data buffer
  this->data_buffer = (char*) calloc(1, sizeof(PLAYER_MAX_MESSAGE_SIZE));
  
  // Data rate
  this->rate = cf->ReadFloat(section, "rate", 10);

  return;
}


////////////////////////////////////////////////////////////////////////////
// Destructor
Dummy::~Dummy()
{
  free(this->data_buffer);

  return;
}


////////////////////////////////////////////////////////////////////////////
// Initialize driver
int Dummy::Setup()
{
  // Start device thread
  this->StartThread();

  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Finalize the driver
int Dummy::Shutdown()
{
  // Stop the device thread
  this->StopThread();
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void Dummy::Main(void)
{
  struct timespec req;

  req.tv_sec = (time_t) (1.0 / this->rate);
  req.tv_nsec = (long) (fmod(1e9 / this->rate, 1e9));

  printf("%ld %ld\n", req.tv_sec, req.tv_nsec);
  
  while (1)
  {
    pthread_testcancel();
    if (nanosleep(&req, NULL) == -1)
      continue;

    this->PutData(this->local_id, this->data_buffer, this->data_len, NULL);
  }
  return;
}
