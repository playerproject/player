/* *  Player - One Hell of a Robot Server *  Copyright (C) 2003
 *     Brian Gerkey
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

/** @ingroup drivers*/
/** @{ */
/** @defgroup driver_tcpstream tcpstream
 *  @brief TCP streaming driver

The tcpstream driver is based on the serialstream driver. It reads from a socket
continuously and publishes the data. Currently this is usable with the SickS3000 driver
and the Nav200 driver. This driver does no interpretation of data output, merely reading
it and publishing it, or, if it is sent a data command it will write whatever it recieves
to the socket.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_opaque

@par Requires

- none

@par Configuration requests

- none

@par Configuration file options

- ip (string)
  - Default: "10.99.10.6"
  - IP address to connect to.

- port (integer)
  - Default: "4002"
  - TCP port to connect to.

- buffer_size (integer)
  - The size of the buffer to be used when reading, this is the maximum that can be read in one read command
  - Default 4096

@par Example

@verbatim
driver
(
  name "sicks3000"
  provides ["laser:0"]
  requires ["opaque:0"]
)

driver
(
  name "tcpstream"
  provides ["opaque:0]
  ip "10.99.10.6"
  port "4002"
)

@endverbatim

@author David Olsen, Toby Collett, Inro Technologies

*/
/** @} */


// ONLY if you need something that was #define'd as a result of configure
// (e.g., HAVE_CFMAKERAW), then #include <config.h>, like so:
/*
#include <config.h>
*/


#include <config.h>

#include <assert.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h> // for htons etc
#include <netinet/in.h>

#include <libplayercore/playercore.h>

#define DEFAULT_TCP_OPAQUE_BUFFER_SIZE 4096
#define DEFAULT_TCP_OPAQUE_IP "127.0.0.1"
#define DEFAULT_TCP_OPAQUE_PORT 4002

////////////////////////////////////////////////////////////////////////////////
// The class for the driver
class TCPStream : public ThreadedDriver
{
  public:

    // Constructor; need that
    TCPStream(ConfigFile* cf, int section);
    virtual ~TCPStream();

    // Must implement the following methods.
    virtual void MainQuit();

    // This method will be invoked on each incoming message
    virtual int ProcessMessage(QueuePointer &resp_queue,
                               player_msghdr * hdr,
                               void * data);

  private:

    // Main function for device thread.
    virtual void Main();

    // Update the data
    virtual void ReadData();

    // Open the terminal
    // Returns 0 on success
    virtual int OpenTerm();

    // Close the terminal
    // Returns 0 on success
    virtual int CloseTerm();

  protected:
	  int sock;

    uint8_t * rx_buffer;

    // Properties
    IntProperty buffer_size;
    StringProperty ip;
    IntProperty port;

    bool connected;

    // This is the data we store and send
    player_opaque_data_t mData;

};

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver*
TCPStream_Init(ConfigFile* cf, int section)
{
  // Create and return a new instance of this driver
  return((Driver*)(new TCPStream(cf, section)));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void tcpstream_Register(DriverTable* table)
{
  table->AddDriver("tcpstream", TCPStream_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
TCPStream::TCPStream(ConfigFile* cf, int section)
    : ThreadedDriver(cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN,
             PLAYER_OPAQUE_CODE),
             buffer_size ("buffer_size", DEFAULT_TCP_OPAQUE_BUFFER_SIZE, 0),
             ip ("ip", DEFAULT_TCP_OPAQUE_IP, 0),
             port ("port", DEFAULT_TCP_OPAQUE_PORT, 0)
{
	this->RegisterProperty ("buffer_size", &this->buffer_size, cf, section);
	this->RegisterProperty ("ip", &this->ip, cf, section);
	this->RegisterProperty ("port", &this->port, cf, section);

	rx_buffer = new uint8_t[buffer_size];
	assert(rx_buffer);

	connected = false;

	return;
}

TCPStream::~TCPStream()
{
	delete [] rx_buffer;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
void TCPStream::MainQuit()
{
  CloseTerm();
}

int TCPStream::ProcessMessage(QueuePointer & resp_queue,
                                 player_msghdr* hdr,
                                 void* data)
{
	// Process messages here.  Send a response if necessary, using Publish().
	// If you handle the message successfully, return 0.  Otherwise,
	// return -1, and a NACK will be sent for you, if a response is required.

	if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_OPAQUE_CMD_DATA, this->device_addr))
	{
	    player_opaque_data_t * recv = reinterpret_cast<player_opaque_data_t * > (data);

	    if (recv->data_count) // If there is something to send.
	    {
	    	int result;
	    	do
	    	{
	    		result = send(sock, recv->data, recv->data_count, 0);
	    	} while (result < 0 && errno == EAGAIN);
	    	if (result < (int)recv->data_count)
	    	{
	    		PLAYER_ERROR2("Error sending data (%d, %s)", errno, strerror(errno));
	    		// Attempt to reconnect.
	    		CloseTerm();
	    	}
	    }

	    return (0);
	}

	return(-1);
}



////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void TCPStream::Main()
{
  // The main loop; interact with the device here
  for(;;)
  {
    if (!connected)
    {
      OpenTerm();
    }
    // we read/connect first otherwise we we wait when we have no data connection
    if (connected)
    {
      // Reads the data from the tcp server and then publishes it
      ReadData();
    }

    ProcessMessages();
    Wait(1);

  }
}

////////////////////////////////////////////////////////////////////////////////
// Open the terminal
// Returns 0 on success
int TCPStream::OpenTerm()
{
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0)
	{
		PLAYER_ERROR("Failed to create socket.");
		return -1;
	}

	sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(ip.GetValue());
	address.sin_port = htons(port.GetValue());
	if (connect(sock, (sockaddr*) &address, sizeof(address)) < 0)
	{
		PLAYER_ERROR("Failed to connect");
		return -1;
	}

	int flags = fcntl(sock, F_GETFL);
	if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		close(sock);
		PLAYER_ERROR("Error changing socket to be non-blocking");
		return -1;
	}

	PLAYER_MSG0(2, "TCP Opaque Driver connected");

	connected = true;
	AddFileWatch(sock);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Close the terminal
// Returns 0 on success
//
int TCPStream::CloseTerm()
{
  RemoveFileWatch(sock);
  close(sock);

  connected = false;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Read range data from laser
//
void TCPStream::ReadData()
{
  // Read a packet from the laser
  //
  int len = recv(sock, rx_buffer, buffer_size, 0);
  if (len < 0 && errno == EAGAIN)
  {
     // PLAYER_MSG0(2, "empty packet");
    return;
  }

  if (len <= 0)
  {
    PLAYER_ERROR2("error reading from socket: %d %s", errno, strerror(errno));
    CloseTerm();
    return;
  }

  if (len == buffer_size)
  {
    PLAYER_WARN("tcpstream:ReadData() filled entire buffer, increasing buffer size will lower latency");
  }

  assert(len <= int(buffer_size));
  mData.data_count = len;
  mData.data = rx_buffer;
  Publish(this->device_addr, PLAYER_MSGTYPE_DATA, PLAYER_OPAQUE_DATA_STATE, reinterpret_cast<void*>(&mData));
}

