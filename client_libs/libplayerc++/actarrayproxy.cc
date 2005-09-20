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

#if HAVE_CONFIG_H
  #include "config.h"
#endif

#include <cassert>
#include <sstream>
#include <iomanip>

#include "playercc.h"
using namespace PlayerCc;

// interface that all proxies SHOULD provide
std::ostream& operator << (std::ostream& os, const PlayerCc::ActArrayProxy& a)
{
  //printf ("#ActArray(%d:%d) - %c\n", m_device_id.code, m_device_id.index, access);
  os << a.numActuators << " actuators:" << std::endl;
  os << "Act |\tType\tMin\tCentre\tMax\tHome\tCfgSpd\tPos\tSpeed\tState\tBrakes" << std::endl;
  for (int ii = 0; ii < numActuators; ii++)
  {
    os <<  ii << (a.actuatorGeom[ii].type ? "Linear" : "Rotary") <<
            a.actuatorGeom[ii].min << a.actuatorGeom[ii].centre << a.actuatorGeom[ii].max << a.actuatorGeom[ii].home << a.actuatorGeom[ii].configSpeed <<
            a.actuatorData[ii].position << a.actuatorData[ii].speed << a.actuatorData[ii].state << (a.actuatorGeom[ii].hasBrakes ? "Y" : "N") << std::endl;
  }
  return os;
}

// Power control
int ActArrayProxy::Power (uint8_t val)
{
  if (!client)
    return -1;

  player_actarray_power_config_t config;
  memset (&config, 0, sizeof (config));
  config.request = PLAYER_ACTARRAY_POWER_REQ;
  config.value = val;
  return (client->Request(m_device_id, reinterpret_cast<const char*>(&config), sizeof(config)));
}

// Brakes control
int ActArrayProxy::Brakes (uint8_t val)
{
  if (!client)
    return -1;

  player_actarray_brakes_config_t config;
  memset (&config, 0, sizeof (config));
  config.request = PLAYER_ACTARRAY_BRAKES_REQ;
  config.value = val;
  return (client->Request(m_device_id, reinterpret_cast<const char*>(&config), sizeof(config)));
}

// Speed config
int ActArrayProxy::SpeedConfig (int8_t joint, int32_t speed)
{
  if (!client)
    return -1;

  player_actarray_speed_config_t config;
  memset (&config, 0, sizeof (config));
  config.request = PLAYER_ACTARRAY_SPEED_REQ;
  config.joint = joint;
  config.speed = speed;
  return (client->Request(m_device_id, reinterpret_cast<const char*>(&config), sizeof(config)));
}

// Send an actuator to a position
int ActArrayProxy::SetPosition (int8_t joint, int32_t position)
{
  if (!client)
    return -1;

  player_actarray_position_cmd_t cmd;
  memset (&cmd, 0, sizeof (cmd));

  cmd.flag = 0;
  cmd.joint = joint;
  cmd.position = position;

  return(client->Write(m_device_id, reinterpret_cast<const char*>(&cmd), sizeof(cmd)));
}

// Move an actuator at a speed
int ActArrayProxy::SetSpeed (int8_t joint, int32_t speed)
{
  if (!client)
    return -1;

  player_actarray_speed_cmd_t cmd;
  memset (&cmd, 0, sizeof (cmd));

  cmd.flag = 0;
  cmd.joint = joint;
  cmd.speed = speed;

  return(client->Write(m_device_id, reinterpret_cast<const char*>(&cmd), sizeof(cmd)));
}

// Send an actuator, or all actuators, home
int ActArrayProxy::Home (int8_t joint)
{
  if (!client)
    return -1;

  player_actarray_home_cmd_t cmd;
  memset (&cmd, 0, sizeof (cmd));

  cmd.flag = 2;
  cmd.joint = joint;

  return(client->Write(m_device_id, reinterpret_cast<const char*>(&cmd), sizeof(cmd)));
}

void ActArrayProxy::FillData (player_msghdr_t hdr, const char* buffer)
{
  if(hdr.size != sizeof(player_actarray_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of actarray data, but received %d. Unexpected results may ensue.\n",
              sizeof(player_actarray_data_t),hdr.size);
  }
  const player_actarray_data_t* lclBuffer = reinterpret_cast<const player_actarray_data_t*>(buffer);

  numActuators = lclBuffer->numactuators;
  for (int ii = 0; ii < numActuators; ii++)
  {
    actuatorData[ii].position = static_cast<int> (ntohl (lclBuffer->actuators[ii].position));
    actuatorData[ii].speed = static_cast<int> (ntohl (lclBuffer->actuators[ii].speed));
    actuatorData[ii].state = static_cast<int> (lclBuffer->actuators[ii].state);
  }
}

ActuatorData ActArrayProxy::GetActuatorData (int actuator)
{
  ActuatorData empty;
  
  if (actuator > numActuators)
  {
    memset (&empty, 0, sizeof (ActuatorData));
    return empty;
  }
  else
    return actuatorData[actuator];
}
    // Same again for getting actuator geometry
ActuatorGeom ActArrayProxy::GetActuatorGeom (int actuator)
{
  ActuatorGeom empty;
  
  if (actuator > numActuators)
  {
    memset (&empty, 0, sizeof (ActuatorGeom));
    return empty;
  }
  else
    return actuatorGeom[actuator];
}

int ActArrayProxy::RequestGeometry (void)
{
  player_msghdr_t hdr;
  player_actarray_geom_t actarray_geom;

  if (!client)
    return -1;

  actarray_geom.subtype = PLAYER_ACTARRAY_GET_GEOM_REQ;

  if ((client->Request (m_device_id, (const char*) &actarray_geom, sizeof (actarray_geom.subtype), &hdr, (char*) &actarray_geom, sizeof (actarray_geom)) < 0) ||
       (hdr.type != PLAYER_MSGTYPE_RESP_ACK))
  {
//    printf ("Didn't get expected response to ActArray::RequestGeometry, hrd type = %d\n", hdr.type);
    return -1;
  }

  numActuators = actarray_geom.numactuators;
  for (int ii = 0; ii < numActuators; ii++)
  {
    actuatorGeom[ii].type = actarray_geom.actuators[ii].type;
    actuatorGeom[ii].min = static_cast<int> (ntohl (actarray_geom.actuators[ii].min));
    actuatorGeom[ii].centre = static_cast<int> (ntohl (actarray_geom.actuators[ii].centre));
    actuatorGeom[ii].max = static_cast<int> (ntohl (actarray_geom.actuators[ii].max));
    actuatorGeom[ii].home = static_cast<int> (ntohl (actarray_geom.actuators[ii].home));
    actuatorGeom[ii].configSpeed = static_cast<int> (ntohl (actarray_geom.actuators[ii].config_speed));
    actuatorGeom[ii].hasBrakes = actarray_geom.actuators[ii].hasbrakes;
  }
  return 0;
}
