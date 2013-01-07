/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2007
*      Radu Bogdan Rusu (rusu@cs.tum.edu)
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
 Desc: Calculates the joint commands for a given End Effector pose using
       ROBOOP's Inverse Kinematics algorithms.
 Author: Radu Bogdan Rusu and Dan Pojar
 Date: 10 May 2007
 CVS: $Id$
*/

/** @ingroup drivers Drivers */
/** @{ */
/** @defgroup driver_eeDHcontroller eeDHcontroller
 * @brief Calculates the joint commands for a given End Effector pose using
ROBOOP's Inverse Kinematics algorithms.

The roboopIK driver performs inverse kinematics calculations using the
ROBOOP library for a given robot arm's end effector, and sends the resulting
joint commands to the appropriate actarray interface. The arm model is
specified in the Player configuration file using the Denavit-Hartenberg
convention parameters (see
http://prt.fernuni-hagen.de/lehre/KURSE/PRT001/course_main/node15.html for
more details).

When a positioning command of the limb is received via
PLAYER_LIMB_CMD_SETPOSITION or PLAYER_LIMB_CMD_SETPOSE, the driver computes
the joint commands and sends them in ascending order (base to end effector)
to the actarray interface using PLAYER_ACTARRAY_CMD_POS.

When a homing command of the limb is received via PLAYER_LIMB_CMD_HOME, the
driver will send a PLAYER_ACTARRAY_CMD_HOME to every joint provided by the
actarray interface in descending order (end effector to base).

The driver also computes the current pose of the end effector using forward
kinematics (given the current joint positions taken from the actarray
interface) and returns it as a data packet.

@par Compile-time dependencies

- the ROBOOP library (http://www.cours.polymtl.ca/roboop/)

@par Provides

- @ref interface_limb

@par Requires

- @ref interface_actarray

@par Configuration requests

- PLAYER_LIMB_REQ_SPEED

@par Configuration file options

- nr_joints (integer)
  - The number of joints that we provide DH parameters for (should be the
    same as the number of actuators the actarray interface provides).

- jointX_DH (integer tuple)
  - [ R/P theta d a alfa th_min th_max ] - DH parameters for joint X

- error_pos (float)
  - Default: 0
  - User allowed error value in position in degrees. This is needed for joints
    who do not change their state (eg. they remain idle) when a command is given
    and the joint is already in that position.

- debug (int)
  - Default: 0
  - Enable debugging mode (detailed information messages are printed on screen)
    Valid values: 0 (disabled), 1 (enabled).

@par Example

@verbatim
driver
(
  name "eeDHcontroller"
  provides ["limb:0"]
  requires ["actarray:0"]

  nr_joints 6

  # [ R/P theta d a alfa th_min th_max ]
  joint1_DH [ 0 0 0.180  0  1.57 -1.57 2.0  ]
  joint2_DH [ 0 0 0.215  0 -1.57 -1.57 1.57 ]
  joint3_DH [ 0 0 0.0    0  1.57 -1.57 1.57 ]
  joint4_DH [ 0 0 0.308  0 -1.57 -1.57 1.57 ]
  joint5_DH [ 0 0 0.0    0  1.57 -2.09 2.09 ]
  joint6_DH [ 0 0 0.2656 0  0     0    0 ]

  # Allowed positioning error in degrees
  error_pos 0.01

  # Enable debug mode
  debug 1
)
@endverbatim

@author Radu Bogdan Rusu and Dan Pojar

*/
/** @} */


#include <robot.h>
#include <libplayercore/playercore.h>

// Structure for the command thread
typedef struct
{
  void *driver;
  ColumnVector q;
} CmdLoopStruc;

////////////////////////////////////////////////////////////////////////////////
// The EEDHController device class.
class EEDHController : public ThreadedDriver
{
  public:
    // Constructor
    EEDHController (ConfigFile* cf, int section);

    // Destructor
    ~EEDHController ();

    // Implementations of virtual functions
    virtual int MainSetup ();
    virtual void MainQuit ();

    // This method will be invoked on each incoming message
    virtual int ProcessMessage (QueuePointer &resp_queue,
                                player_msghdr * hdr,
                                void * data);

  private:

    player_actarray_data_t actarray_data;
    player_limb_data_t     limb_data;
    bool actarray_data_received;

    // device bookkeeping
    player_devaddr_t actarray_addr;
    Device* actarray_device;

    // Main function for device thread.
    virtual void Main ();
    virtual void RefreshData  ();

    // Joints count and their DH parameters
    Matrix DHMatrixModel;
    unsigned int nr_joints;

    // Create two Robot objects (for IK and for FK)
    Robot robot_IK, robot_FK;
    ColumnVector q_cmd;

    // FK and IK methods
    int ComputeQ (unsigned int dof, double x, double y, double z,
                  double roll, double pitch, double yaw, ColumnVector &q);
    ColumnVector ComputeEEPose (ColumnVector q_cmd);
    float m_atan2 (float a, float b);

    // Send commands to the actarray
    void CommandJoints (ColumnVector q_cmd);

    // Keeping track of whether joints are still moving or not
    int *actarray_state;

    // Threads used for homing or moving the actarray
    pthread_t a_th_home, a_th_cmd;
    bool command_thread, homing_thread;

    // Functions defining the threads
    static void* DummyAHomeLoop (void *driver);
    static void* DummyACmdLoop  (void *structure);
    void AHomeLoop ();
    void ACmdLoop  (ColumnVector q);

    int debug;

    // Allowed positioning error in degrees
    float error_pos;
};

////////////////////////////////////////////////////////////////////////////////
// Factory creation function. This functions is given as an argument when
// the driver is added to the driver table
Driver* EEDHController_Init (ConfigFile* cf, int section)
{
  // Create and return a new instance of this driver
  return ((Driver*)(new EEDHController (cf, section)));
}

////////////////////////////////////////////////////////////////////////////////
// Registers the driver in the driver table. Called from the
// player_driver_init function that the loader looks for
void
  eedhcontroller_Register (DriverTable* table)
{
  table->AddDriver ("eedhcontroller", eedhcontroller_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
EEDHController::EEDHController (ConfigFile* cf, int section)
  : ThreadedDriver (cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN,
            PLAYER_LIMB_CODE)
{
  unsigned int i, j;

  // Must have an input actarray
  if (cf->ReadDeviceAddr(&this->actarray_addr, section, "requires",
                         PLAYER_ACTARRAY_CODE, -1, NULL) != 0)
  {
    PLAYER_ERROR ("must have an input actarray");
    this->SetError(-1);
    return;
  }
  this->actarray_device = NULL;
  this->actarray_data_received = false;

  nr_joints = cf->ReadInt (section, "nr_joints", 0);

  // Create initial DH parameters model
  DHMatrixModel = Matrix (nr_joints, 23);
  DHMatrixModel = 0;

  actarray_state = new int[nr_joints];

  for (i = 0; i < nr_joints; i++)
  {
    char joint_nr[10];
    snprintf (joint_nr, 10,"joint%d_DH", (i+1));

    for (j = 0; j < 7; j++)
    {
      float bla = cf->ReadTupleFloat (section, joint_nr, j, 0);
      DHMatrixModel (i+1, j+1) = bla;
    }

    // Set the initial status of joints (IDLE)
    actarray_state[i] = PLAYER_ACTARRAY_ACTSTATE_IDLE;
  }

  // Instantiate the robot(s) with the DH parameter matrix
  robot_IK = Robot (DHMatrixModel);
  robot_FK = Robot (DHMatrixModel);
  q_cmd = ColumnVector (nr_joints);

  debug = cf->ReadInt (section, "debug", 0);

  // Allowed positioning error in degrees
  error_pos = cf->ReadFloat (section, "error_pos", 1.0);
}


////////////////////////////////////////////////////////////////////////////////
// Destructor.
EEDHController::~EEDHController()
{
  if (actarray_state != NULL)
  {
    delete[] actarray_state;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int
  EEDHController::MainSetup ()
{
  PLAYER_MSG0 (1, "> EEDHController starting up... [done]");

  // Subscribe to the actarray
  if (!(this->actarray_device = deviceTable->GetDevice (this->actarray_addr)))
  {
      PLAYER_ERROR ("unable to locate a suitable actarray device");
      return (-1);
  }
  if (this->actarray_device->Subscribe (this->InQueue) != 0)
  {
      PLAYER_ERROR ("unable to subscribe to the actarray device");
      return (-1);
  }

  return (0);
}

////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
void
  EEDHController::MainQuit ()
{
  pthread_cancel (a_th_cmd);
  pthread_cancel (a_th_home);

  this->actarray_device->Unsubscribe (this->InQueue);

  PLAYER_MSG0 (1, "> EEDHController driver shutting down... [done]");
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void
  EEDHController::Main ()
{
  command_thread = homing_thread = false;

  // The main loop; interact with the device here
  while (true)
  {
    Wait();

    // Process any pending messages
    this->ProcessMessages ();

    // Refresh data
    this->RefreshData ();
  }
}

////////////////////////////////////////////////////////////////////////////////
// Compute the joint commands using IK
int
  EEDHController::ComputeQ (unsigned int dof,
                            double x, double y, double z,
                            double roll, double pitch, double yaw,
                            ColumnVector &q)
{
  // Position/Orientation of the end effector
  Matrix result (4, 4);

  // Set the orientation
  ColumnVector orient (3);
  orient (3) = roll;
  orient (2) = pitch;
  orient (1) = yaw;

  result = rpy (orient);

  result (1, 4) = x;
  result (2, 4) = y;
  result (3, 4) = z;

  // Call RoboOp's inv_kin
  bool converged;

  q = robot_IK.inv_kin (result, 1, dof, converged);
  if (q.is_zero () && !converged)
    return (1);

  return (0);
}

////////////////////////////////////////////////////////////////////////////////
// Complete definition range atan2
float
  EEDHController::m_atan2 (float a, float b)
{
  float rad = 0;

  if (a == 0)
    rad = b < 0 ? -M_PI/2 : M_PI/2;
  else
  {
    rad = atan (b / a);
    if (a < 0)
      rad += M_PI;
  }
  if (rad < 0)
    rad += 2*M_PI;
  return rad;
}

////////////////////////////////////////////////////////////////////////////////
// Compute the End-Effector pose from the joint's position using FK
ColumnVector
  EEDHController::ComputeEEPose (ColumnVector q_cmd)
{
  robot_FK.set_q (q_cmd);
  Matrix position (4, 4);

  ColumnVector ret (6);

  // use robot_FK for forward kinematics
  position = robot_FK.kine ();
  // Roll/X, Pitch/Y, Yaw/Z
  ret(1) = m_atan2 (position (3, 3), position (3, 2));
  ret(2) = m_atan2 (sqrt ((position (3, 2) * position (3, 2)) +
                          (position (3, 3) * position (3, 3))),
                    -position (3, 1));
  ret(3) = m_atan2 (position(1, 1), position (2, 1));
  // X, Y, Z
  ret(4) = position (1, 4);
  ret(5) = position (2, 4);
  ret(6) = position (3, 4);

  return ret;
}

////////////////////////////////////////////////////////////////////////////////
// RefreshData function
void
  EEDHController::RefreshData ()
{
  unsigned int i;
  if (actarray_data_received)
  {
    assert (nr_joints == actarray_data.actuators_count);

    // Set the vector state of the actuators
    limb_data.state = PLAYER_LIMB_STATE_BRAKED;
    for (i = 0; i < nr_joints; i++)
    {
      actarray_state[i] = actarray_data.actuators[i].state;
      if (actarray_state[i] == PLAYER_ACTARRAY_ACTSTATE_MOVING)
        limb_data.state = PLAYER_LIMB_STATE_MOVING;
    }
    // Assure that the moving/homing threads are not started
    if ((homing_thread) || (command_thread))
      limb_data.state = PLAYER_LIMB_STATE_MOVING;

    ColumnVector q_cmd (nr_joints);

    // Get the joints positions
    for (i = 0; i < nr_joints; i++)
      q_cmd (i + 1) = actarray_data.actuators[i].position;

    ColumnVector pose = ComputeEEPose (q_cmd);

    // Fill the limb data structure with values and publish it
    limb_data.position.px = pose (4);
    limb_data.position.py = pose (5);
    limb_data.position.pz = pose (6);

    limb_data.approach.px = -1; limb_data.approach.py = -1; limb_data.approach.pz = -1;

    limb_data.orientation.px = pose (1);
    limb_data.orientation.py = pose (2);
    limb_data.orientation.pz = pose (3);

    Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_LIMB_DATA_STATE,
             &limb_data, sizeof (limb_data), NULL);

    actarray_data_received = false;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Dummy start for the AHomeLoop
void*
  EEDHController::DummyAHomeLoop (void *driver)
{
  ((EEDHController*)driver)->AHomeLoop ();
  return (0);
}

////////////////////////////////////////////////////////////////////////////////
// Main AHomeLoop, checks if joints are moving
void
  EEDHController::AHomeLoop ()
{
  timespec sleepTime = {0, 1000};
  player_actarray_home_cmd_t cmd;
  int i;

  // Home one joint at a time
  for (i = nr_joints-1; i > -1; i--)
  {
    cmd.joint = i;

    this->actarray_device->PutMsg (this->InQueue,
                                   PLAYER_MSGTYPE_CMD,
                                   PLAYER_ACTARRAY_CMD_HOME,
                                   (void*)&cmd,
                                   sizeof (cmd), NULL);
    if (debug)
      printf (">> Homing joint %d...", i);
    int p_state = actarray_state[i];
    int c_state = actarray_state[i];

    while (! (
      (p_state != c_state)
      &&
      (c_state != PLAYER_ACTARRAY_ACTSTATE_MOVING)
      ))
    {
      p_state = c_state;
      c_state = actarray_state[i];
      nanosleep (&sleepTime, NULL);
    }
    if (debug)
      printf ("[done]\n");
  }
  if (debug)
    printf (">> Homing complete.\n");
  homing_thread = false;
}

////////////////////////////////////////////////////////////////////////////////
// Send joint command values to the underlying actarray device
void
  EEDHController::CommandJoints (ColumnVector q_cmd)
{
  command_thread = true;
  CmdLoopStruc *s = new CmdLoopStruc;
  s->driver = this;
  s->q      = q_cmd;
  // Start the actarray homing thread
  pthread_create (&a_th_cmd, NULL, &DummyACmdLoop, (void*)s);
}

////////////////////////////////////////////////////////////////////////////////
// Dummy start for the ACmdLoop
void*
  EEDHController::DummyACmdLoop (void *structure)
{
  CmdLoopStruc *s = (CmdLoopStruc*)structure;
  ((EEDHController*)s->driver)->ACmdLoop (s->q);
  return (0);
}

////////////////////////////////////////////////////////////////////////////////
// Main ACmdLoop, checks if joints are moving
void
  EEDHController::ACmdLoop (ColumnVector q_cmd)
{
  timespec sleepTime = {0, 1000};
  player_actarray_position_cmd_t cmd;
  int i;

  // Write the commands on screen if debug enabled
  if (debug)
  {
    printf (">> Sending the following joint commands: ");
    for (i = 0; i < q_cmd.nrows (); i++)
      printf ("%f ", q_cmd (i + 1));
    printf ("\n");
  }

  // Position one joint at a time
  for (i = 0; i < q_cmd.nrows (); i++)
  {
    cmd.joint    = i;
    cmd.position = q_cmd (i + 1);

    if (debug)
      printf (">>> Sending command %f to joint %d... ", cmd.position, cmd.joint);

    // If the current joint is already there +/- some user preferred positioning error
    if (abs(cmd.position - actarray_data.actuators[i].position) < DTOR (error_pos))
      break;

    this->actarray_device->PutMsg (this->InQueue,
                                   PLAYER_MSGTYPE_CMD,
                                   PLAYER_ACTARRAY_CMD_POS,
                                   (void*)&cmd,
                                   sizeof (cmd), NULL);
    int p_state = actarray_state[i];
    int c_state = actarray_state[i];

    while (! (
      (p_state != c_state)
      &&
      (c_state != PLAYER_ACTARRAY_ACTSTATE_MOVING)
      ))
    {
      p_state = c_state;
      c_state = actarray_state[i];
      nanosleep (&sleepTime, NULL);
    }

    if (debug)
      printf ("[done]\n");
  }
  if (debug)
    printf (">> Commands sent.\n");
  command_thread = false;
}

////////////////////////////////////////////////////////////////////////////////
// ProcessMessage function
int
  EEDHController::ProcessMessage (QueuePointer &resp_queue,
                          player_msghdr * hdr,
                          void * data)
{
  // Check for capabilities requests first
  HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_CAPABILITIES_REQ);
  HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_REQ, PLAYER_LIMB_REQ_SPEED);
  HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_LIMB_CMD_SETPOSE);
  HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_LIMB_CMD_SETPOSITION);
  HANDLE_CAPABILITY_REQUEST (device_addr, resp_queue, hdr, data, PLAYER_MSGTYPE_CMD, PLAYER_LIMB_CMD_HOME);

  // First look whether we have incoming data from the actarray interface
  if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_DATA,
    PLAYER_ACTARRAY_DATA_STATE, actarray_addr))
  {
    actarray_data = *((player_actarray_data_t*)data);
    actarray_data_received = true;
    return (0);
  }

  // Set the desired position to the actarray driver
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD,
    PLAYER_LIMB_CMD_SETPOSE, device_addr))
  {
    player_limb_setpose_cmd_t & command =
      *reinterpret_cast<player_limb_setpose_cmd_t * > (data);

    int converged = ComputeQ (nr_joints, command.position.px,
                              command.position.py, command.position.pz,
                              command.orientation.px, command.orientation.py,
                              command.orientation.pz, q_cmd);
    if (converged != 0)
    {
      PLAYER_WARN6 ("Couldn't find solution for %f,%f,%f/%f,%f,%f", command.position.px,
                   command.position.py, command.position.pz, command.orientation.px,
                   command.orientation.py, command.orientation.pz);
      // Do we actually need to send back a NACK ?
      Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, hdr->subtype);
      return (-1);
    }
    CommandJoints (q_cmd);

    return (0);
  }

  // Set the desired position to the actarray driver
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD,
    PLAYER_LIMB_CMD_SETPOSITION, device_addr))
  {
    player_limb_setposition_cmd_t & command =
      *reinterpret_cast<player_limb_setposition_cmd_t * > (data);

    int converged = ComputeQ (nr_joints, command.position.px,
                              command.position.py, command.position.pz, 0, 0, 0, q_cmd);

    if (converged != 0)
    {
      // Do we actually need to send back a NACK ?
      PLAYER_WARN3 ("Couldn't find solution for %f,%f,%f", command.position.px,
                   command.position.py, command.position.pz);
      Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, hdr->subtype);
      return (-1);
    }
    CommandJoints (q_cmd);

    return (0);
  }

  // Home the limb (we do this by homing all the joints)
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_CMD,
    PLAYER_LIMB_CMD_HOME, device_addr))
  {
    homing_thread = true;
    // Start the actarray homing thread
    pthread_create (&a_th_home, NULL, &DummyAHomeLoop, this);
    return (0);
  }


  // POWER_REQ not implemented
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ,
	PLAYER_LIMB_REQ_POWER, device_addr))
  {
    Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, hdr->subtype);
    return (-1);
  }

  // BRAKES_REQ not implemented
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ,
	PLAYER_LIMB_REQ_BRAKES, device_addr))
  {
    Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, hdr->subtype);
    return (-1);
  }

  // GEOM_REQ not implemented
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ,
	PLAYER_LIMB_REQ_GEOM, device_addr))
  {
    Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_NACK, hdr->subtype);
    return (-1);
  }

  // SPEED_REQ - Set the speed on all joints equal to the EE
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ,
	PLAYER_LIMB_REQ_SPEED, device_addr))
  {
    player_limb_speed_req_t & cfg =
      *reinterpret_cast<player_limb_speed_req_t *> (data);

    int i;
    player_actarray_speed_config_t act_cfg;
    for (i = 0; i < q_cmd.nrows (); i++)
    {
      act_cfg.joint = i;
      act_cfg.speed = cfg.speed;

      Message *msg;
      if (!(msg = this->actarray_device->Request (this->InQueue,
                                                  PLAYER_MSGTYPE_REQ,
                                                  PLAYER_ACTARRAY_REQ_SPEED,
                                                  (void*)&act_cfg,
                                                  sizeof (act_cfg), NULL, false)))
      {
        PLAYER_WARN1 ("failed to send speed command to actuator %d", i);
      }
      else
        delete msg;
    }

    Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, hdr->subtype);
    return (0);
  }

  return (0);
}
//------------------------------------------------------------------------------

