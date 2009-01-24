/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003
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

/** @ingroup drivers Drivers */
/** @{ */
/*
 *
The flexiport driver provides access to a data communications device (such as a serial port or a TCP
network port) via the Gearbox flexiport library. Any data received over this device is published,
and any writes to this driver are written to the device. It does not process the data in any way.

@par Compile-time dependencies

 - flexiport (from Gearbox, see http://gearbox.sourceforge.net)

@par Provides

 - @ref opaque

@par Requires

 - none

@par Configuration requests

 - none

@par Configuration file options

 - See @ref Properties.

@par Properties (may also be set in the configuration file)

 - portopts (string)
   - Default: "type=serial,device=/dev/ttyS0,timeout=1"
   - Options to create the Flexiport port with.

 - buffer_size (integer)
   - The size of the buffer to be used when reading. This is the maximum that can be read in one
     read command
   - Default: 4096

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
  name "flexiport"
  provides ["opaque:0]
  portopts "type=serial,device=/dev/ttyACM0"
)

@endverbatim

@author Geoffrey Biggs

*/
/** @} */


#include <libplayercore/playercore.h>
#include <flexiport/flexiport.h>
#include <flexiport/port.h>
#include <string>

const int DEFAULT_OPAQUE_BUFFER_SIZE    = 4096;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Driver object
////////////////////////////////////////////////////////////////////////////////////////////////////

class Flexiport : public ThreadedDriver
{
	public:
		Flexiport (ConfigFile* cf, int section);
		virtual ~Flexiport (void);

		virtual int MainSetup (void);
		virtual void MainQuit (void);
		virtual int ProcessMessage (QueuePointer &respQueue, player_msghdr *hdr, void *data);

	protected:
		IntProperty _bufferSize;
		StringProperty _portOptions;

		uint8_t *_receiveBuffer;

		flexiport::Port *_port;

	private:
		virtual int CreatePort (void);
		virtual void Main();
		virtual void ReadData();
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor/destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

Flexiport::Flexiport (ConfigFile* cf, int section)
	: ThreadedDriver (cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_OPAQUE_CODE),
	_bufferSize ("buffer_size", DEFAULT_OPAQUE_BUFFER_SIZE, 0),
	_portOptions ("portopts", "type=serial,device=/dev/ttyS0,timeout=1", 0),
	_port (NULL)
{
	RegisterProperty ("buffer_size", &_bufferSize, cf, section);
	RegisterProperty ("portopts", &_portOptions, cf, section);

	if ((_receiveBuffer = new uint8_t[_bufferSize]) == NULL)
	{
		PLAYER_ERROR ("Failed to allocate receive buffer.");
	}
}

Flexiport::~Flexiport (void)
{
	if (_receiveBuffer != NULL)
		delete[] _receiveBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Driver implementation
////////////////////////////////////////////////////////////////////////////////////////////////////

int Flexiport::CreatePort (void)
{
	try
	{
		if (_port != NULL)
			delete _port;
		_port = flexiport::CreatePort (_portOptions.GetValue ());
		_port->Open ();
	}
	catch (flexiport::PortException e)
	{
		PLAYER_ERROR1 ("flexiport: Failed to create port instance: %s", e.what ());
		return 1;
	}

	return 0;
}

void Flexiport::Main (void)
{
	while (true)
	{
		pthread_testcancel ();
		ProcessMessages ();
		ReadData ();
		usleep (100000);
	}
}

int Flexiport::ProcessMessage (QueuePointer &respQueue, player_msghdr *hdr, void *data)
{
	// Check for capability requests
	HANDLE_CAPABILITY_REQUEST (device_addr, respQueue, hdr, data,
			PLAYER_MSGTYPE_REQ, PLAYER_CAPABILTIES_REQ);

	// Property handlers that need to be done manually due to calling into the urg_nz library.
	if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_SET_INTPROP_REQ, device_addr))
	{
		player_intprop_req_t *req = reinterpret_cast<player_intprop_req_t*> (data);
		// Change in the baud rate
		if (strncmp (req->key, "buffer_size", 11) == 0)
		{
			// Reallocate the buffer
			uint8_t *newBuffer;
			if ((newBuffer = new uint8_t[req->value]) == NULL)
			{
				PLAYER_ERROR ("flexiport: Failed to reallocate receive buffer.");
				Publish (device_addr, respQueue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_SET_INTPROP_REQ,
						NULL, 0, NULL);
				return 0;
			}

			if (_receiveBuffer != NULL)
				delete[] _receiveBuffer;
			_receiveBuffer = newBuffer;
			_bufferSize.SetValueFromMessage (data);

			Publish (device_addr, respQueue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_SET_INTPROP_REQ, NULL,
					0, NULL);
			return 0;
		}
	}
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_SET_STRPROP_REQ, device_addr))
	{
		player_strprop_req_t *req = reinterpret_cast<player_strprop_req_t*> (data);
		// Change in the port options
		if (strncmp (req->key, "portopts", 8) == 0)
		{
			_portOptions.SetValueFromMessage (data);
			if (CreatePort () != 0)
			{
				PLAYER_ERROR ("flexiport: Failed to create new port with new options.");
				Publish (device_addr, respQueue, PLAYER_MSGTYPE_RESP_NACK, PLAYER_SET_STRPROP_REQ,
						NULL, 0, NULL);
				return 0;
			}
			Publish (device_addr, respQueue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_SET_STRPROP_REQ, NULL,
					0, NULL);
			return 0;
		}
	}

	// Standard opaque messages
	else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_OPAQUE_CMD_DATA, device_addr))
	{
		player_opaque_data_t *recv = reinterpret_cast<player_opaque_data_t *> (data);
		if (recv->data_count <= 0)
			return 0;

		_port->Flush ();

		try
		{
			size_t written;
			if ((written = _port->Write (recv->data, recv->data_count)) < recv->data_count)
			{
				PLAYER_ERROR2 ("flexiport: Wrote less data than given: %d < %d", written,
								recv->data_count);
			}
		}
		catch (flexiport::PortException e)
		{
			PLAYER_ERROR1 ("flexiport: Error writing to port: %s", e.what ());
		}

		return 0;
	}

	return -1;
}

void Flexiport::ReadData (void)
{
	size_t read = _port->Read (_receiveBuffer, _bufferSize);

	if (read <= 0)
	{
		// Timed out/no data available
		return;
	}

	player_opaque_data_t data;
	data.data_count = read;
	data.data = _receiveBuffer;
	Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_OPAQUE_DATA_STATE,
			reinterpret_cast<void*> (&data));
}

int Flexiport::MainSetup (void)
{
	if (CreatePort () != 0)
		return 1;

	return 0;
}

void Flexiport::MainQuit (void)
{
	delete _port;
	_port = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Driver management functions
////////////////////////////////////////////////////////////////////////////////////////////////////

Driver* FlexiportInit (ConfigFile* cf, int section)
{
	return reinterpret_cast <Driver*> (new Flexiport (cf, section));
}

void flexiport_Register (DriverTable* table)
{
	table->AddDriver ("flexiport", FlexiportInit);
}
