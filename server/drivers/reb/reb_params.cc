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
 * reb_params.cc
 *
 * Parameters for the UMass UBot
*/
#include <reb_params.h>

UBotRobotParams_t ubot_slow_params = 
{
  "UBot",  //Class
  "slow", //Subclass
  700, //Max Trans Velocity
  150, // MAX Rot Velocity
  90, // radius in mm
  139, // axle length in mm
  8, // numberof IR
  100, // time between readings in ms
  // the following is # of pulese per mm
  // derived as (pulses in one rev)/(wheel diameter*pi)
  // for the slow ubot the gear reduction is 4*43*16*2:1 = 5504:1
  // the wheel diam is 72 mm
  24.333022, // pulses per mm
  24333022, // previous * 1e6 (REB_FIXED_FACTOR)
  0.041096416, // inverse of previous= mm per pulse
  41096,
  0.24333022, // this is pulses/mm * 0.01 s -> pulses/mm 10 ms
  243330,
  0.0004109641, // inverse of previous
  411,
  {
    {35, 0, 0},
    {25, 25, 45},
    {0, 35, 90},
    {-25, 25, 135},
    {-35, 0, 180},
    {-25, -25, 225},
    {0, -35, 270},
    {25, -25, 315},
  }
};



UBotRobotParams_t PlayerUBotRobotParams[PLAYER_NUM_UBOT_ROBOT_TYPES];

void
initialize_reb_params(void)
{
  PlayerUBotRobotParams[0] = ubot_slow_params;
}
