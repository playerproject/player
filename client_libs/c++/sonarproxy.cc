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

void SonarProxy::GetSonarPose(int s, double* px, double* py, double* pth)
{
    double xx = 0;
    double yy = 0;
    double a = 0;
    double angle, tangle;
    
	  switch( s )
	    {
#ifdef PIONEER1
	    case 0: angle = a - 1.57; break; //-90 deg
	    case 1: angle = a - 0.52; break; // -30 deg
	    case 2: angle = a - 0.26; break; // -15 deg
	    case 3: angle = a       ; break;
	    case 4: angle = a + 0.26; break; // 15 deg
	    case 5: angle = a + 0.52; break; // 30 deg
	    case 6: angle = a + 1.57; break; // 90 deg
#else
	    case 0:
	      angle = a - 1.57;
	      tangle = a - 0.900;
	      xx += 0.172 * cos( tangle );
	      yy += 0.172 * sin( tangle );
	      break; //-90 deg
	    case 1: angle = a - 0.87;
	      tangle = a - 0.652;
	      xx += 0.196 * cos( tangle );
	      yy += 0.196 * sin( tangle );
	      break; // -50 deg
	    case 2: angle = a - 0.52;
	      tangle = a - 0.385;
	      xx += 0.208 * cos( tangle );
	      yy += 0.208 * sin( tangle );
	      break; // -30 deg
	    case 3: angle = a - 0.17;
	      tangle = a - 0.137;
	      xx += 0.214 * cos( tangle );
	      yy += 0.214 * sin( tangle );
	      break; // -10 deg
	    case 4: angle = a + 0.17;
	      tangle = a + 0.137;
	      xx += 0.214 * cos( tangle );
	      yy += 0.214 * sin( tangle );
	      break; // 10 deg
	    case 5: angle = a + 0.52;
	      tangle = a + 0.385;
	      xx += 0.208 * cos( tangle );
	      yy += 0.208 * sin( tangle );
	      break; // 30 deg
	    case 6: angle = a + 0.87;
	      tangle = a + 0.652;
	      xx += 0.196 * cos( tangle );
	      yy += 0.196 * sin( tangle );
	      break; // 50 deg
	    case 7: angle = a + 1.57;
	      tangle = a + 0.900;
	      xx += 0.172 * cos( tangle );
	      yy += 0.172 * sin( tangle );
	      break; // 90 deg
	    case 8: angle = a + 1.57;
	      tangle = a + 2.240;
	      xx += 0.172 * cos( tangle );
	      yy += 0.172 * sin( tangle );
	      break; // 90 deg
	    case 9: angle = a + 2.27;
	      tangle = a + 2.488;
	      xx += 0.196 * cos( tangle );
	      yy += 0.196 * sin( tangle );
	      break; // 130 deg
	    case 10: angle = a + 2.62;
	      tangle = a + 2.755;
	      xx += 0.208 * cos( tangle );
	      yy += 0.208 * sin( tangle );
	      break; // 150 deg
	    case 11: angle = a + 2.97;
	      tangle = a + 3.005;
	      xx += 0.214 * cos( tangle );
	      yy += 0.214 * sin( tangle );
	      break; // 170 deg
	    case 12: angle = a - 2.97;
	      tangle = a - 3.005;
	      xx += 0.214 * cos( tangle );
	      yy += 0.214 * sin( tangle );
	      break; // -170 deg
	    case 13: angle = a - 2.62;
	      tangle = a - 2.755;
	      xx += 0.208 * cos( tangle );
	      yy += 0.208 * sin( tangle );
	      break; // -150 deg
	    case 14: angle = a - 2.27;
	      tangle = a - 2.488;
	      xx += 0.196 * cos( tangle );
	      yy += 0.196 * sin( tangle );
	      break; // -130 deg
	    case 15: angle = a - 1.57;
	      tangle = a - 2.240;
	      xx += 0.172 * cos( tangle );
	      yy += 0.172 * sin( tangle );
	      break; // -90 deg
        default:
          angle = 0;
          xx = 0;
          yy = 0;
          break;
#endif
	    }

      *px = xx;
      *py = -yy;
      *pth = -angle;
}
