/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2002
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *     Nik Melchior
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

#include <rwi_laserproxy.h>
#include <netinet/in.h>
#include <string.h>
#include <math.h>

#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif

void
RWILaserProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
	if (hdr.size != sizeof(player_laser_data_t)) {
		if (player_debug_level(-1) >= 1)
			fprintf(stderr,"WARNING: rwi_laserproxy expected %d bytes of "
			        "laser data, but received %d. Unexpected results may "
			        "ensue.\n",
			        sizeof(player_laser_data_t), hdr.size);
	}

	bzero(ranges,sizeof(ranges));
	range_count = ntohs(((player_laser_data_t *) buffer)->range_count);
	for (unsigned short i = 0; i < range_count; i++) {
		ranges[i] = ntohs(((player_laser_data_t *)buffer)->ranges[i]);

		if (i > (range_count/2) && ranges[i] < min_left) {
			min_left = ranges[i];
		} else if (i < (range_count/2) && ranges[i] < min_right) {
			min_right = ranges[i];
		}
	}
}

// returns coords in mm of laser hit relative to sensor position
// x axis is forwards
int
RWILaserProxy::CartesianCoordinate(const int i, int *x, int *y) const
{
	if(i < 0 || i > range_count)
		return -1;
  	
	// FIXME: generalize to work for range_count<>180
	*x = (int)(ranges[i] * cos(DTOR(i)));
	*y = (int)(ranges[i] * sin(DTOR(i)));

	return true;
}

// interface that all proxies SHOULD provide
void RWILaserProxy::Print()
{
	printf("#RWILaser(%d:%d) - %c\n", device, index, access);
	printf("%d\n", range_count);
	for(unsigned int i = 0; i < range_count; i++)
		printf("%u ", ranges[i]);
	puts("\n");
}

