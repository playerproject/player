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

/*
 * $Id$
 *
 * client-side beacon device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h>

#include "playerpacket.h"

void FiducialProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  player_fiducial_data_t *data = (player_fiducial_data_t*) buffer;
  
  if(hdr.size != sizeof(player_fiducial_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of fiducial data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_fiducial_data_t),hdr.size);
  }

  count = ntohs(data->count);
  bzero(beacons,sizeof(beacons));
  for(unsigned short i = 0; i < count && i < PLAYER_FIDUCIAL_MAX_SAMPLES; i++)
  {
    beacons[i].id = ntohs(data->fiducials[i].id);
    beacons[i].pose[0] = (short)ntohs(data->fiducials[i].pose[0]);
    beacons[i].pose[1] = (short)ntohs(data->fiducials[i].pose[1]);
    beacons[i].pose[2] = (short) ntohs(data->fiducials[i].pose[2]);
    beacons[i].upose[0] = (short)ntohs(data->fiducials[i].upose[0]);
    beacons[i].upose[1] = (short)ntohs(data->fiducials[i].upose[1]);
    beacons[i].upose[2] = (short) ntohs(data->fiducials[i].upose[2]);
  }
}

// interface that all proxies SHOULD provide
void FiducialProxy::Print()
{
  printf("#Fiducial(%d:%d) - %c\n",
         m_device_id.code, m_device_id.index, access);
  puts("#count");
  printf("%d\n", count);
  puts("#id\trange\tbear\torient\tr_err\tb_err\to_err");
  for(unsigned short i=0;i<count && i<PLAYER_FIDUCIAL_MAX_SAMPLES;i++)
    printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\n", 
	   beacons[i].id, 
	   beacons[i].pose[0], beacons[i].pose[1], beacons[i].pose[2],
	   beacons[i].upose[0], beacons[i].upose[1], beacons[i].upose[2]);
}


int FiducialProxy::PrintFOV()
{
  int res = this->GetFOV();  
  if( res < 0 ) return res;

  printf("#Fiducial(%d:%d) - %c\n",
         m_device_id.code, m_device_id.index, access);
  printf( "#FOV\tmin_range\tmax_range\tview_angle\n"
	  "\t%.2f\t\t%.2f\t\t%.2f\n", 
	  this->min_range, this->max_range, this->view_angle );
  return(res);
}

int FiducialProxy::PrintGeometry()
{
  printf("#Fiducial(%d:%d) - %c\n",
         m_device_id.code, m_device_id.index, access);

  int res = this->GetConfigure();  
  if( res < 0 ) return res;
  
  printf( "#geometry:\n  pose [%.2f %.2f %.2f]  size [%.2f %.2f]"
	  "   target size [%.2f %.2f]\n",
	  pose[0], pose[1], pose[2], size[0], size[1], 
	  fiducial_size[0], fiducial_size[1] );
  
  return res;
}



// Get the fiducial geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int FiducialProxy::GetConfigure()
{
  int len;
  player_fiducial_geom_t config;
  player_msghdr_t hdr;

  config.subtype = PLAYER_FIDUCIAL_GET_GEOM;

  len = client->Request(m_device_id,
			(const char*)&config, sizeof(config.subtype),
			&hdr, (char*)&config, sizeof(config) );

  if( len == -1 )
    {
      puts( "fiducial config geometry request failed" );
      return(-1);
    }
  
  this->pose[0] = ((int16_t) ntohs(config.pose[0])) / 1000.0;
  this->pose[1] = ((int16_t) ntohs(config.pose[1])) / 1000.0;
  this->pose[2] = ((int16_t) ntohs(config.pose[2])) * M_PI / 180;
  this->size[0] = ((int16_t) ntohs(config.size[0])) / 1000.0;
  this->size[1] = ((int16_t) ntohs(config.size[1])) / 1000.0;
  
  this->fiducial_size[0] = 
    ((int16_t) ntohs(config.fiducial_size[0])) / 1000.0;
  this->fiducial_size[1] = 
    ((int16_t) ntohs(config.fiducial_size[1])) / 1000.0;
  
  return 0; // OK
}

// Get the fiducial geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int FiducialProxy::GetFOV()
{
  int len;
  player_fiducial_fov_t fov;
  player_msghdr_t hdr;

  fov.subtype = PLAYER_FIDUCIAL_GET_FOV;
  
  len = client->Request(m_device_id,
			(const char*)&fov, sizeof(fov.subtype),
			&hdr, (char*)&fov, sizeof(fov) );
  
  if( len == -1 )
    {
      puts( "fiducial config FOV request failed" );
      return(-1);
    }

  
  FiducialFovUnpack( &fov, 
		     &this->min_range, &this->max_range, &this->view_angle );

  return 0; // OK
}



// Set the fiducial geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int FiducialProxy::SetFOV( double min_range, 
			   double max_range, 
			   double view_angle)
{
  int len;
  player_fiducial_fov_t fov;
  player_msghdr_t hdr;
  
  FiducialFovPack( &fov, 1, min_range, max_range, view_angle );
    
  len = client->Request(m_device_id,
			(const char*)&fov, sizeof(fov),
			&hdr, (char*)&fov, sizeof(fov) );
  
  if( len == -1 )
    {
      puts( "fiducial config FOV request failed" );
      return(-1);
    }

  FiducialFovUnpack( &fov, 
		     &this->min_range, &this->max_range, &this->view_angle );
  
  return 0; // OK
}
