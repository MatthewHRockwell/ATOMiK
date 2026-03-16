/* SPDX-License-Identifier: GPL-2.0 */
/*
 * atomik_net.h — Network delta compression monitoring
 *
 * Intercepts TCP sends via kprobe on tcp_sendmsg() to detect
 * redundant network traffic. Fingerprints outgoing data per
 * socket and tracks consecutive identical sends.
 *
 * v0.4: Observation + metrics. Detects redundant sends without
 * modifying the network path.
 */

#ifndef ATOMIK_NET_H
#define ATOMIK_NET_H

#include <linux/types.h>

int atomik_net_init(void);
void atomik_net_exit(void);

void atomik_net_set_enabled(bool enabled);
bool atomik_net_is_enabled(void);

#endif /* ATOMIK_NET_H */
