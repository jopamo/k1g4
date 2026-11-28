/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _NET_IP_ROUTER_ALERT_H
#define _NET_IP_ROUTER_ALERT_H

#include <linux/in.h>
#include <linux/ip.h>

/* For an extended IPv4 header, return the alert's offset from the start of
 * the header, or zero if it is not an IGMP Router Alert with legal padding.
 * The caller supplies the entire option area; never modify the packet.
 */
static inline unsigned int ip4_igmp_ra_offset(unsigned char protocol,
					      const unsigned char *p,
					      unsigned int len)
{
	unsigned int i = 0, offset = 0;

	if (protocol != IPPROTO_IGMP || len < 4 || len > MAX_IPOPTLEN || (len & 3))
		return 0;
	while (i < len) {
		switch (p[i]) {
		case IPOPT_NOOP:
			i++;
			break;
		case IPOPT_END:
			while (++i < len)
				if (p[i] != IPOPT_END)
					return 0;
			return offset;
		case IPOPT_RA:
			if (offset || len - i < 4 || p[i + 1] != 4 ||
			    p[i + 2] || p[i + 3])
				return 0;
			offset = sizeof(struct iphdr) + i;
			i += 4;
			break;
		default:
			return 0;
		}
	}
	return offset;
}
#endif
