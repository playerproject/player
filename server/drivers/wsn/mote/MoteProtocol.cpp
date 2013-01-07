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

#include "MoteProtocol.h"

#include <stdexcept>

namespace mote {
using std::cout;
/* *********** Packet implementation *********** */

uint8_t *Packet::dump( uint8_t *dst , int os) {
	assert( payload );
	uint8_t *base = dst;
	#ifdef debug
	printf("Type: 0x%02x\n",type);
#endif
	dst = mote::dump( type, dst );
	
#ifdef PUT_SEQN
	if(os == TOS2){
		uint8_t seqn = 0;
		dst = mote::dump( seqn, dst);
	}
#endif
		
	
	dst = payload->dump( dst , os);
	
	crc = computeCRC( base, 0, dst - base );
	
// 	if(os == TOS2 && checkNativeEndianness()==MP_LITTLE_ENDIAN ||
// 		  os == TOS1 && checkNativeEndianness()==MP_BIG_ENDIAN)
// 		crc = reverseEndian(crc);
// 	
	dst = mote::dump( crc, dst );

	return dst;
}

uint8_t *Packet::undump( uint8_t *src, int os) {
	uint8_t *base = src;

	src = mote::undump( type, src );
// 	printf("packet type: %d\n", (int)type);
	switch (type) {
		case P_ACK:	
			this->payload = &payloadACK;	
			break;
		case P_TOS2_PACKET_NO_ACK:
		case P_TOS2_PACKET_ACK:
		case P_TOS1_PACKET_ACK:
		case P_TOS1_PACKET_NO_ACK:
			this->payload = &payloadTOSMessage; 
			break;
		default:
			IOException e;
			e.append( "Packet::compose: Unsupported type" );
			throw e;
	}

	src = payload->undump( src , os);

	uint16_t srcCrc = computeCRC( base, 0, src - base );
	src = mote::undump( crc, src );
	
	
	if (crc != srcCrc ){
 		printf("CRC: expected %04x received %04x\n", srcCrc, crc );
		throw CRCException();
	}

	return	src;
}

void Packet::composeAck() {
	this->type = P_ACK;
	this->payload = &payloadACK;
}

void Packet::compose( uint8_t type, TOSMessage& message ) {
	this->type = type;
	this->payload = &payloadTOSMessage;
	payloadTOSMessage = message;
}

uint16_t Packet::computeCRCByte(uint16_t crc, uint8_t b){
	uint16_t i;
	crc = crc ^ (uint16_t)b << 8;
	for ( i= 0; i < 8; i++){
		if ((crc & 0x8000) == 0x8000)
			crc = crc << 1 ^ 0x1021;
		else
			crc = crc << 1;
	}
	return crc & 0xffff;
}

uint16_t Packet::computeCRC( uint8_t* packet, int index, int count){
	uint16_t crc = 0;

	while (count > 0)
	{
		crc = computeCRCByte(crc, packet[index++]);
		count--;
	}
	return crc;
}

void Packet::getTOSMessage( TOSMessage &message ) {
	message = payloadTOSMessage;
}



/* *********** ACKMessage implementation *********** */

uint8_t *ACKMessage::dump( uint8_t *dst , int os) { *(dst++) = LIMIT; return dst; };
uint8_t *ACKMessage::undump( uint8_t *src, int os ) { return ++src; };




/* *********** TOSMessage implementation *********** */


uint8_t *TOSMessage::dump( uint8_t *dst , int os) {

	if(os == TOS1){
		dst = mote::dump(addr, dst);
		dst = mote::dump(type, dst);
		dst = mote::dump(group, dst);
		dst = mote::dump(length, dst);
		memcpy( dst, data, length );
	}else if(os == TOS2){
		dst = mote::dump(n_msg, dst);
		dst = mote::dump(zero, dst);		
		dst = mote::dump(addr, dst);		
		dst = mote::dump(lnk_src, dst); // Source of the message.
		dst = mote::dump(length, dst);
		dst = mote::dump(group, dst);
		dst = mote::dump(type, dst);
		memcpy( dst, data, length );
	}
	return dst + length;
}

uint8_t *TOSMessage::undump( uint8_t *src , int os) {
// 	printf("undumping message; TOS version: %d\n", os);
	this->os = os;
	if (os == TOS1){
		src = mote::undump(addr, src);
		src = mote::undump(type, src);
		src = mote::undump(group, src);
		src = mote::undump(length, src);
	
	// 	printf( "addr %d\n", addr );
	// 	printf( "type %d\n", type );
	// 	printf( "group %d\n", group );
	// 	printf( "length %d\n", length );
	
	// 	assert( length < MAX_TOS_SIZE );
		memcpy( data, src, length );
	}else if(os == TOS2){
		uint8_t trash;
		// I don't know what is this field
		src = mote::undump(trash, src);
		
		// Destination address
		src = mote::undump(addr, src);
			
		src = mote::undump(lnk_src, src);
		src = mote::undump(length,src);
		src = mote::undump(group,src);
		src = mote::undump(type,src);
		
		// Reverse endian if necessary
		if(checkNativeEndianness()==MP_LITTLE_ENDIAN){
			addr = reverseEndian(addr);
			lnk_src = reverseEndian(lnk_src);
			length = reverseEndian(length);
			group = reverseEndian(group);
			type = reverseEndian(type);
		}
		
		memcpy( data, src, length );
	}
	return src + length;
}


/* void TOSMessage::compose(  uint8_t type, void *data, uint8_t length, uint16_t addr, uint8_t group ) {
	assert( length < MAX_TOS_SIZE );
	this->addr = addr;
	this->type = type;
	this->group = group;
	this->length = length;
	memcpy( this->data, data, length );
} */

void TOSMessage::compose(uint8_t type, void *data, const char *def, uint16_t addr, uint16_t lnk_src, uint8_t group ) {
  uint8_t length = 0;
  int nfields = strlen(def);
  for (int i = 0; i < nfields; i++) {
    if(def[i] == 'b') length += 1;
    else if(def[i] == 's') length += 2;
    else if(def[i] == 'w' || def[i] == 'f') length += 4;
    else if(def[i] == 'l') length += 8;
  }
  compose(type, data, length, def, addr, lnk_src, group );
  return;
}

void TOSMessage::compose(uint8_t type, void *data, uint8_t length, uint16_t addr, uint16_t lnk_src, uint8_t group ) {

	if (length > MAX_TOS_SIZE)
	  std::cerr << "TOSMessage::compose: Warning message too large (" << (int)length << " bytes)" << std::endl;
	// assert( length < MAX_TOS_SIZE );		

	if(os == TOS2 && group == TOS_DEFAULT_GROUP){
		group = TOS2_DEFAULT_GROUP;
	}	
	// Change representation of parameter if necessary
	if((os == TOS2 && checkNativeEndianness()==MP_LITTLE_ENDIAN) ||
	   (os == TOS1 && checkNativeEndianness()==MP_BIG_ENDIAN)){
		this->n_msg = reverseEndian(this->msg_count);
		this->zero = reverseEndian((uint8_t)0x00);
		this->addr = reverseEndian(addr);
		this->lnk_src = reverseEndian(lnk_src);
		this->length = reverseEndian(length);
		this->group = reverseEndian(group);
		this->type = reverseEndian(type);
		
		
		
	}else{
		this->n_msg = this->msg_count;
		this->zero = 0x00;
		this->addr = addr;
		this->lnk_src = lnk_src;
		this->type = type;
		this->group = group;
		this->length = length;
	}
	// It seems that it is not necessary to copy data fields changing the endian
	memcpy( this->data, data, length );
#ifdef debug
	printf("\nTOSMessage::compose (%d)\n n_msg: 0x%02x 0x%02x address: 0x%04x link source: 0x%04x AM(type): 0x%02x group: 0x%02x length: 0x%02x\n                         data: [ ",this->msg_count, this->n_msg, this->zero, this->addr, this->lnk_src = lnk_src, this->type, this->group, this->length);
	for(int i = 0; i < this->length; i++) {
		printf("0x%02x ", this->data[i]);
	}
	printf("]\n");
#endif
	this->msg_count++;
}

void TOSMessage::compose(uint8_t type, void *data, uint8_t length, const char *def, uint16_t addr, uint16_t lnk_src, uint8_t group ) {

	if (length > MAX_TOS_SIZE)
	  std::cerr << "TOSMessage::compose: Warning message too large (" << (int)length << " bytes)" << std::endl;
	// assert( length < MAX_TOS_SIZE );		

	if(os == TOS2 && group == TOS_DEFAULT_GROUP){
		group = TOS2_DEFAULT_GROUP;
	}
	
	// Change representation of parameter if necessary
	if((os == TOS2 && checkNativeEndianness()==MP_LITTLE_ENDIAN)||
	  (os == TOS1 && checkNativeEndianness()==MP_BIG_ENDIAN)){
		this->n_msg = reverseEndian(this->msg_count);
		this->zero = reverseEndian((uint8_t)0x00);
		this->addr = reverseEndian(addr);
		this->lnk_src = reverseEndian(lnk_src);
		this->length = reverseEndian(length);
		this->group = reverseEndian(group);
		this->type = reverseEndian(type);
		
		// It seems that it is not necessary to copy data fields changing the endian
		if (def == NULL) {
			std::cerr << "TOSMessage::compose: Error parsing structure" << std::endl;
			return;
		}
		int nfields = strlen(def);
		int ind = 0;
		for(int i=0;i<nfields;i++){
			int sf;
			if(def[i]=='b'){
				sf = 1;
				uint8_t c;
				memcpy(&c, (uint8_t *)data+ind, sf);
				c = reverseEndian(c);
				memcpy((uint8_t *)(this->data)+ind , &c, sf);
			}else if(def[i]=='s'){
				sf = 2;
				uint16_t c;
				memcpy(&c, (uint8_t *)data+ind, sf);
				c = reverseEndian(c);
				memcpy((uint8_t *)(this->data)+ind , &c, sf);
			}else if(def[i]=='w'){
				sf = 4;
				uint32_t c;
				memcpy(&c, (uint8_t *)data+ind, sf);
				c = reverseEndian(c);
				memcpy((uint8_t *)(this->data)+ind , &c, sf);
			}else if(def[i]=='l'){
				sf = 8;
				uint64_t c;
				memcpy(&c, (uint8_t *)data+ind, sf);
				c = reverseEndian(c);
				memcpy((uint8_t *)(this->data)+ind , &c, sf);
			}else if(def[i]=='f'){
				sf = 4;
				float c;
				memcpy(&c, (uint8_t *)data+ind, sf);
			//	c = reverseEndian(c);
				memcpy((uint8_t *)(this->data)+ind , &c, sf);
			}else{
				std::cerr << "TOSMessage::compose: Error parsing structure" << std::endl;
				return;
			}
			ind += sf;
		}
		
	}else{
		this->n_msg = this->msg_count;
		this->zero = 0x00;
		this->addr = addr;
		this->lnk_src = lnk_src;
		this->type = type;
		this->group = group;
		this->length = length;
		memcpy( this->data, data, length );
	}
#ifdef debug
	printf("\nTOSMessage::compose (%d)\n n_msg: 0x%02x 0x%02x address: 0x%04x link source: 0x%04x AM(type): 0x%02x group: 0x%02x length: 0x%02x data: [ ",this->msg_count, this->n_msg, this->zero, this->addr, this->lnk_src = lnk_src, this->type, this->group, this->length);
	for(int i = 0; i < this->length; i++) {
		printf("0x%02x ", this->data[i]);
	}
	printf("]\n");
#endif
	this->msg_count++;
}

void TOSMessage::getData( void *dst, int size) {
  // Seems like no reverse endian needed for data.
  memcpy( dst, data, size );
  return;
}
void TOSMessage::getData( void *dst,const char *def) {
  int size = 0;
  int nfields = strlen(def);
  for (int i = 0; i < nfields; i++) {
    if(def[i] == 'b') size += 1;
    else if(def[i] == 's') size += 2;
    else if(def[i] == 'w' || def[i] == 'f') size += 4;
    else if(def[i] == 'l') size += 8;
  }
  getData( dst, size, def);
  return;
}

void TOSMessage::getData( void *dst, int size, const char *def) {


	
	if(def == NULL){
		memcpy( dst, data, size );
	}else{
		// TinyOS 1.x is little endian, but 2.x version is big endian
		int endianness = checkNativeEndianness();
		if((endianness == MP_LITTLE_ENDIAN && os == TOS1) ||
			(endianness == MP_BIG_ENDIAN && os == TOS2)){
			memcpy( dst, data, size );
		}else{
			// Copy data fields changing the endian
			int nfields = strlen(def);
// 			printf("Number of fields: %d\n",nfields);
			int ind = 0;
			for(int i=0;i<nfields;i++){
				int sf;
				if(def[i]=='b'){
						sf = 1;
						uint8_t c;
						memcpy(&c, data+ind, sf);
						c = reverseEndian(c);
						memcpy((uint8_t *)(dst)+ind , &c, sf);
				}else if(def[i]=='s'){
						sf = 2;
						uint16_t c;
						memcpy(&c, data+ind, sf);
						c = reverseEndian(c);
						memcpy((uint8_t *)(dst)+ind , &c, sf);
				}else if(def[i]=='w'){
						sf = 4;
						uint32_t c;
						memcpy(&c, data+ind, sf);
						c = reverseEndian(c);
						memcpy((uint8_t *)(dst)+ind , &c, sf);
				}else if(def[i]=='l'){
						sf = 8;
						uint64_t c;
						memcpy(&c, data+ind, sf);
						c = reverseEndian(c);
						memcpy((uint8_t *)(dst)+ind , &c, sf);
				}else if(def[i]=='f'){
						sf = 4;
						float c;
						memcpy(&c, data+ind, sf);
				//		c = reverseEndian(c);
						memcpy((uint8_t *)(dst)+ind , &c, sf);
				}else{
					std::cerr << "TOSMessage::getData: Error parsing structure" << std::endl;
					return;
				}
				ind += sf;
			}
// 			printf("Input data: ");
// 			for(int i=0;i<size;i++)
// 				printf("%d ", ((uint8_t *)data)[i]);
// 			printf("\nOutput data: ");
// 			for(int i=0;i<size;i++)
// 				printf("%d ", ((uint8_t *)dst)[i]);
// 			printf("\n");
		}
		
 	}
	
	

}

void TOSMessage::setOS(int o){
	os = o;
}

/*TOSMessage::TOSMessage(){

	addr = 0x0000;
	type = 0x00;
	group = 0x00;
	length = 0x00;

	// Version of tinyos
	os = TOS1;
	lnk_src = 0x0000; // Link source (only TOS 2.x)
	uint8_t n_msg = 0x00; // Message number (only TOS 2.x)
	uint16_t zero = 0x0000; // Unknown field up to now (only TOS 2.x)
	
	uint8_t msg_count = 0; // Message count


}

TOSMessage::~TOSMessage(){

}
*/

/* *********** MoteProtocol implementation *********** */

MoteProtocol::MoteProtocol(){

	bufferIn = new char[IN_FRAME_SIZE];
	bufferOut = new char[OUT_FRAME_SIZE];

	ins = NULL;
	outs = NULL;

	status = ST_LOST;
	
	// Default OS
	os = TOS1;
}

MoteProtocol::~MoteProtocol(){

	delete[] bufferIn;
	delete[] bufferOut;

}


void MoteProtocol::bind( istream& is, ostream& outs ) {
	this->ins = &is;
	this->outs = &outs;
}



void MoteProtocol::getMessage( TOSMessage& message ) {	
	assert(ins);

	uint8_t b;
	bool received = false;
	bool timeout = false;
	
	int size = 0;
	for ( ;(!timeout) && (!received); )
	{
/*	  #ifdef debug
	  printf("\n");
	  #endif*/	
		switch(status) {

		case ST_LOST:
			(*ins) >> b;
			if ( !(*ins)  ) {
				timeout = true;
				break;
			}
/*#ifdef debug
   			printf("%02x ", b );
#endif*/			

			if( b == LIMIT ) { status = ST_SYNC; }
			break;

		case ST_SYNC:
			(*ins) >> b;
			if ( !(*ins) ) {
				timeout = true;
				status = ST_LOST;
				break;
			}
// #ifdef debug
//     			printf("%02x ", b );
// #endif

			if( b == LIMIT ) {
				status = ST_LOST;
				received = true;
				break;
			}

			if ( b == ESCAPE ) { 
				(*ins) >> b;
				if ( !(*ins)  ) {
					timeout = true;
					status = ST_LOST;
					break;
				}
// #ifdef debug
//     				printf("%02x ", b );
// #endif

 				b ^= 0x20;
			} 

			bufferIn[ size++ ] = b;
		}
	}

	if ( timeout ) {
	  	ins->clear();
		throw TimeoutException();
	}

#ifdef debug
	printf("\n");
 	printf("In : " );
 	for (int i = 0; i< size; i++ ) {
		uint8_t v = bufferIn[i];
  		printf("0x%02x ", v);
 	}
 	printf("\n");
#endif
	
	Packet packet;
	packet.undump( (uint8_t *)bufferIn, os );

	
	if ( os == TOS1 && packet.type == P_TOS1_PACKET_ACK ) {
		packet.composeAck();
		sendPacket( packet );
	}

	if ( os == TOS2 && packet.type == P_TOS2_PACKET_ACK ) {
		packet.composeAck();
		sendPacket( packet );
	}

	packet.getTOSMessage( message );
}


void MoteProtocol::sendMessage( TOSMessage& message, uint8_t type  ) {

	Packet packet;
	packet.compose( type, message );
	sendPacket( packet );
}


	

void MoteProtocol::sendPacket( Packet& packet ) {
	assert( outs );

	uint8_t *end = (uint8_t *)packet.dump( (uint8_t *)bufferOut , os);

	(*outs) << LIMIT;
	
#ifdef debug
	printf("Out: ");
	uint8_t c = LIMIT;
	printf("[ 0x%02x ", c);
#endif	
	uint8_t b;
	for (uint8_t *ptr = (uint8_t *)bufferOut; ptr != end; ptr++ ) {
		b = *ptr;
	
		if( ( b == LIMIT ) || (b == ESCAPE ) ) {
			(*outs) << ESCAPE;
#ifdef debug
			c = ESCAPE;
			printf("0x%02x ", c);
#endif
			b ^= 0x20;
		}
#ifdef debug
		c = b;
		printf("0x%02x ", b);
#endif
		(*outs) << b;
	}

	(*outs) << LIMIT;
	
	outs->flush();
	
#ifdef debug
 	c = LIMIT;
	printf("0x%02x ]\n", c);
#endif
}

void MoteProtocol::setOS(int v){
	if(v != TOS1 && v != TOS2)
		std::cerr << "Error selecting TinyOS version" << std::endl;
	else
		os = v;
}

int MoteProtocol::getOS(){
	return os;
}

int checkNativeEndianness(){
	uint16_t a=0x1234;
	if (*((uint8_t *)&a)==0x12) {
//		printf("BIG ENDIAN\n");
		return MP_BIG_ENDIAN; }
	else {
//	  printf("LITTLE ENDIAN\n");
		return MP_LITTLE_ENDIAN;
	}
}

}

