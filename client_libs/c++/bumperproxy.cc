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

#include <bumperproxy.h>
#include <string.h>
#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif

// enable/disable the bumpers
//
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)
int
BumperProxy::SetBumperState(const unsigned char state) const
{
	if(!client)
		return(-1);

	player_bumper_config_t cfg;

	cfg.request = PLAYER_BUMPER_POWER_REQ;
	cfg.value = state;


	return(client->Request(PLAYER_BUMPER_TYPE, index,
	       (const char *) &cfg, sizeof(cfg)));
}

bool
BumperProxy::BumpedAny() const
{
	for (int i=0; (i < bumper_count)&&(i < PLAYER_NUM_BUMPER_SAMPLES); i++) {
		if (bumpfield & (1 << i))
			return true;
	}
	// if we got this far...
	return false;
}

bool
BumperProxy::Bumped(const unsigned int i) const
{
	if (i < bumper_count)
		return (bumpfield & (1 << i)) ? true : false;
	else
		return false;
}

void
BumperProxy::FillData(player_msghdr_t hdr, const char *buffer)
{
	if(hdr.size != sizeof(player_bumper_data_t)) {
		if(player_debug_level(-1) >= 1)
			fprintf(stderr,"WARNING: bumperproxy expected %d bytes of"
			        " bumper data, but received %d. Unexpected results may"
			        " ensue.\n",
			        sizeof(player_bumper_data_t), hdr.size);
	}

	bumper_count = ((player_bumper_data_t *)buffer)->bumper_count;
	bumpfield = ((player_bumper_data_t *)buffer)->bumpfield;
}

// interface that all proxies SHOULD provide
void
BumperProxy::Print()
{
	printf("#Bumper(%d:%d) - %c\n", device, index, access);
	printf("%d\n", bumper_count);
	for (int i = min(bumper_count, PLAYER_NUM_BUMPER_SAMPLES); i >= 0; i--)
		putchar((bumpfield & (1 << i)) ? '1' : '0');
	puts(" ");
}