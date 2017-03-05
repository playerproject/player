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

#ifndef MOTEPROTOCOL_H
#define MOTEPROTOCOL_H

#include <stdint.h>
#include <memory.h>
#include <cstdio>
#include <iostream>
#include <assert.h>

using std::istream;
using std::ostream;
using std::endl;

#include "MoteException.h"

//#define debug

namespace mote {


// message_t type dispatch

enum {
  TOS_SERIAL_ACTIVE_MESSAGE_ID = 0,
  TOS_SERIAL_CC1000_ID = 1,
  TOS_SERIAL_802_15_4_ID = 2,
  TOS_SERIAL_UNKNOWN_ID = 255,
};

enum { OS_TRANSPARENT = 0, TOS1 = 1, TOS2 = 2, CONTIKI = 3 };

#define LIMIT ((char)0x7E)
#define ESCAPE ((char)0x7D)

#define IN_FRAME_SIZE 100
#define OUT_FRAME_SIZE 100

// packet types
#define P_ACK ((char)0x40)

#define P_TOS1_PACKET_NO_ACK ((char)0x42)
#define P_TOS1_PACKET_ACK ((char)0x41)
#define P_TOS2_ACK ((char)0x43)
#define P_TOS2_PACKET_NO_ACK ((char)0x45)
#define P_TOS2_PACKET_ACK ((char)0x44)
#define P_UNKNOWN ((char)0xFF)

// Maximum TinyOS message data size  TODO revisarlo, me lo he inventao
#define MAX_TOS_SIZE 100
#define TOS_BROADCAST 0xFFFF
#define TOS1_DEFAULT_GROUP 0x7D
#define TOS2_DEFAULT_GROUP 0x00
#define TOS_DEFAULT_GROUP 0x7D
// #define TOS2_DEFAULT_GROUP 0x22
// #define TOS2_DEFAULT_GROUP 0x66

#define MP_LITTLE_ENDIAN 1
#define MP_BIG_ENDIAN 0
	 
//////// FLAGS FOR DEBUGGING ////////

// Plot output before sending to the serial port
// #define DEBUG_OUTPUT			 
			 
// Plot input when read from serial port
// #define DEBUG_INPUT

// Include SEQN field in the TinyOS 2 packet
// #define PUT_SEQN
//////////////
			 	 
struct Dumpable;
struct Packet;
struct ACKMessage;
struct TOSMessage;

/** \brief Implements the basic sending and receiving capabilities on iostreams.

	This class can be used for debugging purpouses, as can be binded to any kind
	of input or output streams, easily sending the packets to cout and reading them
	from cin. For daily purpouses MoteIF should be used instead.

	\code
	#include "MoteProtocol.h"

	#define MY_TYPE 37
	struct MyType {
		uint8_t a;
		uint16_t b;
	}
	
	int main() {

	using namespace mote;

	MoteProtocol mote;
	TOSMessage message;
	MyType myType;
	
	cin.unsetf( std::ios_base::skipws )
	mote.bind( cin, cout );

	try {
		myType.a = 42;
		myType.b = 4242;
		message.compose( MY_TYPE, &myType, sizeof(myType));
		
		mote.sendMessage( message, P_PACKET_NO_ACK );

	} catch (...) {
		// ...exception handling stuff
		throw;
	}

	return 0;
	}
	\endcode
*/
class MoteProtocol {

public:
	MoteProtocol();
	~MoteProtocol();

	void bind( istream& is, ostream& os );

	void getMessage( TOSMessage& message );
	void sendMessage(TOSMessage& message, uint8_t type );
	
	/**
	 * Set TinyOS version
	 * @param TinyOS version. It can be 1 or 2
	 */
	void setOS(int v);
	/**
	 * Get current TinyOS version
	 * @return TinyOS version
	 */
	int getOS();
protected:
	
	void sendPacket( Packet& packet );

	enum Status { ST_LOST, ST_SYNC };

	Status status;

	char *bufferIn;
	char *bufferOut;
	istream *ins;
	ostream *outs;

	int os;
	
};

/** \brief Interface that ensures serialization from and to a memory region.
	
Anything able to be dumped to or read from a buffer. Useful for messages containing
messages, containing messages, ...
*/
struct Dumpable {
	virtual uint8_t *dump( uint8_t *dst , int os = 1) = 0;
	virtual uint8_t *undump( uint8_t *src, int os = 1 ) = 0;
};



struct ACKMessage: public Dumpable {
	uint8_t *dump( uint8_t *dst , int os = 1);
	uint8_t *undump( uint8_t *src, int os = 1 );
};



/// \brief TinyOS generic message.
struct TOSMessage: public Dumpable {

	uint16_t addr;
	uint8_t type;
	uint8_t group;
	uint8_t length ;
	uint8_t data[MAX_TOS_SIZE];
	
	// Version of tinyos
	uint8_t os;
	uint16_t lnk_src; // Link source (only TOS 2.x)
	uint8_t n_msg; // Message number (only TOS 2.x)
	uint8_t zero; // Unknown field up to now (only TOS 2.x)
	
	uint8_t msg_count; // Message count

	TOSMessage(){
		addr = TOS_BROADCAST;
		type = 0;
		group = TOS_DEFAULT_GROUP;
		length = 0;
		os = TOS1;
		lnk_src = 0; // Link source (only TOS 2.x)
		n_msg = 0; // Message number (only TOS 2.x)
		zero = 0;
		msg_count = 0; //0x0e
		
	}

	void compose( uint8_t type, 
		      void *data, 
		      uint8_t length, 
		      uint16_t addr = TOS_BROADCAST, 
		      uint16_t lnk_src = 0,
		      uint8_t group = TOS_DEFAULT_GROUP );
		      
	void compose( uint8_t type, 
		      void *data, 
		      uint8_t length, 
		      const char *def,
		      uint16_t addr = TOS_BROADCAST, 
		      uint16_t lnk_src = 0,
		      uint8_t group = TOS_DEFAULT_GROUP );
			
	void compose( uint8_t type, 
		      void *data,
		      const char *def,
		      uint16_t addr = TOS_BROADCAST, 
		      uint16_t lnk_src = 0,
		      uint8_t group = TOS_DEFAULT_GROUP );

	void getData ( void *dst, int size, const char *def );
	void getData ( void *dst, int size);
	void getData ( void *dst, const char *def );
	
	uint8_t *dump( uint8_t *dst , int os = 1);
	uint8_t *undump( uint8_t *src , int os = 1);
	
	void setOS(int v);
};



/** \brief Data packet as described in the TinyOS serial protocol specifications

	The packets can be sent or received by the MoteIF class. Two payload types are 
	currently suported, TinyOSMessages and the ACKMessage for packets requiring
	low-level acknownledgement.
*/
struct Packet: public Dumpable {

	friend class MoteProtocol;

	uint8_t type;
	Dumpable *payload;
	
	void compose( uint8_t type, TOSMessage& message );

	uint8_t *dump( uint8_t *dst , int os = 1);
	uint8_t *undump( uint8_t *src, int os = 1 );

protected:
	void composeAck();
	void getTOSMessage( TOSMessage &message );
	uint16_t computeCRCByte(uint16_t crc, uint8_t b);
	uint16_t computeCRC(uint8_t *packet, int index, int count);


	// possible payloads
	TOSMessage payloadTOSMessage;
	ACKMessage payloadACK;

	uint16_t crc;
};




inline uint8_t *dump(uint8_t d, uint8_t *dst) { *(dst++) = d; return dst; }
inline uint8_t *dump(int8_t d, uint8_t *dst) { *(dst++) = d; return dst; }
inline uint8_t *dump(uint16_t d, uint8_t *dst) { *((uint16_t *)dst) = d; return dst+2; }
inline uint8_t *dump(int16_t d, uint8_t *dst) { *((int16_t *)dst) = d; return dst+2; }

inline uint8_t *undump(uint8_t& d, uint8_t *dst) { d = *(dst++); return dst; }
inline uint8_t *undump(int8_t& d, uint8_t *dst) { d = *(dst++); return dst; }
inline uint8_t *undump(uint16_t& d, uint8_t *dst) { d = *((uint16_t *)dst); return dst+2; }
inline uint8_t *undump(int16_t& d, uint8_t *dst) { d = *((int16_t *)dst); return dst+2; }

inline ostream& operator<<(ostream& os, TOSMessage& message ) {

	os << "addr " << (int)message.addr << endl;
	os << "type " << (int)message.type << endl;
	os << "group " << (int)message.group << endl;
	os << "data length " << (int)message.length << endl;

	for (int i = 0; i< message.length; i++ ) {
		os << " " << (unsigned int)message.data[i];
	}

	return os;
}


// Function to reverse endianness
template <typename T>
T reverseEndian(T v){
	int s = sizeof(T);
	T r = 0;
	uint8_t *pv = (uint8_t *)&v;
	uint8_t *pr = (uint8_t *)&r;
	
	for(int i=0;i<s;i++){
		pr[i] = pv[s-i-1];
	}	
	return r;
}

/**
 * Default template to define the fields of an structure, in order
 * manage the bytes properly (little or big endian)
 * 		int/uint8  --> 'b' (1 byte)
 * 		int/uint16 --> 's' (2 bytes)
 * ..		int/uint32 --> 'w' (4 bytes)
 * 		int/uint64 --> 'l' (8 bytes)
 *		float	   --> 'f' (4 bytes)
 *		double	   --> 'd' (8 bytes)
 *		pointer    --> 'p'
 *	end of pointer     --> 'r'
 */
template <typename T>
char *defineStruct(const T& obj) {return NULL;}

/**
 * Check if the CPU is little endian or big endian
 */
int checkNativeEndianness();

}


#endif
