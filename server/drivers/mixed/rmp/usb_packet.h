/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2009
 *     Eric Grele and Goutham Mallapragda.
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

#ifndef _USB_PACKET_H_
#define _USB_PACKET_H_

#include "canio.h"
#include <libplayercommon/playercommon.h>

typedef struct usb_packet {
	union {
		unsigned char pkt_data[18];
		struct {
			unsigned char usb_message_header;
			unsigned char usb_command_identifier;
			unsigned char command_type;
			unsigned char unused0;
			unsigned char unused1;
			unsigned char unused2;
			unsigned short can_message_header;
			unsigned char unused3;
			unsigned char can_message[8];
			unsigned char usb_message_checksum;
		};
	};
} usb_packet_t;

class USBpacket {

private:
	unsigned short make_can_header( long id, unsigned int dlc, unsigned int flags );
	unsigned char compute_checksum();


public:

	typedef enum { CANA_DEV, USB_CMD_RESET } CommandType;

	usb_packet_t pkt;

	USBpacket();
	USBpacket( const CanPacket &pkt );
	operator CanPacket ();
	bool check();

	void print();

};


class USBIO {
private:
	int fd;
	bool synced;
public:
	USBIO() { fd = -1; synced = false; }
	int Init(const char *dev);
	int ReadPacket(CanPacket *pkt);
	int SyncRead( USBpacket &p );
	int WritePacket(CanPacket &pkt);
	int Shutdown();
};

#endif
