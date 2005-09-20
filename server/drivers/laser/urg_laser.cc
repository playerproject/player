#include "urg_laser.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>

urg_laser::urg_laser()
{
	laser_port = NULL;
}

urg_laser::~urg_laser()
{
	if (PortOpen())
		fclose(laser_port);
}

void urg_laser::Open(const char * PortName)
{
	if (PortOpen())
		fclose(laser_port);
	
	laser_port = fopen(PortName,"r+");
	if (laser_port == NULL)
	{
		printf("Failed to open Port: %s error = %d:%s\n",PortName,errno,strerror(errno));
	}
	
	int fd = fileno(laser_port);
	// set up new settings
	struct termios newtio;
	memset(&newtio, 0,sizeof(newtio));
	newtio.c_cflag = /*(rate & CBAUD) |*/ CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	newtio.c_lflag = ICANON;
	
	// activate new settings
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);
}


bool urg_laser::PortOpen()
{
	return laser_port != NULL;
}

int ReadUntil(int fd, unsigned char *buf, int len)
{
	int ret;
	int current=0;
	do
	{
		ret = read(fd,&buf[current],len-current);
		if (ret < 0)
			return ret;
		current += ret;
		if (current > 2 && buf[current-2] == '\n' && buf[current-1] == '\n')
		{
			printf("Got an end of command while waiting for more data, this is bad\n");
			return -1;	
		}
	} while(current < len);
	return len;
}

int urg_laser::GetReadings(urg_laser_readings_t * readings)
{
	unsigned char Buffer[11];
	memset(Buffer,0,11);
	assert(readings);
	
	if (!PortOpen())
		return -3;
	
	tcflush(fileno(laser_port), TCIFLUSH);
	// send the command
	fprintf(laser_port,"G00076801\n");

	int file = fileno(laser_port);
		
	// check the returned command
	ReadUntil(file, Buffer,10);
	
	/*	fscanf(laser_port,"%9s",Buffer);*/
	//Buffer[10]='\0';
	//printf("Read: %s",Buffer);
	
	if (strncmp((const char *) Buffer,"G00076801",9))
	{
		printf("Error reading command result: %s %d:%s\n",Buffer,errno,strerror(errno));
		tcflush(fileno(laser_port), TCIFLUSH);
		return -1;
	}
	
	// check the returned status
	ReadUntil(file, Buffer,2);
	//Buffer[2]='\0';
	//printf("Read: %s",Buffer);
	if (Buffer[0] != '0')
		return Buffer[0] - '0';
	
	for (int i=0; ; ++i)
	{
		ReadUntil(file, Buffer,2);
		
		if (Buffer[0] == '\n' && Buffer[1] == '\n')
			break;
		else if (Buffer[0] == '\n')
		{
			Buffer[0] = Buffer[1];
			if (ReadUntil(file, &Buffer[1],1) < 0)
				return -1;
		}
		
		if (i < MAX_READINGS)
		{	
			readings->Readings[i] = ((Buffer[0]-0x30) << 6) | (Buffer[1]-0x30);
		}
		else
		{
			printf("Got too many readings! %d\n",i);
		}
	}
	
	return 0;
}
