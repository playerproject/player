/* -*- Mode: C; tab-width: 2; c-basic-offset: 2; indent-tabs: t -*- */

/**
	* KartoWriter.
	* Copyright (C) 2007, SRI International.  All Rights Reserved.
    *
	* This program is free software; you can redistribute it and/or
	* modify it under the terms of the GNU General Public License
	* as published by the Free Software Foundation; either version 2
	* of the License, or (at your option) any later version.
    *
	* This program is distributed in the hope that it will be useful,
	* but WITHOUT ANY WARRANTY; without even the implied warranty of
	* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	* GNU General Public License for more details.
    *
	* You should have received a copy of the GNU General Public License
	* along with this program; if not, write to the Free Software
	* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
    *
	*  Author(s): Regis Vincent (vincent@ai.sri.com)
	**/
/** @ingroup drivers */
/** @{ */
/** @defgroup driver_kartowriter kartowriter
	* @brief Karto logger

	@par Compile-time dependencies

	- none

	@par Provides

	- @ref log

	@par Requires

	- laser sonar position2d

	@par Configuration requests

	- autorecord (boolean)
		- default: 1 (enable)
		- activate the logging at startup, to disable it, use 0

	- gzip (boolean)
		- default: 0 (disable)
		- compress the log file

	@par Configuration file options
	- file (string)
	- Default: "output.xml"
	- where to log

	@par Configuration requests

	- PLAYER_LOG_REQ_SET_WRITE_STATE
	- PLAYER_LOG_REQ_GET_STATE
	- PLAYER_LOG_REQ_SET_FILENAME (not yet implemented)

	@par Example

	@verbatim
	driver
	(
	name "readlog"
	filename  "jwing.log"
	speed 1
	provides ["log:0" "laser:0" "laser:1" "position2d:0"]
	autoplay 1
	alwayson 1
	)

	driver
	(
	name "kartowriter"
	plugin "libkartowriter_plugin"
	requires [ "laser:0" "laser:1" "position2d:0"]
	provides ["log:1" ]
	file "output.xml"
	alwayson 1
	)
@endverbatim

	@author Regis Vincent

*/
/** @} */
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <libplayercore/playercore.h>
#include <unistd.h>
#include <math.h>
#include <libplayercore/addr_util.h>
#include "sys/types.h"	/* for time_t */
#include "time.h"	/* for struct tm */

#include <list>
#define ID_MAX_SIZE 1024

	using namespace std;

class LogDevice {
private:

public:
	player_devaddr_t addr;
	Device* device;
	char *uid;

	// constructor
	LogDevice(player_devaddr_t* addr);

	//destructor
	~LogDevice();

	//prints the Device unique ID
	char* getUID();

};

LogDevice::LogDevice(player_devaddr_t* addr) {
	this->addr = *addr;
	uid = NULL;
	uid = getUID();
}

LogDevice::~LogDevice() {
	free(uid);
}




class KartoLogger:public Driver
{
private:

	list<LogDevice*> devices;

	char karto_filename[MAX_FILENAME_SIZE];
	FILE *karto_file;

	bool debug; // Debug flag to print more message
	bool enable; // Enabling flag
	bool compress; // Compress flag  (use gzip at the end or not)

		// The current robot pose
	player_pose2d_t position_pose;


	// When we start logging
	double start_time;

	// Functions to open and close the log file
	int OpenLog();
	int CloseLog();

	// Specific functions to write the correct XML format depending
	// of the data
	void WritePosition(player_devaddr_t dev,player_msghdr_t* hdr, player_position2d_data_t* data);
	void WriteLaserScan(player_devaddr_t dev,player_msghdr_t* hdr, player_laser_data_t* scan);
	void WriteSonarScan(player_devaddr_t dev,player_msghdr_t* hdr, player_sonar_data_t* scan);

	// Print the geometry (and all specific info) of any LogDevices
	int logGeometry(LogDevice *d);

public:

	int KartoLoggerDestroy();

	// constructor
	KartoLogger( ConfigFile* cf, int section);

	~KartoLogger();
	virtual void Main();

	int Setup();
	int Shutdown();

	char* getUID(player_devaddr_t dev);
	virtual int Unsubscribe(Device* id);

	// MessageHandler
	int ProcessMessage(QueuePointer &resp_queue, player_msghdr * hdr, void * data);
};

void computeQuaternion(double heading, double attitude, double bank ,double* x,double* y, double* z, double* w);

void
	KartoLogger::Main()
{
	for(;;)
	{
		usleep(20000);

		pthread_testcancel();

		this->ProcessMessages();
		// Verifie qu'on doit toujours marcher
		if(!this->enable)
			continue;
	}
	printf("exiting...\n");
}

int
	KartoLogger::Unsubscribe(Device* device)
{
	return(device->Unsubscribe(this->InQueue));
}

int KartoLogger::ProcessMessage(QueuePointer &resp_queue, player_msghdr * hdr, void * data)
{
	//writelog control
	if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
		PLAYER_LOG_REQ_SET_WRITE_STATE,
		this->device_addr))
	{
		player_log_set_write_state_t* sreq = (player_log_set_write_state_t*)data;

		if(sreq->state)
		{
			puts("KartoLogger is now logging...");
			this->enable = true;
		}
		else
		{
			puts("KartoLogger has stopped logging...");
			this->enable = false;
		}

	// send an empty ACK
		this->Publish(this->device_addr, resp_queue,
			PLAYER_MSGTYPE_RESP_ACK,
			PLAYER_LOG_REQ_SET_WRITE_STATE);
		return(0);
	}
	else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
		PLAYER_LOG_REQ_GET_STATE, this->device_addr))
	{
		player_log_get_state_t greq;

		greq.type = PLAYER_LOG_TYPE_WRITE;
		if(this->enable)
			greq.state = 1;
		else
			greq.state = 0;

		this->Publish(this->device_addr,
			resp_queue,
			PLAYER_MSGTYPE_RESP_ACK,
			PLAYER_LOG_REQ_GET_STATE,
			(void*)&greq, sizeof(greq), NULL);
		return(0);
	}
	else if(hdr->type == PLAYER_MSGTYPE_DATA)
	{
	// If logging is stopped, then don't log
		if(!this->enable)
			return(0);


		list<LogDevice*>::iterator iter;
		for (iter=devices.begin(); iter != devices.end(); iter++)
		{
			LogDevice* device = (*iter);
			if((device->addr.host == hdr->addr.host) &&
				(device->addr.robot == hdr->addr.robot) &&
				(device->addr.interf == hdr->addr.interf) &&
			(device->addr.index == hdr->addr.index)) {
				player_interface_t iface;
				::lookup_interface_code(device->addr.interf, &iface);
				switch (iface.interf)
				{
					case PLAYER_LASER_CODE:
					this->WriteLaserScan(device->addr, hdr, (player_laser_data_t*)data);
					break;
					case PLAYER_POSITION2D_CODE:
					this->WritePosition( device->addr, hdr, (player_position2d_data_t*)data);
					break;
					case PLAYER_SONAR_CODE:
					this->WriteSonarScan(device->addr, hdr, (player_sonar_data_t*)data);
					break;
				}
			}
		}
	}
	return(0);
}


//
// KartoLogger::Shutdown : Delete instances of our KartoLogger
//


int KartoLogger::Shutdown()
{
	// Stop the driver thread.
	this->enable=false;

	KartoLoggerDestroy();
	CloseLog();
	printf("KartoLogger  has been shutdown\n");
	return(0);
}

//
// KartoLogger::KartoLoggerDestroy : Delete instances of LogDevices
//
int KartoLogger::KartoLoggerDestroy()
{
	list<LogDevice*>::iterator iter;
	for (iter=devices.begin(); iter != devices.end(); iter++){
		Unsubscribe((*iter)->device);
		delete (*iter);
	}
	return(0);
}

//
// KartoLogger:
//
int KartoLogger::OpenLog() {
	this->karto_file = fopen(this->karto_filename, "w+");
	if(this->karto_file == NULL)
	{
		PLAYER_ERROR2("unable to open [%s]: %s\n", this->karto_filename, strerror(errno));
		return(-1);
	}
	time_t	t;
	time(&t);
	GlobalTime->GetTimeDouble(&(this->start_time));
	fprintf(this->karto_file,"<?xml version='1.0' encoding='utf-8'?>\n<!DOCTYPE KartoLogger SYSTEM \"http://karto.ai.sri.com/dtd/KartoLogger.dtd\" >\n<KartoLogger version=\"1.0\">\n<UTCTime>\n\t%s</UTCTime>\n",ctime(&t));
        return(0);
}


//
// KartoLogger:
//
int KartoLogger::CloseLog() {
	if(this->karto_file)
	{
		fprintf(this->karto_file,"</DeviceStates>\n</KartoLogger>\n");
		fflush(this->karto_file);
		fclose(this->karto_file);
		if (this->compress)
			printf("Sorry gzip compression is not yet implemented\n");
		this->karto_file = NULL;
	}
        return(0);
}

//
//KartoLogger::Setup Initialize KartoLogger using the KartoLoggerInit function
//
int
	KartoLogger::Setup()
{
	//int ret;
	// Open Log
	OpenLog();

	fprintf(this->karto_file,"<DeviceList>\n");
	list<LogDevice*>::iterator iter;
	for (iter=devices.begin(); iter != devices.end(); iter++)
	{
		(*iter)->device = deviceTable->GetDevice((*iter)->addr);
		printf(" Setup for %s\n",(*iter)->uid);
		if (!(*iter)->device) {
			PLAYER_ERROR("unable to locate suitable position device");
			return(-1);
		}
		if((*iter)->device->Subscribe(this->InQueue) != 0)
		{
			PLAYER_ERROR("unable to subscribe to position device");
			return(-1);
		}
		this->logGeometry((*iter));
	}
	fprintf(this->karto_file,"</DeviceList>\n<DeviceStates>\n");
// Start device thread
	this->StartThread();
	return(0);
}

//
// KartoLogger:
//
KartoLogger::~KartoLogger()
{
	//Shutdown();
}

//
// KartoLogger:KartoLogger constructor
//
KartoLogger::KartoLogger( ConfigFile* cf, int section) :
Driver(cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN,PLAYER_LOG_CODE )
{

	player_devaddr_t addr;

	for (int i = 0; i < cf->GetTupleCount(section, "requires"); i++)
	{
		if (cf->ReadDeviceAddr(&addr, section, "requires", -1, i, NULL) != 0)
		{
			this->SetError(-1);
			return;
		}
		devices.push_front(new LogDevice(&addr));
	}

	strncpy(karto_filename,
		cf->ReadString(section, "file","output.xml"),
		sizeof(karto_filename));
	if (cf->ReadInt(section, "debug",0) == 1)
		debug = true;

	if(cf->ReadInt(section, "autorecord", 1) > 0)
		this->enable = true;
	else
		this->enable = false;

	if(cf->ReadInt(section, "gzip", 0) > 0)
		this->compress = true;
	else
		this->compress = false;

}

//
// KartoLogger:
//
// a factory creation function
Driver* KartoLogger_Init( ConfigFile* cf, int section)
{
	return((Driver*)(new KartoLogger( cf, section)));
}


//
// KartoLogger:
//
// a driver registration function
void
	kartowriter_Register(DriverTable* table)
{
	table->AddDriver("kartowriter", KartoLogger_Init);
}

//
// KartoLogger:
//
int KartoLogger::logGeometry(LogDevice* dev) {
	Message* msg;
	switch(dev->addr.interf) {
		case PLAYER_LASER_CODE:
                  {
		if(!(msg = dev->device->Request(this->InQueue,
			PLAYER_MSGTYPE_REQ,
			PLAYER_LASER_REQ_GET_GEOM,
			NULL, 0, NULL,false)))
		{
			PLAYER_ERROR("failed to get laser geometry");
			return -1;
		}
		// Store the pose
		player_laser_geom_t laser_geom = *((player_laser_geom_t*)msg->GetPayload());
		char hostname[256];
		packedaddr_to_dottedip(hostname,256,dev->addr.host);
		double x,y,z,w;
		computeQuaternion(laser_geom.pose.pyaw,0,0,&x,&y,&z,&w);
		fprintf(this->karto_file,"<LaserRangeFinder>\n\t<ID>%s</ID>\n\t<Pose>\n\t\t<Position>\n\t\t\t<X>%.3f</X>\n\t\t\t<Y>0.0</Y>\n\t\t\t<Z>%.3f</Z>\n\t\t</Position>\n\t\t<Orientation>\n\t\t\t<X>%f</X>\n\t\t\t<Y>%f</Y>\n\t\t\t<Z>%f</Z>\n\t\t\t<W>%f</W>\n\t\t</Orientation>\n\t</Pose>\n</LaserRangeFinder>\n",dev->getUID(),laser_geom.pose.py,laser_geom.pose.px,x,y,z,w);
		break;
                  }
		case PLAYER_SONAR_CODE:
                  {
		// Get the sonar poses
		if(!(msg = dev->device->Request(this->InQueue,
			PLAYER_MSGTYPE_REQ,
			PLAYER_SONAR_REQ_GET_GEOM,
			NULL, 0, NULL,false)))
		{
			PLAYER_ERROR("failed to get sonar geometry");
			return(-1);
		}

		// Store the sonar poses
		// player_sonar_geom_t* sonar_geom = *((player_sonar_geom_t*)msg->GetPayload());
		break;
                  }
		case PLAYER_POSITION2D_CODE:
                  {
			fprintf(this->karto_file,"<Drive>\n\t<ID>%s</ID>\n\t<Pose>\n\t\t<Position>\n\t\t\t<X>0</X>\n\t\t\t<Y>0</Y>\n\t\t\t<Z>0.0</Z>\n\t\t</Position>\n\t\t<Orientation>\n\t\t\t<X>0</X>\n\t\t\t<Y>0</Y>\n\t\t\t<Z>0</Z>\n\t\t\t<W>1</W>\n\t\t</Orientation>\n\t</Pose>\n</Drive>\n",dev->getUID());
		break;
                  }
	}

	//delete msg;
	return(0);
}

//
// KartoLogger:
//
char* KartoLogger::getUID(player_devaddr_t dev) {
	char* id = (char*)malloc(ID_MAX_SIZE);
	char hostname[256];
	packedaddr_to_dottedip(hostname,256,dev.host);
	switch (dev.interf) {
		case PLAYER_LASER_CODE:
			sprintf(id,"%s:%d:laser:%d",hostname,dev.robot,dev.index);
		break;
		case PLAYER_SONAR_CODE:
		sprintf(id,"%s:%d:sonar:%d",hostname,dev.robot,dev.index);
		break;
		case PLAYER_POSITION2D_CODE:
		sprintf(id,"%s:%d:position2d:%d",hostname,dev.robot,dev.index);
		break;
		default:
		sprintf(id,"%s:%d:unknown:%d",hostname,dev.robot,dev.index);
	}
	return (id);
}

//
// KartoLogger:
//
char* LogDevice::getUID() {
	char* id = (char*)malloc(ID_MAX_SIZE);
	if (this->uid == NULL) {
		char hostname[256];
		packedaddr_to_dottedip(hostname,256,this->addr.host);
		switch (this->addr.interf) {
		case PLAYER_LASER_CODE:
			sprintf(id,"%s:%d:laser:%d",hostname,this->addr.robot,this->addr.index);
			break;
		case PLAYER_SONAR_CODE:
			sprintf(id,"%s:%d:sonar:%d",hostname,this->addr.robot,this->addr.index);
			break;
		case PLAYER_POSITION2D_CODE:
			sprintf(id,"%s:%d:position2d:%d",hostname,this->addr.robot,this->addr.index);
			break;
		default:
			sprintf(id,"%s:%d:unknown:%d",hostname,this->addr.robot,this->addr.index);
		}
		return (id);
	}
	else
		return (this->uid);
}

//
// KartoLogger:
//
void
	KartoLogger::WriteLaserScan(player_devaddr_t dev, player_msghdr_t* hdr,
	player_laser_data_t* scan)
{
	double t;
	GlobalTime->GetTimeDouble(&t);
	t = t - this->start_time;
	fprintf(this->karto_file,"<RangeScan>\n\t<DeviceID>%s</DeviceID>\n\t<Time>%.3lf</Time>\n\t\n\t<MinAngle>%.4f</MinAngle>\n\t<MaxAngle>%.4f</MaxAngle>\n\t<Resolution>%f</Resolution>\n\t\t<DistanceMeasurements>\n",getUID(dev),t,scan->min_angle, scan->max_angle,scan->resolution);
	for(unsigned int i=0;i<scan->ranges_count;i++)
	{
		//	fprintf(this->karto_file,"\t\t<float>%.3f</float>\n\t\t<Intensity>%d</Intensity>\n",scan->ranges[i],scan->intensity[i]);
		fprintf(this->karto_file,"\t\t<float>%.3f</float>\n",scan->ranges[i]);
	}
	fprintf(this->karto_file,"\t\t</DistanceMeasurements>\n</RangeScan>\n");
}


//
// KartoLogger:
//
void
	KartoLogger::WritePosition(player_devaddr_t dev, player_msghdr_t* hdr, player_position2d_data_t* data)
{
	double t;
	GlobalTime->GetTimeDouble(&t);
	t = t - this->start_time;
	this->position_pose = data->pos;
	double x,y,z,w;
	computeQuaternion(this->position_pose.pa,0,0,&x,&y,&z,&w);
	fprintf(this->karto_file,"<DrivePose>\n\t<DeviceID>%s</DeviceID>\n\t<Time>%.3lf</Time>\n\t<Pose>\n\t\t<Position>\n\t\t<X>%.3f</X>\n\t\t<Y>0.0</Y>\n\t\t<Z>%.3f</Z>\n\t\t</Position>\n\t\t<Orientation>\n\t\t<X>%f</X>\n\t\t<Y>%f</Y>\n\t\t<Z>%f</Z>\n\t\t<W>%f</W>\n\t\t</Orientation>\n\t</Pose>\n</DrivePose>\n",getUID(dev),t,this->position_pose.py,this->position_pose.px,x,y,z,w);
}


//
// KartoLogger:
//
void
	KartoLogger::WriteSonarScan(player_devaddr_t dev, player_msghdr_t* hdr,
	player_sonar_data_t* scan)
{

	fprintf(this->karto_file,"<RangeScan timestamp=\"%.3lf\">\n\t<SensorID>%s</SensorID>\n\t<RangeCount>%d</RangeCount>\n",hdr->timestamp,getUID(dev),scan->ranges_count);
	for(unsigned int i=0;i<scan->ranges_count;i++)
	{
		fprintf(this->karto_file,"\t\t<Range timestamp=\"%.3lf\">%.3f</Range>\n",hdr->timestamp,scan->ranges[i]);
	}
	fprintf(this->karto_file,"</RangeScan>\n");
}


//
// Compute the quaternion value for a give heading
//
void computeQuaternion(double heading, double attitude, double bank , double* x, double* y, double* z, double* w) {
	double c1,c2,c3,s1,s2,s3;
	c1 = cos(heading/2);
	c2 = cos(attitude/2);
	c3 = cos(bank/2);
	s1 = sin(heading / 2);
	s2 = sin(attitude / 2);
	s3 = sin(bank / 2);
	*w = c1*c2*c3 - s1*s2*s3 ;
	*x = s1* s2* c3 +c1* c2* s3 ;
	*y = s1* c2* c3 + c1* s2* s3;
	*z = c1* s2* c3 - s1* c2 *s3;
}
#if 0
////////////////////////////////////////////////////////////////////////////////
// Extra stuff for building a shared object.

/* need the extern to avoid C++ name-mangling  */
extern "C" {
	int player_driver_init(DriverTable* table)
	{
		puts("KartoLogger driver initializing");
		KartoLogger_Register(table);
		puts("KartoLogger initialization done");
		return(0);
	}
}
#endif
