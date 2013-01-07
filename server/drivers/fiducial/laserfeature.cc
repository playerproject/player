/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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
///////////////////////////////////////////////////////////////////////////
//
// Desc: Driver for extracting line/corner features from a laser scan.
// Author: Andrew Howard
// Date: 29 Aug 2002
// CVS: $Id$
//
// Theory of operation - Uses an EKF to segment the scan into line
// segments, then does a best-fit for each segment.  The EKF is based
// on the approach by Stergios Romelioutis.
//
// Requires - Laser device.
//
///////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_laserfeature laserfeature
 * @brief Extract line/corner features from a laser scan.

The laserfeature driver extracts lines and corners from a laser data scan,
and returns the 2-d positions of each line segment through the Fiducial
interface.

laserfeature uses an EKF to segment the scan into line
segments, then does a best-fit for each segment.  The EKF is based
on the approach by Stergios Romelioutis.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_fiducial
  - This interface returns a list of features extracted from the laser scan.

@par Configuration requests

- fiducial interface
  - PLAYER_POSITION2D_REQ_FIDUCAL_GEOM

@par Requires

- @ref interface_laser
  - Laser scan to find lines in

@par Configuration file options

- model_range_noise (length)
  - Default: 0.02 m
  - Expected range noise in laser measurements

- model_angle_noise (angle)
  - Default: 10 degrees
  - Expected angular noise in laser measurements

- sensor_range_noise (length)
  - Default: 0.05 m
  - Expected range noise in laser sensor.

- segment_range (length)
  - Default: 0.05m

- merge_angle (angle)
  - Default: 10 degrees

- discard_length (length)
  - Default: 1.00 m
  - Threshold for segment length (smaller segments are discarded)

- min_segment_count (int)
  - Default: 4
  - Minimum amount of segments needed to publish Fiducial data

@par Example

@verbatim
driver
(
  name "laserfeature"
  provides ["fiducial:0"]
  requires ["laser:0"]

  model_range_noise 0.05
  angle_range_noise 10
  sensor_range_noise 0.05
  segment_range 0.05
  merge_angle 10
  discard_length 1.00
  min_segment_count 4

)
@endverbatim

@author Andrew Howard, Rich Mattes
 */
/** @} */

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
// needed for usleep, no unistd.h on windows.
#if WIN32 && !__MINGW32__
#include <replace/replace.h>
#else
#include <unistd.h>
#endif

#include <libplayercore/playercore.h>

// Driver for detecting features in laser scan.
class LaserFeature : public ThreadedDriver
{
public:
	// Constructor
	LaserFeature( ConfigFile* cf, int section);

	// Setup/shutdown routines.
	virtual int MainSetup();
	virtual void MainQuit();

	virtual void Main();

	// Message Handling routine
	virtual int ProcessMessage (QueuePointer &resp_queue, player_msghdr * hdr, void * data);

private:
	// Process laser data.  Returns non-zero if the laser data has been
	// updated.
	int UpdateLaser();

	// Write laser data to a file (testing).
	int WriteLaser();

	// Read laser data from a file (testing).
	int ReadLaser();

	// Segment the scan into straight-line segments.
	void SegmentLaser();

	// Update the line filter.  Returns an error signal.
	double UpdateFilter(double x[2], double P[2][2], double Q[2][2],
			double R, double z, double res);

	// Fit lines to the segments.
	void FitSegments();

	// Merge overlapping segments with similar properties.
	void MergeSegments();

	// Update the device data (the data going back to the client).
	void PublishFiducial();

	// Process requests.  Returns 1 if the configuration has changed.
	int HandleRequests();

	// Handle geometry requests.
	void HandleGetGeom(void *client, void *req, int reqlen);

	// Device pose relative to robot.
	double pose[3];

	// Laser stuff (the laser we require)
	Device *laser_device;
	player_laser_data_t laser_data;
	player_devaddr laser_id;
	bool have_new_scan;

	// Fiducial stuff (the data we generate).
	player_fiducial_data_t data;
	player_devaddr_t fiducial_id;
	uint32_t timesec, timeusec;

	// Line filter settings.
	double model_range_noise;
	double model_angle_noise;
	double sensor_range_noise;

	// Threshold for segmentation
	double segment_range;

	// Thresholds for merging.
	double merge_angle;

	// Thresholds for discarding.
	double discard_length;

	// How many segments need to be found before any are reported
	int min_segment_count;

	// Segment mask.
	int mask[4096];

	// Description for each extracted line segment.
	struct segment_t
	{
		int first, last, count;
		double pose[3];
		double length;
	};

	// List of extracted line segments.
	int segment_count;
	segment_t segments[4096];

};


// Initialization function
Driver* LaserFeature_Init( ConfigFile* cf, int section)
{
	return reinterpret_cast <Driver*> (new LaserFeature (cf, section));
}

// Driver registration function
void laserfeature_Register(DriverTable* table)
{
	table->AddDriver("laserfeature", LaserFeature_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
LaserFeature::LaserFeature( ConfigFile* cf, int section)
: ThreadedDriver(cf, section)
{

	// Find the fiducial interface to provide
	if(cf->ReadDeviceAddr(&(this->fiducial_id), section, "provides",
			PLAYER_FIDUCIAL_CODE, -1, NULL) == 0)
	{
		if(this->AddInterface(this->fiducial_id) != 0)
		{
			PLAYER_ERROR("laserfeature: Error adding fidicual interface, please check config file.");
			this->SetError(-1);
			return;
		}
	}
	else
	{
		PLAYER_ERROR("laserfeature: Must provide a fiducial interface, please check config file");
	}

	//Find a laser interface to subscribe to
	if(cf->ReadDeviceAddr(&(this->laser_id), section, "requires",
			PLAYER_LASER_CODE, -1, NULL) != 0)
	{
		PLAYER_ERROR("laserfeature: Must require a laser interface, please check config file");
		this->SetError(-1);
	}

	// Device pose relative to robot.
	this->pose[0] = 0;
	this->pose[1] = 0;
	this->pose[2] = 0;


	// Line filter settings.
	this->model_range_noise = cf->ReadLength(section, "model_range_noise", 0.02);
	this->model_angle_noise = cf->ReadAngle(section, "model_angle_noise", 10 * M_PI / 180);
	this->sensor_range_noise = cf->ReadLength(section, "sensor_range_noise", 0.05);

	// Segmentation settings
	this->segment_range = cf->ReadLength(section, "segment_range", 0.05);

	// Segment merging settings.
	this->merge_angle = cf->ReadAngle(section, "merge_angle", 10 * M_PI / 180);

	// Post-processing
	this->discard_length = cf->ReadLength(section, "discard_length", 1.00);

	// Min Segments
	this->min_segment_count = cf->ReadInt(section, "min_segment_count", 4);

	// Initialise segment list.
	this->segment_count = 0;

	// Fiducial data.
	this->timesec = 0;
	this->timeusec = 0;
	memset(&this->data, 0, sizeof(this->data));
	data.fiducials = NULL;

	// Initialize new data flag
	have_new_scan = false;

	return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int LaserFeature::MainSetup()
{
	if(!(this->laser_device = deviceTable->GetDevice(this->laser_id)))
	{
		PLAYER_ERROR("laserfeature: Unable to get laser device");
		return(-1);
	}
	if(this->laser_device->Subscribe(this->InQueue) != 0 )
	{
		PLAYER_ERROR("laserfeature: Unable to subscribe to laser device");
		return(-1);
	}

	// Get the laser geometry.
	// TODO: no support for this at the moment.
	//this->pose[0] = 0.10;
	//this->pose[1] = 0;
	//this->pose[2] = 0;

	// TESTING
	//this->laser_file = fopen("laser02.log", "r");
	//assert(this->laser_file);

	return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
void LaserFeature::MainQuit()
{
	// Unsubscribe from devices.
	this->laser_device->Unsubscribe(this->InQueue);

	// TESTING
	//fclose(this->laser_file);

	return;
}

////////////////////////////////////////////////////////////////////////////////
// MAIN DRIVER LOOP
void LaserFeature::Main()
{
	for(;;)
	{
		pthread_testcancel();

		ProcessMessages();

		if (this->have_new_scan)
		{
			// Segment the scan into straight-line segments.
			this->SegmentLaser();

			// Fit lines to the segments.
			this->FitSegments();

			// Merge similar segments.
			this->MergeSegments();

			// Re-do the fit for the merged segments.
			this->FitSegments();

			// Publish Fiducial data
			this->PublishFiducial();

			// Don't process again until we get new data
			this->have_new_scan = false;
		}

		// Sleep for a while
		usleep(100000);

	}
}

////////////////////////////////////////////////////////////////////////////////
// Segment the scan into straight-line segments.
void LaserFeature::SegmentLaser()
{
	int i;
	double r, b;
	double res;
	double x[2], P[2][2];
	double Q[2][2], R;
	double err;
	int mask;

	// Angle between successive laser readings.
	res = (double) (this->laser_data.resolution) / 100.0 * M_PI / 180;

	// System noise.
	Q[0][0] = this->model_range_noise * this->model_range_noise;
	Q[0][1] = 0;
	Q[1][0] = 0;
	Q[1][1] = this->model_angle_noise * this->model_angle_noise;

	// Sensor noise.
	R = this->sensor_range_noise * this->sensor_range_noise;

	// Initial estimate and covariance.
	x[0] = 1.0;
	x[1] = M_PI / 2;
	P[0][0] = 100;
	P[0][1] = 0.0;
	P[1][0] = 0.0;
	P[1][1] = 100;

	// Initialise the segments.
	this->segment_count = 0;

	// Apply filter anti-clockwise.
	mask = 0;
	for (i = 0; i < this->laser_data.ranges_count; i++)
	{
		r = (double) (this->laser_data.ranges[i]);
		b = (double) (this->laser_data.min_angle) * M_PI / 180 + i * res;

		err = this->UpdateFilter(x, P, Q, R, r, res);

		if (err < this->segment_range)
		{
			if (mask == 0)
				this->segments[this->segment_count++].first = i;
			this->segments[this->segment_count - 1].last = i;
			mask = 1;
		}
		else
			mask = 0;

		//fprintf(stderr, "%f %f ", r, b);
		//fprintf(stderr, "%f %f %f ", x[0], x[1], err);

		/*
    double psi = M_PI / 2 + b + x[1];

    double px = x[0] * cos(b);
    double py = x[0] * sin(b);

    double qx = px + 0.2 * cos(psi);
    double qy = py + 0.2 * sin(psi);

    fprintf(stderr, "%f %f\n%f %f\n\n", px, py, qx, qy);
		 */

		/*
    // TESTING
    double psi, px, py;
    px = r * cos(b);
    py = r * sin(b);
    psi = M_PI / 2 + b + x[1];
    px += 0.5 * cos(psi);
    py += 0.5 * sin(psi);
    fprintf(stderr, "%f %f\n", sqrt(px * px + py * py), atan2(py, px));
		 */
	}
	//fprintf(stderr, "\n\n");

	// Apply filter clockwise.
	mask = 0;
	for (i = this->laser_data.ranges_count - 1; i >= 0; i--)
	{
		r = (double) (this->laser_data.ranges[i]);
		b = (double) (this->laser_data.min_angle) * M_PI / 180 + i * res;

		err = this->UpdateFilter(x, P, Q, R, r, -res);

		if (err < this->segment_range)
		{
			if (mask == 0)
				this->segments[this->segment_count++].last = i;
			this->segments[this->segment_count - 1].first = i;
			mask = 1;
		}
		else
			mask = 0;

		//fprintf(stderr, "%f %f ", r, b);
		//fprintf(stderr, "%f %f %f\n", x[0], x[1], err);
	}
	//fprintf(stderr, "\n\n");

	return;
}


////////////////////////////////////////////////////////////////////////////////
// Update the line filter.  Returns an error signal.
double LaserFeature::UpdateFilter(double x[2], double P[2][2], double Q[2][2],
		double R, double z, double res)
{
	int i, j, k, l;
	double x_[2], P_[2][2];
	double F[2][2];
	double r, S;
	double K[2];

	// A priori state estimate.
	x_[0] = sin(x[1]) / sin(x[1] - res) * x[0];
	x_[1] = x[1] - res;

	// Jacobian for the system function.
	F[0][0] = sin(x[1]) / sin(x[1] - res);
	F[0][1] = -sin(res) / (sin(x[1] - res) * sin(x[1] - res)) * x[0];
	F[1][0] = 0;
	F[1][1] = 1;

	// Covariance of a priori state estimate.
	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 2; j++)
		{
			P_[i][j] = 0.0;
			for (k = 0; k < 2; k++)
				for (l = 0; l < 2; l++)
					P_[i][j] += F[i][k] * P[k][l] * F[j][l];
			P_[i][j] += Q[i][j];
		}
	}

	// Residual (difference between prediction and measurement).
	r = z - x_[0];

	// Covariance of the residual.
	S = P_[0][0] + R;

	// Kalman gain.
	K[0] = P_[0][0] / S;
	K[1] = P_[1][0] / S;

	// Posterior state estimate.
	x[0] = x_[0] + K[0] * r;
	x[1] = x_[1] + K[1] * r;

	// Posterior state covariance.
	for (i = 0; i < 2; i++)
		for (j = 0; j < 2; j++)
			P[i][j] = P_[i][j] - K[i] * S * K[j];

	return fabs(r); //r * r / S;
}


////////////////////////////////////////////////////////////////////////////////
// Fit lines to the extracted segments.
void LaserFeature::FitSegments()
{
	int i, j;
	segment_t *segment;
	double r, b, x, y;
	double n, sx, sy, sxx, sxy, syy;
	double px, py, pa;
	double ax, ay, bx, by, cx, cy;

	for (j = 0; j < this->segment_count; j++)
	{
		segment = this->segments + j;

		n = sx = sy = sxx = syy = sxy = 0.0;
		ax = ay = bx = by = 0.0;

		for (i = segment->first; i <= segment->last; i++)
		{
			r = (double) (this->laser_data.ranges[i]);
			b = (double) (this->laser_data.min_angle + i * this->laser_data.resolution)
        								 * M_PI / 180;
			x = r * cos(b);
			y = r * sin(b);

			if (i == segment->first)
			{
				ax = x;
				ay = y;
			}
			else if (i == segment->last)
			{
				bx = x;
				by = y;
			}

			n += 1;
			sx += x;
			sy += y;
			sxx += x * x;
			syy += y * y;
			sxy += x * y;
		}

		px = sx / n;
		py = sy / n;
		pa = atan2((n * sxy  - sy * sx), (n * sxx - sx * sx));

		// Make sure the orientation is normal to surface.
		pa += M_PI / 2;
		if (fabs(NORMALIZE(pa - atan2(py, px))) < M_PI / 2)
			pa += M_PI;

		segment->count = (int) n;
		segment->pose[0] = px;
		segment->pose[1] = py;
		segment->pose[2] = pa;

		cx = bx - ax;
		cy = by - ay;
		segment->length = sqrt(cx * cx + cy * cy);
	}

	return;
}


////////////////////////////////////////////////////////////////////////////////
// Merge overlapping segments with similar properties.
void LaserFeature::MergeSegments()
{
	int i, j;
	segment_t *sa, *sb;
	double da;

	for (i = 0; i < this->segment_count; i++)
	{
		for (j = i + 1; j < this->segment_count; j++)
		{
			sa = this->segments + i;
			sb = this->segments + j;

			// See if segments overlap...
			if (sb->first <= sa->last && sa->first <= sb->last)
			{
				// See if the segments have the same orientation...
				da = NORMALIZE(sb->pose[2] - sa->pose[2]);
				if (fabs(da) < this->merge_angle)
				{
					sa->first = MIN(sa->first, sb->first);
					sa->last = MAX(sa->last, sb->last);
					sb->first = 0;
					sb->last = -1;
				}
			}
		}
	}
	return;
}


////////////////////////////////////////////////////////////////////////////////
// Update the device data (the data going back to the client).
void LaserFeature::PublishFiducial()
{
	int i;
	double px, py, pa;
	double r, b, o;
	segment_t *segment;

	this->data.fiducials_count = 0;

	if (this->data.fiducials)
	{
		delete[] this->data.fiducials;
	}
	this->data.fiducials = new player_fiducial_item_t[this->segment_count];


	for (i = 0; i < this->segment_count; i++)
	{
		segment = this->segments + i;

		if (segment->count >= this->min_segment_count && segment->length >= this->discard_length)
		{
			px = segment->pose[0];
			py = segment->pose[1];
			pa = segment->pose[2];

			r = sqrt(px * px + py * py);
			b = atan2(py, px);
			o = pa;

			this->data.fiducials[this->data.fiducials_count].id = 0;
			this->data.fiducials[this->data.fiducials_count].pose.px = r;
			this->data.fiducials[this->data.fiducials_count].pose.py = b;
			this->data.fiducials[this->data.fiducials_count].pose.pz = 0;
			this->data.fiducials[this->data.fiducials_count].pose.proll = 0;
			this->data.fiducials[this->data.fiducials_count].pose.ppitch = 0;
			this->data.fiducials[this->data.fiducials_count].pose.pyaw = o;

			this->data.fiducials_count++;
		}
	}
	Publish(this->fiducial_id, PLAYER_MSGTYPE_DATA, PLAYER_FIDUCIAL_DATA_SCAN, (void*)&this->data, sizeof(this->data), NULL);

	return;
}

////////////////////////////////////////////////////////////////////////////////
// Handle all incoming messages
int LaserFeature::ProcessMessage (QueuePointer &resp_queue, player_msghdr * hdr, void * data)
{
  HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILITIES_REQ);
  HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_FIDUCIAL_REQ_GET_GEOM);
	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_FIDUCIAL_REQ_GET_GEOM, this->fiducial_id))
	{
		player_fiducial_geom_t geom;
		memset(&geom, 0, sizeof(player_fiducial_geom_t));

		geom.pose.px = this->pose[0];
		geom.pose.py = this->pose[1];
		geom.pose.pyaw = this->pose[2];

		Publish(this->fiducial_id, PLAYER_MSGTYPE_RESP_ACK, PLAYER_FIDUCIAL_REQ_GET_GEOM, &geom, sizeof(geom), NULL);

		return 0;

	}
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCAN, this->laser_id))
	{
		this->laser_data = *(player_laser_data_t*) data;

		have_new_scan = true;
		return 0;
	}
	else
	{
		return -1;
	}
}

