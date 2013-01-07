/********************************************************************
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 ********************************************************************/
/*********************************************************************
 * TinyOS data structures.
 * Portions borrowed from the TinyOS project (http://www.tinyos.net), 
 * distributed according to the Intel Open Source License.
 *********************************************************************/
/***************************************************************************
 * Desc: Library for generic Crossbow WSN nodes communication
 * Author: Jose Manuel Sanchez Matamoros, Adrian Jimenez Gonzalez
 * Date: 15 Aug 2011
 **************************************************************************/

#ifndef MOTEIF_H
#define MOTEIF_H

#include <SerialStream.h>
#include <iostream>
#include <iomanip>

#include "MoteProtocol.h"

namespace mote {

using namespace LibSerial;


/** \brief This class provides serial-port-ready access to the mote.

	This class is a wrapper for MoteProtocol, and includes a SerialStream for
	serial port handling. It makes very easy to receive a message of any kind via
	serial port.
	\code 
		// myType is an instance of the struct MyType, for example.

		TOSMessage message;
		MoteIF mote;

		mote.open();
		mote.getMessage( message );

		if (message.type == MY_TYPE ) {
			message.getData( &myType, sizeof( myType ));
		}

		mote.close();
	\endcode
*/
class MoteIF {

public:	
	~MoteIF();

	void open( const char *tty = "/dev/ttyUSB0", SerialStreamBuf::BaudRateEnum baud_rate = SerialStreamBuf::BAUD_115200 );
	void close();

	void getMessage( TOSMessage& message );
	void sendMessage( TOSMessage& message, uint8_t type = 0 );

	bool setTiming( short vmin, short vtime );

	void setOS(int v);
	int getOS();
protected:
	int os;
	
	MoteProtocol protocol;
	SerialStream serial;
//	std::ifstream *ifs;

};


inline void MoteIF::getMessage( TOSMessage& message ) { 

	protocol.getMessage( message ); 
}

/** \brief Encapsulates a TinyOS message in a packet and sends it to the mote.
  \param message Message to be sent as payload in the packet.
  \param type Type of the packet. Should be { P_PACKET_NO_ACK, P_PACKET_ACK }. The default
	value is P_PACKET_NO_ACK, i.e, acknownledgement not required.
*/
inline	void MoteIF::sendMessage( TOSMessage& message, uint8_t type ) { 
	
	if(type == 0){
		if(os == TOS1)
			type = P_TOS1_PACKET_NO_ACK;
		else if(os == TOS2)
			type = P_TOS2_PACKET_NO_ACK;
	} else {
		if(os == TOS1)
			type = P_TOS1_PACKET_ACK;
		else if(os == TOS2)
			type = P_TOS2_PACKET_ACK;
	}
	
	protocol.sendMessage( message, type ); 
};

}

#endif
