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
 
/** @ingroup utils */
/** @{ */
/** @defgroup util_playerprop playerprop
 * @brief Get and set driver properties from the command line

@par Synopsis

playerprop is a console-based client that allows the user to
get and set different driver properties.  playerprop supports
all four property types (double, bool, int, and string)


@par Usage

playerprop is installed alongside player in $prefix/bin, so if player is
in your PATH, then playerprop should also be.  Command-line usage is:
@verbatim
$ playerprop -d <device> [-i <index> -h <host> -p <port>] <command> <args>
@endverbatim
Where command can be:
- getbool <prop name>         Get a boolean property
- getint <prop name>          Get an interger property
- getdbl <prop name>          Get a double property
- getstr <prop name>          Get a string property
- setbool <prop name> <value> Set a boolean property
- setint <prop name> <value>  Set an interger property
- setdbl <prop name> <value>  Set a double property
- setstr <prop name> <value>  Set a string property

The device argument is the interface name to query
The index argument is the interface index to query
The host argument is the IP or hostname of the computer running the Player server
The port argument is the TCP port over which to connect to Player

@author Geoff Biggs, Toby Collett, Brian Gerkey
*/

/** @} */

#if !defined (WIN32)
  #include <unistd.h>
#endif
#include <cstdlib>
#include <iostream>
#include <string>
#include <cstring>

#include <config.h>
#if !HAVE_GETOPT
  #include <replace/replace.h>
#endif

#include <libplayerc++/playerc++.h>

using namespace std;
using namespace PlayerCc;

string host = "localhost";
int port = 6665;
string device = "position2d";
unsigned int devIndex = 0;

void PrintUsage (void)
{
	cout	<< "Usage: playerprop -d <device> [-i <index> -h <host> -p <port>] <command> <args>" << endl << endl
			<< "Commands:" << endl
			<< "getbool <prop name>         Get a boolean property" << endl
			<< "getint <prop name>          Get an interger property" << endl
			<< "getdbl <prop name>          Get a double property" << endl
			<< "getstr <prop name>          Get a string property" << endl
			<< "setbool <prop name> <value> Set a boolean property" << endl
			<< "setint <prop name> <value>  Set an interger property" << endl
			<< "setdbl <prop name> <value>  Set a double property" << endl
			<< "setstr <prop name> <value>  Set a string property" << endl;
}

int GetOptions (int argc, char *argv[])
{
	char c;
	const char *opts = "d:h:i:p:";

	if (argc < 3)
	{
		PrintUsage ();
		return -1;
	}

	while ((c = getopt (argc, argv, opts)) != -1)
	{
		switch (c)
		{
		case 'd':
			device = optarg;
			break;
		case 'i':
			devIndex = atoi (optarg);
			break;
		case 'h':
			host = optarg;
			break;
		case 'p':
			port = atoi (optarg);
			break;
		default:
			PrintUsage ();
			return -1;
		}
	}
	return optind;
}


int main (int argc, char *argv[])
{
	ClientProxy* deviceProxy;
	bool boolValue;
	int32_t argsIndex = 0, intValue;
	double dblValue;
	char *strValue;

	if ((argsIndex = GetOptions (argc, argv)) < 0)
		exit (1);

	PlayerClient client (host, port, PLAYERC_TRANSPORT_TCP);

	int code = client.LookupCode(device);
	switch (code)
	{
	case PLAYER_ACTARRAY_CODE:
		deviceProxy = (ClientProxy*) new ActArrayProxy (&client, devIndex);
		break;
	case PLAYER_AUDIO_CODE:
		deviceProxy = (ClientProxy*) new AudioProxy (&client, devIndex);
		break;
	case PLAYER_AIO_CODE:
		deviceProxy = (ClientProxy*) new AioProxy (&client, devIndex);
		break;
	case PLAYER_BLOBFINDER_CODE:
		deviceProxy = (ClientProxy*) new BlobfinderProxy (&client, devIndex);
		break;
	case PLAYER_BUMPER_CODE:
		deviceProxy = (ClientProxy*) new BumperProxy (&client, devIndex);
		break;
	case PLAYER_CAMERA_CODE:
		deviceProxy = (ClientProxy*) new CameraProxy (&client, devIndex);
		break;
	case PLAYER_DIO_CODE:
		deviceProxy = (ClientProxy*) new DioProxy (&client, devIndex);
		break;
	case PLAYER_FIDUCIAL_CODE:
		deviceProxy = (ClientProxy*) new FiducialProxy (&client, devIndex);
		break;
	case PLAYER_GRAPHICS2D_CODE:
		deviceProxy = (ClientProxy*) new Graphics2dProxy (&client, devIndex);
		break;
	case PLAYER_GRAPHICS3D_CODE:
		deviceProxy = (ClientProxy*) new Graphics3dProxy (&client, devIndex);
		break;
	case PLAYER_GRIPPER_CODE:
		deviceProxy = (ClientProxy*) new GripperProxy (&client, devIndex);
		break;
	case PLAYER_IMU_CODE:
		deviceProxy = (ClientProxy*) new ImuProxy (&client, devIndex);
		break;
	case PLAYER_IR_CODE:
		deviceProxy = (ClientProxy*) new IrProxy (&client, devIndex);
		break;
	case PLAYER_LASER_CODE:
		deviceProxy = (ClientProxy*) new LaserProxy (&client, devIndex);
		break;
	case PLAYER_LIMB_CODE:
		deviceProxy = (ClientProxy*) new LimbProxy (&client, devIndex);
		break;
	case PLAYER_LOCALIZE_CODE:
		deviceProxy = (ClientProxy*) new LocalizeProxy (&client, devIndex);
		break;
	case PLAYER_LOG_CODE:
		deviceProxy = (ClientProxy*) new LogProxy (&client, devIndex);
		break;
	case PLAYER_MAP_CODE:
		deviceProxy = (ClientProxy*) new MapProxy (&client, devIndex);
		break;
	case PLAYER_OPAQUE_CODE:
		deviceProxy = (OpaqueProxy*) new OpaqueProxy (&client, devIndex);
		break;
	case PLAYER_PLANNER_CODE:
		deviceProxy = (ClientProxy*) new PlannerProxy (&client, devIndex);
		break;
	case PLAYER_POSITION1D_CODE:
		deviceProxy = (ClientProxy*) new Position1dProxy (&client, devIndex);
		break;
	case PLAYER_POSITION2D_CODE:
		deviceProxy = (ClientProxy*) new Position2dProxy (&client, devIndex);
		break;
	case PLAYER_POSITION3D_CODE:
		deviceProxy = (ClientProxy*) new Position3dProxy (&client, devIndex);
		break;
	case PLAYER_POWER_CODE:
		deviceProxy = (ClientProxy*) new PowerProxy (&client, devIndex);
		break;
	case PLAYER_PTZ_CODE:
		deviceProxy = (ClientProxy*) new PtzProxy (&client, devIndex);
		break;
	case PLAYER_SIMULATION_CODE:
		deviceProxy = (ClientProxy*) new SimulationProxy (&client, devIndex);
		break;
	case PLAYER_SONAR_CODE:
		deviceProxy = (ClientProxy*) new SonarProxy (&client, devIndex);
		break;
	case PLAYER_SPEECH_CODE:
		deviceProxy = (ClientProxy*) new SpeechProxy (&client, devIndex);
		break;
	case PLAYER_VECTORMAP_CODE:
		deviceProxy = (ClientProxy*) new VectorMapProxy (&client, devIndex);
		break;
	default:
		cout << "Unknown interface " << device << endl;
		exit (-1);
	}

	if (strncmp (argv[argsIndex], "getbool", 7) == 0)
	{
		if (deviceProxy->GetBoolProp (argv[argsIndex + 1], &boolValue) >= 0)
			cout << "Property " << argv[argsIndex + 1] << " = " << boolValue << endl;
	}
	else if (strncmp (argv[argsIndex], "getint", 6) == 0)
	{
		if (deviceProxy->GetIntProp (argv[argsIndex + 1], &intValue) >= 0)
			cout << "Property " << argv[argsIndex + 1] << " = " << intValue << endl;
	}
	else if (strncmp (argv[argsIndex], "getdbl", 6) == 0)
	{
		if (deviceProxy->GetDblProp (argv[argsIndex + 1], &dblValue) >= 0)
			cout << "Property " << argv[argsIndex + 1] << " = " << dblValue << endl;
	}
	else if (strncmp (argv[argsIndex], "getstr", 6) == 0)
	{
		if (deviceProxy->GetStrProp (argv[argsIndex + 1], &strValue) >= 0)
		{
			cout << "Property " << argv[argsIndex + 1] << " = " << strValue << endl;
			free (strValue);
		}
	}
	else if (strncmp (argv[argsIndex], "setbool", 7) == 0)
	{
		if (strncmp (argv[argsIndex + 2], "true", 4) == 0)
			deviceProxy->SetBoolProp (argv[argsIndex + 1], true);
		else
			deviceProxy->SetBoolProp (argv[argsIndex + 1], false);
	}
	else if (strncmp (argv[argsIndex], "setint", 6) == 0)
	{
		deviceProxy->SetIntProp (argv[argsIndex + 1], atoi (argv[argsIndex + 2]));
	}
	else if (strncmp (argv[argsIndex], "setdbl", 6) == 0)
	{
		deviceProxy->SetDblProp (argv[argsIndex + 1], atof (argv[argsIndex + 2]));
	}
	else if (strncmp (argv[argsIndex], "setstr", 6) == 0)
	{
		deviceProxy->SetStrProp (argv[argsIndex + 1], argv[argsIndex + 2]);
	}

	return 0;
}
