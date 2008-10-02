/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2008 XORP, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 2, June
 * 1991 as published by the Free Software Foundation. Redistribution
 * and/or modification of this program under the terms of any other
 * version of the GNU General Public License is not permitted.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
 * see the GNU General Public License, Version 2, a copy of which can be
 * found in the XORP LICENSE.gpl file.
 * 
 * XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
 * http://xorp.net
 */

#ident "$XORP: xorp/contrib/win32/xorprtm/print_rtmsg.c,v 1.7 2008/10/02 02:37:42 pavlin Exp $"

/*
 * Copyright (c) 1983, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <winsock2.h>
#include <ws2tcpip.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bsdroute.h"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

int warnx(char *str, ...)
{
  int ret;
  va_list ap;

  va_start(ap, str);
  ret = vfprintf(stderr, str, ap);
  va_end(ap);

  exit(1);

  return ret;
}

int warn(char *str,...)
{
  int ret;
  va_list ap;

  va_start(ap, str);
  ret = vfprintf(stderr, str, ap);
  va_end(ap);

  return ret;
}

int
snprintf(char *cp, size_t sz, char *fmt, ...)
{
 int ret;
 va_list ap;

 va_start(ap, fmt);
 ret = vsprintf(cp, fmt, ap);
 va_end(ap);

 return ret;
}

union	sockunion {
	struct	sockaddr sa;
	struct	sockaddr_in sin;
#ifdef INET6
	struct	sockaddr_in6 sin6;
#endif
	struct	sockaddr_storage ss; /* added to avoid memory overrun */
} so_dst, so_gate, so_mask, so_genmask, so_ifa, so_ifp;

typedef union sockunion *sup;
int	pid, rtm_addrs;
int	s;
int	forcehost, forcenet, doflush, nflag, af, qflag, tflag, keyword();
int	iflag, aflen = sizeof (struct sockaddr_in);
int	locking, lockrest, debugonly;
struct	rt_metrics rt_metrics;
u_long  rtm_inits;
const char	*routename(), *netname();
void	flushroutes(), newroute(), monitor(), sockaddr(), sodump(), bprintf();
void	print_getmsg(), print_rtmsg(), pmsg_common(), pmsg_addrs(), mask_addr();
static int inet6_makenetandmask(struct sockaddr_in6 *, char *);
int	getaddr(), rtmsg();
int	prefixlen();


int verbose = 1;

const char *
routename(sa)
	struct sockaddr *sa;
{
	char *cp;
	static char line[MAXHOSTNAMELEN + 1];
	struct hostent *hp;
	static char domain[MAXHOSTNAMELEN + 1];
	static int first = 1, n;

	if (first) {
		first = 0;
		if (gethostname(domain, MAXHOSTNAMELEN) == 0 &&
		    (cp = strchr(domain, '.'))) {
			domain[MAXHOSTNAMELEN] = '\0';
			(void) strcpy(domain, cp + 1);
		} else
			domain[0] = 0;
	}

	switch (sa->sa_family) {

	case AF_INET:
	    {	struct in_addr in;
		in = ((struct sockaddr_in *)sa)->sin_addr;

		cp = 0;
		if (in.s_addr == INADDR_ANY /*|| sa->sa_len < 4*/)
			cp = "default";
		if (cp == 0 && !nflag) {
			hp = gethostbyaddr((char *)&in, sizeof (struct in_addr),
				AF_INET);
			if (hp) {
				if ((cp = strchr(hp->h_name, '.')) &&
				    !strcmp(cp + 1, domain))
					*cp = 0;
				cp = hp->h_name;
			}
		}
		if (cp) {
			strncpy(line, cp, sizeof(line) - 1);
			line[sizeof(line) - 1] = '\0';
		} else
			(void) sprintf(line, "%s", inet_ntoa(in));
		break;
	    }

#ifdef INET6
	case AF_INET6:
	{
		struct sockaddr_in6 sin6; /* use static var for safety */
		int niflags = 0;

		memset(&sin6, 0, sizeof(sin6));
		memcpy(&sin6, sa, sizeof(sin6));
		sin6.sin6_family = AF_INET6;
#ifdef __KAME__
		if ((IN6_IS_ADDR_LINKLOCAL(&sin6.sin6_addr) ||
		     IN6_IS_ADDR_MC_LINKLOCAL(&sin6.sin6_addr)) &&
		    sin6.sin6_scope_id == 0) {
			sin6.sin6_scope_id =
			    ntohs(*(USHORT *)&sin6.sin6_addr.s6_addr[2]);
			sin6.sin6_addr.s6_addr[2] = 0;
			sin6.sin6_addr.s6_addr[3] = 0;
		}
#endif
		if (nflag)
			niflags |= NI_NUMERICHOST;
		if (getnameinfo((struct sockaddr *)&sin6,
		    sizeof(struct sockaddr_in6),
		    line, sizeof(line), NULL, 0, niflags) != 0)
			strncpy(line, "invalid", sizeof(line));

		return (line);
	}
#endif

	default:
	    {	u_short *s = (u_short *)sa;
		/*u_short *slim = s + ((sa->sa_len + 1) >> 1);*/
		u_short *slim = s + ((sizeof(struct sockaddr_storage) + 1) >> 1);
		char *cp = line + sprintf(line, "(%d)", sa->sa_family);
		char *cpe = line + sizeof(line);

		/* XXX: If not first, and family 0, assume it's
		 * a network mask in a 'struct sockaddr_storage' and
		 * just treat it as an inet mask.
		 */
		if (!first && sa->sa_family == 0) {
		    slim = s + ((sizeof(struct sockaddr_in) + 1) >> 1);
		}

		while (++s < slim && cp < cpe) /* start with sa->sa_data */
			if ((n = snprintf(cp, cpe - cp, " %x", *s)) > 0)
				cp += n;
			else
				*cp = '\0';
		break;
	    }
	}
	return (line);
}

/*
 * Return the name of the network whose address is given.
 * The address is assumed to be that of a net or subnet, not a host.
 */
const char *
netname(sa)
	struct sockaddr *sa;
{
	char *cp = 0;
	static char line[MAXHOSTNAMELEN + 1];
	struct netent *np = 0;
	u_long net, mask;
	u_long i;
	int n, subnetshift;

	switch (sa->sa_family) {

	case AF_INET:
	    {	struct in_addr in;
		in = ((struct sockaddr_in *)sa)->sin_addr;

		i = in.s_addr = ntohl(in.s_addr);
		if (in.s_addr == 0)
			cp = "default";
		else if (!nflag) {
			if (IN_CLASSA(i)) {
				mask = IN_CLASSA_NET;
				subnetshift = 8;
			} else if (IN_CLASSB(i)) {
				mask = IN_CLASSB_NET;
				subnetshift = 8;
			} else {
				mask = IN_CLASSC_NET;
				subnetshift = 4;
			}
			/*
			 * If there are more bits than the standard mask
			 * would suggest, subnets must be in use.
			 * Guess at the subnet mask, assuming reasonable
			 * width subnet fields.
			 */
			while (in.s_addr &~ mask)
				mask = (long)mask >> subnetshift;
			net = in.s_addr & mask;
			while ((mask & 1) == 0)
				mask >>= 1, net >>= 1;
			np = NULL;
		}
#define C(x)	(unsigned)((x) & 0xff)
		if (cp)
			strncpy(line, cp, sizeof(line));
		else if ((in.s_addr & 0xffffff) == 0)
			(void) sprintf(line, "%u", C(in.s_addr >> 24));
		else if ((in.s_addr & 0xffff) == 0)
			(void) sprintf(line, "%u.%u", C(in.s_addr >> 24),
			    C(in.s_addr >> 16));
		else if ((in.s_addr & 0xff) == 0)
			(void) sprintf(line, "%u.%u.%u", C(in.s_addr >> 24),
			    C(in.s_addr >> 16), C(in.s_addr >> 8));
		else
			(void) sprintf(line, "%u.%u.%u.%u", C(in.s_addr >> 24),
			    C(in.s_addr >> 16), C(in.s_addr >> 8),
			    C(in.s_addr));
#undef C
		break;
	    }

#ifdef INET6
	case AF_INET6:
	{
		struct sockaddr_in6 sin6; /* use static var for safety */
		int niflags = 0;

		memset(&sin6, 0, sizeof(sin6));
		memcpy(&sin6, sa, sizeof(sin6));
		sin6.sin6_family = AF_INET6;
#ifdef __KAME__
		if (/*sa->sa_len == sizeof(struct sockaddr_in6) &&*/
		    (IN6_IS_ADDR_LINKLOCAL(&sin6.sin6_addr) ||
		     IN6_IS_ADDR_MC_LINKLOCAL(&sin6.sin6_addr)) &&
		    sin6.sin6_scope_id == 0) {
			sin6.sin6_scope_id =
			    ntohs(*(USHORT *)&sin6.sin6_addr.s6_addr[2]);
			sin6.sin6_addr.s6_addr[2] = 0;
			sin6.sin6_addr.s6_addr[3] = 0;
		}
#endif
		if (nflag)
			niflags |= NI_NUMERICHOST;
		if (getnameinfo((struct sockaddr *)&sin6,
		    sizeof(struct sockaddr_in6),
		    line, sizeof(line), NULL, 0, niflags) != 0)
			strncpy(line, "invalid", sizeof(line));

		return (line);
	}
#endif

	default:
	    {	u_short *s = (u_short *)sa->sa_data;
		u_short *slim = s + ((sizeof(struct sockaddr_storage) + 1)>>1);
		char *cp = line + sprintf(line, "af %d:", sa->sa_family);
		char *cpe = line + sizeof(line);

		while (s < slim && cp < cpe)
			if ((n = snprintf(cp, cpe - cp, " %x", *s++)) > 0)
				cp += n;
			else
				*cp = '\0';
		break;
	    }
	}
	return (line);
}

void
inet_makenetandmask(net, sin, bits)
	u_long net, bits;
	struct sockaddr_in *sin;
{
	u_long addr, mask = 0;
	char *cp;

	rtm_addrs |= RTA_NETMASK;
	if (net == 0)
		mask = addr = 0;
	else if (net < 128) {
		addr = net << IN_CLASSA_NSHIFT;
		mask = IN_CLASSA_NET;
	} else if (net < 65536) {
		addr = net << IN_CLASSB_NSHIFT;
		mask = IN_CLASSB_NET;
	} else if (net < 16777216L) {
		addr = net << IN_CLASSC_NSHIFT;
		mask = IN_CLASSC_NET;
	} else {
		addr = net;
		if ((addr & IN_CLASSA_HOST) == 0)
			mask =  IN_CLASSA_NET;
		else if ((addr & IN_CLASSB_HOST) == 0)
			mask =  IN_CLASSB_NET;
		else if ((addr & IN_CLASSC_HOST) == 0)
			mask =  IN_CLASSC_NET;
		else
			mask = -1;
	}
	if (bits)
		mask = 0xffffffff << (32 - bits);
	sin->sin_addr.s_addr = htonl(addr);
	sin = &so_mask.sin;
	sin->sin_addr.s_addr = htonl(mask);
	sin->sin_family = 0;
	cp = (char *)(&sin->sin_addr + 1);
	while (*--cp == 0 && cp > (char *)sin)
		;
}

#ifdef INET6
/*
 * XXX the function may need more improvement...
 */
static int
inet6_makenetandmask(sin6, plen)
	struct sockaddr_in6 *sin6;
	char *plen;
{
	struct in6_addr in6;

	if (!plen) {
		if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr) &&
		    sin6->sin6_scope_id == 0) {
			plen = "0";
		} else if ((sin6->sin6_addr.s6_addr[0] & 0xe0) == 0x20) {
			/* aggregatable global unicast - RFC2374 */
			memset(&in6, 0, sizeof(in6));
			if (!memcmp(&sin6->sin6_addr.s6_addr[8],
				    &in6.s6_addr[8], 8))
				plen = "64";
		}
	}

	if (!plen || strcmp(plen, "128") == 0)
		return 1;
	rtm_addrs |= RTA_NETMASK;
	(void)prefixlen(plen);
	return 0;
}
#endif

int
prefixlen(s)
	char *s;
{
	int len = atoi(s), q, r;
	int max;
	char *p;

	rtm_addrs |= RTA_NETMASK;	
	switch (af) {
#ifdef INET6
	case AF_INET6:
		max = 128;
		p = (char *)&so_mask.sin6.sin6_addr;
		break;
#endif
	case AF_INET:
		max = 32;
		p = (char *)&so_mask.sin.sin_addr;
		break;
	default:
		(void) fprintf(stderr, "prefixlen not supported in this af\n");
		exit(1);
		/*NOTREACHED*/
	}

	if (len < 0 || max < len) {
		(void) fprintf(stderr, "%s: bad value\n", s);
		exit(1);
	}
	
	q = len >> 3;
	r = len & 7;
	so_mask.sa.sa_family = af;
	memset((void *)p, 0, max / 8);
	if (q > 0)
		memset((void *)p, 0xff, q);
	if (r > 0)
		*((u_char *)p + q) = (0xff00 >> r) & 0xff;
	if (len == max)
		return -1;
	else
		return len;
}

struct {
	struct	rt_msghdr m_rtm;
	char	m_space[512];
} m_rtmsg;

char *msgtypes[] = {
	"",
	"RTM_ADD: Add Route",
	"RTM_DELETE: Delete Route",
	"RTM_CHANGE: Change Metrics or flags",
	"RTM_GET: Report Metrics",
	"RTM_LOSING: Kernel Suspects Partitioning",
	"RTM_REDIRECT: Told to use different route",
	"RTM_MISS: Lookup failed on this address",
	"RTM_LOCK: fix specified metrics",
	"RTM_OLDADD: caused by SIOCADDRT",
	"RTM_OLDDEL: caused by SIOCDELRT",
	"RTM_RESOLVE: Route created by cloning",
	"RTM_NEWADDR: address being added to iface",
	"RTM_DELADDR: address being removed from iface",
	"RTM_IFINFO: iface status change",
	"RTM_NEWMADDR: new multicast group membership on iface",
	"RTM_DELMADDR: multicast group membership removed from iface",
	"RTM_IFANNOUNCE: interface arrival/departure",
	0,
};

char metricnames[] =
"\011pksent\010rttvar\7rtt\6ssthresh\5sendpipe\4recvpipe\3expire\2hopcount"
"\1mtu";
char routeflags[] =
"\1UP\2GATEWAY\3HOST\4REJECT\5DYNAMIC\6MODIFIED\7DONE\010MASK_PRESENT"
"\011CLONING\012XRESOLVE\013LLINFO\014STATIC\015BLACKHOLE\016b016"
"\017PROTO2\020PROTO1\021PRCLONING\022WASCLONED\023PROTO3\024CHAINDELETE"
"\025PINNED\026LOCAL\027BROADCAST\030MULTICAST";
char ifnetflags[] =
"\1UP\2BROADCAST\3DEBUG\4LOOPBACK\5PTP\6b6\7RUNNING\010NOARP"
"\011PPROMISC\012ALLMULTI\013OACTIVE\014SIMPLEX\015LINK0\016LINK1"
"\017LINK2\020MULTICAST";
char addrnames[] =
"\1DST\2GATEWAY\3NETMASK\4GENMASK\5IFP\6IFA\7AUTHOR\010BRD";

void
print_rtmsg(rtm, msglen)
	struct rt_msghdr *rtm;
	int msglen;
{
	struct if_msghdr *ifm;
	struct ifa_msghdr *ifam;
#ifdef RTM_NEWMADDR
	struct ifma_msghdr *ifmam;
#endif
	struct if_announcemsghdr *ifan;
	char *state;

	if (rtm->rtm_version != RTM_VERSION) {
		(void) printf("routing message version %d not understood\n",
		    rtm->rtm_version);
		return;
	}
	if (msgtypes[rtm->rtm_type] != NULL)
		(void)printf("%s: ", msgtypes[rtm->rtm_type]);
	else
		(void)printf("#%d: ", rtm->rtm_type);
	(void)printf("len %d, ", rtm->rtm_msglen);
	switch (rtm->rtm_type) {
        case RTM_IFINFO:
                ifm = (struct if_msghdr *)rtm;
                (void) printf("if# %d, ", ifm->ifm_index);
                switch (ifm->ifm_data.ifi_link_state) {
                case LINK_STATE_DOWN:
                        state = "down";
                        break;
                case LINK_STATE_UP:
                        state = "up";
                        break;
                default:
                        state = "unknown";
                        break;
                }
                (void) printf("link: %s, flags:", state);
                bprintf(stdout, ifm->ifm_flags, ifnetflags);
                pmsg_addrs((char *)(ifm + 1), ifm->ifm_addrs);
                break;
        case RTM_NEWADDR:
        case RTM_DELADDR:
                ifam = (struct ifa_msghdr *)rtm;
                (void) printf("metric %d, flags:", ifam->ifam_metric);
                bprintf(stdout, ifam->ifam_flags, routeflags);
                pmsg_addrs((char *)(ifam + 1), ifam->ifam_addrs);
                break;
	case RTM_IFANNOUNCE:
	    ifan = (struct if_announcemsghdr *)rtm;
	    (void) printf("if# %d, what: ", ifan->ifan_index);
	    switch (ifan->ifan_what) {
	    case IFAN_ARRIVAL:
		printf("arrival");
		break;
	    case IFAN_DEPARTURE:
		printf("departure");
		break;
	    default:
		printf("#%d", ifan->ifan_what);
		break;
	    }
	    printf("\n");
	    break;

	default:
		(void) printf("pid: %ld, seq %d, errno %d, flags:",
			(long)rtm->rtm_pid, rtm->rtm_seq, rtm->rtm_errno);
		bprintf(stdout, rtm->rtm_flags, routeflags);
		pmsg_common(rtm);
	}
}

void
print_getmsg(rtm, msglen)
	struct rt_msghdr *rtm;
	int msglen;
{
	struct sockaddr *dst = NULL, *gate = NULL, *mask = NULL;
	struct sockaddr_dl *ifp = NULL;
	struct sockaddr *sa;
	char *cp;
	int i;

	(void) printf("   route to: %s\n", routename(&so_dst));
	if (rtm->rtm_version != RTM_VERSION) {
		warnx("routing message version %d not understood",
		     rtm->rtm_version);
		return;
	}
	if (rtm->rtm_msglen > msglen) {
		warnx("message length mismatch, in packet %d, returned %d",
		      rtm->rtm_msglen, msglen);
	}
	if (rtm->rtm_errno)  {
		errno = rtm->rtm_errno;
		warn("message indicates error %d", errno);
		return;
	}
	cp = ((char *)(rtm + 1));
	if (rtm->rtm_addrs)
		for (i = 1; i; i <<= 1)
			if (i & rtm->rtm_addrs) {
				sa = (struct sockaddr *)cp;
				switch (i) {
				case RTA_DST:
					dst = sa;
					break;
				case RTA_GATEWAY:
					gate = sa;
					break;
				case RTA_NETMASK:
					mask = sa;
					break;
				case RTA_IFP:
					break;
				}
				cp += SA_SIZE(sa);
			}
	if (dst && mask)
		mask->sa_family = dst->sa_family;	/* XXX */
	if (dst)
		(void)printf("destination: %s\n", routename(dst));
	if (mask) {
		int savenflag = nflag;

		nflag = 1;
		(void)printf("       mask: %s\n", routename(mask));
		nflag = savenflag;
	}
	if (gate && rtm->rtm_flags & RTF_GATEWAY)
		(void)printf("    gateway: %s\n", routename(gate));
	(void)printf("      flags: ");
	bprintf(stdout, rtm->rtm_flags, routeflags);

#define	RTA_IGN	(RTA_DST|RTA_GATEWAY|RTA_NETMASK|RTA_IFP|RTA_IFA|RTA_BRD)
	if (verbose)
		pmsg_common(rtm);
	else if (rtm->rtm_addrs &~ RTA_IGN) {
		(void) printf("sockaddrs: ");
		bprintf(stdout, rtm->rtm_addrs, addrnames);
		putchar('\n');
	}
#undef	RTA_IGN
}

void
pmsg_common(rtm)
	struct rt_msghdr *rtm;
{
	(void) printf(" inits: ");
	bprintf(stdout, rtm->rtm_inits, metricnames);
	pmsg_addrs(((char *)(rtm + 1)), rtm->rtm_addrs);
}

void
pmsg_addrs(cp, addrs)
	char	*cp;
	int	addrs;
{
	struct sockaddr *sa;
	int i;

	if (addrs == 0) {
		(void) putchar('\n');
		return;
	}
	(void) printf("\nsockaddrs: ");
	bprintf(stdout, addrs, addrnames);
	(void) putchar('\n');
	for (i = 1; i; i <<= 1)
		if (i & addrs) {
			sa = (struct sockaddr *)cp;
			(void) printf(" %s", routename(sa));
			cp += SA_SIZE(sa);
		}
	(void) putchar('\n');
	(void) fflush(stdout);
}

void
bprintf(fp, b, s)
	FILE *fp;
	int b;
	u_char *s;
{
	int i;
	int gotsome = 0;

	if (b == 0)
		return;
	while ((i = *s++) != 0) {
		if (b & (1 << (i-1))) {
			if (gotsome == 0)
				i = '<';
			else
				i = ',';
			(void) putc(i, fp);
			gotsome = 1;
			for (; (i = *s) > 32; s++)
				(void) putc(i, fp);
		} else
			while (*s > 32)
				s++;
	}
	if (gotsome)
		(void) putc('>', fp);
}

void
sodump(su, which)
	sup su;
	char *which;
{
	switch (su->sa.sa_family) {
	case AF_INET:
		(void) printf("%s: inet %s; ",
		    which, inet_ntoa(su->sin.sin_addr));
		break;
	}
	(void) fflush(stdout);
}
