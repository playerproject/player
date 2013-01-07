/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Andrew Howard
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
/*
 * Desc: Driver generating dummy data
 * Authors: Andrew Howard, Radu Bogdan Rusu
 * Date: 15 Sep 2004
 * CVS: $Id$
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_dummy dummy
 * @brief Dummy driver

The dummy driver generates dummy data and consumes dummy commands for
any interface; useful for debugging client libraries and benchmarking
server performance.

@par Compile-time dependencies

- none

@par Provides

This driver can theoretically support any interface.  Currently supported interfaces: 
- @ref interface_camera
- @ref interface_laser
- @ref interface_ranger
- @ref interface_position2d
- @ref interface_ptz
- @ref interface_wsn
- @ref interface_gps
- @ref interface_position3d
- @ref interface_blobfinder
- @ref interface_joystick
- @ref interface_power
- @ref interface_pointcloud3d
- @ref interface_imu
- @ref interface_fiducial
- @ref interface_wifi
- @ref interface_actarray
- @ref interface_opaque
- @ref interface_dio
- @ref interface_aio

@par Requires

- none

@par Configuration requests

- This driver will consume any configuration requests.

@par Configuration file options

  - rate (float)
  - Default: 10
  - Data rate (Hz); e.g., rate 20 will generate data at 20Hz.

@par Example

@verbatim
driver
(
  name "dummy"
  provides ["laser:0"]  # Generate dummy laser data
  rate 75               # Generate data at 75Hz
)
@endverbatim

@author Andrew Howard, Radu Bogdan Rusu, Rich Mattes
*/
/** @} */

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> // required for in.h on OS X

// Includes needed for player
#include <libplayercore/playercore.h>
#if !HAVE_NANOSLEEP
  #include <replace/replace.h>
#endif

// The Dummy driver
class Dummy: public ThreadedDriver 
{
    public:
        // Constructor
        Dummy (ConfigFile* cf, int section);

        // Destructor
        ~Dummy ();
        
        // Process Messages (so we can NACK to all requests that come in)
        virtual int ProcessMessage(QueuePointer & resp_queue,
                                     player_msghdr * hdr,
                                     void * data);

    private:

        // Main function for device thread.
        virtual void Main ();

        // Data rate
        double rate;
};



////////////////////////////////////////////////////////////////////////////////
// Factory creation function. This functions is given as an argument when
// the driver is added to the driver table
Driver* Dummy_Init(ConfigFile* cf, int section)
{
    return ((Driver*) (new Dummy(cf, section)));
}


////////////////////////////////////////////////////////////////////////////
// Device factory registration
void dummy_Register(DriverTable* table)
{
    table->AddDriver("dummy", Dummy_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
Dummy::Dummy(ConfigFile* cf, int section)
	: ThreadedDriver(cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
    // Look for our default device id
    if (cf->ReadDeviceAddr(&this->device_addr, section, "provides",
                        0, -1, NULL) != 0)
    {
        this->SetError(-1);
        return;
    }

    // Add our interface
    if (this->AddInterface(this->device_addr) != 0)
    {
        this->SetError(-1);
        return;
    }

    // Data rate
    this->rate = cf->ReadFloat(section, "rate", 10);

    return;
}


////////////////////////////////////////////////////////////////////////////
// Destructor
Dummy::~Dummy()
{
    return;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void Dummy::Main(void)
{
    unsigned int i = 0;
    struct timespec req;
    int loopcount = 0;
    //  void *client;

    req.tv_sec = (time_t) (1.0 / this->rate);
    req.tv_nsec = (long) (fmod(1e9 / this->rate, 1e9));

    while (1)
    {
        TestCancel();
        if (nanosleep(&req, NULL) == -1)
        continue;

    ProcessMessages();
    // Process pending configuration requests
/*    while (this->GetConfig(this->local_id, &client, this->req_buffer,
    PLAYER_MAX_REQREP_SIZE, NULL))
    {
    if (this->PutReply(this->local_id, client, PLAYER_MSGTYPE_RESP_NACK, NULL) != 0)
    PLAYER_ERROR("PutReply() failed");
}*/

        // Write data
        switch (this->device_addr.interf)
        {
            case PLAYER_CAMERA_CODE:
            {
                player_camera_data_t data;
                int w = 320;
                int h = 240;

                data.width = w;
                data.height = h;
                data.bpp = 24;
                data.format = PLAYER_CAMERA_FORMAT_RGB888;
                data.compression = PLAYER_CAMERA_COMPRESS_RAW;
                data.image_count = w * h * 3;
                
                data.image = new uint8_t[data.image_count];
                
                
                
                for (int j = 0; j < h; j++)
                {
                    for (int i = 0; i < w; i++)
                    {
                        data.image[(i + j * w) * 3 + 0] = loopcount;//((i + j) % 2) * 255;
                        data.image[(i + j * w) * 3 + 1] = loopcount;//((i + j) % 2) * 255;
                        data.image[(i + j * w) * 3 + 2] = loopcount;//((i + j) % 2) * 255;
                    }
                }

                int data_len = sizeof(data);// - sizeof(data.image) + w * h * 3;

                Publish (device_addr, PLAYER_MSGTYPE_DATA,
                         PLAYER_CAMERA_DATA_STATE, (void*)&data, data_len,
                         NULL);
                delete[] data.image;
                loopcount++;
                if (loopcount > 255)
                  loopcount = 0;
                
                break;
            }
            case PLAYER_LASER_CODE:
            {
                // Bogus data borrowed from Stage
                player_laser_data_t data;
                data.min_angle  = -1.5707964;
                data.max_angle  = 1.5707964;
                data.resolution = .5 * M_PI/180;
                data.max_range  = 8.0;
                data.ranges_count    = 361;
                data.intensity_count = 361;
                data.ranges = (float *) 
                              malloc( data.ranges_count * sizeof(float) );

                data.intensity = (uint8_t *) 
                              malloc( data.ranges_count * sizeof(uint8_t) );
                for (i = 0; i < data.ranges_count; i++)
                {
                    data.ranges[i]    = data.max_range;
                    data.intensity[i] = 1;
                }
                data.id = 1;

                Publish (device_addr, PLAYER_MSGTYPE_DATA,
                         PLAYER_LASER_DATA_SCAN, (void*)&data, sizeof(data),
                         NULL);
                free(data.ranges);
                free(data.intensity);
                break;
            }
            case PLAYER_RANGER_CODE:
            {
                // Printing simple ranger data
                player_ranger_data_range data;
                data.ranges_count = 361;
                data.ranges = (double *) 
                              malloc( data.ranges_count * sizeof(double) );

                for (i = 0; i < data.ranges_count; i++)
                {
                    data.ranges[i] = 8;
                }

                Publish (device_addr, PLAYER_MSGTYPE_DATA,
                         PLAYER_RANGER_DATA_RANGE, (void*)&data, sizeof(data),
                         NULL);
                free(data.ranges);
                break;
            }
            case PLAYER_POSITION2D_CODE:
            {
                player_position2d_data_t data;
                data.pos.px = 1.0;
                data.pos.py = 1.0;
                data.pos.pa = 1.0;
                data.vel.px = 1.0;
                data.vel.py = 1.0;
                data.vel.pa = 1.0;
                data.stall  = 0;
                Publish (device_addr, PLAYER_MSGTYPE_DATA,
                         PLAYER_POSITION2D_DATA_STATE, (void*)&data,
                         sizeof (data), NULL);
                break;
            }
            case PLAYER_PTZ_CODE:
            {
                player_ptz_data_t data;
                data.pan  = 1.0;
                data.tilt = 1.0;
                data.zoom = 1.0;
                data.panspeed  = 1.0;
                data.tiltspeed = 1.0;
                Publish (device_addr, PLAYER_MSGTYPE_DATA,
                         PLAYER_PTZ_DATA_STATE, (void*)&data,
                         sizeof (data), NULL);
                break;
            }
            case PLAYER_WSN_CODE:
            {
                player_wsn_data_t data;
                data.node_type      = 132;
                data.node_id        = 1;
                data.node_parent_id = 125;
                // Fill in the data packet with bogus values
                data.data_packet.light   = 779;
                data.data_packet.mic     = 495;
                data.data_packet.accel_x = 500;
                data.data_packet.accel_y = 500;
                data.data_packet.accel_z = 500;
                data.data_packet.magn_x  = 224;
                data.data_packet.magn_y  = 224;
                data.data_packet.magn_z  = 224;
                data.data_packet.temperature = 500;
                data.data_packet.battery = 489;

                Publish (device_addr, PLAYER_MSGTYPE_DATA,
                         PLAYER_WSN_DATA_STATE, (void*)&data,
                         sizeof (data), NULL);
                break;
            }
            case PLAYER_GPS_CODE:
            {
                player_gps_data_t data;
                data.time_sec = 1234567890;
                data.time_usec = 1;
                data.latitude = 1e7;
                data.longitude = 1e7;
                data.altitude = 1e3;
                data.utm_e = 10.0;
                data.utm_n = 10.0;
                data.quality = 2;
                data.num_sats = 7;
                data.hdop = 10;
                data.vdop = 10;
                data.err_horz = 1.0;
                data.err_vert = 1.0;
                Publish (device_addr, PLAYER_MSGTYPE_DATA,
                         PLAYER_GPS_DATA_STATE, (void*)&data,
                         sizeof (data), NULL);
                break;
            }
            case PLAYER_POSITION3D_CODE:
            {
                player_position3d_data_t data;
                data.pos.px = 1.0;
                data.pos.py = 1.0;
                data.pos.pz = 1.0;
                data.pos.proll = 1.0;
                data.pos.ppitch = 1.0;
                data.pos.pyaw = 1.0;
                data.vel.px = 1.0;
                data.vel.py = 1.0;
                data.vel.pz = 1.0;
                data.vel.proll = 0;
                data.vel.ppitch = 0;
                data.vel.pyaw = 0;
                data.stall  = 0;
                Publish (device_addr, PLAYER_MSGTYPE_DATA,
                         PLAYER_POSITION3D_DATA_STATE, (void*)&data,
                         sizeof (data), NULL);
                break;
            }
            case PLAYER_JOYSTICK_CODE:
            {
                player_joystick_data_t data;
                data.pos[0] = 1.0;
                data.pos[1] = 1.0;
                data.pos[2] = 1.0;
                data.scale[0] = 1.0;
                data.scale[1] = 1.0;
                data.scale[2] = 1.0;
                data.buttons  = 7;
                Publish (device_addr, PLAYER_MSGTYPE_DATA,
                         PLAYER_JOYSTICK_DATA_STATE, (void*)&data,
                         sizeof (data), NULL);
                break;
            }
            case PLAYER_BLOBFINDER_CODE:
            {
                player_blobfinder_data_t data;
                data.width = 320;
                data.height = 240;
                data.blobs_count = 2;
                data.blobs = new player_blobfinder_blob_t[data.blobs_count];
                for(int b=0; b < (int)data.blobs_count; b++)
                {
                  data.blobs[b].id = b;
                  data.blobs[b].color = b;
                  data.blobs[b].area = b;
                  data.blobs[b].x = b;
                  data.blobs[b].y = b;
                  data.blobs[b].left = b;
                  data.blobs[b].right = b;
                  data.blobs[b].top = b;
                  data.blobs[b].bottom = b;
                  data.blobs[b].range = b;
                }
                
                Publish (device_addr, PLAYER_MSGTYPE_DATA,
                         PLAYER_BLOBFINDER_DATA_BLOBS, (void*)&data,
                         sizeof(data), NULL);
                         
                delete[] data.blobs;
                
                break;
            }
            case PLAYER_POWER_CODE:
            {
                player_power_data_t data;
                data.valid = 0xFF;
                data.volts = 1.0;
                data.percent = 1.0;
                data.joules = 1.0;
                data.watts = 1.0;
                data.charging = 1;
                Publish (device_addr, PLAYER_MSGTYPE_DATA,
                         PLAYER_POWER_DATA_STATE, (void*)&data,
                         sizeof (data), NULL);
                break;
            }
            case PLAYER_POINTCLOUD3D_CODE:
            {
              player_pointcloud3d_data_t data;
              data.points_count = 10;
              data.points = new player_pointcloud3d_element_t[data.points_count];
              for (int i=0; i<(int)data.points_count;i++)
              {
                data.points[i].point.px = i;
                data.points[i].point.py = i;
                data.points[i].point.pz = i;
                
                data.points[i].color.alpha = 0;
                data.points[i].color.red = 128;
                data.points[i].color.green = 128;
                data.points[i].color.blue = 128;
              }
              
              Publish(device_addr, PLAYER_MSGTYPE_DATA,
              		PLAYER_POINTCLOUD3D_DATA_STATE, (void*) &data,
              		sizeof(data) , NULL);
              delete[] data.points;
              
              break;
            }
            case PLAYER_IMU_CODE:
            {
              player_imu_data_state_t data;
              
              data.pose.px = 1;
              data.pose.py = 1;
              data.pose.pz = 1;
              data.pose.proll = 1;
              data.pose.ppitch = 1;
              data.pose.pyaw = 1;
              
              Publish(device_addr, PLAYER_MSGTYPE_DATA,
              		PLAYER_IMU_DATA_STATE, (void*) &data,
              		sizeof(data), NULL);
              break;
            }
            case PLAYER_FIDUCIAL_CODE:
            {
              player_fiducial_data_t data;
              data.fiducials_count = 5;
              
              data.fiducials = new player_fiducial_item_t[data.fiducials_count];
              for (int i=0;i<(int)data.fiducials_count;i++)
              {
                data.fiducials[i].id = i;
                data.fiducials[i].pose.px = i;
                data.fiducials[i].pose.py = i;
                data.fiducials[i].pose.pz = i;
                data.fiducials[i].pose.proll = i;
                data.fiducials[i].pose.ppitch = i;
                data.fiducials[i].pose.pyaw = i;
                data.fiducials[i].upose.px = i;
                data.fiducials[i].upose.py = i;
                data.fiducials[i].upose.pz = i;
                data.fiducials[i].upose.proll = i;
                data.fiducials[i].upose.ppitch = i;
                data.fiducials[i].upose.pyaw = i;
              }
              Publish(device_addr, PLAYER_MSGTYPE_DATA,
              		PLAYER_FIDUCIAL_DATA_SCAN, (void*)&data,
              		sizeof(data), NULL);
              delete [] data.fiducials;
              break;
            }
            case PLAYER_WIFI_CODE:
            {
              player_wifi_data_t data;
              data.links_count = 2;
              data.throughput = 1;
              data.bitrate = 54;
              data.mode = 1;
              data.qual_type = 1;
              data.maxqual = 1;
              data.maxlevel = 1;
              data.maxnoise = 10;
              strcpy(data.ap, "AccessPoint");
              
              data.links = new player_wifi_link_t[data.links_count];
              for (int i=0;i<(int)data.links_count;i++)
              {
                data.links[i].mac_count = 18;
                memcpy(data.links[i].mac, "00:11:22:33:44:55\0", sizeof(char)*18);
                data.links[i].ip_count = 10;
                memcpy(data.links[i].ip,"127.0.0.1\0",sizeof(char)*10);
                data.links[i].essid_count = 6;
                memcpy(data.links[i].essid, "ESSID\0", sizeof(char)*6);
                data.links[i].mode = 1;
                data.links[i].freq = 1;
                data.links[i].encrypt = 1;
                data.links[i].qual = 1;
                data.links[i].level = 1;
                data.links[i].noise = 1;	
              }
              Publish(device_addr, PLAYER_MSGTYPE_DATA,
              		PLAYER_WIFI_DATA_STATE, (void*)&data,
              		sizeof(data), NULL);
              delete [] data.links;
              break;
              
            }
            case PLAYER_ACTARRAY_CODE:
            {
              player_actarray_data_t data;
              data.actuators_count = 2;
              data.motor_state = 1;
              data.actuators = new player_actarray_actuator_t[data.actuators_count];
              for (int i=0;i<(int)data.actuators_count;i++)
              {
                data.actuators[i].position = 1;
                data.actuators[i].speed = 1;
                data.actuators[i].acceleration = 1;
                data.actuators[i].current = 1;
                data.actuators[i].state = 1;	
              }
              Publish(device_addr, PLAYER_MSGTYPE_DATA,
              		PLAYER_ACTARRAY_DATA_STATE, (void*)&data,
              		sizeof(data), NULL);
              delete[] data.actuators;
              break;
            }
            case PLAYER_OPAQUE_CODE:
            {
              player_opaque_data_t data;
              data.data_count = 8;
              data.data = new uint8_t[data.data_count];
              for(int i=0;i<(int)data.data_count;i++)
              {
                data.data[i] = i;
              }
              Publish(device_addr, PLAYER_MSGTYPE_DATA,
              		PLAYER_OPAQUE_DATA_STATE, (void*)&data, sizeof(data), NULL);
              delete [] data.data;
              break;
            }
            case PLAYER_DIO_CODE:
            {
              player_dio_data_t data;
              data.count = 8;
              data.bits = 0xAA;
              Publish(device_addr, PLAYER_MSGTYPE_DATA,
              		PLAYER_DIO_DATA_VALUES, (void*)&data, sizeof(data), NULL);
              break;
            }
            case PLAYER_AIO_CODE:
            {
              player_aio_data_t data;
              data.voltages_count = 5;
              data.voltages = new float[data.voltages_count];
              for (int i=0; i<(int)data.voltages_count;i++)
              {
                data.voltages[i] = (float)i;
              }
              Publish(device_addr, PLAYER_MSGTYPE_DATA,
              		PLAYER_AIO_DATA_STATE, (void*)&data, sizeof(data), NULL);
              delete[] data.voltages;
              break;
            }
        }
    }
    return;
}
////////////////////////////////////////////////////////////////////////////////
// Process Message function to deny config requests.
int Dummy::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr,void * data)
{
  switch (hdr->type){
    case PLAYER_MSGTYPE_REQ:
      hdr->type = PLAYER_MSGTYPE_RESP_NACK;
      this->Publish(resp_queue, hdr, data);
      return (0);
    default:
      return (-1);
  }
  return (-1);
}
