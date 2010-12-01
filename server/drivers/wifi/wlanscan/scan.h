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

#ifndef WLANSCAN_SCAN_H
#define WLANSCAN_SCAN_H

#include <libplayerinterface/player.h>

#ifdef __cplusplus
extern "C" {
#endif

	struct nl80211_state {
		struct nl_handle *nl_handle;
		struct nl_cache *nl_cache;
		struct genl_family *nl80211;
	};

	struct interface {
		const char *ifname;
		unsigned int ifindex;
	};

	extern int nl80211_init(struct nl80211_state *state);
	extern void nl80211_cleanup(struct nl80211_state *state);

	extern int trigger_scan(struct nl80211_state *state,
		       unsigned int passive, struct interface *netif);
	extern unsigned int wait_scan(struct nl80211_state *state,
			struct interface *netif); 
	extern int get_scan_results(struct nl80211_state *state,
			struct interface *netif,
			player_wifi_data_t *wifi_data);

#ifdef __cplusplus
}
#endif
#endif
