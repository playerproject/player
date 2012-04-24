Libmote 2.0
===========

This library allows the communication between a PC and a wsn node 
programmed with TinyOS 1.x and TinyOS 2.x, using the serial port. 

Authors: Jose Manuel Sanchez-Matamoros, Adrian Jimenez Gonzalez

****************************************************************
Example of usage:

	#include "MoteIF.h"
	using namespace mote;
	
	// Message definition
	typedef struct Msg{
		uint16_t nodeId;
		uint8_t value1;
		uint16_t value2;
		uint32_t value3;
		uint64_t value4;
	} __attribute__ ((packed)) Msg_t;
	
	// Definition of the struct of the message
	//    b --> 1 byte field
	//    s --> 2 bytes field
	//    w --> 4 bytes field
	//    l --> 8 bytes field
	namespace mote{
		template<> const char *defineStruct<Msg_t>(const Msg_t 
		&obj){ return "sbswl"; };
	}
	
	int main(){
		MoteIF mote;
		TOSMessage rmsg, smsg;
		Msg_t rm, sm;
		
		mote.open(SERIAL_PORT);
		mote.setTiming(0,20);
		// Version of TinyOS

		mote.setVersion(2);		
		rmsg.setVersion(2);
		smsg.setVersion(2);
		
		tyr{
		
			// Receive message from a node
			mote.getMessage(rmsg);
			// Parse message and store it in a struct
			rmsg.getData(&rm, sizeof(rm), defineStruct(rm));
		
			// Compose message
			smsg.composeX(addr, &sm, sizeof(sm), 
			defineStruct(sm));
			// Send message through the serial port
			mote.sendMessage(smsg);
		}catch(TimeoutException &e){
		}catch(CRCException &e){
		}
		mote.close();
		
		return 0;
	}
****************************************************************

