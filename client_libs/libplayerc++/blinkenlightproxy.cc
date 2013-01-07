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
 *
 * client-side blobfinder device
 */

#include "playerc++.h"

void BlinkenlightProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  player_blinkenlight_data_t *data = (player_blinkenlight_data_t*)buffer;

  if(hdr.size != sizeof(player_blinkenlight_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of blinkenlight data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_blinkenlight_data_t),hdr.size);
  }

  this->enable = data->enable;
  this->period_ms = (uint16_t)ntohs(data->period_ms);
}

// interface that all proxies SHOULD provide
void BlinkenlightProxy::Print()
{
  printf("#Blinkenlight(%d:%d) - %c\n",
         m_device_id.code, m_device_id.index, access);
  printf( " enable: %s  period %d ms.\n",
    this->enable ? "true" : "false",
    this->period_ms );
}

// Set the state of the indicator light. A period of zero means the
// light will be unblinkingly on or off.
int BlinkenlightProxy::SetLight( bool enable, int period_ms )
{
  player_blinkenlight_cmd_t cmd;
  memset( &cmd, 0, sizeof(cmd) );

  cmd.enable = enable;
  cmd.period_ms = htons((uint16_t)period_ms);

  return(client->Write(m_device_id,
                       (const char*)&cmd,sizeof(cmd)));
}
