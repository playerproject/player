/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2006 Radu Bogdan Rusu (rusu@cs.tum.edu)
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
 Desc: Driver for the Crossbow Mica2/Mica2Dot mote
 Author: Radu Bogdan Rusu
 Date: 09 Mar 2006
*/

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_mica2 mica2
 * @brief Crossbow Mica2/Mica2Dot mote sensor node

The mica2 driver controls the Crossbow Mica2/Mica2DOT mote sensor node. The 
MTS310 and MTS510 boards are supported.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_wsn

@par Requires

- none

@par Configuration requests

- PLAYER_WSN_REQ_POWER
- PLAYER_WSN_REQ_DATATYPE
- PLAYER_WSN_REQ_DATAFREQ

@par Configuration file options

- port (string)
  - Default: "/dev/ttyS0"
  - Serial port to which the Crossbow MIB510 is attached.  If you are
    using a USB/232 or USB/422 converter, this will be "/dev/ttyUSBx".

- rate (integer)
  - Default: 57600
  - Baud rate. Valid values are 19200 for MICA2DOT and 57600 for MICA2.

- node (integer tupple)
  - These are the calibration values for -1G/+1G for the ADXL202JE accelerometer
    sensor (see the appropriate data sheet on how to obtain it). Each sepparate
    board *MUST* be calibrated! Only usable when requesting data in converted
    metric units mode.
  - The tuple means: [node_id
                      group_id
                      calibration_negative_1g_x_axis
                      calibration_positive_1g_x_axis
                      calibration_negative_1g_y_axis
                      calibration_positive_1g_y_axis
                      calibration_negative_1g_z_axis
                      calibration_positive_1g_z_axis
                     ]

@par Example 

@verbatim
driver
(
  name "mica2"
  provides ["wsn:0"]
  port "/dev/ttyS0"
  rate "57600"
  # Calibrate node 0 from group 125 (default) with X={419,532} and Y={440,552}
  node [0 125 419 532 440 552 0 0]
  # Calibrate node 2 from group 125 (default) with X={447,557} and Y={410,520}
  node [2 125 447 557 410 520 0 0]
)
@endverbatim

@author Radu Bogdan Rusu

*/
/** @} */

#include "mica2.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <math.h>
#include <vector>

// Includes needed for player
#include <libplayercore/playercore.h>

// The Mica2 device class.
class Mica2 : public Driver
{
    public:
    	// Constructor
	Mica2 (ConfigFile* cf, int section);

	// Destructor
	~Mica2 ();

	// Implementations of virtual functions
	int Setup ();
	int Shutdown ();

	// This method will be invoked on each incoming message
	virtual int ProcessMessage (MessageQueue* resp_queue, 
				    player_msghdr * hdr,
				    void * data);
    private:

	// Main function for device thread.
	virtual void Main ();
	void RefreshData  ();
		
	// Port file descriptor
	int               fd;
		
	// Does the user want RAW or converted values?
	int               raw_or_converted;
		
	// Is the base node awake or sleeping?
	int               base_node_status;

	// RFID interface
	player_wsn_data_t data;
	player_wsn_cmd_t  cmd;
		
	const char*       port_name;
	int               port_speed;
		
	// Calibration values
	int               nodes_count;
	NCV               ncv;
		
	// Calibration values
	int               calibration_values[6];
	// Calibration node ID
	int               calibration_node_id;
		
	int ReadSerial   (unsigned char *buffer);
	int WriteSerial  (unsigned char *buffer, int length);
	NodeCalibrationValues FindNodeValues (unsigned int nodeID);
	player_wsn_data_t DecodeSerial (unsigned char *buffer, int length);

	int BuildXCommandHeader (unsigned char* buffer, int command, 
				 int node_id, int group_id, 
				 int device, int state, 
				 int rate);
	int calcByte (int crc, int b);
	void calcCRC (unsigned char *packet, int length);
		
	void ChangeNodeState (int node_id, int group_id, unsigned char state, 
			      int device, int enable, double rate);
	float ConvertAccel (unsigned short raw_accel, int neg_1g, int pos_1g);
		
};

////////////////////////////////////////////////////////////////////////////////
//Factory creation function. This functions is given as an argument when
// the driver is added to the driver table
Driver* Mica2_Init (ConfigFile* cf, int section)
{
    // Create and return a new instance of this driver
    return ((Driver*)(new Mica2 (cf, section)));
}

////////////////////////////////////////////////////////////////////////////////
//Registers the driver in the driver table. Called from the 
// player_driver_init function that the loader looks for
void Mica2_Register (DriverTable* table)
{
    table->AddDriver ("mica2", Mica2_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
Mica2::Mica2 (ConfigFile* cf, int section)
	: Driver (cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, 
		  PLAYER_WSN_CODE)
{
    int i = 0;
    int j = 0;
	
    port_name   = cf->ReadString (section, "port", DEFAULT_MICA2_PORT);
    port_speed  = cf->ReadInt (section, "speed", DEFAULT_MICA2_RATE);
    nodes_count = cf->ReadInt (section, "nodes", 0);
	
    for (i = 0; i < nodes_count; i++)
    {
    	char node_nr[7];
	sprintf (node_nr, "node%d", (i+1));
	NodeCalibrationValues n;
	n.node_id  = cf->ReadTupleInt (section, node_nr, 0, 0);
	n.group_id = cf->ReadTupleInt (section, node_nr, 1, 0);
	for (j = 0; j < 6; j++)
	    n.c_values[j] = cf->ReadTupleInt (section, node_nr, j+2, 0);
	    ncv.push_back (n);
    }

    // Defaults to converted values
    raw_or_converted = 1;
	
    return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor.
Mica2::~Mica2()
{
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int Mica2::Setup ()
{
    // Open serial port
    fd = open (port_name, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
	PLAYER_ERROR2 ("> Connecting to MIB510 on [%s]; [%s]...[failed!]",
		      (char*) port_name, strerror (errno));
	return (-1);
    }
    PLAYER_MSG0 (1, "> Connecting to MIB510... [done]");

    // Change port settings
    struct termios options;
    bzero (&options, sizeof (options));  // clear the struct for new settings
    // read satisfied if one char received
    options.c_cc[VMIN] = 1;
    options.c_cflag = CS8 | CLOCAL | CREAD;
    options.c_iflag = IGNBRK | IGNPAR;

    switch (port_speed)
    {
	case 19200:
	{
	    port_speed = B19200;
	    break;
	}
	case 57600:
	{
	    port_speed = B57600;
	    break;
	}
	default:
	{
	    port_speed = B57600;
	    break;
	}
    }
    // Set the baudrate to the given portSpeed
    cfsetispeed (&options, port_speed);
    cfsetospeed (&options, port_speed);

    // Activate the settings for the port
    if (tcsetattr (fd, TCSANOW, &options) < 0)
    {
	PLAYER_ERROR (">> Unable to set serial port attributes !");
	return (-1);
    }

    // Make sure queues are empty before we begin
    tcflush (fd, TCIFLUSH);

    // Start the device thread
    StartThread ();

    return (0);
}

////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int Mica2::Shutdown ()
{
    // Stop the driver thread
    StopThread ();

    // Close the serial port
    close (fd);
	
    PLAYER_MSG0 (1, "> Mica2 driver shutting down... [done]");
    return (0);
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void Mica2::Main () 
{
    // Zero data
    memset (&data, 0, sizeof (player_wsn_data_t));
	
    timespec sleepTime = {0, 0};
	
    // The main loop; interact with the device here
    while (true)
    {
	// test if we are supposed to cancel
	pthread_testcancel ();
	
	// Process any pending messages
	ProcessMessages();
		
	// Interact with the device, and push out the resulting data.
	if (base_node_status != 0) // if the base node is asleep, no serial 
	                    	   // data can be read
	    RefreshData ();
		
	nanosleep (&sleepTime, NULL);
    }
}

////////////////////////////////////////////////////////////////////////////////
// ProcessMessage function
int Mica2::ProcessMessage (MessageQueue* resp_queue, 
			   player_msghdr * hdr,
			   void * data)
{	
    assert (hdr);
    assert (data);
	
    if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD, 
	PLAYER_WSN_CMD_DEVSTATE, device_addr))
    {
    	// Actuate various devices on the node
	player_wsn_cmd_t *command = (player_wsn_cmd_t*)data;
		
	ChangeNodeState (command->node_id, command->group_id, 
			 2, command->device, command->enable, -1);
		
	return 0;
    }
    else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, 
	 PLAYER_WSN_REQ_POWER, device_addr))
    {
	// Put the node in sleep mode (0) or wake it up (1)
	player_wsn_power_config *powerconfig = 
			(player_wsn_power_config*)data;
		
	// Only allow 0/1 values here
	if ((powerconfig->value != 0) && (powerconfig->value != 1))
	{
	    Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, 
		     hdr->subtype);
	    return -1;
	}
			
	ChangeNodeState (powerconfig->node_id, powerconfig->group_id, 
			 powerconfig->value, -1, -1, -1);
		
	Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, 
		 hdr->subtype);
	return 0;
    }
    else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, 
	 PLAYER_WSN_REQ_DATATYPE, device_addr))
    {
	// Change the data type to RAW or converted metric units
	player_wsn_datatype_config *datatype = 
			(player_wsn_datatype_config*)data;
	if (datatype->value == 1)
	    raw_or_converted = 1;
	else
	    raw_or_converted = 0;

	Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, 
		 hdr->subtype);
	return 0;
    }
    else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ, 
	 PLAYER_WSN_REQ_DATAFREQ, device_addr))
    {
	// Change the data frequency rate
	player_wsn_datafreq_config *datafreq = 
			(player_wsn_datafreq_config*)data;
	ChangeNodeState (datafreq->node_id, datafreq->group_id, 
			 3, -1, -1, datafreq->frequency);

	Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, 
		 hdr->subtype);
	return 0;
    }
    else
    {
	return -1;
    }
	
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// calcByte function
int Mica2::calcByte (int crc, int b)
{
    crc = crc ^ (int)b << 8;
	
    for (int i=0; i < 8; i++)
    {
	if ((crc & 0x8000) == 0x8000)
	    crc = crc << 1 ^ 0x1021;
	else
	    crc = crc << 1;
    }

    return (crc & 0xFFFF);
}

////////////////////////////////////////////////////////////////////////////////
// calcCRC function
void Mica2::calcCRC (unsigned char *packet, int length)
{
    int crc   = 0;
    int index = 1;
    int count = length - 3;
	
    while (count > 0) {
    	crc = calcByte (crc, packet[index++]);
	count--;
    }
    packet[length-2] = (char)(crc & 0xFF);
    packet[length-1] = (char)((crc >> 8) & 0xFF);
}

////////////////////////////////////////////////////////////////////////////////
// BuildXCommandHeader function
int Mica2::BuildXCommandHeader (unsigned char* buffer, int command, 
				int node_id, int group_id, 
				int device, int state, 
				int rate)
{
    // TinyOS header
    XCommandMsg *msg = (XCommandMsg *)buffer;
    msg->tos.addr    = (unsigned short)node_id;// Broadcast address: 0xFFFF
    msg->tos.type    = 0x30;                   // AMTYPE_XCOMMAND
    msg->tos.group   = (unsigned char)group_id;// Broadcast: 0xFF
    msg->tos.length  = sizeof (XCommandMsg) - sizeof (TOSMsgHeader);

    // Data payload
    msg->seq_no      = 0xFF; // Broadcast sequence number
    msg->destination_id = (unsigned short)node_id; // Broadcast address: 0xFFFF
    msg->inst[0].cmd    = (unsigned short)command;
    if (device != -1)
    {
	msg->inst[0].param.actuate.device = (unsigned short)device;
	msg->inst[0].param.actuate.state  = (unsigned short)state;
	return sizeof (XCommandMsg);
    }
    if (rate != -1)
    {
	msg->inst[0].param.new_rate = (unsigned int)rate;
	return sizeof (XCommandMsg);
    }
    msg->inst[0].param.new_rate = 0xCCCCCCCC;  // Fill unused in known way
    return sizeof (XCommandMsg);
}


////////////////////////////////////////////////////////////////////////////////
// ChangeNodeState function
void Mica2::ChangeNodeState (int node_id, int group_id, unsigned char state, 
			     int device, int enable, double rate)
{
    unsigned char buffer[255];
    int index = 0;
	
    // Assign defaults to {node,group}_id
    if ((group_id == -1) || (group_id == 0))
    	group_id = 0xFF;
    if (node_id == -1)
	node_id = 0xFFFF;
		
    int node_sleep = 0;
    if (rate != -1)
	node_sleep =  static_cast<int> (1000 / rate);

    // Start constructing the TinyOS packet
    buffer[index++] = 0x7e;                // serial start byte
    buffer[index++] = 0x41;                // P_PACKET_ACK
    buffer[index++] = 0xFF;                // Broadcast sequence number
	
    switch (state)
    {
    	case 0:                            // sleep (XCOMMAND_SLEEP)
	{
	    index += BuildXCommandHeader 
			(buffer+index, 0x11, node_id, group_id, -1, -1, -1);
	    if (node_id == 0)             // base node?
			base_node_status = 0;
			
	    break;
	}
	case 1:                            // wake up (XCOMMAND_WAKEUP)
	{
	    index += BuildXCommandHeader 
			(buffer+index, 0x12, node_id, group_id, -1, -1, -1);
	    if (node_id == 0)             // base node?
			base_node_status = 1;
	    break;
	}
	case 2:                            // actuate (XCOMMAND_ACTUATE)
	{
	    index += BuildXCommandHeader 
			(buffer+index, 0x40, node_id, group_id, device, enable, -1);
	    break;
	}
	case 3:                            // set_rate (XCOMMAND_SET_RATE)
	{
	    index += BuildXCommandHeader 
			(buffer+index, 0x20, node_id, group_id, -1, -1, node_sleep);
	    break;
	}
    }
	
    index += 2;
    calcCRC (buffer, index);               // calculate and add CRC
	
    buffer[index] = 0x7e;                  // SYNC_BYTE
	
    WriteSerial (buffer, ++index);
}

////////////////////////////////////////////////////////////////////////////////
// RefreshData function
void Mica2::RefreshData ()
{
    int length;
    unsigned char buffer[255];
    memset (&data, 0, sizeof (player_wsn_data_t));
	
    // Get the time at which we started reading
    // This will be a pretty good estimate of when the phenomena occured
    struct timeval time;
    GlobalTime->GetTime(&time);
	
    length = ReadSerial (buffer);
    if (length < 16)        // minimum valid packet size
	return;             // ignore partial packets

    data = DecodeSerial (buffer, length);

    // Write the WSN data
    Publish (device_addr, NULL, PLAYER_MSGTYPE_DATA, PLAYER_WSN_DATA,
	     &data, sizeof (player_wsn_data_t), NULL);
	
    return;
}

//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
// ReadSerial function - reads one XSensorPacket from the serial port
int Mica2::ReadSerial (unsigned char *buffer)
{
    unsigned char c;
    int err, i = 0;

    buffer[i] = 0x7e;          // serial start byte
    while (1) {
    	err = read (fd, &c, 1);
	if (err < 0)
	{
	    PLAYER_ERROR (">> Error reading from serial port !");
	    return (-1);
	}
	if (err == 1)
	{
	    if (++i > 255) return i;
	    buffer[i] = c;
	    if (c == 0x7e) return i;
	}
    }
}

////////////////////////////////////////////////////////////////////////////////
// WriteSerial function - write one XSensorPacket to the serial port
int Mica2::WriteSerial (unsigned char *buffer, int length)
{
    unsigned char c;
    int err, i = 0;

    c = buffer[0];
    while (1)
    {
	if (i>= length)
	    return length;
	c = buffer[i++];

	err = write (fd, &c, 1);

	if (err < 0)
	{
	    PLAYER_ERROR (">> Error writing to serial port !");
	    return (-1);
	}
    }
}

////////////////////////////////////////////////////////////////////////////////
// FindNodeValues function - find the appropriate calibration values for nodeID
NodeCalibrationValues Mica2::FindNodeValues (unsigned int nodeID)
{
    NodeCalibrationValues n;
	
    unsigned int i = 0;
	
    for (i = 0; i < ncv.size (); i++)
    {
    	n = ncv.at (i);
	
	if (n.node_id == nodeID)
			break;
	}
	
	return n;
}

////////////////////////////////////////////////////////////////////////////////
// DecodeSerial function - decode a XSensorPacket
player_wsn_data_t Mica2::DecodeSerial (unsigned char *buffer, int length)
{
    NodeCalibrationValues node_values;
	
    player_wsn_data_t temp_data;
    if (length < 2) return temp_data;

    int i = 0, o = 2;    // index and offset

    while (i < length)
    {
	if (buffer[o] == 0x7d) {          // handle escape characters
	    buffer[i++] = buffer[++o] ^ 0x20;
	    ++o;
	} 
	else 
	{
	    buffer[i++] = buffer[o++];
	}
    }
	
    switch (buffer[2])
    {
	case 0x03:
	{                       // a HEALTH packet
	    // Health offset to data payload
	    //SensorPacket *packet = (SensorPacket *)(buffer + 5);
	    //HealthData *data = (HealthData *)packet;
	    break;
	}
	case 0x33:
	{                       // a MULTIHOP packet
	    // Multihop offset to data payload
	    SensorPacket *packet = (SensorPacket *)(buffer + 12);
	    switch (packet->board_id)
	    {
		case 0x02:
		{                       // MTS510
		    if (packet->packet_id == 1)
		    {
			MTS510Data *data = (MTS510Data *)packet->data;
			temp_data.node_type      = packet->board_id;
			temp_data.node_id        = packet->node_id;
						
			temp_data.node_parent_id = packet->parent;
						
			temp_data.data_packet.light       = data->light;
			int sound = (data->sound[0] + data->sound[1] + 
				     data->sound[2] + data->sound[3] + 
				     data->sound[4]) / 5;
			temp_data.data_packet.mic         = sound;
						
			if (raw_or_converted == 1)
			{
			    node_values = FindNodeValues (packet->node_id);
							
			    temp_data.data_packet.accel_x = ConvertAccel 
				(data->accelX, 
				 node_values.c_values[0], 
				 node_values.c_values[1]);
			    temp_data.data_packet.accel_y = ConvertAccel 
				(data->accelY,
				node_values.c_values[2],
				node_values.c_values[3]);

//			    temp_data.data_packet.battery     =
// 				(unsigned short)(614400 / (float)data->vref);
			} 
			else
			{
			    temp_data.data_packet.accel_x     = data->accelX;
			    temp_data.data_packet.accel_y     = data->accelY;
			}
			temp_data.data_packet.accel_z     = -1;
			temp_data.data_packet.magn_x      = -1;
			temp_data.data_packet.magn_y      = -1;
			temp_data.data_packet.magn_z      = -1;
			temp_data.data_packet.temperature = -1;
			temp_data.data_packet.battery     = -1;
		    }
		    break;
		}
		case 0x84:
		{                       // MTS310
		    if (packet->packet_id == 1)
		    {
			MTS310Data *data = (MTS310Data *)packet->data;
			temp_data.node_type      = packet->board_id;
			temp_data.node_id        = packet->node_id;
			temp_data.node_parent_id = packet->parent;
				
			temp_data.data_packet.mic         = data->mic;
				
		    	if (raw_or_converted == 1)
			{
			    node_values = FindNodeValues (packet->node_id);
							
			    temp_data.data_packet.accel_x = ConvertAccel 
				(data->accelX,
				 node_values.c_values[0],
				 node_values.c_values[1]);
			    temp_data.data_packet.accel_y = ConvertAccel 
				(data->accelY,
				 node_values.c_values[2],
				 node_values.c_values[3]);

			    // Convert battery to Volts
			    temp_data.data_packet.battery     =
				(1252352 / (float)data->vref) / 1000;
							
			    // Convert temperature to degrees Celsius
			    float thermistor = (float)data->thermistor;
			    unsigned short rthr = (unsigned short)
				(10000 * (1023 - thermistor) / thermistor);
							
			    temp_data.data_packet.temperature = 
				(1 / (0.001307050f + 0.000214381f *
				log (rthr) + 0.000000093f *
				pow (log (rthr),3))) - 273.15;
							
			    // Convert the magnetometer data to Gauss
			    temp_data.data_packet.magn_x      = 
				(data->magX / (1.023*2.262*3.2)) / 1000;
			    temp_data.data_packet.magn_y      = 
				(data->magY / (1.023*2.262*3.2)) / 1000;
							
			    // Convert the light to mV
			    temp_data.data_packet.light       = (data->light *
				temp_data.data_packet.battery / 1023);
			} 
			else
			{
			    temp_data.data_packet.accel_x     = data->accelX;
			    temp_data.data_packet.accel_y     = data->accelY;
			    temp_data.data_packet.battery     = data->vref;
			    temp_data.data_packet.temperature = 
				data->thermistor;
			    temp_data.data_packet.magn_x      = data->magX;
			    temp_data.data_packet.magn_y      = data->magY;
			    temp_data.data_packet.light       = data->light;
			}
			temp_data.data_packet.accel_z     = -1;
			temp_data.data_packet.magn_z      = -1;
		    }
		    break;
		}
	    }
	    break;
	}
	default:
	{
	    // we only handle HEALTH and MULTIHOP package types for now
	    break;
	}
    }
	
    return temp_data;
}

////////////////////////////////////////////////////////////////////////////////
// ConvertAccel function - convert RAW accel. data to metric units (m/s^2)
float Mica2::ConvertAccel (unsigned short raw_accel, int neg_1g, int pos_1g)
{
    if (neg_1g == 0)
	neg_1g = 450;
    if (pos_1g == 0)
	pos_1g = 550;
	
    float sensitivity  = (pos_1g - neg_1g) / 2.0f;
    float offset       = (pos_1g + neg_1g) / 2.0f;
    float acceleration = (raw_accel - offset) / sensitivity;
    return acceleration * 9.81;
}
//------------------------------------------------------------------------------

