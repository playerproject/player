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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

/*
 * Based on code from 'iw':
 *
 * Copyright (c) 2007, 2008        Johannes Berg
 * Copyright (c) 2007              Andy Lutomirski
 * Copyright (c) 2007              Mike Kershaw
 * Copyright (c) 2008-2009         Luis R. Rodriguez
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/*
 * Integration as a player driver was done by Michael Bienia.
 *
 * Copyright © 2010 Michael Bienia <m.bienia@stud.fh-dortmund.de>
 *
 */

#include <cstring>
#include <unistd.h>
#include <net/if.h>

#include <libplayercore/playercore.h>

#include "scan.h"


class WlanScan : public ThreadedDriver
{
	public:
		WlanScan(ConfigFile *cf, int section);
		~WlanScan();

		int MainSetup();
		void MainQuit();
		void Main();

	private:
		/* Wireless device to use for scanning */
		struct interface netif;
		/* Passive or active scanning */
		bool passive;

		/* netlink state */
		struct nl80211_state nlstate;
};

/* 
 * check for supported interfaces.
 *
 * returns: pointer to new WlanScan driver if supported, NULL else
 */
Driver *
WlanScan_Init(ConfigFile *cf, int section)
{
  return ((Driver*)(new WlanScan(cf, section)));
}

/* 
 * register with drivertable
 *
 * returns:
 */
void
wlanscan_Register(DriverTable *table)
{
  table->AddDriver("wlanscan", WlanScan_Init);
}

/*
 * Constructor
 */
WlanScan::WlanScan(ConfigFile *cf, int section) :
	ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_WIFI_CODE)
{
	netif.ifname = cf->ReadString(section, "interface", "wlan0");
	netif.ifindex = if_nametoindex(netif.ifname);
	if (netif.ifindex == 0)
		PLAYER_ERROR1("no such device: %s", netif.ifname);

	passive = cf->ReadBool(section, "passive", false);
}

WlanScan::~WlanScan()
{
}

int
WlanScan::MainSetup()
{
	/* init netlink state */
	return nl80211_init(&nlstate);
}

void
WlanScan::MainQuit()
{
	/* cleanup netlink state */
	nl80211_cleanup(&nlstate);
}

void
WlanScan::Main()
{
	double time;
	player_wifi_data_t wifi_data;

	for(;;) {
		// test if we are supposed to cancel
		pthread_testcancel();

		// sleep 1 sec
		usleep(1e6);

		ProcessMessages();

		// Get the time at which we started reading.
		// This is not a great estimate of when the phenomena occurred.
		GlobalTime->GetTimeDouble(&time);
		

		/* clear wifi data */
		memset(&wifi_data, 0, sizeof(wifi_data));

		trigger_scan(&nlstate, passive, &netif);
		wait_scan(&nlstate, &netif);
		get_scan_results(&nlstate, &netif, &wifi_data);

		this->Publish(this->device_addr, PLAYER_MSGTYPE_DATA,
				PLAYER_WIFI_DATA_STATE, &wifi_data, 0, &time);
		if (wifi_data.links)
		       free(wifi_data.links);
	}
}
