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
 * client-side localization device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <math.h>

LocalizeProxy::~LocalizeProxy()
{ 
  if(map_cells)
    free(map_cells);
}

    
void LocalizeProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  player_localize_data_t* buf = (player_localize_data_t*)buffer;

  // Byte-swapping goodness
  hypoth_count = ntohl(buf->hypoth_count);
  bzero(hypoths,sizeof(hypoths));
  for(int i=0;i<hypoth_count;i++)
  {
    for(int j=0;j<3;j++)
      hypoths[i].mean[j] = (int)ntohl(buf->hypoths[i].mean[j]);
    for(int j=0;j<3;j++)
    {
      for(int k=0;k<3;k++)
        hypoths[i].cov[j][k] = (int)ntohll(buf->hypoths[i].cov[j][k]);
    }
    hypoths[i].weight = ntohl(buf->hypoths[i].alpha);
  
    // Unit conversions 
    hypoths[i].mean[0] /= 1000.0;
    hypoths[i].mean[1] /= 1000.0;
    hypoths[i].mean[2] /= 3600.0;
    hypoths[i].cov[0][0] /= (1000.0 * 1000.0);
    hypoths[i].cov[0][1] /= (1000.0 * 1000.0);
    hypoths[i].cov[1][0] /= (1000.0 * 1000.0);
    hypoths[i].cov[1][1] /= (1000.0 * 1000.0);
    hypoths[i].cov[2][2] /= (3600.0 * 3600.0);
    hypoths[i].weight /= 1e6;
  }
}

int 
LocalizeProxy::SetPose(double pose[3], double cov[3][3])
{
  player_localize_set_pose_t req;
  req.subtype = PLAYER_LOCALIZE_SET_POSE_REQ;

  req.mean[0] = htonl((int)(pose[0] * 1000.0));
  req.mean[1] = htonl((int)(pose[1] * 1000.0));
  req.mean[2] = htonl((int)(pose[2] * 3600));

  req.cov[0][0] = htonll((int64_t)(cov[0][0] * 1000.0 * 1000.0));
  req.cov[0][1] = htonll((int64_t)(cov[0][1] * 1000.0 * 1000.0));
  req.cov[0][2] = 0;

  req.cov[1][0] = htonll((int64_t)(cov[1][0] * 1000.0 * 1000.0));
  req.cov[1][1] = htonll((int64_t)(cov[1][1] * 1000.0 * 1000.0));
  req.cov[1][2] = 0;

  req.cov[2][0] = 0;
  req.cov[2][1] = 0;
  req.cov[2][2] = htonll((int64_t)(cov[2][2] * 3600 * 3600));

  return(client->Request(PLAYER_LOCALIZE_CODE,index,
                         (const char*)&req,sizeof(req)));
}

int
LocalizeProxy::GetNumParticles()
{
  player_localize_config_t req,rep;
  player_msghdr_t rephdr;

  req.subtype = PLAYER_LOCALIZE_GET_CONFIG_REQ;
  req.num_particles = 0;

  if(client->Request(PLAYER_LOCALIZE_CODE,index,(const char*)&req,
                     sizeof(req),&rephdr,(char*)&rep,sizeof(rep)) < 0)
    return(-1);

  return(ntohl(rep.num_particles));
}

int
LocalizeProxy::GetMap()
{
  int i, j;
  int ni, nj;
  int sx, sy;
  int si, sj;
  int len;
  size_t reqlen;
  player_localize_map_info_t inforeq,inforep;
  player_localize_map_data_t datareq,datarep;
  player_msghdr_t rephdr;

  inforeq.subtype = PLAYER_LOCALIZE_GET_MAP_INFO_REQ;
  if(client->Request(PLAYER_LOCALIZE_CODE,index,(const char*)&inforeq,
                     sizeof(inforeq.subtype),&rephdr,(char*)&inforep,
                     sizeof(inforep)) < 0)
    return(-1);

  map_size_x = ntohl(inforep.width);
  map_size_y = ntohl(inforep.height);
  map_scale = 1000.0 / ((double) (int32_t) ntohl(inforep.scale));

  if(map_cells)
    free(map_cells);
  assert(map_cells = 
         (int8_t*)malloc(map_size_x * map_size_y * sizeof(map_cells[0])));
  
  // Tile size
  sx = (int) sqrt(sizeof(datareq.data));
  sy = sx;
  assert(sx * sy < sizeof(datareq.data));

  // Get the map data in tiles
  for (j = 0; j < map_size_y; j += sy)
  {
    for (i = 0; i < map_size_x; i += sx)
    {
      si = MIN(sx, map_size_x - i);
      sj = MIN(sy, map_size_y - j);

      datareq.subtype = PLAYER_LOCALIZE_GET_MAP_DATA_REQ;
      datareq.col = htonl(i);
      datareq.row = htonl(j);
      datareq.width = htonl(si);
      datareq.height = htonl(sj); 
      reqlen = sizeof(datareq) - sizeof(datareq.data);
    
      if(client->Request(PLAYER_LOCALIZE_CODE,index,(const char*)&datareq,
                         reqlen,&rephdr,(char*)&datarep,sizeof(datarep)) < 0)
        return(-1);

      for (nj = 0; nj < sj; nj++)
        for (ni = 0; ni < si; ni++)
          map_cells[(i + ni) + (j + nj) * map_size_x] = 
                  datarep.data[ni + nj * si];
    }
  }
  
  return 0;
}

// interface that all proxies SHOULD provide
void LocalizeProxy::Print()
{
  printf("#Localize(%d:%d) - %c\n", PLAYER_LOCALIZE_CODE,
         index, access);
  printf("%d hypotheses\n", hypoth_count);
  for(int i=0;i<hypoth_count;i++)
    printf("%d (weight %f): [ %f %f %f ]\n", 
           i, hypoths[i].weight,
           hypoths[i].mean[0], hypoths[i].mean[1], hypoths[i].mean[2]);
}

