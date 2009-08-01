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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <sys/time.h>
#include "usb_packet.h"


USBpacket::USBpacket( ) {
	pkt.usb_message_header = 0xf0;
	pkt.usb_command_identifier = 0x55;
	// the docs say send 0x01 but the sample code sends a 0
	pkt.command_type = 0;
	pkt.unused0 = 0;
	pkt.unused1 = 0;
	pkt.unused2 = 0;
	pkt.unused3 = 0;

	return;
}

#ifndef canMSG_RTR
#define canMSG_RTR 1
#endif

unsigned short USBpacket::make_can_header( long id, unsigned int dlc, unsigned int flags ) {
	unsigned short hdr;

	/* not really a CANBUS header
	hdr = 0x0000;
	hdr = ((unsigned short) (((unsigned long) id) & 0x000007ff)) << 5;
	hdr += ( dlc & 0x0000000f );
	if( flags & canMSG_RTR )
		hdr = hdr | 0x0010;
	 */
	hdr = (unsigned short)(( id & 0x000000ff ) << 8);
	hdr += (unsigned short)(( id & 0x0000ff00 ) >> 8);

	return hdr;
}


USBpacket::USBpacket( const CanPacket &can ) {
	pkt.usb_message_header = 0xf0;
	pkt.usb_command_identifier = 0x55;
	// the docs say send 0x01 but the sample code sends a 0
	pkt.command_type = 0;
	pkt.unused0 = 0;
	pkt.unused1 = 0;
	pkt.unused2 = 0;
	pkt.can_message_header = make_can_header( can.id, can.dlc, can.flags );
	pkt.unused3 = 0;
	memcpy( pkt.can_message, can.msg, sizeof(pkt.can_message) );

	pkt.usb_message_checksum = compute_checksum();
	return;
}

USBpacket::operator CanPacket() {
	CanPacket can;
	unsigned short hi, lo;
	/* not really a CANBUS header
        can.id = pkt.can_message_header >> 5;
	can.dlc = pkt.can_message_header & 0x000f;
	 */
	hi = pkt.pkt_data[5];
	hi = (hi>>5) & 0x0007;
	lo = pkt.pkt_data[4];
	lo = (lo<<3);
	can.id = ( hi | lo ) & 0x0fff;

	if( pkt.can_message_header & 0x0010 )
		can.flags = canMSG_RTR;
	else
		can.flags = canMSG_STD;
	memcpy( can.msg, pkt.can_message, sizeof(can.msg) );
	return can;
}

bool USBpacket::check() {
	return compute_checksum() == pkt.usb_message_checksum;
}

unsigned char USBpacket:: compute_checksum() {

	unsigned int i;
	unsigned short chk, high;

	chk = 0;
	for( i=0; i<17; i++ ) {
		chk += (unsigned short)pkt.pkt_data[i];
	}

	high = (unsigned short)(chk >> 8);
	chk = (0x00ff & chk) + high;
	high = (unsigned short)(chk >> 8);
	chk = (0x00ff & chk) + high;
	chk = 0x00ff & (~chk + 1);

	return (unsigned char) chk;
}

void USBpacket::print() {

	int i;
	printf( "USBpacket raw " );
	for( i=0; i<18; i++ )
		printf( " %02x", pkt.pkt_data[i] );

	return;
}

int USBIO::Init( const char *dev ) {

	struct termios term;
	int flags;

	fd = open( dev, O_RDWR | O_SYNC | O_NONBLOCK, S_IRUSR | S_IWUSR );
	if( fd < 0 ) {
		PLAYER_ERROR2( "open of usb device %s failed: %s\n", dev, strerror(errno) );
		fd = -1;
		return -1;
	}

	if(tcflush(fd, TCIFLUSH ) < 0 ) {
		PLAYER_ERROR1("tcflush() failed: %s", strerror(errno));
		close(fd);
		fd = -1;
		return -1;
	}


	if(tcgetattr(fd, &term) < 0 ) {
		PLAYER_ERROR1("tcgetattr() failed: %s", strerror(errno));
		close(fd);
		fd = -1;
		return -1;
	}

	cfmakeraw(&term);
	cfsetispeed(&term, B460800);
	cfsetospeed(&term, B460800);

	if(tcsetattr(fd, TCSAFLUSH, &term) < 0 ) {
		PLAYER_ERROR1("tcsetattr() failed: %s", strerror(errno));
		close(fd);
		fd = -1;
		return -1;
	}

	if((flags = fcntl(fd, F_GETFL)) < 0) {
		PLAYER_ERROR1("fcntl() failed: %s", strerror(errno));
		close(fd);
		fd = -1;
		return -1;
	}

	if(fcntl(fd, F_SETFL, flags ^ O_NONBLOCK) < 0) {
		PLAYER_ERROR1("fcntl() failed: %s", strerror(errno));
		close(fd);
		fd = -1;
		return -1;
	}

	return 0;
}

int USBIO::ReadPacket(CanPacket *pkt) {

	int i;
	int ret;
	USBpacket p;
	const int max_packets = 100;

	for( i=0; i<max_packets; i++ ) {
		if( !synced ) {
			ret = SyncRead( p );
		}
		else {
			ret = read( fd, &p.pkt, sizeof( p.pkt ) );
		}

		if( ret < 0 ) {
			PLAYER_ERROR1("read() failed: %s", strerror(errno));
			return ret;
		}


		if( p.pkt.usb_message_header != 0xf0 )
			synced = false;
		else if( !p.check() )
			continue;
		else if( p.pkt.pkt_data[2] == 0xbb )
			continue;
		else
			break;
	}

	if( !synced || i >= max_packets ) {
		PLAYER_ERROR("reading from usb failed: too many bad packets\n" );
		return -1;
	}

	*pkt = p;

	if( pkt->id == 0x402 ) {
		struct timeval now;
		short s1, s2;
		s1 = (pkt->msg[0] << 8 ) + pkt->msg[1];
		s2 = (pkt->msg[2] << 8 ) + pkt->msg[3];
		gettimeofday( &now, 0 );
	}

	return (int) (p.pkt.can_message_header & 0x000f);
}

int USBIO::SyncRead( USBpacket &p ) {

	int ret;

	do {
		ret = read( fd, &p.pkt, 1 );
		if( ret < 0 ) {
			perror( "USBIO::SyncRead reading first byte" );
			return -1;
		}
	} while( p.pkt.pkt_data[0] != 0xf0 );
	ret = read( fd, &(p.pkt.pkt_data[1]), sizeof(p.pkt)-1 );
	if( ret < 0 ) {
		perror( "USBIO::SyncRead reading remaining packet" );
		return -1;
	}

	synced = true;

	return ret+1;
}

int USBIO::WritePacket(CanPacket &pkt) {
	int ret;
	USBpacket up( pkt );

	ret = write( fd, &up, sizeof(up) );

	struct timeval now;
	short s1, s2;
	s1 = (pkt.msg[0] << 8 ) + pkt.msg[1];
	s2 = (pkt.msg[2] << 8 ) + pkt.msg[3];
	gettimeofday( &now, 0 );

	return ret;
}

int USBIO::Shutdown() {
	if( fd >= 0 )
		close(fd);

	return 0;
}

