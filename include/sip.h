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
 *
 * part of the P2OS parser.  methods for filling and parsing server
 * information packets (SIPs)
 */
#ifndef _SIP_H
#define _SIP_H

#include <values.h>
#include <messages.h>
#include <p2osdevice.h>

class CSIP 
{
 private:
  int PositionChange( unsigned short, unsigned short );

 public:
  bool lwstall, rwstall;
  
  unsigned char status, battery, sonarreadings, analog, digin, digout;
  unsigned short ptu, compass, timer, rawxpos, rawypos, frontbumpers, rearbumpers;
  short angle, lvel, rvel, control;
  short sonars[PLAYER_NUM_SONAR_SAMPLES];
  int xpos, ypos;

  /* returns 0 if Parsed correctly otherwise 1 */
  void Parse( unsigned char *buffer );
  void Print();
  void PrintSonars();
  void Fill( player_p2os_data_t* data,  struct timeval timeBegan_tv);

  CSIP() {
    for(int i=0;i<(int)PLAYER_NUM_SONAR_SAMPLES;i++) 
      sonars[i] = 0;

    xpos = MAXINT;
    ypos = MAXINT;
  }
};

#endif
