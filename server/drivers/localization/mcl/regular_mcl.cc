/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
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

/*
 *   the mcl (Monte-Carlo Localization) device. This device implements
 *   the regular MCL only. Other extensions (i.g. mixture MCL, adaptive MCL)
 *   will be implemented in separate MCL devices.
 */
#include "regular_mcl.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>

#include <playertime.h>
extern PlayerTime* GlobalTime;

// so we can access the deviceTable and extract pointers to the sonar
// and position objects
#include <devicetable.h>
extern CDeviceTable* deviceTable;
extern int global_playerport; // used to get at devices


// constructor
RegularMCL::RegularMCL(char* interface, ConfigFile* cf, int section)
    : PSDevice(sizeof(player_localization_data_t),0,10,10)
{
    // load configuration : update speed
    this->frequency = cf->ReadFloat(section, "frequency", 1.0);

    // load configuration : the number of particles
    this->num_particles = cf->ReadInt(section, "num_particles", 5000);

    // load configuration : a distance sensor
    const char *sensor_str = cf->ReadString(section, "sensor_type", "sonar");
    if (strcmp(sensor_str, "sonar") == 0)
	this->sensor_type = PLAYER_MCL_SONAR;
    else if (strcmp(sensor_str, "laser") == 0)
	this->sensor_type = PLAYER_MCL_LASER;
    else
	this->sensor_type = PLAYER_MCL_NOSENSOR;
    this->sensor_index = cf->ReadInt(section, "sensor_index", 0);
    this->sensor_max = cf->ReadInt(section, "sensor_max", 5000);
    this->sensor_num_samples = cf->ReadInt(section, "sensor_num_samples", 0);

    // load configuration : a motion sensor
    this->motion_index = cf->ReadInt(section, "motion_index", 0);

    // load configuration : a world model (map)
    this->map_file = cf->ReadString(section, "map", NULL);
    this->map_ppm = cf->ReadInt(section, "map_ppm", 10);
    this->map_threshold = cf->ReadInt(section, "map_threshold", 240);

    // load configuration : a sensor model
    this->sm_s_hit = cf->ReadFloat(section, "sm_s_hit", 300.0);
    this->sm_lambda = cf->ReadFloat(section, "sm_lambda", 0.001);
    this->sm_o_small = cf->ReadFloat(section, "sm_o_small", 100.0);
    this->sm_z_hit = cf->ReadFloat(section, "sm_z_hit", 50.0);
    this->sm_z_unexp = cf->ReadFloat(section, "sm_z_unexp", 30.0);
    this->sm_z_max = cf->ReadFloat(section, "sm_z_max", 5.0);
    this->sm_z_rand = cf->ReadFloat(section, "sm_z_rand", 200.0);
    this->sm_precompute = cf->ReadInt(section, "sm_precompute", 1);

    // load configuration : an action model
    this->am_a1 = cf->ReadFloat(section, "am_a1", 0.01);
    this->am_a2 = cf->ReadFloat(section, "am_a2", 0.0002);
    this->am_a3 = cf->ReadFloat(section, "am_a3", 0.03);
    this->am_a4 = cf->ReadFloat(section, "am_a4", 0.1);

    // initialize clustering algorithm
    this->clustering = new MCLClustering(this->num_particles);
}


// when the first client subscribes to a device
int RegularMCL::Setup(void)
{
    printf("RegularMCL initialising...\n");

    // set update speed
    this->period = 1.0 / this->frequency;

    // get the pointer to the distance sensor
    player_device_id_t device;
    device.port = device_id.port;
    if (this->sensor_type == PLAYER_MCL_SONAR) device.code = PLAYER_SONAR_CODE;
    else if (this->sensor_type == PLAYER_MCL_LASER) device.code = PLAYER_LASER_CODE;
    else {
	PLAYER_ERROR("  invalid distance sensor.");
	return 1;
    }
    device.index = this->sensor_index;
    this->distance_device = deviceTable->GetDevice(device);
    if (!this->distance_device) {
	PLAYER_ERROR("  unable to find a distance sensor device");
	return (-1);
    }

    // subscribe to the distance sensor device, but fail if it fails
    if (this->distance_device->Subscribe(this) != 0) {
	PLAYER_ERROR("  unable to subscribe to the distance sensor device");
	return (-1);
    }

    // read the configuartion of the distance sensor
    this->ReadConfiguration();

    // get the pointer to the motion sensor
    device.port = device_id.port;
    device.code = PLAYER_POSITION_CODE;
    device.index = this->motion_index;
    this->motion_device = deviceTable->GetDevice(device);
    if (!this->motion_device) {
	PLAYER_ERROR("  unable to find a motion sensor device");
	return (-1);
    }

    // subscribe to the motion sensor device, but fail if it fails
    if (this->motion_device->Subscribe(this) != 0) {
	PLAYER_ERROR("  unable to subscribe to the motion sensor device");
	return (-1);
    }

    // construct a world model (map)
    this->map = new WorldModel(this->map_file,
	    		       this->map_ppm,
			       this->sensor_max,
			       this->map_threshold);
    if (! this->map->isLoaded())
	PLAYER_ERROR1("  cannot load the map file (%s)", this->map_file);

    // construct a sensor model
    this->sensor_model = new MCLSensorModel(this->sensor_type,
	    				    this->num_ranges,
					    this->poses,
					    this->sensor_max,
					    this->sm_s_hit,
					    this->sm_lambda,
					    this->sm_o_small,
					    this->sm_z_hit,
					    this->sm_z_unexp,
					    this->sm_z_max,
					    this->sm_z_rand,
					    (this->sm_precompute)?true:false);

    // construct an action model
    this->action_model = new MCLActionModel(this->am_a1,
					    this->am_a2,
					    this->am_a3,
					    this->am_a4);

    // initialize particles
    this->Reset();

    // wait until the sensors are ready
    while (this->distance_device->GetNumData(this) == 0)
	usleep(10000);
    while (this->motion_device->GetNumData(this) == 0)
	usleep(10000);

    // done
    puts("RegularMCL is ready.");

    // start the device thread
    StartThread();

    return 0;
}


// when the last client unsubscribes from a device
int RegularMCL::Shutdown(void)
{
    // unsubscribe from the distance sensor device
    this->distance_device->Unsubscribe(this);
    this->motion_device->Unsubscribe(this);

    // shutdown MCL device
    StopThread();

    // delete models
    delete this->map;
    delete this->sensor_model;
    delete this->action_model;

    // de-allocate memory
    delete[] this->poses;
    delete[] this->ranges;

    // done
    puts("RegularMCL has been shutdown");

    return 0;
}


// main update function
void RegularMCL::Main(void)
{
    double last = 0;
    while (true)
    {
	// test if we are supposed to cancel
	pthread_testcancel();
	
	// Update the configuration.
	UpdateConfig();

	// Get the time at which we started reading
	// This will be a pretty good estimate of when the phenomena occured
	struct timeval time;
	GlobalTime->GetTime(&time);
	double current = time.tv_sec + time.tv_usec/1000000.0;

	// perform the localization process if it is required
	if ((current-last) >= this->period)
	{
	    // collect sensor data
	    pose_t c_odometry;
	    if (!this->ReadRanges(this->ranges) || !this->ReadOdometry(c_odometry)) {
		usleep(100000);
		continue;		// do nothing until sensors are ready
	    }

	    // when a robot didn't move, don't update particles
	    if (c_odometry == this->p_odometry) {
		player_localization_data_t data;
		HypothesisConstruction(data);
		PutData((uint8_t*) &data, sizeof(data), time.tv_sec, time.tv_usec);
		last = current;
		usleep(100);
		continue;
	    }

	    //////////////////////////////////////////////////////////////////
	    // Monte-Carlo Localization Algorithm
	    //
	    // [step 1] : draw new samples from the previous PDF
	    SamplingImportanceResampling(this->p_odometry, c_odometry);

	    // [step 2] : update importance factors based on the sensor model
	    ImportanceFactorUpdate(this->ranges);

	    // [step 3] : generate hypothesis by grouping particles
	    player_localization_data_t data;
	    HypothesisConstruction(data);
	    //
	    //////////////////////////////////////////////////////////////////

	    // remember the current odometry information
	    this->p_odometry = c_odometry;

	    // Make data available
	    PutData((uint8_t*) &data, sizeof(data), time.tv_sec, time.tv_usec);

	    // remember when MCL is performed last time
	    last = current;
	}

	// yeild the control to other threads
	usleep(100);
    }
}


// Process configuration requests.
void RegularMCL::UpdateConfig(void)
{
    int len;
    void *client;
    static char buffer[PLAYER_MAX_REQREP_SIZE];
  
    while ((len = GetConfig(&client, &buffer, sizeof(buffer))))
    {
	switch (buffer[0])
	{
	    case PLAYER_LOCALIZATION_RESET_REQ:
	    {
		player_localization_reset_t reset;
		// check if the config request is valid
		if (len != sizeof(player_localization_reset_t))
		{
		    PLAYER_ERROR2("config request len is invalid (%d != %d)", len, sizeof(reset));
		    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
			PLAYER_ERROR("PutReply() failed");
		    continue;
		}
		// reset MCL device
		this->Reset();
		// send an acknowledgement to the client
	        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &reset, sizeof(reset)) != 0)
		    PLAYER_ERROR("PutReply() failed");
		break;
	    }

	    case PLAYER_LOCALIZATION_GET_CONFIG_REQ:
	    {
		player_localization_config_t config;
		// check if the config request is valid
		if (len != sizeof(config.subtype))
		{
		    PLAYER_ERROR2("config request len is invalid (%d != %d)", len, sizeof(config.subtype));
		    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
			PLAYER_ERROR("PutReply() failed");
		    continue;
		}
		// load configuration info
		config.num_particles = htonl(this->num_particles);
		// send the information to the client
	        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &config, sizeof(config)) != 0)
		    PLAYER_ERROR("PutReply() failed");
		break;
	    }

	    case PLAYER_LOCALIZATION_SET_CONFIG_REQ:
	    {
		player_localization_config_t config;
		// check if the config request is valid
		if (len != sizeof(player_localization_config_t))
		{
		    PLAYER_ERROR2("config request len is invalid (%d != %d)", len, sizeof(config));
		    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
			PLAYER_ERROR("PutReply() failed");
		    continue;
		}
		// change the current configuration
		memcpy(&config, buffer, sizeof(config));
		this->num_particles = (uint32_t) ntohl(config.num_particles);
		// reset the device after changing configuration
		this->Reset();
		// send an acknowledgement to the client
	        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &config, sizeof(config)) != 0)
		    PLAYER_ERROR("PutReply() failed");
		break;
	    }

	    case PLAYER_LOCALIZATION_GET_MAP_HDR_REQ:
	    {
		player_localization_map_header_t map_header;
		// check if the config request is valid
		if (len != sizeof(player_localization_map_header_t))
		{
		    PLAYER_ERROR2("config request len is invalid (%d != %d)", len, sizeof(map_header));
		    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
			PLAYER_ERROR("PutReply() failed");
		    continue;
		}
		// load the size information of map
		memcpy(&map_header, buffer, sizeof(map_header));
		uint8_t scale = map_header.scale;
		map_header.width = htonl((uint32_t)(this->map->width / float(scale) + 0.5));
		map_header.height = htonl((uint32_t)(this->map->height / float(scale) + 0.5));
		map_header.ppkm = htonl((uint32_t)(this->map->ppm * 1000 / float(scale) + 0.5));
		// send the information to the client
	        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &map_header, sizeof(map_header)) != 0)
		    PLAYER_ERROR("PutReply() failed");
		break;
	    }

	    case PLAYER_LOCALIZATION_GET_MAP_DATA_REQ:
	    {
		player_localization_map_data_t map_data;
		// check if the config request is valid
		if (len != sizeof(player_localization_map_data_t))
		{
		    PLAYER_ERROR2("config request len is invalid (%d != %d)", len, sizeof(map_data));
		    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
			PLAYER_ERROR("PutReply() failed");
		    continue;
		}
		// retrieve user input
		memcpy(&map_data, buffer, sizeof(map_data));
		uint32_t scale = map_data.scale;
		uint32_t row = (uint32_t) ntohs(map_data.row);
		// compute the size of scaled world
		uint32_t width = (uint32_t)(this->map->width / float(scale) + 0.5);
		uint32_t height = (uint32_t)(this->map->height / float(scale) + 0.5);
		// check if the request is valid
		if (width >= PLAYER_MAX_REQREP_SIZE-4 || row >= height) {
		    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
			PLAYER_ERROR("PutReply() failed");
		    continue;
		}
		// fill out the map data
		memset(map_data.data, 255, sizeof(map_data.data));
		uint32_t nrows = sizeof(map_data.data) / width;
		if (row+nrows > height) nrows = height - row;
		for (uint32_t i=0; i<nrows; i++, row++)
		    for (uint32_t h=row*scale; h<(row+1)*scale; h++)
			for (uint32_t w=0; w<this->map->width; w++) {
			    uint8_t np = this->map->map[h*this->map->width + w];
			    uint32_t idx = i*width + (uint32_t)(w/float(scale)+0.5);
			    if (map_data.data[idx] > np) map_data.data[idx] = np;
			}
		// send the information to the client
	        if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &map_data, sizeof(map_data)) != 0)
		    PLAYER_ERROR("PutReply() failed");
		break;
	    }

	    default:
	    {
		if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
		    PLAYER_ERROR("PutReply() failed");
		break;
	    }
	}
    }
}


// Reset MCL device
void RegularMCL::Reset(void)
{
    // pre-compute constants for efficiency
    float width_ratio = float(RAND_MAX) / this->map->Width();
    float height_ratio = float(RAND_MAX) / this->map->Height();
    float yaw_ratio = float(RAND_MAX) / 360;

    // re-generate particles in random
    this->unit_importance = 1.0f / this->num_particles;
    this->particles.clear();
    this->p_buffer.clear();
    particle_t p;
    for (uint32_t i=0; i<this->num_particles; i++) {
	// random pose
	p.pose.x = int(rand() / width_ratio);
	p.pose.y = int(rand() / height_ratio);
	p.pose.a = int(rand() / yaw_ratio);
	// uniform importance
	p.importance = this->unit_importance;
	// cumulative importance
	p.cumulative = this->unit_importance * (i+1);
	// store the particle
	this->particles.push_back(p);
    }

    // re-initialize a clustering algorithm
    this->clustering->Reset(this->map->Width(), this->map->Height());

    // store the current odometry data
    this->ReadOdometry(this->p_odometry);
}


// Read the configuration of a distance sensor
void RegularMCL::ReadConfiguration(void)
{
    timeval tv;
    unsigned short reptype;

    // read a sonar configuration
    if (this->sensor_type == PLAYER_MCL_SONAR) {
	player_sonar_geom_t config;
	config.subtype = PLAYER_SONAR_GET_GEOM_REQ;
	this->distance_device->Request(&(this->distance_device->device_id), this,
				       (void*)&config, sizeof(config.subtype), &reptype, &tv,
				       (void*)&config, sizeof(config));
	if (reptype == PLAYER_MSGTYPE_RESP_ERR)
	    PLAYER_ERROR("  could not read a sonar configuration");
	// store proper settings
	int pose_count = (uint16_t) ntohs(config.pose_count);
	if (this->sensor_num_samples) {		// subsampling is required
	    int subsampling = pose_count / this->sensor_num_samples;
	    this->num_ranges = this->sensor_num_samples;
	    this->poses = new pose_t[this->sensor_num_samples];
	    for (int i=0, s_idx=0; i<this->sensor_num_samples; i++, s_idx+=subsampling) {
		this->poses[i].x = (int16_t) ntohs(config.poses[s_idx][0]);
		this->poses[i].y = (int16_t) ntohs(config.poses[s_idx][1]);
		this->poses[i].a = (int16_t) ntohs(config.poses[s_idx][2]);
	    }
	    this->ranges = new uint16_t[this->sensor_num_samples];
	    this->inc = subsampling;
	}
	else {					// use a sensor default
	    this->num_ranges = pose_count;
	    this->poses = new pose_t[pose_count];
	    for (int i=0; i<pose_count; i++) {
		this->poses[i].x = (int16_t) ntohs(config.poses[i][0]);
		this->poses[i].y = (int16_t) ntohs(config.poses[i][1]);
		this->poses[i].a = (int16_t) ntohs(config.poses[i][2]);
	    }
	    this->ranges = new uint16_t[pose_count];
	    this->inc = 1;
	}
    }

    // read a laser configuration
    else if (this->sensor_type == PLAYER_MCL_LASER) {
	player_laser_geom_t geom;
	geom.subtype = PLAYER_LASER_GET_GEOM;
	this->distance_device->Request(&(this->distance_device->device_id), this,
				       (void*)&geom, sizeof(geom.subtype), &reptype, &tv,
				       (void*)&geom, sizeof(geom));
	if (reptype == PLAYER_MSGTYPE_RESP_ERR)
	    PLAYER_ERROR("  could not read a laser geometry configuration");
	player_laser_config_t config;
	config.subtype = PLAYER_LASER_GET_CONFIG;
	this->distance_device->Request(&(this->distance_device->device_id), this,
				       (void*)&config, sizeof(config.subtype), &reptype, &tv,
				       (void*)&config, sizeof(config));
	if (reptype == PLAYER_MSGTYPE_RESP_ERR)
	    PLAYER_ERROR("  could not read a laser configuration");
	// store proper settings
	float cx = (int16_t) ntohs(geom.pose[0]);
	float cy = (int16_t) ntohs(geom.pose[1]);
	float ca = (int16_t) ntohs(geom.pose[2]);
	float max_angle = (int16_t) ntohs(config.max_angle) / 100.0;
	float min_angle = (int16_t) ntohs(config.min_angle) / 100.0;
	float resolution = (uint16_t) ntohs(config.resolution) / 100.0;
	int pose_count = int((max_angle - min_angle) / resolution);
	if (this->sensor_num_samples) {		// subsampling is required
	    int subsampling = pose_count / this->sensor_num_samples;
	    this->num_ranges = this->sensor_num_samples;
	    this->poses = new pose_t[this->sensor_num_samples];
	    for (int i=0, s_idx=0; i<this->sensor_num_samples; i++, s_idx+=subsampling) {
		this->poses[i].x = cx;
		this->poses[i].y = cy;
		this->poses[i].a = ca + min_angle + (i*resolution*subsampling);
	    }
	    this->ranges = new uint16_t[this->sensor_num_samples];
	    this->inc = subsampling;
	}
	else {					// use a sensor default
	    this->num_ranges = pose_count;
	    this->poses = new pose_t[pose_count];
	    for (int i=0; i<pose_count; i++) {
		this->poses[i].x = cx;
		this->poses[i].y = cy;
		this->poses[i].a = ca + min_angle + (i*resolution);
	    }
	    this->ranges = new uint16_t[pose_count];
	    this->inc = 1;
	}
    }

}


// Read range data from a distance sensor
bool RegularMCL::ReadRanges(uint16_t* buffer)
{
    uint32_t ts_sec, ts_usec;

    // retrieve data from a sonar device
    if (this->sensor_type == PLAYER_MCL_SONAR) {
	player_sonar_data_t data;
	size_t r = this->distance_device->GetData(this, (unsigned char*)&data, sizeof(data),
						  &ts_sec, &ts_usec);
	if (r != sizeof(data))
	    return false;
	// copy the data into the given buffer
	for (int i=0, s_idx=0; i<this->num_ranges; i++, s_idx+=this->inc)
	    buffer[i] = (uint16_t) ntohs(data.ranges[s_idx]);
	return true;
    }

    // retrieve data from a laser device
    else if (this->sensor_type == PLAYER_MCL_LASER) {
	player_laser_data_t data;
	size_t r = this->distance_device->GetData(this, (unsigned char*)&data, sizeof(data),
						  &ts_sec, &ts_usec);
	if (r != sizeof(data))
	    return false;
	// copy the data into the given buffer
	for (int i=0, s_idx=0; i<this->num_ranges; i++, s_idx+=this->inc)
	    buffer[i] = (uint16_t) ntohs(data.ranges[s_idx]);
	return true;
    }

    return false;
}


// Read odometry data from a motion sensor
bool RegularMCL::ReadOdometry(pose_t& pose)
{
    // retrieve data from the motion device
    player_position_data_t data;
    uint32_t ts_sec, ts_usec;
    size_t r = this->motion_device->GetData(this, (unsigned char*)&data, sizeof(data),
	    				    &ts_sec, &ts_usec);
    if (r != sizeof(data))
	return false;
    // return only pose data
    pose.x = (int32_t) ntohl(data.xpos);
    pose.y = (int32_t) ntohl(data.ypos);
    pose.a = ((int32_t)ntohl(data.yaw));
    return true;
}


// draw new sample from the previous PDF
void RegularMCL::SamplingImportanceResampling(pose_t from, pose_t to)
{
    for (uint32_t i=0; i<this->num_particles; i++)
    {
	// select a random particle according to the current PDF
	double selected = rand() / double(RAND_MAX);
	int start = 0;
	int end = this->num_particles - 1;
	while (end - start > 1) {
	    int center = (end + start) / 2;
	    if (this->particles[center].cumulative > selected)
		end = center;
	    else if (this->particles[center].cumulative < selected)
		start = center;
	    else {	// bingo!
		start = end = center;
		break;
	    }
	}

	// generate a new particle based on the action model
	particle_t p;
	p.pose = this->action_model->sample(this->particles[end].pose,
					    from, to);
	this->p_buffer.push_back(p);
    }

    // done
    this->p_buffer.swap(this->particles);
    this->p_buffer.clear();
}


// update importance factors based on the sensor model
void RegularMCL::ImportanceFactorUpdate(uint16_t *ranges)
{
    vector<particle_t>::iterator p;

    // position constraints
    float w_width = this->map->Width();
    float w_height = this->map->Height();

    // update the importance factors
    double sum = 0.0;
    for (p=this->particles.begin(); p<this->particles.end(); p++) {
	// check if the particle is inside of valid environment
	if (p->pose.x >= 0 && p->pose.x <= w_width &&
			      p->pose.y >=0 && p->pose.y <= w_height) {
	    p->importance = this->sensor_model->probability(ranges, p->pose, this->map);
	    sum += p->importance;
	}
	else				// importance of an outside particle will
	    p->importance = 0.0;	// zero, which means it will be never selected
    }

    if (sum == 0.0) {
	PLAYER_WARN("the sum of all importance factors is zero. system is reset.");
	this->Reset();
	return;
    }

    // normalize the importance factors
    for (p=this->particles.begin(); p<this->particles.end(); p++)
	p->importance /= sum;

    // compute the cumulative importance factors
    sum = 0.0f;
    for (p=this->particles.begin(); p<this->particles.end(); p++) {
	sum += p->importance;
	p->cumulative = sum;
    }
}


// construct hypothesis by grouping particles
void RegularMCL::HypothesisConstruction(player_localization_data_t& data)
{
    // reset the clustering algorithm
    this->clustering->Reset(this->map->Width(), this->map->Height());

    // run clustering algorithm
    this->clustering->cluster(this->particles);

    // Prepare packet and byte swap
    int n = 0;
    for (int i=0; i<PLAYER_LOCALIZATION_MAX_HYPOTHESIS; i++)
    {
	if (this->clustering->pi[i] > 0.0) {
	    data.hypothesis[n].alpha = htonl((uint32_t)(this->clustering->pi[i] * 1000000000));

	    data.hypothesis[n].mean[0] = htonl((int32_t)(this->clustering->mean[i][0]));
	    data.hypothesis[n].mean[1] = htonl((int32_t)(this->clustering->mean[i][1]));
	    data.hypothesis[n].mean[2] = htonl((int32_t)(this->clustering->mean[i][2]));

	    data.hypothesis[n].cov[0][0] = htonl((int32_t)(this->clustering->cov[i][0][0]));
	    data.hypothesis[n].cov[0][1] = htonl((int32_t)(this->clustering->cov[i][0][1]));
	    data.hypothesis[n].cov[0][2] = htonl((int32_t)(this->clustering->cov[i][0][2]));
	    data.hypothesis[n].cov[1][0] = htonl((int32_t)(this->clustering->cov[i][1][0]));
	    data.hypothesis[n].cov[1][1] = htonl((int32_t)(this->clustering->cov[i][1][1]));
	    data.hypothesis[n].cov[1][2] = htonl((int32_t)(this->clustering->cov[i][1][2]));
	    data.hypothesis[n].cov[2][0] = htonl((int32_t)(this->clustering->cov[i][2][0]));
	    data.hypothesis[n].cov[2][1] = htonl((int32_t)(this->clustering->cov[i][2][1]));
	    data.hypothesis[n].cov[2][2] = htonl((int32_t)(this->clustering->cov[i][2][2]));

	    n++;
	}
    }
    data.num_hypothesis = htonl(n);
}


// a factory create function
CDevice* RegularMCL_Init(char* interface, ConfigFile* cf, int section)
{
    if (strcmp(interface, PLAYER_LOCALIZATION_STRING)) {
	PLAYER_ERROR1("driver \"regular_mcl\" does not support interface \"%s\"\n",
		      interface);
	return (NULL);
    }
    else
	return ((CDevice*)(new RegularMCL(interface, cf, section)));
}


// a driver registration function
void RegularMCL_Register(DriverTable* table)
{
    table->AddDriver("regular_mcl", PLAYER_READ_MODE, RegularMCL_Init);
}
