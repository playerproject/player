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

#ifndef WLANSCAN_HANDLER_H
#define WLANSCAN_HANDLER_H

#include <asm/types.h>
#include <netlink/handlers.h>

#ifdef __cplusplus
extern "C" {
#endif

	struct family_handler_args {
		const char *group;
		int id; /* return value */
	};

	struct wait_event_args {
		int n_cmds;
		const unsigned int *cmds;
		unsigned int devidx;
		unsigned int cmd; /* return value */
	};

	/* Scan handler */
	extern int scan_handler(struct nl_msg *msg, void *arg);

	/* */
	extern int family_handler(struct nl_msg *msg, void *arg);
	extern int no_seq_check(struct nl_msg *msg, void *arg);
	extern int wait_event(struct nl_msg *msg, void *arg);

	/* Generic callback handler */
	extern int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg);
	extern int ack_handler(struct nl_msg *msg, void *arg);
	extern int finish_handler(struct nl_msg *msg, void *arg);

#ifdef __cplusplus
}
#endif
#endif
