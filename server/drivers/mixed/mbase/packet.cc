// -*- mode:C++; tab-width:2; c-basic-offset:2; indent-tabs-mode:1; -*-

/**
  	*  Copyright (C) 2010
  	*     Ana Teresa Hernández Malagón <anat.hernandezm@gmail.com>
  	*	Movirobotics <athernandez@movirobotics.com>
	*  Copyright (C) 2006
	*     Videre Design
	*  Copyright (C) 2000  
	*     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
	*
	*  Videre Erratic robot driver for Player
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
	*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
	*
**/

/**
	Movirobotic's mBase robot driver for Player 3.0.1 based on erratic driver developed by Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard. Developed by Ana Teresa Herández Malagón.
**/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "packet.h"
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <stdlib.h> /* for exit() */
//#include <sys/poll.h>

#include "mbasedriver.h"

/**
	Print
*/
void mbasedriverPacket::Print() 
{
	int tam = packet[2] +4;
	if (packet) 
	{
		printf("\"");
		for(int i=0;i<tam;i++)
		{
			printf("%u ", packet[i]);
		}
		puts("\"");
	}
}

/**
	PrintHex
*/
void mbasedriverPacket::PrintHex() 
{
	int tam = packet[2] +4;
	if (packet) 
	{
		printf("\"Hex: ");
		for(int i=0;i<tam; i++)
		{
			printf("%.2x ", packet[i]);
		}
		puts("\"");
	}
}

/**
	Check
*/
bool mbasedriverPacket::Check() 
{
	uint8_t chksum = CalcChkSum();
	int tam = packet[2] +4;
	uint8_t received_chksum = packet[(int)tam-1];//size-1];
	if ( chksum == received_chksum ) 
	{
		if (debug_mbasedriver) 
		{
			printf("**Good packet: ");
			PrintHex();
		}
		return(true);
	}
	if (debug_mbasedriver) 
	{
		printf("**This packet failed checksum control (%x instead of %x): ", received_chksum, chksum);
		PrintHex();
	}
	return(false);
}

/**
	CalcChkSum
*/
uint16_t mbasedriverPacket::CalcChkSum() 
{
	uint8_t *buffer = &packet[2];
	int fin = packet[2];
	uint8_t c = 0;
	for(int i=0; i<=fin; i++)
	{
		c += (*buffer);
		buffer+=1;
	}
	if(debug_mbasedriver) 
	{
		printf("\n");
		printf("[CalcChkSum] chk = %02x\n", c);
	}
	return(c);
}

/**
	Receive
	- You can supply a timeout in milliseconds
*/
int mbasedriverPacket::Receive( int fd, uint16_t wait ) 
{
	uint8_t prefix[3];
	uint32_t skipped;
	uint16_t cnt;

	if (debug_mbasedriver)	printf("**Check for packets in Receive()\n");

	memset(packet,0,sizeof(packet));
	struct pollfd readpoll;
	readpoll.fd = fd; 
	readpoll.events = POLLIN | POLLPRI;
	readpoll.revents = 0;
//2.1.2 Aqui ifdef
#ifdef USE_SELECT
	fd_set read_set, error_set;
	struct timeval tv;
//#ifdef USE_SELECT ->2.0.5
	FD_ZERO(&read_set); 
	FD_ZERO(&error_set);
	FD_SET(fd, &read_set); 
	FD_SET(fd, &error_set);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	if (wait >= 1000)	tv.tv_sec = wait/1000;
	else			tv.tv_usec = wait*1000;
#endif

	// This block will terminate or move on when there is data to read
	if (wait) 
	{
		while (1) 
		{
#ifdef USE_SELECT
			int ret = select(fd+1, &read_set, 0, &error_set, &tv);
			if (ret)
			{
				if (debug_mbasedriver)	printf("**Data waiting\n");
				break;
			}
			else
				if (debug_mbasedriver)	printf("**No data\n");
#endif
			int8_t stuff = poll(&readpoll, 1, wait);
			if (stuff < 0) 
			{
				if (errno == EINTR) 
				{
					continue;
				}
				return 1;
			}
			if (stuff == 0) 
			{
				return (receive_result_e)timeout;
			}
			if (readpoll.revents & POLLIN)	break;
			printf("Serial port error\n");
			return (receive_result_e)failure;
		}
	}
	
	do 
	{
		memset(prefix,0,sizeof(prefix));
		skipped = 0;
		while(1) 
		{
			cnt = 0;
			// Once we've started receiving a packet, we have a tight timeout
			// Trouble with FTDI USB interface: needs longer timeout
			while( cnt!=1 ) 
			{
				if (wait) 
				{
					while (1) 
					{

#ifdef USE_SELECT						
						FD_ZERO(&read_set); 
						FD_ZERO(&error_set);
						FD_SET(fd, &read_set); 
						FD_SET(fd, &error_set);
						tv.tv_sec = 0;
						tv.tv_usec = 100*1000; // in usec's
						int ret = select(fd+1, &read_set, 0, &error_set, &tv);
						if (ret)	break;
#endif
						int8_t stuff = poll(&readpoll, 1, 100);
						if (stuff < 0) 
						{
							if (errno == EINTR) 
							{
								continue;
							}
							return 1;
						}
						if (stuff == 0) 
						{
							return (receive_result_e)timeout;
						}
						if (readpoll.revents & POLLIN)
							break;
						printf("Serial port error\n");
						return (receive_result_e)failure;
					}
				}
				int newcnt = read( fd, &prefix[2], 1 );
				if (newcnt < 0 && errno == EAGAIN)
				{
					if (debug_mbasedriver)	printf("__ continue\n");
					continue;
				}
				else if (newcnt < 0) {
					perror("mbasedriver::Receive:read:");
					return(1);
				}
				cnt++;
			}
			if (prefix[0]==0xFA && prefix[1]==0xFB) 
			{
				break;	
			}
			prefix[0]=prefix[1];
			prefix[1]=prefix[2];
			skipped++;
			
			if (skipped > 200) return (receive_result_e)timeout;
		}
		if (skipped>2 && debug_mbasedriver) printf("**Skipped %d bytes\n", skipped);
		size = prefix[2]+0x04;
		memcpy( packet, prefix, 3);
		cnt = 0;
		while( cnt!=prefix[2]+1 ) 
		{
			if (wait) 
			{
				while (1) 
				{
#ifdef USE_SELECT
					FD_ZERO(&read_set); 
					FD_ZERO(&error_set);
					FD_SET(fd, &read_set); 
					FD_SET(fd, &error_set);
					tv.tv_sec = 0;
					tv.tv_usec = 100*1000;	// in usec's

					int ret = select(fd+1, &read_set, 0, &error_set, &tv);
					if (ret)	break;
#endif
					int8_t stuff = poll(&readpoll, 1, 100);

					if (stuff < 0) 
					{
						if (errno == EINTR) 
						{
							continue;
						}
						return 1;
					}

					if (stuff == 0) 
					{
						return (receive_result_e)timeout;
					}

					if (readpoll.revents & POLLIN)	break;

					printf("Serial port error\n");
					return (receive_result_e)failure;
				}
			}
			//añadido +1 YO
			int newcnt = read( fd, &packet[3+cnt], prefix[2]-cnt+1);
			if (newcnt < 0 && errno == EAGAIN)	continue;
			else if (newcnt < 0) 
			{
				perror("mbasedriver::Receive:read:");
				return(1);
			}
			cnt += newcnt;
		}
	} while (!Check());  
	return(0);
}

/**
	Build
*/
int mbasedriverPacket::Build( unsigned char *data, unsigned char datasize ) 
{
	uint8_t chksum;
	size = datasize + 4;//5;
	int tam = datasize + 4;//5;
	/* header */
	packet[0]=0xFA;
	packet[1]=0xFB;
	if ( tam>198)
	{
		printf("mbasedriver driver error: Packet to robot can't be larger than 200 bytes");
		return(1);
	}
	packet[2] = datasize;//+2;
	memcpy( &packet[3], data, datasize );
	chksum = CalcChkSum();
	packet[3+(int)datasize] = chksum;//>> 8;
	if(debug_mbase_send_msj)
	{
		printf("BUILD	Paquete: %02x	%02x	%02x", packet[0], packet[1], packet[2]);
		for(int i=0; i<datasize; i++)	printf("	%02x", packet[i+3]);
		printf("	%02x\n", packet[3+datasize]);
	}
	return(0);
}

/**
	Send
*/
int mbasedriverPacket::Send( int fd) 
{
	int cnt=0;
	int tam = packet[2] +4;
	//printf("__SEND: size = %d	tam %d\n", size, tam);
	while(cnt!=tam)//size)
	{
		if((cnt += write( fd, packet+cnt, tam-cnt/*size-cnt*/ )) < 0) 
		{
			perror("Send");
			return(1);
		}
	}
	return(0);
}
