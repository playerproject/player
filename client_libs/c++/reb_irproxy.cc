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

/* Copyright (C) 2002
 *   John Sweeney, UMASS, Amherst, Laboratory for Perceptual Robotics
 *
 * $Id$
 * 
 * implementation of REBIRProxy.
 *
 */

#include <playerclient.h>
#include <math.h>
#include <netinet/in.h>

REBIRProxy::REBIRProxy(PlayerClient *pc, unsigned short index,
		       unsigned char access) :
  ClientProxy(pc, PLAYER_REB_IR_CODE, index, access) 
{
  memset(&ir_pose, 0, sizeof(ir_pose));

  // now set up default M & B values for the IRs
  for (int i =0;i < PLAYER_REB_NUM_IR_SENSORS; i++) {
    params[i][IRPROXY_M_PARAM] = IRPROXY_DEFAULT_DIST_M_VALUE;
    params[i][IRPROXY_B_PARAM] = IRPROXY_DEFAULT_DIST_B_VALUE;

    sparams[i][IRPROXY_M_PARAM] = IRPROXY_DEFAULT_STD_M_VALUE;
    sparams[i][IRPROXY_B_PARAM] = IRPROXY_DEFAULT_STD_B_VALUE;

    voltages[i] = 0;
    ranges[i] = 0;
    stddev[i] = 0.0;
  }
}

/* enable/disable the IRs
 * 1 for enable
 * 0 for disable
 *
 * returns: 0 if ok, -1 else
 */
int
REBIRProxy::SetIRState(unsigned char state)
{
  if (!client) {
    return -1;
  }

  player_reb_ir_power_req_t req;

  req.subtype = PLAYER_REB_IR_POWER_REQ;
  req.state = state;

  return client->Request(PLAYER_REB_IR_CODE, index, 
			 (const char *)&req, sizeof(req));
}

/* this will get the poses of all the IR sensors on the robot
 * and write them to ir_pose
 *
 * returns: 0 if ok, -1 else
 */
int
REBIRProxy::GetIRPose()
{
  if (!client) {
    return -1;
  }

  player_msghdr_t hdr;
  player_reb_ir_pose_req_t req;

  req.subtype = PLAYER_REB_IR_POSE_REQ;
  
  if ((client->Request(PLAYER_REB_IR_CODE, index, (const char *)&req,
		       sizeof(req), &hdr, (char *)&ir_pose,
		       sizeof(ir_pose)) < 0) ||
      hdr.type != PLAYER_MSGTYPE_RESP_ACK) {
    return -1;
  }

  // now change the byte ordering
  for (int i =0; i < PLAYER_REB_NUM_IR_SENSORS; i++) {
    ir_pose.poses[i][0] = ntohs(ir_pose.poses[i][0]);
    ir_pose.poses[i][1] = ntohs(ir_pose.poses[i][1]);
    ir_pose.poses[i][2] = ntohs(ir_pose.poses[i][2]);
  }

  return 0;
}

/* give the device the parameters for doing range estimation
 * we use an exponential regression by doing linear regression
 * in log space.  so we just need an m and b value for each sensor
 *
 * M is the slope of the regression line, B is the intercept
 *
 * returns: 
 */
void
REBIRProxy::SetRangeParams(int which, double m, double b)
{
  params[which][IRPROXY_M_PARAM] = m;
  params[which][IRPROXY_B_PARAM] = b;
}


/* sets the parameters (slope [m] and intercept [b]) for doing
 * a linear regression to estimate the standard deviation in the 
 * distance estimate.  this is for a particular sensor which
 *
 * returns: 
 */
void
REBIRProxy::SetStdDevParams(int which, double m, double b)
{
  sparams[which][IRPROXY_M_PARAM] = m;
  sparams[which][IRPROXY_B_PARAM] = b;
}

/* fill data
 *
 * returns:
 */
void
REBIRProxy::FillData(player_msghdr_t hdr, const char *buffer)
{
  unsigned short new_range;

  if (hdr.size != sizeof(player_reb_ir_data_t)) {
    fprintf(stderr, "REBIRPROXY: expected %d bytes but only got %d\n",
	    sizeof(player_reb_ir_data_t), hdr.size);
  }

  for (int i =0; i < PLAYER_REB_NUM_IR_SENSORS; i++) {
    voltages[i] = ntohs( ((player_reb_ir_data_t *)buffer)->voltages[i] );
    // calc range in mm
    new_range = (unsigned short) rint(exp( (log( (double)voltages[i] ) - params[i][IRPROXY_B_PARAM] ) /
		     params[i][IRPROXY_M_PARAM]));

    // if the range is obviously too far, then dont do the
    // std dev calc.  This threshold should probably be much lower.
    if (new_range <= 8000) {
      ranges[i] = new_range;
      stddev[i] = CalcStdDev(i, ranges[i]);
    } else {
      stddev[i] = 1.0;
    }
  }
}


/* calculate the standard deviation given a distance measurement
 * for sensor w
 *
 * returns: the estimated standard deviation
 */
double
REBIRProxy::CalcStdDev(int w, unsigned  short range)
{
  double ret = exp( log( (double)range ) * sparams[w][IRPROXY_M_PARAM] +
		     sparams[w][IRPROXY_B_PARAM] );

  return ret;
}

/* print out the good stuff
 *
 * returns:
 */
void
REBIRProxy::Print()
{
  printf("#REB IR(%d:%d) - %c\n", device, index, access);
  for (int i = 0;i < PLAYER_REB_NUM_IR_SENSORS; i++) {
    printf("IR%d:\tR=%d\tV=%d\tSTD=%g\n", i, ranges[i], voltages[i], 
	   stddev[i]);
  }
}


