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
 * client-side HUD device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h>
#include <math.h>
#include <limits.h>

// Constructor
HUDProxy::HUDProxy( PlayerClient *pc, unsigned short index,
    unsigned char access)
  : ClientProxy(pc, PLAYER_SIMULATION_CODE, index, access)
{
}

// Destructor
HUDProxy::~HUDProxy()
{
}

// Set the drawing color
void HUDProxy::SetColor( float *color )
{
  this->color[0] = color[0];
  this->color[1] = color[1];
  this->color[2] = color[2];
}

// Set the drawing style
void HUDProxy::SetStyle( int filled )
{
  this->filled = filled;
}

// Remove the element with id
int HUDProxy::Remove(int id)
{
  if (!client)
    return (-1);

  player_hud_config_t config;

  // Dummy value
  config.subtype = PLAYER_HUD_BOX;

  config.id = (int32_t)htonl(id);
  config.remove = 0xff;

  return(client->Request(m_device_id, (const char*)&config, sizeof (config)));
}

// Draw a box on the screen
int HUDProxy::DrawBox(int id, int ax, int ay, int bx, int by)
{
  if (!client)
    return (-1);

  player_hud_config_t config;

  config.subtype = PLAYER_HUD_BOX;

  config.id = (int32_t)htonl(id);

  config.pt1[0] = (int16_t)htons(ax);
  config.pt1[1] = (int16_t)htons(ay);

  config.pt2[0] = (int16_t)htons(bx);
  config.pt2[1] = (int16_t)htons(by);

  config.color[0] = (int16_t)htons((int)(this->color[0]*100));
  config.color[1] = (int16_t)htons((int)(this->color[1]*100));
  config.color[2] = (int16_t)htons((int)(this->color[2]*100));

  config.filled = this->filled;

  config.remove = 0x00;

  return(client->Request(m_device_id, (const char*)&config, sizeof (config)));
}

// Draw a line on , end point define by (ax,ay) and (bx,by)
int HUDProxy::DrawLine(int id, int ax, int ay, int bx, int by)
{
  if (!client)
    return (-1);

  player_hud_config_t config;

  config.subtype = PLAYER_HUD_LINE;

  config.id = (int32_t)htonl(id);

  config.pt1[0] = (int16_t)htons(ax);
  config.pt1[1] = (int16_t)htons(ay);

  config.pt2[0] = (int16_t)htons(bx);
  config.pt2[1] = (int16_t)htons(by);

  config.color[0] = (int16_t)htons((int)(this->color[0]*100));
  config.color[1] = (int16_t)htons((int)(this->color[1]*100));
  config.color[2] = (int16_t)htons((int)(this->color[2]*100));

  config.remove = 0x00;

  return(client->Request(m_device_id, (const char*)&config, sizeof (config)));
}

// Draw text at (x,y) 
int HUDProxy::DrawText(int id, const char *text, int x, int y)
{
  if (!client)
    return (-1);

  player_hud_config_t config;

  config.subtype = PLAYER_HUD_TEXT;

  config.id = (int32_t)htonl(id);

  config.pt1[0] = (int16_t)htons(x);
  config.pt1[1] = (int16_t)htons(y);

  config.color[0] = (int16_t)htons((int)(this->color[0]*100));
  config.color[1] = (int16_t)htons((int)(this->color[1]*100));
  config.color[2] = (int16_t)htons((int)(this->color[2]*100));

  if (strlen(text)+1 > PLAYER_MAX_DEVICE_STRING_LEN)
  {
    strncpy(config.text,text,PLAYER_MAX_DEVICE_STRING_LEN);
    config.text[PLAYER_MAX_DEVICE_STRING_LEN-1] = '\0';
  } else {
    strcpy(config.text,text);
  }
  
  config.remove = 0x00;

  return(client->Request(m_device_id, (const char*)&config, sizeof (config)));

}

int HUDProxy::DrawCircle( int id, int cx, int cy, int radius)
{
  if (!client)
    return (-1);

  player_hud_config_t config;

  config.subtype = PLAYER_HUD_CIRCLE;

  config.id = (int32_t)htonl(id);

  config.pt1[0] = (int16_t)htons(cx);
  config.pt1[1] = (int16_t)htons(cy);

  config.value1 = (int16_t)htons(radius);

  config.color[0] = (int16_t)htons((int)(this->color[0]*100));
  config.color[1] = (int16_t)htons((int)(this->color[1]*100));
  config.color[2] = (int16_t)htons((int)(this->color[2]*100));

  config.remove = 0x00;

  config.filled = this->filled;

  return(client->Request(m_device_id, (const char*)&config, sizeof (config)));
}

void HUDProxy::FillData( player_msghdr_t hdr, const char *buffer)
{
  // HUD doesn't return data
  return;
}

