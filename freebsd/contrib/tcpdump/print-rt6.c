/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /tcpdump/master/tcpdump/print-rt6.c,v 1.18 2001/06/15 22:17:34 fenner Exp $";
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef INET6

#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>


#include <netinet/in.h>

#include <stdio.h>

#include "ip6.h"

#include "interface.h"
#include "addrtoname.h"

int
rt6_print(register const u_char *bp, register const u_char *bp2)
{
	register const struct ip6_rthdr *dp;
	register const struct ip6_rthdr0 *dp0;
	register const struct ip6_hdr *ip;
	register const u_char *ep;
	int i, len;
	register const struct in6_addr *addr;

	dp = (struct ip6_rthdr *)bp;
	ip = (struct ip6_hdr *)bp2;
	len = dp->ip6r_len;

	/* 'ep' points to the end of available data. */
	ep = snapend;

	TCHECK(dp->ip6r_segleft);

	printf("srcrt (len=%d", dp->ip6r_len);	/*)*/
	printf(", type=%d", dp->ip6r_type);
	printf(", segleft=%d", dp->ip6r_segleft);

	switch (dp->ip6r_type) {
#ifndef IPV6_RTHDR_TYPE_0
#define IPV6_RTHDR_TYPE_0 0
#endif
	case IPV6_RTHDR_TYPE_0:
		dp0 = (struct ip6_rthdr0 *)dp;

		TCHECK(dp0->ip6r0_reserved);
		if (dp0->ip6r0_reserved || vflag) {
			printf(", rsv=0x%0x",
			    (u_int32_t)ntohl(dp0->ip6r0_reserved));
		}

		if (len % 2 == 1)
			goto trunc;
		len >>= 1;
		addr = &dp0->ip6r0_addr[0];
		for (i = 0; i < len; i++) {
			if ((u_char *)(addr + 1) > ep)
				goto trunc;
		
			printf(", [%d]%s", i, ip6addr_string(addr));
			addr++;
		}
		/*(*/
		printf(") ");
		return((dp0->ip6r0_len + 1) << 3);
		break;
	default:
		goto trunc;
		break;
	}

 trunc:
	fputs("[|srcrt]", stdout);
	return 65535;		/* XXX */
}
#endif /* INET6 */
