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

/*
 * $Id$
 */

//#define DEBUG

#include <truthproxy.h>
#include <netinet/in.h>
#include <string.h>
#include <math.h>

#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif


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
  x = (double)ntohl(((player_truth_data_t*)buffer)->px)  / 1000.0;
  y = (double)ntohl(((player_truth_data_t*)buffer)->py) / 1000.0;
  // heading in NBO integer degrees to double radians
  a = DTOR((double)(int32_t)ntohl(((player_truth_data_t*)buffer)->pa));
}

// interface that all proxies SHOULD provide
void TruthProxy::Print()
{
  printf("#GROUND TRUTH POSE (%d:%d) - %c\n", device, index, access);
  puts("#(Xm,Ym,THradians)");
  printf("%.3f\t%.3f\t%.3f\n", x,y,a);
}

// Get the object pose - sends a request and waits for a reply
int TruthProxy::GetPose( double *px, double *py, double *pa )
{
  player_truth_pose_t config;
  player_msghdr_t hdr;
  
  config.subtype = PLAYER_TRUTH_GET_POSE;
  
  if(client->Request(PLAYER_TRUTH_CODE,index,
                     (const char*)&config, sizeof(config.subtype),
                     &hdr, (char*)&config, sizeof(config)) < 0)
    return(-1);
  
  *px = (double)(int32_t)ntohl(config.px) / 1000.0;
  *py = (double)(int32_t)ntohl(config.py) / 1000.0;
  *pa = DTOR((double)(int32_t)ntohl(config.pa));
  
  // update the internal pose record too.
  x = *px;
  y = *py;
  a = *pa;

  return 0;
}


// Set the object pose by sending a config request
int TruthProxy::SetPose( double px, double py, double pa )
{
  int len;
  player_truth_pose_t config;
  
  config.subtype = PLAYER_TRUTH_SET_POSE;
  config.px = htonl((int) (px * 1000));
  config.py = htonl((int) (py * 1000));
  config.pa = htonl((int) (pa * 180 / M_PI)); 
  
  len = client->Request( PLAYER_TRUTH_CODE, index, 
			 (const char*)&config, sizeof(config));
  if (len < 0)
    return -1;
  
  // TODO: check for a NACK
  
  return 0;
}

