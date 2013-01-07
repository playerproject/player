/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2007
 *     Nico Blodow and Radu Bogdan Rusu
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

/*
 Desc: Driver for the SICK LMS400 unit
 Author: Nico Blodow and Radu Bogdan Rusu
 Date: 7 Feb 2007
 CVS: $Id$
*/

#include "config.h"

#include <netdb.h>
#include <sys/types.h>
#include <vector>
#include <netinet/in.h>
#include <libplayerinterface/player.h>
#include <iostream>

#define BUF_SIZE 1024

////////////////////////////////////////////////////////////////////////////////
typedef struct
{
  unsigned char* string;
  int length;
} MeasurementQueueElement_t;

////////////////////////////////////////////////////////////////////////////////
typedef struct
{
  uint16_t Format;
  uint16_t DistanceScaling;
  int32_t  StartingAngle;
  uint16_t AngularStepWidth;
  uint16_t NumberMeasuredValues;
  uint16_t ScanningFrequency;
  uint16_t RemissionScaling;
  uint16_t RemissionStartValue;
  uint16_t RemissionEndValue;
} MeasurementHeader_t;


////////////////////////////////////////////////////////////////////////////////
class lms400_cola
{
  public:
    lms400_cola (const char* host, int port, int debug_mode);

    // Creates socket, connects
    int Connect ();
    int Disconnect ();

    // Configuration parameters
    int SetAngularResolution (const char* password, float ang_res, float angle_start, float angle_range);
    int SetScanningFrequency (const char* password, float freq, float angle_start, float angle_range);
    int SetResolutionAndFrequency (float freq, float ang_res, float angle_start, float angle_range);

    int StartMeasurement (bool intensity = true);
    player_laser_data ReadMeasurement  ();
    int StopMeasurement  ();

    int SetUserLevel  (int8_t userlevel, const char* password);
    int GetMACAddress (char** macadress);

    int SetIP         (char* ip);
    int SetGateway    (char* gw);
    int SetNetmask    (char* mask);
    int SetPort       (uint16_t port);

    int ResetDevice            ();
    int TerminateConfiguration ();

    int SendCommand   (const char* cmd);
    int ReadResult    ();
    // for "Variables", Commands that only reply with one Answer message
    int ReadAnswer    ();
    // for "Procedures", Commands that reply with a Confirmation message and an Answer message
    int ReadConfirmationAndAnswer ();

    int EnableRIS (int onoff);
    player_laser_config GetConfiguration ();
    int SetMeanFilterParameters (int num_scans);
    int SetRangeFilterParameters (float *ranges);
    int EnableFilters (int filter_mask);

    // turns a string holding an ip address into long
    unsigned char* ParseIP (char* ip);

  private:
    // assembles STX's, length field, message, checksum ready to be sent. Cool.
    int assemblecommand (unsigned char* command, int len);

    const char* hostname;
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
#if HAVE_GETADDRINFO
    struct addrinfo *addr_ptr;
#else
    struct hostent *server;
#endif

    // Internal Parameters:
    int verbose;
    int ExtendedRIS;
    int MeanFilterNumScans;
    float RangeFilterTopLimit;
    float RangeFilterBottomLimit;
    int FilterMask;
    player_laser_config Configuration;

    // for reading:
    unsigned char buffer[4096];
    unsigned int bufferlength;

    // for sending:
    unsigned char command[BUF_SIZE];
    int commandlength;
    std::vector<MeasurementQueueElement_t>* MeasurementQueue;
};
