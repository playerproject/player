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
 * client-side ptz device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h>
    
// send a camera command
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int PtzProxy::SetCam(short pan, short tilt, short zoom)
{
  if(!client)
    return(-1);

  player_ptz_cmd_t cmd;

  memset(&cmd,0,sizeof(cmd));
  cmd.pan = htons(pan);
  cmd.tilt = htons(tilt);
  cmd.zoom = htons(zoom);

  return(client->Write(m_device_id,
                       (const char*)&cmd,sizeof(cmd)));
}

int PtzProxy::SetSpeed(short panspeed, short tiltspeed)
{
  if(!client)
    return(-1);

  player_ptz_cmd_t cmd;

  memset(&cmd,0,sizeof(cmd));
  cmd.panspeed = htons(panspeed);
  cmd.tiltspeed = htons(tiltspeed);

  return(client->Write(m_device_id,
                       (const char*)&cmd,sizeof(cmd)));
}

int
PtzProxy::SendConfig(uint8_t *bytes, size_t len, uint8_t *reply, 
		     size_t reply_len)
{
  player_ptz_generic_config_t cfg;
  player_msghdr_t hdr;

  if (len > PLAYER_PTZ_MAX_CONFIG_LEN) {
    fprintf(stderr, "config too long to send!\n");
    return -1;
  }

  cfg.subtype = PLAYER_PTZ_GENERIC_CONFIG_REQ;
  memcpy(&cfg.config, bytes, len);
  cfg.length = htons((short)len);
  
  if (client->Request(m_device_id, (const char *)&cfg,
		      sizeof(cfg), &hdr,
		      (char *)&cfg, sizeof(cfg)) < 0) {
    fprintf(stderr, "PTZPROXY: error on SendConfig request\n");
    return -1;
  }
  
  memcpy(reply, &cfg.config, reply_len);

  return 0;
}

int
PtzProxy::SelectControlMode(uint8_t mode)
{
  player_ptz_controlmode_config cfg;

  cfg.subtype = PLAYER_PTZ_CONTROL_MODE_REQ;
  cfg.mode = mode;

  return(client->Request(m_device_id,(const char*)&cfg,
                         sizeof(cfg)));
}
  

void PtzProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  if(hdr.size != sizeof(player_ptz_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of ptz data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_ptz_data_t),hdr.size);
  }

  pan = (short)ntohs(((player_ptz_data_t*)buffer)->pan);
  tilt = (short)ntohs(((player_ptz_data_t*)buffer)->tilt);
  zoom = (short)ntohs(((player_ptz_data_t*)buffer)->zoom);
  panspeed = (short)ntohs(((player_ptz_data_t*)buffer)->panspeed);
  tiltspeed = (short)ntohs(((player_ptz_data_t*)buffer)->tiltspeed);
}

// interface that all proxies SHOULD provide
void PtzProxy::Print()
{
  printf("#Ptz(%d:%d) - %c\n", m_device_id.code,
         m_device_id.index, access);
  puts("#pan\ttilt\tzoom\tpanspeed\ttiltspeed");
  printf("%d\t%d\t%u\t%d\t%d\n", pan,tilt,zoom,panspeed,tiltspeed);
}

