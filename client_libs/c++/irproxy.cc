/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
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
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Copyright (C) 2002
 *   John Sweeney, UMASS, Amherst, Laboratory for Perceptual Robotics
 *
 * $Id$
 * 
 * implementation of IRProxy.
 *
 */

#include <playerclient.h>
#include <math.h>
#include <netinet/in.h>

IRProxy::IRProxy(PlayerClient *pc, unsigned short index,
                 unsigned char access) :
  ClientProxy(pc, PLAYER_IR_CODE, index, access) 
{
  memset(&ir_pose, 0, sizeof(ir_pose));
  GetIRPose();

  // now set up default M & B values for the IRs
  for (int i =0;i < PLAYER_IR_MAX_SAMPLES; i++) {
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
IRProxy::SetIRState(unsigned char state)
{
  if (!client) {
    return -1;
  }

  player_ir_power_req_t req;

  req.subtype = PLAYER_IR_POWER_REQ;
  req.state = state;

  return client->Request(m_device_id,
			 (const char *)&req, sizeof(req));
}

/* this will get the poses of all the IR sensors on the robot
 * and write them to ir_pose
 *
 * returns: 0 if ok, -1 else
 */
int
IRProxy::GetIRPose()
{
  if (!client) {
    return -1;
  }

  player_msghdr_t hdr;
  player_ir_pose_req_t req;

  req.subtype = PLAYER_IR_POSE_REQ;
  
  if ((client->Request(m_device_id, (const char *)&req,
		       sizeof(req.subtype), &hdr, (char *)&req,
		       sizeof(req)) < 0) ||
      hdr.type != PLAYER_MSGTYPE_RESP_ACK) {
    return -1;
  }

   ir_pose = req.poses;

	ir_pose.pose_count = ntohs(ir_pose.pose_count);

  // now change the byte ordering
  for (int i =0; i < PLAYER_IR_MAX_SAMPLES; i++) {
    ir_pose.poses[i][0] = ntohs(ir_pose.poses[i][0]);
    ir_pose.poses[i][1] = ntohs(ir_pose.poses[i][1]);
    ir_pose.poses[i][2] = ntohs(ir_pose.poses[i][2]);

    //    printf("IRPROXY: IRPOSE%d: %d %d %d\n", i,
    //	   ir_pose.poses[i][0], ir_pose.poses[i][1], ir_pose.poses[i][2]);
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
IRProxy::SetRangeParams(int which, double m, double b)
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
IRProxy::SetStdDevParams(int which, double m, double b)
{
  sparams[which][IRPROXY_M_PARAM] = m;
  sparams[which][IRPROXY_B_PARAM] = b;
}

/* fill data
 *
 * returns:
 */
void
IRProxy::FillData(player_msghdr_t hdr, const char *buffer)
{

  unsigned short new_range;

  if (hdr.size != sizeof(player_ir_data_t)) {
    fprintf(stderr, "REBIRPROXY: expected %d bytes but only got %d\n",
	    sizeof(player_ir_data_t), hdr.size);
  }

  for (int i =0; i < PLAYER_IR_MAX_SAMPLES; i++) {
    new_range = ntohs(((player_ir_data_t *)buffer)->ranges[i]);
      voltages[i] = ntohs( ((player_ir_data_t *)buffer)->voltages[i] );
 
    // if range is 0, then this is from real IR data
    // so we do a regression.  otherwise, its been done
    // for us by stage

    if (new_range == 0) {
      // calc range in mm
      new_range = (unsigned short) rint(exp( (log( (double)voltages[i] ) - params[i][IRPROXY_B_PARAM] ) /
					     params[i][IRPROXY_M_PARAM]));
    }
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
IRProxy::CalcStdDev(int w, unsigned  short range)
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
IRProxy::Print()
{
  printf("#IR(%d:%d) - %c\n", m_device_id.code,
         m_device_id.index, access);
  for (int i = 0;i < ir_pose.pose_count; i++) {
    printf("IR%d:\tR=%d\tV=%d\tSTD=%g\n", i, ranges[i], voltages[i], 
	   stddev[i]);
  }
}


