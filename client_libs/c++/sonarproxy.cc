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
 * client-side sonar device 
 */

#include <sonarproxy.h>
#include <netinet/in.h>
#include <string.h>
#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif
    
// enable/disable the sonars
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int SonarProxy::SetSonarState(unsigned char state)
{
  if(!client)
    return(-1);

  char buffer[2];

  buffer[0] = PLAYER_SONAR_POWER_REQ;
  buffer[1] = state;


  return(client->Request(PLAYER_SONAR_CODE,index,(const char*)buffer,
                         sizeof(buffer)));
}

void SonarProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  if(hdr.size != sizeof(player_sonar_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of sonar data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_sonar_data_t),hdr.size);
  }

  bzero(ranges,sizeof(ranges));
  for(size_t i=0;i<PLAYER_NUM_SONAR_SAMPLES;i++)
  {
    ranges[i] = ntohs(((player_sonar_data_t*)buffer)->ranges[i]);
  }
}

// interface that all proxies SHOULD provide
void SonarProxy::Print()
{
  printf("#Sonar(%d:%d) - %c\n", device, index, access);
  for(size_t i=0;i<PLAYER_NUM_SONAR_SAMPLES;i++)
    printf("%u\n", ranges[i]);
}

