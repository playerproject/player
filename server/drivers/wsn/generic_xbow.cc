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
 * Desc: Driver for generic Crossbow WSN nodes
 * Author: Adrian Jimenez Gonzalez, Jose Manuel Sanchez Matamoros
 * Date: 15 Aug 2011
 **************************************************************************/


/** @ingroup drivers */
/** @{ */
/** @defgroup driver_generic_xbow generic_xbow
 * @brief Crossbow Mica2, MicaZ, Iris and TelosB, TinyOS and Contiki operating systems compatible driver

The Generic XBow driver controls the Crossbow Mica2, MicaZ, TelosB among other mote sensor node. The
MTS310 and MTS510 boards are supported. TinyOS 1.x and 2.x operating systems compatible. Also compatible with WSN simulator Cooja.

@par Compile-time dependencies

- lserial
- socat

@par Provides

- @ref interface_coopobject

@par Requires

- none

@par Configuration requests

@par Configuration file options

- port (string)
  - Default: "/dev/ttyUSB0"
  - Serial port to which the platform is attached.  If you are
    using a USB/232 or USB/422 converter, this will be "/dev/ttyUSBx".

- platform (string)
  - Default: "telosb"
  - Platform attached. Options include telosb, mica2,  micaz, mica2dot, iris, etc...

- os (string)
  - Default: "tos2x"
  - Operating System used by the platform attached. Options include tos1x (TinyOS 1.x) tos2x (TinyOS 2.x) and contiki.
@par Example

@verbatim
driver
(
  name "generic_xbow"
  provides ["coopobject:0" ]
  port "/dev/ttyS0"
  platform "telosb"
  os "tos2x"

)

@endverbatim

@author Adrian Jimenez Gonzalez, Jose Manuel Sanchez-Matamoros


*/
/** @} */

#include <iostream>
#include <sys/types.h>
#include <libplayercore/playercore.h>

#include "generic_xbow.h"

// The GenericXBow device class.
class GenericXBow : public ThreadedDriver
{
    public:
    	// Constructor
	GenericXBow (ConfigFile* cf, int section);

	// Destructor
	~GenericXBow ();

	// Implementations of virtual functions
	virtual int MainSetup       ();
	virtual void MainQuit    ();
// 	virtual int Subscribe   (player_devaddr_t id);
// 	virtual int Unsubscribe (player_devaddr_t id);

	// This method will be invoked on each incoming message
	virtual int ProcessMessage (QueuePointer & resp_queue,
				    player_msghdr * hdr,
				    void * data);
    private:

	// Main function for device thread.
	virtual void Main ();
	virtual void RefreshData  ();

	MoteIF tos_mote;
	SerialStream transparent_mote;
	
	pthread_mutex_t mutexmote;
	// Interfaces that we might be using
	// WSN interface
//	player_devaddr_t   wsn_addr;
	bool               provideWSN;
	int                wsn_subscriptions;
	
	const char*			port_name;
	int				platform;
	SerialStreamBuf::BaudRateEnum	baud_rate;
	int			        os;
	int 				tos_ack;

	// Process payload of a data message
//	void processPayload(void *payload,uint8_t size, player_coopobject_data_header_t  wsn_header);

	// Send status message to WSN node
	void sendPosition(player_coopobject_position_t *pos);
	// Send user defined message to WSN node
	void sendUserdata(player_coopobject_data_userdefined_t *udata);
	// Send request message to WSN node
	void sendRequest(player_coopobject_req_t *req);
	// Send command message to WSN node
	void sendCommand(player_coopobject_cmd_t *command);

	
	uint8_t *createMsg(void* vdata, uint8_t *size, char* &old_def);	
	void sendMsg(uint8_t *msg, uint8_t msg_size, char *def, uint16_t node_id);
	void getTOSData(TOSMessage *tos_msg, void *vdata, char *data_def, uint8_t header_size = 0);


	protected:

	int computePlatform(const char* platform_str);
	int computeOS(const char* os_str);

};

////////////////////////////////////////////////////////////////////////////////
//Factory creation function. This functions is given as an argument when
// the driver is added to the driver table
Driver* GenericXBow_Init (ConfigFile* cf, int section)
{
    // Create and return a new instance of this driver
    return ((Driver*)(new GenericXBow (cf, section)));
}

////////////////////////////////////////////////////////////////////////////////
//Registers the driver in the driver table. Called from the
// player_driver_init function that the loader looks for
void generic_xbow_Register (DriverTable* table)
{
    table->AddDriver ("generic_xbow", GenericXBow_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
GenericXBow::GenericXBow (ConfigFile* cf, int section)
 : ThreadedDriver(cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_COOPOBJECT_CODE) {

    port_name   = cf->ReadString (section, "port", DEFAULT_GENERICXBOW_PORT);
    platform = computePlatform(cf->ReadString (section, "platform", DEFAULT_GENERICXBOW_PLATFORM));

    switch (platform) {
	case MICA2DOT:
	  baud_rate = SerialStreamBuf::BAUD_19200;
	  break;
	case MICA2:
	case MICAZ:
	case IRIS:
	  baud_rate = SerialStreamBuf::BAUD_57600;
	  break;
	case TELOS:
	case TELOSB:
	case TMOTE:
	case EYES:
	case INTELMOTE2:
	  baud_rate = SerialStreamBuf::BAUD_115200;

	  break;
    }
    
    const char* server_port_str = cf->ReadString (section, "port_virtual_server", NULL);

    if (server_port_str != NULL) {
	char cmd[128];
	int server_port = atoi(server_port_str);

	sprintf(cmd, "/usr/bin/socat PTY,link=%s,raw,echo=0,unlink-close=0 TCP4-LISTEN:%d,reuseaddr,fork,end-close &", port_name, server_port);

	printf("Virtual serial port %s at network port %d\n", port_name, server_port);
	/*printf("cmd: %s\n", cmd);*/

	/* XXX TODO terminate process (cleanly) */
        system(cmd);
        sleep(1);

	baud_rate = (SerialStreamBuf::BaudRateEnum)0;
    }

    const char* os_str = cf->ReadString (section, "os", DEFAULT_GENERICXBOW_OS);
    os = computeOS(os_str);

    PLAYER_MSG0(1,"> GenericXBow Driver initialising");
#ifdef debug
    PLAYER_MSG2(1,"> BaudRate: %d Operating System: %d", (int)baud_rate, os);
#endif
    // Open serial port
    if(os == OS_TRANSPARENT) {
      transparent_mote.Open( port_name );
      if (baud_rate)
	transparent_mote.SetBaudRate( baud_rate );
      transparent_mote.SetParity( SerialStreamBuf::PARITY_NONE );
      transparent_mote.SetCharSize( SerialStreamBuf::CHAR_SIZE_8  );
      transparent_mote.SetFlowControl( SerialStreamBuf::FLOW_CONTROL_NONE );
      transparent_mote.SetNumOfStopBits( 1 );
      transparent_mote.unsetf( std::ios_base::skipws ) ;
    } else {
      tos_mote.open(port_name,baud_rate);
      tos_mote.setTiming(0,3);
      tos_mote.setOS(os);
    }

    pthread_mutex_init(&mutexmote, NULL);
	
    return;
}

////////////////////////////////////////////////////////////////////////////////
// Destructor.
GenericXBow::~GenericXBow()
{
    PLAYER_MSG0 (1, "> GenericXBow driver shutting down...");
    // Close the serial port
  if (os == OS_TRANSPARENT) {
    if (transparent_mote.IsOpen() ){
	transparent_mote.Close();
    }
  } else tos_mote.close();
    
  PLAYER_MSG0 (1, "> [done]");
  
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int GenericXBow::MainSetup () {
    return (0);
}

////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
void GenericXBow::MainQuit ()
{
  // Stop the driver thread
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void GenericXBow::Main () {
    // The main loop; interact with the device here
    while (true) {
		// test if we are supposed to cancel
		pthread_testcancel ();

		// Process any pending messages
		ProcessMessages ();

		RefreshData ();
		usleep(10000);
    }
}

////////////////////////////////////////////////////////////////////////////////
// ProcessMessage function
int GenericXBow::ProcessMessage (QueuePointer & resp_queue, player_msghdr * hdr, void * data) {

  assert (hdr);
  assert (data);

  if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_COOPOBJECT_CMD_DATA, this->device_addr)) {	// Status or undefined message

    player_coopobject_data_userdefined_t *userdata = (player_coopobject_data_userdefined_t*)data;
    sendUserdata(userdata);

  } else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_COOPOBJECT_CMD_POSITION, this->device_addr)) {	// Command message

    player_coopobject_position_t *position = (player_coopobject_position_t*)data;
    sendPosition(position);

  } else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, PLAYER_COOPOBJECT_CMD_STANDARD, this->device_addr)) {	// Command message

    player_coopobject_cmd_t *command = (player_coopobject_cmd_t*)data;
    sendCommand(command);

  } else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, PLAYER_COOPOBJECT_REQ_STANDARD, this->device_addr)) {	// Request message

    player_coopobject_req_t *request = (player_coopobject_req_t*)data;
    sendRequest(request);
    Publish(device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_COOPOBJECT_REQ_STANDARD);
  }

  return 0;

}

void GenericXBow::sendMsg(uint8_t *msg, uint8_t msg_size, char *def, uint16_t node_id) {
  
  if (this->os == OS_TRANSPARENT) {

    pthread_mutex_lock(&mutexmote);
    for(int i = 0; i < msg_size; i++) {
      transparent_mote << msg[i];
  #ifdef debug
      PLAYER_MSG2(1,"sending transparent msg[%d] = 0x%02x",i,msg[i]);
  #endif
    }
    pthread_mutex_unlock(&mutexmote);
          
  } else {
    
    TOSMessage tos_msg;
    tos_msg.setOS(this->os);
    tos_msg.compose(AM_PLAYER_TO_WSN, msg, msg_size, def,  node_id); //reverse endianness
//    tos_msg.compose(AM_PLAYER_TO_WSN, msg, msg_size, node_id); 
    pthread_mutex_lock(&mutexmote);
    tos_mote.sendMessage(tos_msg,tos_ack);
    pthread_mutex_unlock(&mutexmote);
  }
  return;
}

void GenericXBow::sendUserdata(player_coopobject_data_userdefined_t *userdata) {

  UserDataMsg um;
  
  um.type = userdata->type;
  um.id = userdata->header.parent_id; // we use this field to send the robot ID
  um.parent_id = 0xFFFF;
  
  if(userdata->data_count > MAX_TOS_PAYLOAD){
    PLAYER_WARN("User message too large. Data will be truncated");
    um.data_size = MAX_TOS_PAYLOAD;
  } else um.data_size = (uint8_t)userdata->data_count;
  
  um.data = new uint8_t[um.data_size];
  #ifdef debug
  PLAYER_MSG1(1,"um size %d", um.data_size);
  #endif
  for(int i = 0; i < um.data_size; i++) {
    um.data[i] = userdata->data[i];
      #ifdef debug
  PLAYER_MSG2(1,"um data[%d] 0x%x", i, um.data[i]);
  #endif
  }
  
  char *def = defineStruct(um);
  uint8_t *msg;
  uint8_t msg_size;
  
  msg = createMsg(&um, &msg_size, def);
  
  sendMsg(msg, msg_size, def, userdata->header.id);
  
  PLAYER_MSG2(1,"> Sending user data %d to CoopObj %d", um.type, userdata->header.id);
  
  delete []msg;
  delete []def;
}

void GenericXBow::sendPosition(player_coopobject_position_t *pos) {
  
  PositionMsg pm;
  // Fill the message
  pm.type = (uint8_t)PLAYER_COOPOBJECT_MSG_POSITION;
  pm.id = pos->header.parent_id;
  pm.parent_id = 0xFFFF;
  pm.x = pos->x;
  pm.y = pos->y;
  pm.z = pos->z;
  pm.status = pos->status;
    
  char *def = defineStruct(pm);
  uint8_t *msg;
  uint8_t msg_size;
  
  msg = createMsg((void*)&pm, &msg_size, def);
  
  sendMsg(msg, msg_size, def, pos->header.id);
  
  PLAYER_MSG4(1,"> Sending robot position (%f,%f,%f) to CoopObj %d",pm.x,pm.y,pm.z,pos->header.id);

  delete []msg;
  delete []def;
}

void GenericXBow::sendRequest(player_coopobject_req_t *req){

  RequestMsg rm;
  rm.type = (uint8_t)PLAYER_COOPOBJECT_MSG_REQUEST;
  rm.id = req->header.parent_id; // we use this field to send the robot ID
  rm.parent_id = 0xFFFF;
  rm.request = req->request;
  
  if(req->parameters_count > MAX_TOS_PAYLOAD){
    PLAYER_WARN("User message too large. Data will be truncated");
    rm.parameters_size = MAX_TOS_PAYLOAD;
  } else rm.parameters_size = (uint8_t)req->parameters_count;
  
  rm.parameters = new uint8_t[rm.parameters_size];
  for(int i = 0; i < rm.parameters_size; i++)
    rm.parameters[i] = req->parameters[i];
  
  char *def = defineStruct(rm);
  uint8_t *msg;
  uint8_t msg_size;
  
  msg = createMsg((void*)&rm, &msg_size, def);
    
  sendMsg(msg, msg_size, def, req->header.id);
  
  PLAYER_MSG2(1,"> Sending request %d to CoopObj %d", rm.request, req->header.id);
  
  delete []msg;
  delete []def;
}

void GenericXBow::sendCommand(player_coopobject_cmd_t *command){
  
  CommandMsg cm;
  cm.type = (uint8_t)PLAYER_COOPOBJECT_MSG_COMMAND;
  cm.id = command->header.parent_id; // we use this field to send the robot ID
  cm.parent_id = 0xFFFF;
  cm.command = command->command;
  
  if(command->parameters_count > MAX_TOS_PAYLOAD){
    PLAYER_WARN("User message too large. Data will be truncated");
    cm.parameters_size = MAX_TOS_PAYLOAD;
  } else cm.parameters_size = (uint8_t)command->parameters_count;
  
  cm.parameters = new uint8_t[cm.parameters_size];
  for(int i = 0; i < cm.parameters_size; i++)
    cm.parameters[i] = command->parameters[i];
  
  char *def = defineStruct(cm);
  uint8_t *msg;
  uint8_t msg_size;
  
  msg = createMsg((void*)&cm, &msg_size, def);
    
  sendMsg(msg, msg_size, def, command->header.id);
  
  PLAYER_MSG2(1,"> Sending command %d to CoopObj %d", cm.command, command->header.id);

  delete []msg;
  delete []def;
}


////////////////////////////////////////////////////////////////////////////////
// RefreshData function
void GenericXBow::RefreshData () {
  
  if (this->os == OS_TRANSPARENT) {
    uint8_t transparent_msg[MAX_TRANSP_SIZE];
    pthread_mutex_lock(&mutexmote);
    int i = 0;
    if(!transparent_mote.good() || transparent_mote.rdbuf()->in_avail() == 0) {
      transparent_mote.clear();
      pthread_mutex_unlock(&mutexmote);
      //      this->Unlock();
      return;
    }
    transparent_mote >> transparent_msg[i++];
    
    if(transparent_mote.rdbuf()->in_avail() == 0) {
      usleep(50000);
      if(transparent_mote.rdbuf()->in_avail() == 0) return;
    }
    bool timeout = false;
    while(transparent_mote.good() &&  !timeout && i < MAX_TRANSP_SIZE) {
      transparent_mote >> transparent_msg[i++];
      if(transparent_mote.rdbuf()->in_avail() == 0) {
	usleep(50000);
	if(transparent_mote.rdbuf()->in_avail() == 0) timeout = true;
      }
    }
    pthread_mutex_unlock(&mutexmote);   
 
    switch (transparent_msg[0]) {
      
      case PLAYER_COOPOBJECT_MSG_POSITION:
      {
	player_coopobject_position_t wsn_data;
	uint8_t indx = 1;
	wsn_data.header.origin = PLAYER_COOPOBJECT_ORIGIN_MOTE;
	wsn_data.header.id = *(uint16_t *)(transparent_msg+indx);
	indx += sizeof(uint16_t);
	wsn_data.header.parent_id = *(uint16_t *)(transparent_msg+indx);
	indx += sizeof(uint16_t);
	wsn_data.x = *(float *)(transparent_msg+indx);
	indx += sizeof(float);
	wsn_data.y = *(float *)(transparent_msg+indx);
	indx += sizeof(float);
	wsn_data.z = *(float *)(transparent_msg+indx);
	indx += sizeof(float);
	wsn_data.status = *(uint8_t*)(transparent_msg+indx);
	
	PLAYER_MSG4 (1,"> Received position (%f,%f,%f) from CoopObj %d", wsn_data.header.id, wsn_data.x, wsn_data.y, wsn_data.z);
        // Publish message
	Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_COOPOBJECT_DATA_POSITION,  reinterpret_cast<void*>(&wsn_data), sizeof(wsn_data), NULL);
	break;
      }
      case PLAYER_COOPOBJECT_MSG_RSSI:
      {
	player_coopobject_rssi_t wsn_data;
	uint8_t indx = 1;
	wsn_data.header.origin = PLAYER_COOPOBJECT_ORIGIN_MOTE;
	wsn_data.header.id = *(uint16_t *)(transparent_msg+indx);
	indx += sizeof(uint16_t);
	wsn_data.header.parent_id = *(uint16_t *)(transparent_msg+indx);
	indx += sizeof(uint16_t);
	wsn_data.sender_id = *(uint16_t *)(transparent_msg+indx);
	indx += sizeof(uint16_t);
	wsn_data.rssi = *(uint16_t *)(transparent_msg+indx);
	indx += sizeof(uint16_t);
	wsn_data.stamp = *(uint16_t *)(transparent_msg+indx);
	indx += sizeof(uint16_t);
	wsn_data.nodeTimeHigh = *(uint32_t *)(transparent_msg+indx);
	indx += sizeof(uint32_t);
	wsn_data.nodeTimeLow = *(uint32_t *)(transparent_msg+indx);
	indx += sizeof(uint32_t);
	wsn_data.x = *(float *)(transparent_msg+indx);
	indx += sizeof(float);
	wsn_data.y = *(float *)(transparent_msg+indx);
	indx += sizeof(float);
	wsn_data.z = *(float *)(transparent_msg+indx);
	
	PLAYER_MSG6 (1,"> Received RSSI=%d in msg from CoopObj %d to CoopObj %d at (%f,%f,%f)", wsn_data.rssi, wsn_data.sender_id, wsn_data.header.id, wsn_data.x, wsn_data.y, wsn_data.z);
       // Publish message
	Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_COOPOBJECT_DATA_RSSI,  reinterpret_cast<void*>(&wsn_data), sizeof(wsn_data), NULL);
	break;
      }
      case PLAYER_COOPOBJECT_MSG_SENSOR:
      {
	player_coopobject_data_sensor_t wsn_data;
	uint8_t indx = 1;
	wsn_data.header.origin = PLAYER_COOPOBJECT_ORIGIN_MOTE;
	wsn_data.header.id = *(uint16_t *)(transparent_msg+indx);
	indx += sizeof(uint16_t);
	wsn_data.header.parent_id = *(uint16_t *)(transparent_msg+indx);
	indx += sizeof(uint16_t);
	wsn_data.data_count = (uint32_t)(*(uint8_t *)(transparent_msg+indx));
	indx += sizeof(uint8_t);
	wsn_data.data = new player_coopobject_sensor_t[wsn_data.data_count];
	for(uint32_t i = 0; i < wsn_data.data_count; i++) {
	  wsn_data.data[i].type = *(uint8_t *)(transparent_msg+indx);
	  indx += sizeof(uint8_t);
	  wsn_data.data[i].value = *(int16_t *)(transparent_msg+indx);
	  indx += sizeof(int16_t);
	}
	
	PLAYER_MSG1 (1,"> Received Sensor data from CoopObj %d", wsn_data.header.id);
#ifdef debug
	for(uint32_t i = 0; i < wsn_data.data_count; i++)
	  PLAYER_MSG2 (0,"                                       [%d] %d",wsn_data.data[i].type, wsn_data.data[i].value);
#endif
        // Publish message
	Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_COOPOBJECT_DATA_SENSOR,  reinterpret_cast<void*>(&wsn_data), sizeof(wsn_data), NULL);
	break;
      }
      case PLAYER_COOPOBJECT_MSG_ALARM:
      {
	player_coopobject_data_sensor_t wsn_data;
	uint8_t indx = 1;
	wsn_data.header.origin = PLAYER_COOPOBJECT_ORIGIN_MOTE;
	wsn_data.header.id = *(uint16_t *)(transparent_msg+indx);
	indx += sizeof(uint16_t);
	wsn_data.header.parent_id = *(uint16_t *)(transparent_msg+indx);
	indx += sizeof(uint16_t);
	wsn_data.data_count = (uint32_t)(*(uint8_t *)(transparent_msg+indx));
	indx += sizeof(uint8_t);
	wsn_data.data = new player_coopobject_sensor_t[wsn_data.data_count];
	for(uint32_t i = 0; i < wsn_data.data_count; i++) {
	  wsn_data.data[i].type = *(uint8_t *)(transparent_msg+indx);
	  indx += sizeof(uint8_t);
	  wsn_data.data[i].value = *(int16_t *)(transparent_msg+indx);
	  indx += sizeof(int16_t);
	}
	
	PLAYER_MSG1 (1,"> Received Alarm data from CoopObj %d", wsn_data.header.id);
#ifdef debug
	for(uint32_t i = 0; i < wsn_data.data_count; i++)
	  PLAYER_MSG2 (0,"                                       [%d] %d",wsn_data.data[i].type, wsn_data.data[i].value);
#endif
        // Publish message
	Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_COOPOBJECT_DATA_ALARM,  reinterpret_cast<void*>(&wsn_data), sizeof(wsn_data), NULL);
	break;
      }
      case PLAYER_COOPOBJECT_MSG_REQUEST:
      {
	player_coopobject_req_t wsn_data;
	uint8_t indx = 1;
	wsn_data.header.origin = PLAYER_COOPOBJECT_ORIGIN_MOTE;
	wsn_data.header.id = *(uint16_t *)(transparent_msg+indx);
	indx += sizeof(uint16_t);
	wsn_data.header.parent_id = *(uint16_t *)(transparent_msg+indx);
	indx += sizeof(uint16_t);
	wsn_data.request = *(uint8_t *)(transparent_msg+indx);
	indx += sizeof(uint8_t);
	wsn_data.parameters_count = (uint32_t)(*(uint8_t *)(transparent_msg+indx));
	indx += sizeof(uint8_t);
	wsn_data.parameters = new uint8_t[wsn_data.parameters_count];
	for(uint32_t i = 0; i < wsn_data.parameters_count; i++) {
	  wsn_data.parameters[i] = *(uint8_t *)(transparent_msg+indx);
	  indx += sizeof(uint8_t);
	}
	
	PLAYER_MSG2 (1,"> Received Request %d from CoopObj %d", wsn_data.request, wsn_data.header.id);
 #ifdef debug
	for(uint32_t i = 0; i < wsn_data.parameters_count; i++)
	  PLAYER_MSG2 (0,"                                        [%d] 0x%02x",i, wsn_data.parameters[i]);
#endif
	// Publish message
	Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_COOPOBJECT_DATA_REQUEST,  reinterpret_cast<void*>(&wsn_data), sizeof(wsn_data), NULL);
	break;
      }
      case PLAYER_COOPOBJECT_MSG_COMMAND:
      {
	player_coopobject_cmd_t wsn_data;
	uint8_t indx = 1;
	wsn_data.header.origin = PLAYER_COOPOBJECT_ORIGIN_MOTE;
	wsn_data.header.id = *(uint16_t *)(transparent_msg+indx);
	indx += sizeof(uint16_t);
	wsn_data.header.parent_id = *(uint16_t *)(transparent_msg+indx);
	indx += sizeof(uint16_t);
	wsn_data.command = *(uint8_t *)(transparent_msg+indx);
	indx += sizeof(uint8_t);
	wsn_data.parameters_count = (uint32_t)(*(uint8_t *)(transparent_msg+indx));
	indx += sizeof(uint8_t);
	wsn_data.parameters = new uint8_t[wsn_data.parameters_count];
	for(uint32_t i = 0; i < wsn_data.parameters_count; i++) {
	  wsn_data.parameters[i] = *(uint8_t *)(transparent_msg+indx);
	  indx += sizeof(uint8_t);
	}
	
	PLAYER_MSG2 (1,"> Received Command %d from CoopObj %d", wsn_data.command, wsn_data.header.id);
 #ifdef debug
	for(uint32_t i = 0; i < wsn_data.parameters_count; i++)
	  PLAYER_MSG2 (0,"                                        [%d] 0x%02x",i, wsn_data.parameters[i]);
#endif
	// Publish message
	Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_COOPOBJECT_DATA_COMMAND,  reinterpret_cast<void*>(&wsn_data), sizeof(wsn_data), NULL);
	break;
      }
      default:		// any other will be taken as user defined data
      {
	player_coopobject_data_userdefined_t wsn_data;
	uint8_t indx = 1;
	wsn_data.header.origin = PLAYER_COOPOBJECT_ORIGIN_MOTE;
	wsn_data.header.id = *(uint16_t *)(transparent_msg+indx);
	indx += sizeof(uint16_t);
	wsn_data.header.parent_id = *(uint16_t *)(transparent_msg+indx);
	indx += sizeof(uint16_t);
	wsn_data.data_count = (uint32_t)(*(uint8_t *)(transparent_msg+indx));
	indx += sizeof(uint8_t);
	wsn_data.data = new uint8_t[wsn_data.data_count];
	for(uint32_t i = 0; i < wsn_data.data_count; i++) {
	  wsn_data.data[i] = *(uint8_t *)(transparent_msg+indx);
	  indx += sizeof(uint8_t);
	}
	
	PLAYER_MSG1 (1,"> Received User defined data from CoopObj %d", wsn_data.header.id);
 #ifdef debug
	for(uint32_t i = 0; i < wsn_data.data_count; i++)
	  PLAYER_MSG2 (0,"                                       [%d] 0x%02x",i, wsn_data.data[i]);
#endif
	// Publish message
	Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_COOPOBJECT_DATA_USERDEFINED,  reinterpret_cast<void*>(&wsn_data), sizeof(wsn_data), NULL);
	break;
      }
    }
  } else {
  
    TOSMessage tos_msg;
    XMeshHeader xmeshheader;
    this->Lock();
    pthread_mutex_lock(&mutexmote);
    try{
	    tos_mote.getMessage(tos_msg);
    } catch ( IOException &e ){
    } catch ( TimeoutException &e ){
    } catch ( CRCException &e){
    }
    pthread_mutex_unlock(&mutexmote);
    this->Unlock();

    player_coopobject_header_t  wsn_header;
    // Fill time fields on wsn_data
    uint8_t type = 0;

    switch(tos_msg.type)
    {
      case AM_MOTE_MESSAGE: // Message from mote interface
	wsn_header.origin = PLAYER_COOPOBJECT_ORIGIN_MOTE;
	type = tos_msg.data[0];
	break;
      case AM_BASE_MESSAGE: // Message from WSN interface
	switch ( tos_msg.data[6] ){ // xmeshheader.appId
	  case ID_MOBILE_DATA:
	    wsn_header.origin = PLAYER_COOPOBJECT_ORIGIN_MOBILEBASE;
	    type = tos_msg.data[sizeof(XMeshHeader)];
	    break;
	  case ID_FIXED_DATA:						
	    wsn_header.origin = PLAYER_COOPOBJECT_ORIGIN_STATICBASE;
	    type = tos_msg.data[sizeof(XMeshHeader)];
	    break;
	  case ID_HEALTH:						
	    wsn_header.origin = PLAYER_COOPOBJECT_ORIGIN_STATICBASE;
	    HealthMsg hmsg;
	    getTOSData(&tos_msg, (void *)&hmsg, defineStruct(hmsg));
	    wsn_header.id = hmsg.id;
	    wsn_header.parent_id = hmsg.parent_id;
	    PLAYER_MSG1 (1,"> Received Health message from CoopObj %d ", wsn_header.id);
	    // Publish health msg
	    Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_COOPOBJECT_DATA_HEALTH, reinterpret_cast<void*>(&wsn_header), sizeof (wsn_header), NULL);
	    break;
	  default:
	    PLAYER_MSG0 (1,"> Received unknown CoopObj message");
	    break;
	  }
	break;
	  
      default:
	return;
	break;
    }

   switch(type){
     case PLAYER_COOPOBJECT_MSG_HEALTH: break;
    case PLAYER_COOPOBJECT_MSG_RSSI:
      {
	RSSIBeaconMsg wsn_msg;
	uint8_t header_size = 0;
	if (wsn_header.origin == PLAYER_COOPOBJECT_ORIGIN_MOBILEBASE || wsn_header.origin == PLAYER_COOPOBJECT_ORIGIN_STATICBASE)
	  header_size = sizeof(xmeshheader);
      
	getTOSData(&tos_msg, (void *)&wsn_msg, defineStruct(wsn_msg), header_size);

	player_coopobject_rssi_t wsn_data;

	wsn_data.header.origin = wsn_header.origin;
	// Fill rssi data
	wsn_data.header.id = wsn_msg.node_id;
	wsn_data.header.parent_id = 0xFFFF;
	wsn_data.sender_id = wsn_msg.sender_id;
	wsn_data.rssi = wsn_msg.rssi;
	wsn_data.stamp = wsn_msg.stamp;
	wsn_data.nodeTimeHigh = wsn_msg.timehigh;
	wsn_data.nodeTimeLow = wsn_msg.timelow;
	wsn_data.x = wsn_msg.x;
	wsn_data.y = wsn_msg.y;
	wsn_data.z = wsn_msg.z;
	PLAYER_MSG6 (1,"> Received RSSI %d in signal from CoopObj %d to CoopObj %d at (%f,%f,%f)", wsn_data.rssi, wsn_data.sender_id, wsn_data.header.id, wsn_data.x, wsn_data.y, wsn_data.z);
 	// Publish message
	Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_COOPOBJECT_DATA_RSSI,  reinterpret_cast<void*>(&wsn_data), sizeof(wsn_data), NULL);

	break;
      }
      case PLAYER_COOPOBJECT_MSG_POSITION:
      {
	PositionMsg wsn_msg;
	uint8_t header_size = 0;
	if (wsn_header.origin == PLAYER_COOPOBJECT_ORIGIN_MOBILEBASE || wsn_header.origin == PLAYER_COOPOBJECT_ORIGIN_STATICBASE)
	  header_size = sizeof(xmeshheader);
      
	getTOSData(&tos_msg, &wsn_msg, defineStruct(wsn_msg), header_size);

	player_coopobject_position_t wsn_data;

	wsn_data.header.origin = wsn_header.origin;
	// Fill rssi data
	wsn_data.header.id = wsn_msg.id;
	wsn_data.header.parent_id = wsn_msg.parent_id;
	wsn_data.x = wsn_msg.x;
	wsn_data.y = wsn_msg.y;
	wsn_data.z = wsn_msg.z;
	wsn_data.status = wsn_msg.status;
	PLAYER_MSG5 (1,"> Received position (%f,%f,%f), %d from CoopObj %d", wsn_data.x, wsn_data.y, wsn_data.z, wsn_data.status, wsn_data.header.id);
	// Publish message
	Publish (device_addr, PLAYER_MSGTYPE_DATA,PLAYER_COOPOBJECT_DATA_POSITION,  reinterpret_cast<void*>(&wsn_data), sizeof(wsn_data), NULL);

	break;
      }
      case PLAYER_COOPOBJECT_MSG_SENSOR:
      {
	SensorMsg wsn_msg;
	uint8_t header_size = 0;
	if (wsn_header.origin == PLAYER_COOPOBJECT_ORIGIN_MOBILEBASE || wsn_header.origin == PLAYER_COOPOBJECT_ORIGIN_STATICBASE)
	  header_size = sizeof(xmeshheader);
	  
	wsn_msg.sensor_count = tos_msg.data[header_size+5];
	getTOSData(&tos_msg, (void *)&wsn_msg, defineStruct(wsn_msg), header_size);

	player_coopobject_data_sensor_t wsn_data;

	wsn_data.header.origin = wsn_header.origin;

	wsn_data.header.id = wsn_msg.id;
	wsn_data.header.parent_id = wsn_msg.parent_id;
	

	wsn_data.data_count = wsn_msg.sensor_count;
	wsn_data.data = new player_coopobject_sensor_t[wsn_data.data_count];
	
	for(uint32_t i = 0; i < wsn_data.data_count; i++) {
		wsn_data.data[i].type = wsn_msg.sensor[i].type;
		wsn_data.data[i].value = wsn_msg.sensor[i].value;
	}
	PLAYER_MSG2 (1,"> Received %d sensors data from CoopObj %d", wsn_data.data_count, wsn_data.header.id);
 #ifdef debug
	for(uint32_t i = 0; i < wsn_data.data_count; i++)
	  PLAYER_MSG2 (0,"                                       [%d] %d",wsn_data.data[i].type, wsn_data.data[i].value);
#endif
	// Publish message
	Publish (device_addr, PLAYER_MSGTYPE_DATA,PLAYER_COOPOBJECT_DATA_SENSOR,  reinterpret_cast<void*>(&wsn_data), sizeof(wsn_data), NULL);

	break;
      }
      case PLAYER_COOPOBJECT_MSG_ALARM:
      {
	SensorMsg wsn_msg;
	uint8_t header_size = 0;
	if (wsn_header.origin == PLAYER_COOPOBJECT_ORIGIN_MOBILEBASE || wsn_header.origin == PLAYER_COOPOBJECT_ORIGIN_STATICBASE)
	  header_size = sizeof(xmeshheader);
	  
	wsn_msg.sensor_count = tos_msg.data[header_size+5];
	getTOSData(&tos_msg, (void *)&wsn_msg, defineStruct(wsn_msg), header_size);

	player_coopobject_data_sensor_t wsn_data;

	wsn_data.header.origin = wsn_header.origin;

	wsn_data.header.id = wsn_msg.id;
	wsn_data.header.parent_id = wsn_msg.parent_id;
	

	wsn_data.data_count = wsn_msg.sensor_count;
	wsn_data.data = new player_coopobject_sensor_t[wsn_data.data_count];
	
	for(uint32_t i = 0; i < wsn_data.data_count; i++) {
		wsn_data.data[i].type = wsn_msg.sensor[i].type;
		wsn_data.data[i].value = wsn_msg.sensor[i].value;
	}
	PLAYER_MSG2 (1,"> Received %d alarm data from CoopObj %d", wsn_data.data_count, wsn_data.header.id);
 #ifdef debug
	for(uint32_t i = 0; i < wsn_data.data_count; i++)
	  PLAYER_MSG2 (0,"                                       [%d] %d",wsn_data.data[i].type, wsn_data.data[i].value);
#endif
	// Publish message
	Publish (device_addr, PLAYER_MSGTYPE_DATA,PLAYER_COOPOBJECT_DATA_ALARM,  reinterpret_cast<void*>(&wsn_data), sizeof(wsn_data), NULL);
	break;
      }
      case PLAYER_COOPOBJECT_MSG_REQUEST:
      {

	RequestMsg wsn_msg;
	uint8_t header_size = 0;
	if (wsn_header.origin == PLAYER_COOPOBJECT_ORIGIN_MOBILEBASE || wsn_header.origin == PLAYER_COOPOBJECT_ORIGIN_STATICBASE)
	  header_size = sizeof(xmeshheader);
	  
	wsn_msg.parameters_size = tos_msg.data[header_size+6];
	getTOSData(&tos_msg, (void *)&wsn_msg, defineStruct(wsn_msg), header_size);
	
	player_coopobject_req_t wsn_data;

	wsn_data.header.origin = wsn_header.origin;

	wsn_data.header.id = wsn_msg.id;
	wsn_data.header.parent_id = wsn_msg.parent_id;
	
	wsn_data.request = wsn_msg.request;
	
	wsn_data.parameters_count = wsn_msg.parameters_size;
	wsn_data.parameters = new uint8_t[wsn_data.parameters_count];
	
	for(uint32_t i = 0; i < wsn_data.parameters_count; i++)
		wsn_data.parameters[i] = wsn_msg.parameters[i];

	PLAYER_MSG2 (1,"> Received Request %d from CoopObj %d", wsn_data.request, wsn_data.header.id);
 #ifdef debug
	for(uint32_t i = 0; i < wsn_data.parameters_count; i++)
	  PLAYER_MSG2 (0,"                                        [%d] 0x%02x",i, wsn_data.parameters[i]);
#endif
	// Publish message
	Publish (device_addr, PLAYER_MSGTYPE_DATA,PLAYER_COOPOBJECT_DATA_REQUEST,  reinterpret_cast<void*>(&wsn_data), sizeof(wsn_data), NULL);
	break;
      }
      case PLAYER_COOPOBJECT_MSG_COMMAND:
      {
	CommandMsg wsn_msg;
	uint8_t header_size = 0;
	if (wsn_header.origin == PLAYER_COOPOBJECT_ORIGIN_MOBILEBASE || wsn_header.origin == PLAYER_COOPOBJECT_ORIGIN_STATICBASE)
	  header_size = sizeof(xmeshheader);
	  
	wsn_msg.parameters_size = tos_msg.data[header_size+6];
	getTOSData(&tos_msg, (void *)&wsn_msg, defineStruct(wsn_msg), header_size);
	
	player_coopobject_cmd_t wsn_data;

	wsn_data.header.origin = wsn_header.origin;

	wsn_data.header.id = wsn_msg.id;
	wsn_data.header.parent_id = wsn_msg.parent_id;
	
	wsn_data.command = wsn_msg.command;
	
	wsn_data.parameters_count = wsn_msg.parameters_size;
	wsn_data.parameters = new uint8_t[wsn_data.parameters_count];
	
	for(uint32_t i = 0; i < wsn_data.parameters_count; i++)
		wsn_data.parameters[i] = wsn_msg.parameters[i];

	PLAYER_MSG2 (1,"> Received Command %d from CoopObj %d", wsn_data.command, wsn_data.header.id);
 #ifdef debug
	for(uint32_t i = 0; i < wsn_data.parameters_count; i++)
	  PLAYER_MSG2 (0,"                                        [%d] 0x%02x",i, wsn_data.parameters[i]);
#endif
	// Publish data
	Publish(device_addr, PLAYER_MSGTYPE_DATA, PLAYER_COOPOBJECT_DATA_COMMAND, reinterpret_cast<void*>(&wsn_data), sizeof(wsn_data), NULL);

	break;
      }
      default: // Undefined message
      {
	UserDataMsg wsn_msg;
	uint8_t header_size = 0;
	if (wsn_header.origin == PLAYER_COOPOBJECT_ORIGIN_MOBILEBASE || wsn_header.origin == PLAYER_COOPOBJECT_ORIGIN_STATICBASE)
	  header_size = sizeof(xmeshheader);
	  
	wsn_msg.data_size = tos_msg.data[header_size+5];
	getTOSData(&tos_msg, (void *)&wsn_msg, defineStruct(wsn_msg), header_size);
	
	player_coopobject_data_userdefined_t wsn_data;

	wsn_data.header.origin = wsn_header.origin;

	wsn_data.header.id = wsn_msg.id;
	wsn_data.header.parent_id = wsn_msg.parent_id;
	
	wsn_data.type = type;

	wsn_data.data_count = wsn_msg.data_size;
	wsn_data.data = new uint8_t[wsn_data.data_count];
	
	for(uint32_t i = 0; i < wsn_data.data_count; i++)
		wsn_data.data[i] = wsn_msg.data[i];
	PLAYER_MSG2 (1,"> Received User data %d from CoopObj %d", wsn_data.type, wsn_data.header.id);
 #ifdef debug
	for(uint32_t i = 0; i < wsn_data.data_count; i++)
	  PLAYER_MSG2 (0,"                                       [%d] 0x%02x",i, wsn_data.data[i]);
#endif
	      // Publish message
	Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_COOPOBJECT_DATA_USERDEFINED,  reinterpret_cast<void*>(&wsn_data), sizeof(wsn_data), NULL);

	break;
      }
    }
  }
}

int GenericXBow::computePlatform(const char* platform_str) {
	if (!strcmp(platform_str, "telos"))
		return TELOS;
	else if (!strcmp(platform_str, "telosb"))
		return TELOSB;
	else if (!strcmp(platform_str, "tmote"))
		return TMOTE;
	else if (!strcmp(platform_str, "eyes"))
		return EYES;
	else if (!strcmp(platform_str, "intelmote2"))
		return INTELMOTE2;
	else if (!strcmp(platform_str, "mica2"))
		return MICA2;
	else if (!strcmp(platform_str, "mica2dot"))
		return MICA2DOT;
	else if (!strcmp(platform_str, "micaz"))
		return MICAZ;
	else if (!strcmp(platform_str, "iris"))
		return IRIS;
	else {
	  PLAYER_ERROR("Error parsing WSN platform");
	  return -1;
	}
}
int GenericXBow::computeOS(const char* os_str) {

	if (!strcmp(os_str, "transparent"))
		return OS_TRANSPARENT;
	else if (!strcmp(os_str, "tos1x") || !strcmp(os_str, "tos1"))
		{tos_ack = 0; return TOS1;}
	else if (!strcmp(os_str, "tos2x") || !strcmp(os_str, "tos2"))
		{tos_ack = 1; return TOS2;}
	else if (!strcmp(os_str, "contiki"))
		return CONTIKI;
	PLAYER_ERROR("Error parsing WSN operating system");	  
	return -1;

}

void GenericXBow::getTOSData(TOSMessage *tos_msg, void *vdata, char *data_def, uint8_t header_size) {
//PLAYER_MSG1(1,"GenericXBow::getData: data_def '%s'",data_def);
  uint8_t *data = (uint8_t*)vdata;
  char *msg_def;
  uint8_t msg_size = header_size; // in bytes
  int msg_nfields = 0;
  int data_nfields = strlen(data_def);
  for (int i = 0; i < data_nfields; i++) {
    if(data_def[i] == 'b' || data_def[i] == 'c') { msg_size += 1; msg_nfields++;}
    else if(data_def[i] == 's') { msg_size += 2; msg_nfields++;}
    else if(data_def[i] == 'w' || data_def[i] == 'f' || data_def[i] == 'd') { msg_size += 4; msg_nfields++;}
    else if(data_def[i] == 'l') { msg_size += 8; msg_nfields++; }
  }
  msg_def = new char[msg_nfields+1];

  // take out the pointers counters and returns
  int j = 0;
  for(int i = 0;i < data_nfields && j < msg_nfields;i++) {
    if( data_def[i]=='b' || data_def[i]=='s' || data_def[i]=='w' || data_def[i]=='l' || data_def[i]=='f') {
      msg_def[j] = data_def[i];
      j++;
    } else if( data_def[i]=='d') {
      msg_def[j] = 'f';
      j++;
    } else if( data_def[i]=='c') {
      msg_def[j] = 'b';
      j++;
    } else if (data_def[i] != 'p' && data_def[i] != 'r'){ 
      PLAYER_ERROR("GenericXBow::getData: Error parsing structure new_def.\n");
      return;
    }
  }
  msg_def[j] = '\0';
  #ifdef debug
PLAYER_MSG1(1,"GenericXBow::getData: new_def '%s'",msg_def);
PLAYER_MSG1(1,"GenericXBow::getData: data_def '%s'",data_def);
  #endif
  uint8_t *msg = new uint8_t[msg_size-header_size];
  
  tos_msg->getData(msg,msg_size,msg_def);
//  tos_msg->getData(msg,msg_size);
#ifdef debug
  for(int i = 0; i < msg_size; i++)
    PLAYER_MSG2(1,"msg[%d] = 0x%02x",i, msg[i]);
#endif

  int data_ind = 0;
  int msg_ind = header_size;

  for(int i = 0;i < data_nfields ;i++) {
    if(data_def[i]=='b') { 
      *((uint8_t *)(data+data_ind)) = *((uint8_t *)(msg+msg_ind)); 
      data_ind += 1; msg_ind += 1; 
    } else if(data_def[i]=='c') {
      *((uint32_t *)(data+data_ind)) = (uint32_t)*((uint8_t *)(msg+msg_ind)); 
      data_ind += 4;msg_ind += 1; 
    } else if(data_def[i]=='s') {
      *((uint16_t *)(data+data_ind)) = *((uint16_t *)(msg+msg_ind));
      #ifdef debug
      PLAYER_MSG3(1,"msg[%d] = %d, at %d",i, *((uint16_t *)(data+data_ind)), data_ind);
#endif
      data_ind += 2; msg_ind += 2; 
    } else if(data_def[i]=='w') {
      *((uint32_t *)(data+data_ind)) = *((uint32_t *)(msg+msg_ind)); 
      data_ind += 4;msg_ind += 4; 
    } else if(data_def[i]=='l') {
      *((uint64_t *)(data+data_ind)) = *((uint64_t *)(msg+msg_ind)); 
      data_ind += 8; msg_ind += 8;
    } else if(data_def[i]=='f') {
      *((float *)(data+data_ind)) = *((float *)(msg+msg_ind));
#ifdef debug
      PLAYER_MSG3(1,"msg[%d] = %f, at %d",i, *((float *)(data+data_ind)), data_ind);
#endif
      data_ind += 4; msg_ind += 4;
    } else if(data_def[i]=='d') {
#ifdef debug
      PLAYER_MSG2(1,"msg[%d] = %f",i, *((float *)(msg+msg_ind)));
#endif
      *((double *)(data+data_ind)) = (double)(*((float *)(msg+msg_ind)));
      data_ind += 8; msg_ind += 4;
    } else if(data_def[i]=='p') {
      uint8_t array_size = 0;
      uint8_t array_nfields = 0;
      uint8_t array_ind = 0;
      while(data_def[i+array_nfields+1] != 'r' && data_def[i+array_nfields+1] != '\0') {
	switch(data_def[i+array_nfields+1]) {
	  case 'b': array_size +=1; array_nfields++; break;
	  case 's': array_size +=2; array_nfields++; break;
	  case 'w': case 'f': case 'd': array_size += 4; array_nfields++; break;
	  case 'l': array_size += 8; array_nfields++; break;
	  default: PLAYER_ERROR("GenericXBow::getData: Error parsing structure in 'p'.\n"); return;
	}
      }
 #ifdef debug
	  PLAYER_MSG1(1,"pointer array size = %d",array_size);
#endif
      *(uint8_t **)(data+data_ind) = new uint8_t[array_size];
      uint8_t *array = *(uint8_t **)(data+data_ind);
      for(uint32_t j = 1; j <= array_nfields; j++) {
	if(data_def[i+j]=='b') {  
	  *((uint8_t *)array+array_ind) = *((uint8_t *)msg+msg_ind);
#ifdef debug
	  PLAYER_MSG3(1,"pointer[%d] = %d, at %d",j, *((uint8_t *)(array+array_ind)), array_ind);
#endif
	  array_ind += 1; msg_ind += 1; 
	} else if(data_def[i+j]=='s') {
	  *((uint16_t *)(array+array_ind)) = *((uint16_t *)(msg+msg_ind));
#ifdef debug
	  PLAYER_MSG3(1,"pointer[%d] = %d, at %d",j, *((uint16_t *)(array+array_ind)), array_ind);
#endif
	  array_ind += 2; msg_ind += 2; 
	} else if(data_def[i+j]=='w') { 
	  *((uint32_t *)(array+array_ind)) = *((uint32_t *)(msg+msg_ind)); 
	  array_ind += 4; msg_ind += 4; 
	} else if(data_def[i+j]=='l') {
	  *((uint64_t *)(array+array_ind)) = *((uint64_t *)(msg+msg_ind)); 
	  array_ind += 8; msg_ind += 8;
	} else if(data_def[i+j]=='f') {;
	  *((float *)(array+array_ind)) = *((float *)(msg+msg_ind));
	  array_ind += 4; msg_ind += 4;
	} else if(data_def[i+j]=='d') { 
	  *((double *)(array+array_ind)) = (double)(*((float *)(msg+msg_ind)));
	  array_ind += 8; msg_ind += 4;
	} 
      }
      i += array_nfields+1;
      data_ind += sizeof(uint8_t*); //! NOTA: hay que quitarlo si va todo continuo
    } else if (data_def[i] != 'r') { 
      PLAYER_ERROR1("GenericXBow::getData: Error parsing structure in '%c'.\n",data_def[i]);
      return;
    }
  }

  delete [] msg_def;
  delete [] msg;

  return; //return the size of the actual message
 }

uint8_t *GenericXBow::createMsg(void* vdata, uint8_t *size, char* &old_def) {
  uint8_t *data = (uint8_t*)vdata;
  uint8_t msg_size = 0; // in bytes
  int msg_nfields = 0;
  int old_nfields = strlen(old_def);
  for (int i = 0; i < old_nfields; i++) {
    if(old_def[i] == 'b' || old_def[i] == 'c') { msg_size += 1; msg_nfields++; }
    else if(old_def[i] == 's') { msg_size += 2; msg_nfields++; }
    else if(old_def[i] == 'w' || old_def[i] == 'f' || old_def[i] == 'd') { msg_size += 4; msg_nfields++;}
    else if(old_def[i] == 'l') { msg_size += 8; msg_nfields++; }
  }
  char *msg_def = new char[msg_nfields+1];
  uint8_t *msg = new uint8_t[msg_size];
  *size = msg_size;
  
  int data_ind = 0, prev_data_ind = 0;
  int msg_ind = 0;
  int j = 0;
  uint8_t *prev_data = NULL;
  #ifdef debug
     PLAYER_MSG2(1,"creating new message with old_def[%d] '%s'",old_nfields,old_def);
     #endif
  for(int i = 0;i < old_nfields && j < msg_nfields;i++) {
    if(old_def[i]=='b') {
      *((uint8_t *)(msg+msg_ind)) = *((uint8_t *)(data+data_ind));
      #ifdef debug
      PLAYER_MSG2(1,"msg[%d] = 0x%02x",j, *((uint8_t *)(msg+msg_ind)));
      PLAYER_MSG2(1,"data[%d] = 0x%02x",j, *((uint8_t *)(data+data_ind)));
#endif
      msg_ind += 1; data_ind += 1;
      msg_def[j] = old_def[i];
      j++;
    } else if(old_def[i]=='c') {
      *((uint8_t *)(msg+msg_ind)) = (uint8_t)(*((uint32_t *)(data+data_ind)));
      #ifdef debug
      PLAYER_MSG2(1,"msg[%d] = %d",j, *((uint8_t *)(msg+msg_ind)));
      #endif
      msg_ind += 1; data_ind += 4;
      msg_def[j] = 'b';
      j++;
    } else if(old_def[i]=='s') {
      *((uint16_t *)(msg+msg_ind)) = *((uint16_t *)(data+data_ind));
      #ifdef debug
      PLAYER_MSG2(1,"msg[%d] = %d",j, *((uint16_t *)(msg+msg_ind)));
      #endif
      msg_ind += 2; data_ind += 2;
      msg_def[j] = old_def[i];
      j++;
    } else if(old_def[i]=='w') {
      *((uint32_t *)(msg+msg_ind)) = *((uint32_t *)(data+data_ind));
//      PLAYER_MSG2(1,"msg[%d] = %d",j, *((uint32_t *)(msg+msg_ind)));
      msg_ind += 4; data_ind += 4;
      msg_def[j] = old_def[i];
      j++;
    } else if(old_def[i]=='l') {
      *((uint64_t *)(msg+msg_ind)) = *((uint64_t *)(data+data_ind));
//    PLAYER_MSG2(1,"msg[%d] = %ld",j, *((uint64_t *)(msg+msg_ind)));
      msg_ind += 8; data_ind += 8;
      msg_def[j] = old_def[i];
      j++;
    } else if(old_def[i]=='f') {
      *((float *)(msg+msg_ind)) = *((float *)(data+data_ind));
      msg_ind += 4; data_ind += 4;
      msg_def[j] = old_def[i];
//      PLAYER_MSG2(1,"msg[%d] = %f",j, *((float *)(msg+msg_ind)));
      j++;
    } else if(old_def[i]=='d') {
      *((float *)(msg+msg_ind)) = (float)(*((double *)(data+data_ind)));
//      PLAYER_MSG2(1,"msg[%d] = %f",j, *((float *)(msg+msg_ind)));
//      PLAYER_MSG2(1,"data[%d] = %f",j, (float)*((double *)(data+data_ind)));
      msg_ind += sizeof(float); data_ind += sizeof(double);
      msg_def[j] = 'f';
      j++;
    } else if(old_def[i]=='p') {
      prev_data = data;
      data = *(uint8_t **)(data+data_ind);
      prev_data_ind = data_ind;
      data_ind = 0;
    } else if(old_def[i]=='r') {
      data_ind = prev_data_ind + sizeof(uint8_t*); //! NOTA: hay que quitarlo si va todo continuo
      data = prev_data;
      prev_data_ind = 0;
      prev_data = NULL;
    } else {
      PLAYER_ERROR1("GenericXBow::createMsg: Error parsing structure 1 in '%c'\n",old_def[i] );
      return NULL;
    }
  } 
  msg_def[j] = '\0';
#ifdef debug
  PLAYER_MSG2(1,"new_def[%d] '%s'",msg_nfields,msg_def);
#endif
  delete [] old_def; // this should be freed somewhere somehow
  old_def = msg_def;
  return msg; //return pointer to the message ready to be sent;
 }
