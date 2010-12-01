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

#include <errno.h>
#include <net/if.h>

#include <netlink/netlink.h>
#include <netlink/genl/ctrl.h>  
#include <netlink/genl/family.h>
#include <netlink/genl/genl.h>

#include <libplayercommon/error.h>

#include "nl80211.h"
#include "scan.h"
#include "handler.h"

int
nl80211_init(struct nl80211_state *state)
{
	int err;

	state->nl_handle = nl_handle_alloc();
	if (!state->nl_handle) {
		PLAYER_ERROR("failed to allocate netlink socket.");
		return -ENOMEM;
	}

	if (genl_connect(state->nl_handle)) {
		PLAYER_ERROR("failed to connect to generic netlink.");
		err = -ENOLINK;
		goto out_handle_destroy;
	}

	if (!(state->nl_cache = genl_ctrl_alloc_cache(state->nl_handle))) {
		PLAYER_ERROR("failed to allocate generic netlink cache.");
		err = -ENOMEM;
		goto out_handle_destroy;
	}

	state->nl80211 = genl_ctrl_search_by_name(state->nl_cache, "nl80211");
	if (!state->nl80211) {
		PLAYER_ERROR("nl80211 not found.");
		err = -ENOENT;
		goto out_cache_free;
	}

	return 0;

 out_cache_free:
	nl_cache_free(state->nl_cache);
 out_handle_destroy:
	nl_handle_destroy(state->nl_handle);
	return err;
}


void
nl80211_cleanup(struct nl80211_state *state)
{
	genl_family_put(state->nl80211);
	nl_cache_free(state->nl_cache);
	nl_handle_destroy(state->nl_handle);
}

static int
nl_get_multicast_id(struct nl_handle *handle, const char *family, const char *group)
{
	struct nl_msg *msg;
	struct nl_cb *cb;
	int ret, ctrlid;
	struct family_handler_args grp = {
		.group = group,
		.id = -ENOENT,
	};

	msg = nlmsg_alloc();
	if (!msg)
		return -ENOMEM;

	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb) {
		ret = -ENOMEM;
		goto out_fail_cb;
	}

	ctrlid = genl_ctrl_resolve(handle, "nlctrl");

	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, ctrlid, 0, 0, CTRL_CMD_GETFAMILY, 0);

	ret = -ENOBUFS;
	NLA_PUT_STRING(msg, CTRL_ATTR_FAMILY_NAME, family);

	ret = nl_send_auto_complete(handle, msg);
	if (ret < 0)
		goto out;

	ret = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &ret);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &ret);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, family_handler, &grp);

	while (ret > 0)
		nl_recvmsgs(handle, cb);

	if (ret == 0)
		ret = grp.id;
 nla_put_failure:
 out:
	nl_cb_put(cb);
 out_fail_cb:
	nlmsg_free(msg);
	return ret;
}

static int
listen_events(struct nl80211_state *state,
		const int n_waits, const unsigned int *waits,
		unsigned int devidx)
{
	int mcid;
	struct nl_cb *cb = nl_cb_alloc(NL_CB_DEFAULT);
	struct wait_event_args wait = {
		.n_cmds = n_waits,
		.cmds = waits,
		.devidx = devidx,
		.cmd = 0
	};

	/* Subscribe to scan multicast group */
	mcid = nl_get_multicast_id(state->nl_handle, "nl80211", "scan");
	if (mcid >= 0) {
		nl_socket_add_membership(state->nl_handle, mcid);
	}

	if (!cb) {
		fprintf(stderr, "failed to allocate netlink callbacks\n");
		return -ENOMEM;
	}

	/* no sequence checking for multicast messages */
	nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, wait_event, &wait);

	while (!wait.cmd)
		nl_recvmsgs(state->nl_handle, cb);

	nl_cb_put(cb);

	return wait.cmd;
}

/*
 * Trigger scan
 */
int
trigger_scan(struct nl80211_state *state, unsigned int passive, struct interface *netif)
{
	int err;
	struct nl_msg *ssids;
	struct nl_msg *msg;
	struct nl_cb *cb;

	ssids = nlmsg_alloc();
	if (!ssids) {
		PLAYER_ERROR("failed to allocate netlink message");
		return -ENOMEM;
	}

	msg = nlmsg_alloc();
	if (!msg) {
		nlmsg_free(ssids);
		PLAYER_ERROR("failed to allocate netlink message");
		return -ENOMEM;
	}

	cb = nl_cb_alloc(NL_CB_DEFAULT);
        if (!cb) {
                PLAYER_ERROR("failed to allocate netlink callbacks");
                err = -ENOMEM;
                goto out_free_msg;
        }


	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, genl_family_get_id(state->nl80211), 0,
			0, NL80211_CMD_TRIGGER_SCAN, 0);
	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, netif->ifindex);

	if (!passive) {
		NLA_PUT(ssids, 1, 0, "");
		nla_put_nested(msg, NL80211_ATTR_SCAN_SSIDS, ssids);
	}

	err = nl_send_auto_complete(state->nl_handle, msg);
        if (err < 0)
                goto out;

        err = 1;

        nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
        nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
        nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);

        while (err > 0)
                nl_recvmsgs(state->nl_handle, cb);
		
	PLAYER_MSG1(2, "scan triggered on device %s", netif->ifname);

	err = 0;
out:
	nl_cb_put(cb);
out_free_msg:
	nlmsg_free(ssids);
	nlmsg_free(msg);
	return err;
nla_put_failure:
	PLAYER_ERROR("building netlink message failed");
	goto out;
}

/*
 * Wait for scan results
 */
unsigned int
wait_scan(struct nl80211_state *state, struct interface *netif)
{
	/*
	 * WARNING: DO NOT COPY THIS CODE INTO YOUR APPLICATION
	 *
	 * This code has a bug, which requires creating a separate
	 * nl80211 socket to fix:
	 * It is possible for a NL80211_CMD_NEW_SCAN_RESULTS or
	 * NL80211_CMD_SCAN_ABORTED message to be sent by the kernel
	 * before (!) we listen to it, because we only start listening
	 * after we send our scan request.
	 *
	 * Doing it the other way around has a race condition as well,
	 * if you first open the events socket you may get a notification
	 * for a previous scan.
	 *
	 * The only proper way to fix this would be to listen to events
	 * before sending the command, and for the kernel to send the
	 * scan request along with the event, so that you can match up
	 * whether the scan you requested was finished or aborted (this
	 * may result in processing a scan that another application
	 * requested, but that doesn't seem to be a problem).
	 *
	 * Alas, the kernel doesn't do that (yet).
	 */

	unsigned int cmd;
	unsigned int cmds[] = {
                NL80211_CMD_NEW_SCAN_RESULTS,
                NL80211_CMD_SCAN_ABORTED,
        };
#	define ARRAY_SIZE(ar) (sizeof(ar)/sizeof(ar[0]))

	cmd = listen_events(state, ARRAY_SIZE(cmds), cmds, netif->ifindex);
	if (cmd == NL80211_CMD_SCAN_ABORTED)
		PLAYER_MSG1(2, "scan aborted on device %s", netif->ifname);
	return cmd;
}

int
get_scan_results(struct nl80211_state *state, struct interface *netif,
		player_wifi_data_t *wifi_data)
{
	int err;
	struct nl_msg *msg;
	struct nl_cb *cb;

	msg = nlmsg_alloc();
	if (!msg) {
		PLAYER_ERROR("failed to allocate netlink message");
		return -ENOMEM;
	}

	cb = nl_cb_alloc(NL_CB_DEFAULT);
        if (!cb) {
                PLAYER_ERROR("failed to allocate netlink callbacks");
                err = -ENOMEM;
                goto out_free_msg;
        }

	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, genl_family_get_id(state->nl80211), 0,
			NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);
	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, netif->ifindex);
	
	err = nl_send_auto_complete(state->nl_handle, msg);
        if (err < 0)
                goto out;

        err = 1;

        nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
        nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
        nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, scan_handler, wifi_data);

	while (err > 0)
                nl_recvmsgs(state->nl_handle, cb);

	err = 0;
out:
	nl_cb_put(cb);
out_free_msg:
	nlmsg_free(msg);
	return err;
nla_put_failure:
	PLAYER_ERROR("building netlink message failed");
	goto out;
}
