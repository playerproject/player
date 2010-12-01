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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <netlink/msg.h>
#include <netlink/genl/genl.h>

#include <libplayerinterface/player.h>
#include <libplayercommon/error.h>

#include "nl80211.h"
#include "handler.h"

/*
 * Helpers
 */
static int
mac_addr_n2a(char *mac_addr, unsigned char *arg)
{
	int i, l;

	l = 0;
	for (i = 0; i < 6 ; i++) {
		if (i == 0) {
			sprintf(mac_addr+l, "%02x", arg[i]);
			l += 2;
		} else {
			sprintf(mac_addr+l, ":%02x", arg[i]);
			l += 3;
		}
	}
	return l;
}

static int
escape_ssid(char *ssid, const uint8_t len, const uint8_t *data)
{
	int i, l;

	l = 0;
	for (i = 0; i < len && l < 32; i++) {
		if (isprint(data[i])) {
			ssid[l] = data[i];
			l++;
		} else {
			sprintf(ssid+l, "\\x%.2x", data[i]);
			l += 4;
		}
	}

	return l;
}


/*
 * Scan results handler
 */
int
scan_handler(struct nl_msg *msg, void *arg)
{
	player_wifi_data_t *wifi_data = arg;
	player_wifi_link_t *link;
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct nlattr *bss[NL80211_BSS_MAX + 1];
	static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
		[NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
		[NL80211_BSS_BSSID] = { },
		[NL80211_BSS_INFORMATION_ELEMENTS] = { },
		[NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
		[NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
		[NL80211_BSS_BEACON_IES] = { },
	};

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			genlmsg_attrlen(gnlh, 0), NULL);
	if (!tb[NL80211_ATTR_BSS]) {
		PLAYER_WARN("bss info missing!");
		return NL_SKIP;
	}
	if (nla_parse_nested(bss, NL80211_BSS_MAX,
			     tb[NL80211_ATTR_BSS],
			     bss_policy)) {
		PLAYER_WARN("failed to parse nested attributes!");
		return NL_SKIP;
	}
	if (!bss[NL80211_BSS_BSSID])
		return NL_SKIP;

	/* create new link data */
	wifi_data->links_count++;
	wifi_data->links = realloc(wifi_data->links, wifi_data->links_count * sizeof(player_wifi_link_t));
	assert(wifi_data->links);
	link = wifi_data->links + (wifi_data->links_count - 1);
	memset(link, 0, sizeof(player_wifi_link_t));

	/* MAC address */
	link->mac_count = mac_addr_n2a((char *)link->mac, nla_data(bss[NL80211_BSS_BSSID]));

	/* SSID */
	if (bss[NL80211_BSS_BEACON_IES]) {
		unsigned char *ie = nla_data(bss[NL80211_BSS_BEACON_IES]);
		int ielen = nla_len(bss[NL80211_BSS_BEACON_IES]);

		while (ielen >= 2 && ielen >= ie[1]) {
			if (ie[0] == 0) {
				link->essid_count = escape_ssid((char *)link->essid, ie[1], ie + 2);
				break;
			}
			ielen -= ie[1] + 2;
			ie += ie[1] + 2;
		}
	} else if (bss[NL80211_BSS_INFORMATION_ELEMENTS]) {
		unsigned char *ie = nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]);
		int ielen = nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]);

		while (ielen >= 2 && ielen >= ie[1]) {
			if (ie[0] == 0) {
				link->essid_count = escape_ssid((char *)link->essid, ie[1], ie + 2);
				break;
			}
			ielen -= ie[1] + 2;
			ie += ie[1] + 2;
		}
	}

	/* freq */
	if (bss[NL80211_BSS_FREQUENCY])
		link->freq = nla_get_u32(bss[NL80211_BSS_FREQUENCY]);

	/* level */
	if (bss[NL80211_BSS_SIGNAL_MBM])
		link->level = abs((int)nla_get_u32(bss[NL80211_BSS_SIGNAL_MBM])/100);
	else if (bss[NL80211_BSS_SIGNAL_UNSPEC])
		link->level = nla_get_u8(bss[NL80211_BSS_SIGNAL_UNSPEC]);

	return NL_SKIP;
}


/*
 *
 */
int
family_handler(struct nl_msg *msg, void *arg)
{
	struct family_handler_args *grp = arg;
	struct nlattr *tb[CTRL_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *mcgrp;
	int rem_mcgrp;

	nla_parse(tb, CTRL_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			genlmsg_attrlen(gnlh, 0), NULL);

	if (!tb[CTRL_ATTR_MCAST_GROUPS])
		return NL_SKIP;

	nla_for_each_nested(mcgrp, tb[CTRL_ATTR_MCAST_GROUPS], rem_mcgrp) {
		struct nlattr *tb_mcgrp[CTRL_ATTR_MCAST_GRP_MAX + 1];

		nla_parse(tb_mcgrp, CTRL_ATTR_MCAST_GRP_MAX,
				nla_data(mcgrp), nla_len(mcgrp), NULL);
		if (!tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME] ||
		    !tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID])
			continue;
		if (strncmp(nla_data(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME]),
					grp->group, nla_len(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME])))
			continue;
		grp->id = nla_get_u32(tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID]);
		break;
	}

	return NL_SKIP;
}

int
no_seq_check(struct nl_msg *msg, void *arg)
{
	return NL_OK;
}

int
wait_event(struct nl_msg *msg, void *arg)
{
	struct wait_event_args *wait = arg;
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	int i;

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			genlmsg_attrlen(gnlh, 0), NULL);

	/* Is the event for our device? */
	if (tb[NL80211_ATTR_IFINDEX] &&
		       wait->devidx == nla_get_u32(tb[NL80211_ATTR_IFINDEX])) {
		/* Is this the event we are waiting for? */
		for (i = 0; i < wait->n_cmds; i++) {
			if (gnlh->cmd == wait->cmds[i]) {
				wait->cmd = gnlh->cmd;
			}
		}
	}

	return NL_SKIP;
}

/*
 * Generic callback handler
 */
int
error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
	int *ret = arg;
	*ret = err->error;
	return NL_STOP;
}

int
ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}

int
finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_SKIP;
}
