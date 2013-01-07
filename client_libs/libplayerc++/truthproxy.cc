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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */
/********************************************************************
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
 *
 ********************************************************************/

/*
 * $Id$
 */

#include "playerc++.h"

// angles must be transmitted as integers on [0..359]
#define RAD_TO_POS_DEG(x) (((((int) (x * 180 / M_PI)) % 360) + 360) % 360)

void TruthProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  if(hdr.size != sizeof(player_truth_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of truth data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_truth_data_t),hdr.size);
  }

  // convert pos from NBO integer mm to double meters
  px = ((int32_t)ntohl(((player_truth_data_t*)buffer)->pos[0])) / 1e3;
  py = ((int32_t)ntohl(((player_truth_data_t*)buffer)->pos[1])) / 1e3;
  pz = ((int32_t)ntohl(((player_truth_data_t*)buffer)->pos[2])) / 1e3;

  // heading in NBO integer degrees to double radians
  rx = ((int32_t)ntohl(((player_truth_data_t*)buffer)->rot[0])) / 1e3;
  ry = ((int32_t)ntohl(((player_truth_data_t*)buffer)->rot[1])) / 1e3;
  rz = ((int32_t)ntohl(((player_truth_data_t*)buffer)->rot[2])) / 1e3;
}

// interface that all proxies SHOULD provide
void TruthProxy::Print()
{
  printf("#GROUND TRUTH POSE (%d:%d) - %c\n",
         m_device_id.code, m_device_id.index, access);
  puts("#(X,Y,Z,roll,pitch,yaw) (m,m,m,rad,rad,rad)");
  printf("%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\n", px,py,pz,rx,ry,rz);
}

// Get the object pose - sends a request and waits for a reply
int TruthProxy::GetPose( double *px, double *py, double *pz,
                         double *rx, double *ry, double *rz )
{
  player_truth_pose_t config;
  player_msghdr_t hdr;

//  config.subtype = PLAYER_TRUTH_GET_POSE;

  if(client->Request(m_device_id,PLAYER_TRUTH_GET_POSE,
                     (const char*)&config, sizeof(config),
                     &hdr, (char*)&config, sizeof(config)) < 0)
    return(-1);

  *px = ((int32_t)ntohl(config.pos[0])) / 1e3;
  *py = ((int32_t)ntohl(config.pos[1])) / 1e3;
  *pz = ((int32_t)ntohl(config.pos[2])) / 1e3;

  *rx = ((int32_t)ntohl(config.rot[0])) / 1e3;
  *ry = ((int32_t)ntohl(config.rot[1])) / 1e3;
  *rz = ((int32_t)ntohl(config.rot[2])) / 1e3;

  // update the internal pose record too.
  this->px = *px;
  this->py = *py;
  this->pz = *pz;
  this->rx = *rx;
  this->ry = *ry;
  this->rz = *rz;

  return 0;
}

// Set the object pose by sending a config request
int TruthProxy::SetPose(double px, double py, double pz,
                        double rx, double ry, double rz)
{
  int len;
  player_truth_pose_t config;

//  config.subtype = PLAYER_TRUTH_SET_POSE;
  config.pos[0] = htonl((int32_t)(px * 1000));
  config.pos[1] = htonl((int32_t)(py * 1000));
  config.pos[2] = htonl((int32_t)(pz * 1000));
  config.rot[0] = htonl((int32_t)(rx * 1000));
  config.rot[1] = htonl((int32_t)(ry * 1000));
  config.rot[2] = htonl((int32_t)(rz * 1000));

  len = client->Request(m_device_id,PLAYER_TRUTH_SET_POSE,
       (const char*)&config, sizeof(config));
  if (len < 0)
    return -1;

  // TODO: check for a NACK

  return 0;
}

// Set the object pose by sending a config request
int TruthProxy::SetPoseOnRoot(double px, double py, double pz,
                              double rx, double ry, double rz)
{
  int len;
  player_truth_pose_t config;

//  config.subtype = PLAYER_TRUTH_SET_POSE_ON_ROOT;
  config.pos[0] = htonl((int32_t)(px * 1000));
  config.pos[1] = htonl((int32_t)(py * 1000));
  config.pos[2] = htonl((int32_t)(pz * 1000));
  config.rot[0] = htonl((int32_t)(rx * 1000));
  config.rot[1] = htonl((int32_t)(ry * 1000));
  config.rot[2] = htonl((int32_t)(rz * 1000));

  len = client->Request( m_device_id,PLAYER_TRUTH_SET_POSE_ON_ROOT,
       (const char*)&config, sizeof(config));
  if (len < 0)
    return -1;

  // TODO: check for a NACK

  return 0;
}

int TruthProxy::GetFiducialID( int16_t* id )
{
  player_truth_fiducial_id_t config;
  player_msghdr_t hdr;

//  config.subtype = PLAYER_TRUTH_GET_FIDUCIAL_ID;

  if(client->Request(m_device_id, PLAYER_TRUTH_GET_FIDUCIAL_ID,
                     (const char*)&config, sizeof(config),
                     &hdr, (char*)&config, sizeof(config)) < 0)
    return(-1);

  if( id ) *id = (int16_t)ntohs(config.id);

  return 0;
}


int TruthProxy::SetFiducialID( int16_t id )
{
  int len;
  player_truth_fiducial_id_t config;

//  config.subtype = PLAYER_TRUTH_SET_FIDUCIAL_ID;

  config.id = htons(id);

  len = client->Request(m_device_id,PLAYER_TRUTH_SET_FIDUCIAL_ID,
      (const char*)&config, sizeof(config));
  if (len < 0)
    return -1;

  // TODO: check for a NACK
  return 0;
}
