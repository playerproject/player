/*
 *  Kinect Driver for Player
 *  Copyright (C) 2011
 *     Rich Mattes
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_kinect kinect
 * @brief USB camera driver for Microsoft Kinect camera

Device driver for Microsoft Kinect webcams.  This driver handles processing
the RGB Color images and the greyscale depth images provided by the Kinect,
as well as motor control for PTZ, and reading the accelerometer.  The Kinect
also supports audio and LED control, these capabilities are under development.

This driver is based on the latest version of the libfreenect API, which
is currently under heavy development.

Heatmap code and USB connection code based on "glview" example in libfreenect
project.

@par Compile-time dependencies

- libfreenect - http://github.com/OpenKinect/libfreenect
- libusb-1.0 - http://www.libusb.org/wiki/libusb-1.0

@par Requires

- none

@par Provides

- @ref interface_camera - Color image (mandatory)
- @ref interface_camera - Depth image (optional)
- @ref interface_ptz - Tilt motor
- @ref interface_imu - Accelerometer

@par Configuration requests

- None

@par Configuration file options

- See @ref Properties

@par Properties
- heatmap (bool)
  - Default: false
  - When set to false, the Depth image is published as a greyscale image
  - When set to true, the Depth image is colorized and published as an RGB heatmap

- downsample (bool)
  - Default: false
  - When set to false, the Depth image is published as a greyscale MONO16 image
  - When set to true, the Depth image is downsampled into a greyscale MONO8 image
  - If heatmap is set to true, this option has no effect.

- color_resolution (enumeration)
  - Default: 2
  - Corresponds to the resolution of the color image frame.
  - 2 = High Resolution = 1280x1024
  - 1 = Med Resolution = 640x480

- depth_resolution (enumeration)
  - Default: 1
  - Corresponds to the resolution of the depth image frame
  - 1 = Med resolution = 640x488
  - 0 = Low Resolution = 320x240

@par Example

@verbatim
driver
(
  name "kinect"
  provides ["color:::camera:0" "depth:::camera:1" "ptz:0" "imu:0"]
  heatmap 0
  downsample 1
  color_resolution 2
  depth_resolution 1
)
@endverbatim

@authors Rich Mattes
 */
/** @} */

//TODO: Add support for LEDs, pointcloud, user-defined image sizes
//TODO: Less judicious use of memcpy

#if !defined (WIN32)
#include <unistd.h>
#endif
#include <string.h>
#include <cmath>

#include <libplayercore/playercore.h>
#include <libusb-1.0/libusb.h>
#include <libfreenect/libfreenect.h>


// Callback functions for processing depth and color image buffers
void DepthImageCallback(freenect_device *dev, void *depth, uint32_t timestamp);
void ColorImageCallback(freenect_device *dev, void *rgb, uint32_t timestamp);

// Storage for image data and metadata
static uint16_t* DepthImage;
static uint8_t* ColorImage;
pthread_mutex_t kinect_mutex = PTHREAD_MUTEX_INITIALIZER;
static int newcdata, newddata;
static freenect_frame_mode colorImageMode;
static freenect_frame_mode depthImageMode;

const bool DEFAULT_HEATMAP = false;
const bool DEFAULT_DOWNSAMPLE = false;
const int DEFAULT_COLOR_RESOLUTION = FREENECT_RESOLUTION_HIGH;
const int DEFAULT_DEPTH_RESOLUTION = FREENECT_RESOLUTION_MEDIUM;

////////////////////////////////////////////////////////////////////////////////
// The class for the driver
class KinectDriver : public ThreadedDriver
{
public:

	// Constructor; need that
	KinectDriver(ConfigFile* cf, int section);

	// This method will be invoked on each incoming message
	virtual int ProcessMessage(QueuePointer &resp_queue,
			player_msghdr * hdr,
			void * data);

private:

	// Main function for device thread.
	virtual void Main();
	virtual int MainSetup();
	virtual void MainQuit();

	// Routines to publish different data
	int PublishColorImage();
	int PublishDepthImage();
	int PublishPTZ();
	int PublishAccelerometer();

	// Handles for libfreenect
	freenect_context *fctx;
	freenect_device *fdev;

	// Device addresses for all possible provided interfaces
	player_devaddr_t color_camera_id;
	player_devaddr_t depth_camera_id;
	player_devaddr_t ptz_id;
	player_devaddr_t imu_id;

	// Storage for outgoing interface data
	player_camera_data_t colordata;
	player_camera_data_t depthdata;
	player_ptz_data_t ptzdata;
	player_imu_data_calib_t imudata;

	// Flags for interfaces we provide
	int providedepthimage;
	int provideptz;
	int provideimu;

	// Timers for publishing ptz and accelerometers regularly
	double last_acc_pub;
	double last_ptz_pub;

	// Config file options
	BoolProperty heatmap;
	BoolProperty downsample;
	IntProperty color_resolution;
	IntProperty depth_resolution;

	// Lookup table for depth image colorization
	uint16_t t_gamma[2048];
};


// Startup instance of the driver
Driver* KinectDriver_Init(ConfigFile* cf, int section)
{
	// Create and return a new instance of this driver
	return((Driver*)(new KinectDriver(cf, section)));
}

// Register driver with server
void kinect_Register(DriverTable* table)
{
	table->AddDriver("kinect", KinectDriver_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
KinectDriver::KinectDriver(ConfigFile* cf, int section)
: ThreadedDriver(cf, section),
  heatmap("heatmap", DEFAULT_HEATMAP, false),
  downsample("downsample", DEFAULT_DOWNSAMPLE, false),
  color_resolution("color_resolution", DEFAULT_COLOR_RESOLUTION, false),
  depth_resolution("depth_resolution", DEFAULT_DEPTH_RESOLUTION, false)
{
	// Initialize flags:
	providedepthimage = 0;
	provideptz = 0;
	provideimu = 0;

	//Add interface from Configuration File
	if (cf->ReadDeviceAddr(&(this->color_camera_id), section, "provides", PLAYER_CAMERA_CODE, -1, "image"))
	{
		PLAYER_ERROR("Kinect's Camera interface not started: config file doesn't provide \"image:::camera:n\"");
		this->SetError(-1);
		return;
	}
	if (this->AddInterface(this->color_camera_id))
	{
		PLAYER_ERROR("Kinect's Camera interface failed to be added.");
		this->SetError(-1);
		return;
	}
	if (cf->ReadDeviceAddr(&(this->depth_camera_id), section, "provides", PLAYER_CAMERA_CODE, -1, "depth"))
	{
		PLAYER_WARN("Kinect's Depth interface not started: config file doesn't provide \"depth:::camera:n\"");
	}
	else
	{
		if (this->AddInterface(this->depth_camera_id))
		{
			PLAYER_ERROR("Kinect's Depth Camera interface failed to be added.");
			this->SetError(-1);
			return;
		}
		providedepthimage = 1;
	}

	// Check to see if we provide the PTZ interface
	if (cf->ReadDeviceAddr(&(this->ptz_id), section, "provides", PLAYER_PTZ_CODE, -1, NULL))
	{
		PLAYER_WARN("Kinect driver not providing PTZ.");
	}
	else{
		if (this->AddInterface(this->ptz_id))
		{
			PLAYER_ERROR("Kinect's PTZ interface failed to be added.");
			this->SetError(-1);
			return;
		}
		provideptz = 1;
	}

	// Check to see if we provide the IMU interface
	if (cf->ReadDeviceAddr(&(this->imu_id), section, "provides", PLAYER_IMU_CODE, -1, NULL))
	{
		PLAYER_WARN("Kinect driver not providing IMU.");
	}
	else{
		if (this->AddInterface(this->imu_id))
		{
			PLAYER_ERROR("Kinect's IMU interface failed to be added.");
			this->SetError(-1);
			return;
		}
		provideimu = 1;
	}

	// Read config file options
	RegisterProperty("heatmap", &this->heatmap, cf, section);
	RegisterProperty("downsample", &this->downsample, cf, section);
	RegisterProperty("color_resolution", &this->color_resolution, cf, section);
	RegisterProperty("depth_resolution", &this->depth_resolution, cf, section);


	// Initialize the color map lookup table
	for (int i=0; i<2048; i++) {
			float v = i/2048.0;
			v = powf(v, 3)* 6;
			t_gamma[i] = v*6*256;
		}

	return;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int KinectDriver::MainSetup()
{
	PLAYER_MSG0(1,"Kinect driver initializing...");
	memset(&kinect_mutex, 0, sizeof(pthread_mutex_t));

	if (freenect_init(&fctx, NULL) < 0)
	{
		PLAYER_ERROR("Error initializing Kinect");
		return -1;
	}
	if (freenect_open_device(fctx, &fdev, 0) < 0)
	{
		PLAYER_ERROR("Error opening Kinect");
		return -1;
	}
	//Presto reduced the update size by 80% (from 305 M to 62 M).


	freenect_set_depth_callback(fdev, DepthImageCallback);
	freenect_set_video_callback(fdev, ColorImageCallback);
	colorImageMode = freenect_find_video_mode((freenect_resolution)color_resolution.GetValue(), FREENECT_VIDEO_RGB);
	freenect_set_video_mode(fdev, colorImageMode);
	depthImageMode = freenect_find_depth_mode((freenect_resolution)depth_resolution.GetValue(), FREENECT_DEPTH_11BIT);
	freenect_set_depth_mode(fdev, depthImageMode);

	colordata.image = NULL;
	depthdata.image = NULL;

	freenect_start_depth(fdev);
	freenect_start_video(fdev);

	last_acc_pub = 0;
	last_ptz_pub = 0;

	return(0);
}

////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
void KinectDriver::MainQuit()
{
	PLAYER_MSG0(2,"Kinect driver shutting down...");

	freenect_stop_depth(fdev);
	freenect_stop_video(fdev);
	freenect_shutdown(fctx);

	PLAYER_MSG0(2,"Kinect driver has been shut down.");
}

////////////////////////////////////////////////////////////////////////////////
// Message handling function
int KinectDriver::ProcessMessage(QueuePointer & resp_queue,
		player_msghdr * hdr,
		void * data)
{
	// Respond to this so clients know that a NACK for every other request is intended
	HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILITIES_REQ);

	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_SET_INTPROP_REQ, this->color_camera_id))
	{
		player_intprop_req_t *propreq = reinterpret_cast<player_intprop_req_t*>(data);
		if (!strncmp(propreq->key, "color_resolution", 17))
		{
			int newres = propreq->value;
			if (newres > FREENECT_RESOLUTION_HIGH || newres < FREENECT_RESOLUTION_LOW)
			{
				// Desired value is out of range, warn the user and ignore the request
				PLAYER_WARN1("Property value %d for \"color_resolution\" is out of range, ignoring...", newres);
				return -1;
			}
			else if(newres == colorImageMode.resolution)
			{
				// Already at this resolution, don't do anything and indicate that everything is fine
				Publish(hdr->addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_SET_INTPROP_REQ, NULL, 0, NULL);
				return 0;
			}
			else
			{
				PLAYER_WARN1("Setting color image resolution to %d", newres);
				// Have a different resolution, do something about it.
				freenect_stop_video(fdev);
				pthread_mutex_lock(&kinect_mutex);
				colorImageMode = freenect_find_video_mode((freenect_resolution)newres, FREENECT_VIDEO_RGB);
				freenect_set_video_mode(fdev, colorImageMode);
				pthread_mutex_unlock(&kinect_mutex);
				freenect_start_video(fdev);
				Publish(hdr->addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_SET_INTPROP_REQ, NULL, 0, NULL);
				return 0;
			}
		}
	}

	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_SET_INTPROP_REQ, this->depth_camera_id))
		{
			player_intprop_req_t *propreq = reinterpret_cast<player_intprop_req_t*>(data);
			if (!strncmp(propreq->key, "depth_resolution", 17))
			{
				int newres = propreq->value;
				if (newres > FREENECT_RESOLUTION_MEDIUM || newres < FREENECT_RESOLUTION_LOW)
				{
					// Desired value is out of range, warn the user and ignore the request
					PLAYER_WARN1("Property value %d for \"depth_resolution\" is out of range, ignoring...", newres);
					return -1;
				}
				else if(newres == depthImageMode.resolution)
				{
					// Already at this resolution, don't do anything and indicate that everything is fine
					Publish(hdr->addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_SET_INTPROP_REQ, NULL, 0, NULL);
					return 0;
				}
				else
				{
					// Have a different resolution, do something about it.
					freenect_stop_depth(fdev);
					pthread_mutex_lock(&kinect_mutex);
					colorImageMode = freenect_find_depth_mode((freenect_resolution)newres, FREENECT_DEPTH_11BIT);
					freenect_set_depth_mode(fdev, depthImageMode);
					pthread_mutex_unlock(&kinect_mutex);
					freenect_start_depth(fdev);
					Publish(hdr->addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, PLAYER_SET_INTPROP_REQ, NULL, 0, NULL);
					return 0;
				}
			}
		}

	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_PTZ_CMD_STATE, this->ptz_id))
	{
		player_ptz_cmd_t *ptzcmd = (player_ptz_cmd_t*) data;
		int tiltcmd = round(ptzcmd->tilt * 180.0 / M_PI);

		if (tiltcmd < -30)
		{
			PLAYER_WARN1("Kinect tilt command (%d deg) out of range, limiting to (-30 deg)", tiltcmd);
			tiltcmd = -30;
		}
		else if (tiltcmd > 30)
		{
			PLAYER_WARN1("Kinect tilt command (%d deg) out of range, limiting to (+30 deg)", tiltcmd);
			tiltcmd = 30;
		}

		// Only do something if the last command is different from the current one
		if (tiltcmd != round(this->ptzdata.tilt * 180.0 / M_PI))
		{
			freenect_set_tilt_degs(fdev, (double)tiltcmd);
			this->ptzdata.tilt = ptzcmd->tilt;
		}
		return 0;
	}

	// Don't understand the message, return with an error
	return(-1);
}
////////////////////////////////////////////////////////////////////////////////
// Publish color image data to camera interface
int KinectDriver::PublishColorImage()
{
	if (colordata.image){
		delete[] colordata.image;
	}
	colordata.image = new uint8_t[colorImageMode.bytes];

	pthread_mutex_lock(&kinect_mutex);
	colordata.width = colorImageMode.width;
	colordata.height = colorImageMode.height;
	memcpy(colordata.image, ColorImage, colorImageMode.bytes);
	pthread_mutex_unlock(&kinect_mutex);

	colordata.bpp = 24;
	colordata.compression = PLAYER_CAMERA_COMPRESS_RAW;
	colordata.fdiv = 1;
	colordata.image_count = colorImageMode.bytes;
	colordata.format = PLAYER_CAMERA_FORMAT_RGB888;

	PLAYER_MSG2(4,"Writing Color Image size %d, %d", colordata.width, colordata.height);
	Publish(color_camera_id, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, (void*)&colordata);
	newcdata = 0;

	return 0;

}
////////////////////////////////////////////////////////////////////////////////
// Publish depth image data to camera interface
int KinectDriver::PublishDepthImage()
{

	if (depthdata.image){
		delete[] depthdata.image;
	}
	// Allocate space for maximum possible display image size (RGB888 with depth image's wxh)
	depthdata.image = new uint8_t[depthImageMode.width * depthImageMode.height * 3];

	if (heatmap) // Publish colorized RGB888
	{
		pthread_mutex_lock(&kinect_mutex);
		depthdata.width = depthImageMode.width;
		depthdata.height = depthImageMode.height;
		for (unsigned int i=0; i<depthdata.width*depthdata.height; i++) {
			int pval = t_gamma[DepthImage[i]];
			int lb = pval & 0xff;
			switch (pval>>8) {
			case 0:
				depthdata.image[3*i+0] = 255;
				depthdata.image[3*i+1] = 255-lb;
				depthdata.image[3*i+2] = 255-lb;
				break;
			case 1:
				depthdata.image[3*i+0] = 255;
				depthdata.image[3*i+1] = lb;
				depthdata.image[3*i+2] = 0;
				break;
			case 2:
				depthdata.image[3*i+0] = 255-lb;
				depthdata.image[3*i+1] = 255;
				depthdata.image[3*i+2] = 0;
				break;
			case 3:
				depthdata.image[3*i+0] = 0;
				depthdata.image[3*i+1] = 255;
				depthdata.image[3*i+2] = lb;
				break;
			case 4:
				depthdata.image[3*i+0] = 0;
				depthdata.image[3*i+1] = 255-lb;
				depthdata.image[3*i+2] = 255;
				break;
			case 5:
				depthdata.image[3*i+0] = 0;
				depthdata.image[3*i+1] = 0;
				depthdata.image[3*i+2] = 255-lb;
				break;
			default:
				depthdata.image[3*i+0] = 0;
				depthdata.image[3*i+1] = 0;
				depthdata.image[3*i+2] = 0;
				break;
			}
		}
		pthread_mutex_unlock(&kinect_mutex);

		depthdata.bpp = 24;
		depthdata.compression = PLAYER_CAMERA_COMPRESS_RAW;
		depthdata.fdiv = 1;
		depthdata.image_count = depthdata.width*depthdata.height*3;
		depthdata.format = PLAYER_CAMERA_FORMAT_RGB888;
	}
	else if (downsample) //Publish downsampled MONO8
	{
		pthread_mutex_lock(&kinect_mutex);
		depthdata.width = depthImageMode.width;
		depthdata.height = depthImageMode.height;
		for (unsigned int i=0; i < depthdata.width* depthdata.height; i++)
		{
			uint16_t pixel = DepthImage[i];
			uint8_t dpixel = (uint8_t)((double)pixel / 2048.0 * 255.0); 
			depthdata.image[i] = dpixel;
		}
		pthread_mutex_unlock(&kinect_mutex);

		depthdata.bpp = 8;
		depthdata.compression = PLAYER_CAMERA_COMPRESS_RAW;
		depthdata.fdiv = 1;
		depthdata.image_count = depthImageMode.width * depthImageMode.height;
		depthdata.format = PLAYER_CAMERA_FORMAT_MONO8;
	}
	else // Publish MONO16	
	{
		pthread_mutex_lock(&kinect_mutex);
		depthdata.width = depthImageMode.width;
		depthdata.height = depthImageMode.height;
		memcpy((void*)depthdata.image, (void*)DepthImage, depthImageMode.bytes);
		pthread_mutex_unlock(&kinect_mutex);


		depthdata.bpp = 16;
		depthdata.compression = PLAYER_CAMERA_COMPRESS_RAW;
		depthdata.fdiv = 1;
		depthdata.image_count = depthImageMode.bytes;
		depthdata.format = PLAYER_CAMERA_FORMAT_MONO16;
	}

	PLAYER_MSG2(4,"Writing Depth Image size %d, %d", depthdata.width, depthdata.height);
	newddata = 0;
	Publish(depth_camera_id, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, (void*)&depthdata);

	return 0;
}
int KinectDriver::PublishPTZ()
{
	Publish(this->ptz_id, PLAYER_MSGTYPE_DATA, PLAYER_PTZ_DATA_STATE, (void*)&ptzdata, sizeof(ptzdata));
	return 0;
}

int KinectDriver::PublishAccelerometer()
{
	if (freenect_update_tilt_state(fdev))
	{
		double x, y, z;
		freenect_raw_tilt_state *rawstate = freenect_get_tilt_state(fdev);
		if( rawstate )
		{
			freenect_get_mks_accel(rawstate, &x, &y, &z);
			imudata.accel_x = x;
			imudata.accel_y = y;
			imudata.accel_z = z;

			Publish(this->imu_id, PLAYER_MSGTYPE_DATA, PLAYER_IMU_DATA_CALIB, (void*)&imudata, sizeof(imudata));
			return 0;
		}
	}
	// Didn't get to return 0
	PLAYER_WARN("Error retrieving accelerometer data.");
	return -1;
}
////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void KinectDriver::Main()
{
	// The main loop; interact with the device here
	for(;;)
	{
		// test if we are supposed to cancel
		pthread_testcancel();

		// Cycle libusb
		freenect_process_events(fctx);

		// Process incoming messages.  KinectDriver::ProcessMessage() is
		// called on each message.
		ProcessMessages();

		// Check each camera to see if there's new data available
		// from the callbacks.  If so, publish it.
		if (newcdata)
		{
			PublishColorImage();
		}
		if (providedepthimage && newddata)
		{
			PublishDepthImage();
		}

		double now;
		GlobalTime->GetTimeDouble(&now);
		// If this happens on every iteration, the image frames
		// don't get through.  Limit it to updating 20ish times a second
		if (provideimu && (now - last_acc_pub) > .05)
		{
			PublishAccelerometer();
			last_acc_pub = now;
		}
		// PTZ state data isn't terribly important, publish every
		// half second or so.
		if (provideptz && (now - last_ptz_pub) > 0.5)
		{
			PublishPTZ();
			last_ptz_pub = now;
		}

		// Sleep so we don't kill the processor
		usleep(10);
	}
}
////////////////////////////////////////////////////////////////////////////////
// Grab depth image buffer returned by libfreenect and store it in a static buffer
void DepthImageCallback(freenect_device *dev, void *imagedata, uint32_t timestamp)
{
	pthread_mutex_lock(&kinect_mutex);
	if(DepthImage)
	{
		delete[] DepthImage;
	}

	DepthImage = new uint16_t[depthImageMode.bytes];
	memcpy(DepthImage, imagedata, depthImageMode.bytes);
	newddata = 1;
	pthread_mutex_unlock(&kinect_mutex);
	return;
}
////////////////////////////////////////////////////////////////////////////////
// Grab color image buffer returned by libfreenect and store it in a static buffer
void ColorImageCallback(freenect_device *dev, void *imagedata, uint32_t timestamp)
{
	pthread_mutex_lock(&kinect_mutex);
	if(ColorImage)
	{
		delete[] ColorImage;
	}
	ColorImage = new uint8_t[colorImageMode.bytes];
	memcpy(ColorImage, imagedata, colorImageMode.bytes);
	newcdata = 1;
	pthread_mutex_unlock(&kinect_mutex);
	return;
}

