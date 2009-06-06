/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2008 Walter Bamberger
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

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_AioToSonar AioToSonar
 * @brief Array of analogue sonar sensors connected to an aio device.

The AioToSonar driver offers the sonar interface for a group of
analogue sonar sensors. It accesses the sensors through an analogue IO
interface. (For example, you could connect several Phidets sonar
sensors to a Phidgets Interface Kit.)

@par Compile-time dependencies

  - none

@par Provides

  - @ref interface_sonar

@par Requires

  - @ref interface_aio: Offers the voltage values of the sensors.

@par Configuration requests

- PLAYER_SONAR_REQ_GET_GEOM

- On a PLAYER_SONAR_REQ_POWER request a NACK is replied only

@par Configuration file options

- sonarXX (Tuple)
    @verbatim
    sonarXX [port x y z roll pitch yaw]
    @endverbatim
  - Default: None. You must specify all sensors here.
  - Configures each sonar sensor of the array numbered with XX
    (00-99). The numbering must be continuous.
  - port (int): The number of the input port in the aio interface,
      which this sensor is connected to.
  - x/y/z/roll/pitch/yaw (float): The pose of the sensor.
- samplingperiod (float)
  - Default: 0.02 s
  - Time (in seconds) between to consecutive message queue
    processings. Should be synchronised with the sampling period of
    the aio device (e.g. samplingperiod_sonar ~
    samplingperiod_aio/2). Because this driver does not access any
    hardware, waking up is computationally lightweight.
- voltagetometers (float)
  - Default: 2.5918 m/V
  - distance = voltagetometers * voltage
  - Factor to convert the analogue input values (voltages) into
    distance values (meters). The default value fits for the Phidgets
    sonar sensor.

@par Example

@verbatim
driver
(
  name "AioToSonar"
  provides ["sonar:0"]
  requires ["aio:0"]

  # Sensor name, AIO Port and Pose (angles in degree) for each sensor
  # Name [port x y z roll pitch yaw]
  sonar00 [0  0.3  0.3  0.3  0.0  0.0  0.0]
  sonar01 [4 -0.3  0.3  0.3  180  0.0  0.0]
)
@endverbatim

@author Walter Bamberger

*/
/** @} */


#include <list>
#if !defined (WIN32)
  #include <sys/time.h>
#endif

#include <libplayercore/playercore.h>

#if !HAVE_NANOSLEEP
  #include <replace/replace.h>
#endif

using namespace std;


class AioToSonar : public ThreadedDriver
{
public:
  AioToSonar(ConfigFile *cf, int section);
  virtual ~AioToSonar(void){}

  virtual int MainSetup(void);
  virtual void MainQuit(void);

  virtual void Main(void);

  virtual int ProcessMessage(QueuePointer &resp_queue,
			     player_msghdr *hdr,
			     void *data);


private:
  timespec ComputeSleepDuration(const timeval &tLastStart,
				const timeval &tLastEnd,
				const timeval &tNow);

  /** The address and the pointer of the AIO device to interact
   *  with. The address is read from the config file. */
  player_devaddr aioDevAddr;
  Device *aioDev;

  /** Sensor configuration of the sonar array. On which analog port is
   *  a sensor with which pose? */
  struct SensorConfiguration {
    int port;
    player_pose3d pose;
  };
  list<SensorConfiguration> sensorList;

  /** Target duration of one cycle. This is the time between two tests
   *  for messages in the InQueue. It is measured in microseconds here
   *  (different from the configuration file. */
  long samplingperiod;

  /** Conversion factor from the voltage value to the distance
      value. */
  float voltagetometers;

  /** Internal state of the sleep duration control algorithm. */
  double weightedError;

  // List of all cycle periods for debugging the control algorithm
  // list<long> allCycleDurations;
};




/**
 * Factory function to create an object of this driver.
 *
 * This functionality is not object specific but class specific. It
 * could also be realised as a static function within the class.
 *
 * @return  a pointer to the created object
 */
Driver *AioToSonar_Init(ConfigFile *cf, int section)
{
  PLAYER_MSG0(2, "Create an AioToSonar driver object");

  // Create and return a new instance of this driver
  return((Driver*) new AioToSonar(cf, section));
}


/**
 * Registers the driver with its initialisation function in the
 * driver table. Should be called once on start up.
 *
 * This functionality is not object specific but class specific.
 *
 * @arg table a pointer to the table this driver should be registered
 *            in
 */
void AioToSonar_Register(DriverTable *table)
{
  PLAYER_MSG0(2, "Register the AioToSonar driver");

  table->AddDriver("AioToSonar", AioToSonar_Init);
}




/**
 * Initialise the driver object by reading the configuration file.
 */
AioToSonar::AioToSonar(ConfigFile *cf, int section)
  : ThreadedDriver(cf,
	   section,
	   true,               // Do new commands overwrite old ones?
	   PLAYER_MSGQUEUE_DEFAULT_MAXLEN, // Max length of the incomming queue
	   PLAYER_SONAR_CODE), // Which interface does AioToSonar implement?
    aioDev(NULL), weightedError(0.0)
{
  PLAYER_MSG0(2, "Initialise the AioToSonar driver");


  /* Read the address of the AIO device to connect to */
  if (cf->ReadDeviceAddr(&aioDevAddr,
			 section,
			 "requires",
			 PLAYER_AIO_CODE,
			 -1, NULL) != 0){
    PLAYER_ERROR("Could not read the address of the AIO device from the config file.");
  }


  /* Read the configuration (AIO port and pose) of all sensors in the
   * array */
  int sensorNumber = 0;
  char sensorName[8];  // Pattern: "sonarXX"
  SensorConfiguration sensorConfig;

  sprintf(sensorName, "sonar%02i", sensorNumber);
  while (cf->GetTupleCount(section, sensorName) == 7){

    sensorConfig.port = cf->ReadTupleInt(section, sensorName, 0, -1);
    if (sensorConfig.port < 0){
      PLAYER_ERROR1("Could not read the port configuration of sensor %s.", sensorName);

    } else {
      sensorConfig.pose.px = cf->ReadTupleLength(section, sensorName, 1, 0.0);
      sensorConfig.pose.py = cf->ReadTupleLength(section, sensorName, 2, 0.0);
      sensorConfig.pose.pz = cf->ReadTupleLength(section, sensorName, 3, 0.0);
      sensorConfig.pose.proll = cf->ReadTupleAngle(section, sensorName, 4, 0.0);
      sensorConfig.pose.ppitch = cf->ReadTupleAngle(section, sensorName, 5, 0.0);
      sensorConfig.pose.pyaw = cf->ReadTupleAngle(section, sensorName, 6, 0.0);

      sensorList.push_back(sensorConfig);
    }

    ++sensorNumber;
    if (sensorNumber < 100){
      sprintf(sensorName, "sonar%02i", sensorNumber);

    } else {
      break;
    }
  }


  /* Read the samplingperiod configuration value and convert it to
     microseconds. */
  samplingperiod = static_cast<long>(
      cf->ReadFloat(section, "samplingperiod", 0.02) * 1000000
  );

  /* Read the conversion factor from the configuration file. */
  voltagetometers = cf->ReadFloat(section, "voltagetometers", 2.5918);
}


/**
 * Connect to the aio device and start the thread.
 *
 * @return 0 if everything has been set up well
 */
int AioToSonar::MainSetup(void)
{
  PLAYER_MSG0(2, "Connect to the aio device and start the thread");

  aioDev = deviceTable->GetDevice(aioDevAddr);
  if (!aioDev){
    PLAYER_ERROR("Could not find the aio device");
  }

  if (aioDev->Subscribe(InQueue)){
    PLAYER_ERROR("Could not subscribe to the aio device");
  }

  PLAYER_MSG0(2, "AioToSonar has been set up");

  return 0;
}


/**
 * Stops the thread and disconnect from the aio device.
 *
 * @return 0 if everything has been released well
 */
void AioToSonar::MainQuit(void)
{
  PLAYER_MSG0(2, "Stop the thread and disconnect from the aio device.");
  if (aioDev->Unsubscribe(InQueue)){
    PLAYER_WARN("Could not unsubscribe from the AIO device device");
  }

  /* For debugging the control algorithm

  long mean = 0;
  for (list<long>::iterator it = allCycleDurations.begin();
       it != allCycleDurations.end();
       ++it){
    mean += *it;
  }
  mean /= allCycleDurations.size();
  cout << "Mean: " << mean << endl;
  */

  PLAYER_MSG0(2, "AioToSonar has been shut down");

}


/**
 * Main loop of the driver's thread. Processes all messages, publishes
 * new sensor values and maintains the thread.
 */
void AioToSonar::Main(void) {
  timespec sleepDuration = {0, 0};  // Seconds and nanoseconds
  timeval tLastStart, tLastEnd, tNow; // Seconds and microseconds

  PLAYER_MSG0(2, "Starting the main loop of the AioToSonar driver");

  gettimeofday(&tLastEnd, NULL);
  tLastStart = tLastEnd;
  // The correction factor comes from the weightings in the
  // ComputeSleepDuration function
  weightedError = - static_cast<double>(samplingperiod) * 0.4 / 0.6;

  while (true){
    // Should this thread stop?
    pthread_testcancel();

    // Handle messages in the queue
    ProcessMessages();

    // Wait (polling mode)
    gettimeofday(&tNow, NULL);
    sleepDuration = ComputeSleepDuration(tLastStart, tLastEnd, tNow);
    nanosleep(&sleepDuration, NULL);

    // Update the cycle timers
    tLastStart = tLastEnd;
    gettimeofday(&tLastEnd, NULL);
  }
}


/**
 * A simple control algorithm for the sampling period. It implements
 * the loop controller for the (controlled) sampling system.
 *
 * System description: The controller is an IIR (infinite impulse
 * response) system with one internal state (weightedError).
 * Inputs:
 *   c: The period of the last cycle (lastCycleDuration)
 *   p: The duration of the processing in the current cycle
 *      (processingDuration)
 *
 * State:
 *   x: This state commulates the error (weightedError)
 *
 * Output:
 *   s: The time to sleep in the current cycle
 *
 * Computation:
 *   x[i] = 0.6 x[i-1] + 0.4 (samplingperiod - c[i])
 *   s[i] = x[i] - p + samplingperiod
 */
timespec AioToSonar::ComputeSleepDuration(const timeval &tLastStart,
					  const timeval &tLastEnd,
					  const timeval &tNow)
{
  long lastCycleDuration =
    (tLastEnd.tv_sec - tLastStart.tv_sec) * 1000000
    + tLastEnd.tv_usec - tLastStart.tv_usec;
  // When modifying the weightings please look in the Main method too.
  weightedError =
    0.6 * weightedError
    + 0.4 * static_cast<double>(samplingperiod - lastCycleDuration);
  PLAYER_MSG1(8, "lastCycleDuration %i", lastCycleDuration);

  // To debug the control algorithm
  // allCycleDurations.push_back(lastCycleDuration);

  long processingDuration =
    (tNow.tv_sec - tLastEnd.tv_sec) * 1000000
    + tNow.tv_usec - tLastEnd.tv_usec;

  long sleepDuration =
    static_cast<long>(weightedError)
    + samplingperiod
    - processingDuration;

  timespec sleepDurationTs;
  sleepDurationTs.tv_sec = sleepDuration / 1000000;
  sleepDurationTs.tv_nsec = (sleepDuration % 1000000) * 1000;

  /* To debug the control algorithm

  cout << "e/c/p/s: " << weightedError
       << " / " << lastCycleDuration
       << " / " << processingDuration
       << " / " << sleepDurationTs.tv_sec << " " << sleepDurationTs.tv_nsec
       << endl;
  */

  return sleepDurationTs;
}


/**
 * Processes the messages of the sonar interface.
 *
 * Depending on the message this method currently does the following:
 *
 *   PLAYER_SONAR_REQ_GET_GEOM: Replies the geometry of the array.
 *   PLAYER_SONAR_REQ_POWER: Replies with an NACK.
 *   PLAYER_AIO_DATA_STATE: Converts the voltage to a distance measure
 *   and publishes the distance then.
 *
 * @return  0 if the message has been handled
 *         -1 otherwise.
 */
int AioToSonar::ProcessMessage(QueuePointer &resp_queue,
			       player_msghdr *hdr,
			       void *data)
{
  assert(hdr);


  if (Message::MatchMessage(hdr,
			    PLAYER_MSGTYPE_REQ,
			    PLAYER_SONAR_REQ_GET_GEOM,
			    device_addr)){

    PLAYER_MSG0(4, "PLAYER_SONAR_REQ_GET_GEOM received");

    player_sonar_geom geometry;
    geometry.poses_count = sensorList.size();
    geometry.poses = new player_pose3d[geometry.poses_count];

    player_pose3d *poseIt = geometry.poses;
    for (list<SensorConfiguration>::iterator it=sensorList.begin();
	 it!=sensorList.end();
	 ++it, ++poseIt){

      *poseIt = it->pose;
    }

    Publish(device_addr,
	    resp_queue,
	    PLAYER_MSGTYPE_RESP_ACK,
	    PLAYER_SONAR_REQ_GET_GEOM,
	    (void *) &geometry);

    delete[] geometry.poses;

    return 0;

  } else if (Message::MatchMessage(hdr,
				   PLAYER_MSGTYPE_REQ,
				   PLAYER_SONAR_REQ_POWER,
				   device_addr)){

    PLAYER_MSG0(4, "PLAYER_SONAR_REQ_POWER received");

    Publish(device_addr,
	    resp_queue,
	    PLAYER_MSGTYPE_RESP_NACK,
	    hdr->subtype);

    return 0;

  } else if (Message::MatchMessage(hdr,
				   PLAYER_MSGTYPE_DATA,
				   PLAYER_AIO_DATA_STATE,
				   aioDevAddr)){

    PLAYER_MSG0(6, "PLAYER_AIO_DATA_STATE");

    player_aio_data *received_data =
      reinterpret_cast<player_aio_data *>(data);
    player_sonar_data sonarData;
    sonarData.ranges_count = sensorList.size();
    sonarData.ranges = new float[sonarData.ranges_count];

    float *rangesIt = sonarData.ranges;
    for (list<SensorConfiguration>::iterator it=sensorList.begin();
	 it!=sensorList.end();
	 ++it, ++rangesIt){
      if (it->port < static_cast<int> (received_data->voltages_count)){
	*rangesIt = received_data->voltages[it->port] * voltagetometers;

	Publish(device_addr,
		PLAYER_MSGTYPE_DATA,
		PLAYER_SONAR_DATA_RANGES,
		(void *) &sonarData);

      } else {
	*rangesIt = 0;
      }
    }

    delete[] sonarData.ranges;

    return 0;
  }

  PLAYER_MSG0(2, "Received an unknown message type");
  return -1;
}
