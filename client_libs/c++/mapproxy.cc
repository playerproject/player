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
 * client-side map device 
 */

#include <math.h>

#include <playerclient.h>

MapProxy::MapProxy( PlayerClient *pc, unsigned short index,
    unsigned char access)
  : ClientProxy (pc, PLAYER_MAP_CODE, index, access), cells(NULL)
{
  return;
}

MapProxy::~MapProxy()
{
  return;
}

// Get the map, which is stored in the proxy 
int MapProxy::GetMap()
{
  int reqlen;
  int i,j;
  int oi,oj;
  int sx,sy;
  int si,sj;
  char *cell;

  player_map_info_t infoReq;
  player_msghdr_t hdr;
  player_map_data_t dataReq;

  memset( &infoReq, 0, sizeof(infoReq));
  infoReq.subtype = PLAYER_MAP_GET_INFO_REQ;

  if (client->Request( m_device_id, (const char*)&infoReq,
        sizeof(infoReq.subtype), &hdr, (char*)&infoReq, sizeof(infoReq)) <0)
  {
    PLAYER_ERROR("failed to get map info");
    return -1;
  }

  this->resolution = 1/(ntohl(infoReq.scale) / 1e3);
  this->width = ntohl(infoReq.width);
  this->height = ntohl(infoReq.height);


  // allocate space for cells
  if (this->cells!=NULL)
    delete [] this->cells;

  assert( this->cells = new char[this->width * this->height] );


  // Get the map, in tiles
  dataReq.subtype = PLAYER_MAP_GET_DATA_REQ;

  // Tile size
  sy = sx = (int)sqrt(sizeof(dataReq.data));
  assert(sx * sy < (int)sizeof(dataReq.data));
  oi = oj = 0;

  while ((oi < this->width) && (oj < this->height))
  {
    si = MIN( sx, this->width - oi );
    sj = MIN( sy, this->height - oj );

    dataReq.col = htonl(oi);
    dataReq.row = htonl(oj);
    dataReq.width = htonl(si);
    dataReq.height = htonl(sj);

    reqlen = sizeof(dataReq) - sizeof(dataReq.data);

    if (client->Request(m_device_id, (const char*)&dataReq,
            reqlen, &hdr, (char *)&dataReq, sizeof(dataReq)) != 0)
    {
      PLAYER_ERROR("failed to get map data");
      return -1;
    } else if (hdr.size != (reqlen + si * sj)) {
      PLAYER_ERROR2("go less map data than expected (%d != %d)",
          hdr.size, reqlen + si*sj);
      return -1;
    }

    // Copy the map data
    for (j=0; j<sj; j++)
    {
      for (i=0; i<si; i++)
      {
        cell = this->cells + this->GetCellIndex(oi+i, oj+j);
        *cell = dataReq.data[j*si +i];
      }
    }

    oi += si;
    if (oi >= this->width)
    {
      oi = 0;
      oj += sj;
    }
  }

  return 0;
}


/** Return the index of the (x,y) item in the cell array */
int MapProxy::GetCellIndex( int x, int y)
{
  return y*this->width + x;
}

/** Get the (x,y) cell; returns 0 on success, -1 on failure (i.e., indexes
 * out of bounds) */
int MapProxy::GetCell( char* cell, int x, int y )
{
  int index = this->GetCellIndex(x,y);

  if ( index < this->height*this->width)
  {
    *cell = this->cells[index];
    return(0);
  }
  else
    return(-1);
}

// interface that all proxies must provide
void MapProxy::FillData(player_msghdr_t hde, const char *buffer)
{
  // This shouldn't be called
  return;
}
