// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the Xorp LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the Xorp LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/fea/fti_rtsock.cc,v 1.4 2003/03/10 23:20:14 hodson Exp $"

#error "OBSOLETE FILE"

#include "fea_module.h"
#include "config.h"
#include "libxorp/xorp.h"

#include <net/if.h>
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif

#include <cstdio>

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "fticonfig.hh"
#include "fti_rtsock.hh"

/* ------------------------------------------------------------------------- */
/* Helper functions */

inline static string ifname(int index)
{
    char t[IFNAMSIZ];
    if (if_indextoname(index, t))
	return string(t);
    return string();
}

inline static int ifindex(const string& ifname)
{
    return if_nametoindex(ifname.c_str());
}

/*
 * Round up to nearest integer value of step.
 * Taken from ROUND_UP macro in Stevens.
 */
static inline size_t round_up(size_t val, size_t step)
{
    return (val & (step - 1)) ? (1 + (val | (step - 1))) : val;
}

/*
 * Step to next socket address pointer.
 * Taken from NEXT_SA macro in Stevens.
 */
static inline sockaddr* next_sa(sockaddr* sa)
{
    const size_t min_size = sizeof(u_long);
    size_t sa_size = sa->sa_len ? round_up(sa->sa_len, min_size) : min_size;
    uint8_t* p = reinterpret_cast<uint8_t*>(sa) + sa_size;
    return reinterpret_cast<sockaddr*>(p);
}

/* ------------------------------------------------------------------------- */
/* RoutingSocketFti */

RoutingSocketFti::RoutingSocketFti(RoutingSocket& rs)
    : RoutingSocketObserver(rs), _cache_valid(false)
{

    /*
    ** Open routing socket.
    */
    if (rs.is_open() == false) {
	xorp_throw(FtiConfigError,
		   c_format("RoutingSocketFti::RoutingSocketFti: "
			    "opening routing socket %s",
			    strerror(errno)));
    }

    /*
    ** Enable forwarding.
    */
    size_t enable = 1;
    if (-1 == sysctlbyname("net.inet.ip.forwarding",
			  0, 0, &enable, sizeof(enable))) {
	xorp_throw(FtiConfigError,
		   c_format("RoutingSocketFti::RoutingSocketFti: "
			    "unable to enable forwarding %s",
			    strerror(errno)));
    }

#if	0
    throw FtiError("BSD forwarding table interface not available");
#endif
}

RoutingSocketFti::~RoutingSocketFti()
{
}

bool
RoutingSocketFti::start_configuration()
{
    // Nothing particular to do at start of configuration, just label start.
    return mark_configuration_start();
}

bool
RoutingSocketFti::end_configuration()
{
    // Nothing particular to do at start of configuration, just label start.
    return mark_configuration_end();
}

bool
RoutingSocketFti::add_entry4(const Fte4& f)
{
    if (in_configuration() == false)
	return false;

    debug_msg("%s\n", f.str().c_str());

    /* XXX
    ** See the comment in RoutingSocketFti::del()
    */
    struct request {
	struct rt_msghdr hdr;
	struct sockaddr_in dst;

	union {
	    struct sockaddr sa;
	    struct sockaddr_dl gateway_dl;
	    struct sockaddr_in gateway_in;
	} gateway;

	struct sockaddr_in netmask;

    } req;

    memset(&req, 0, sizeof(req));

    req.hdr.rtm_msglen = sizeof(req);
    req.hdr.rtm_version = RTM_VERSION;
    req.hdr.rtm_type = RTM_ADD;
    req.hdr.rtm_index = /*ifname(f.ifname)*/0;
    if (f.gateway() != IPv4::ZERO()) {
	if (f.is_host_route())
	    req.hdr.rtm_flags = RTF_HOST;
	else
	    req.hdr.rtm_flags = RTF_GATEWAY;
    }
    req.hdr.rtm_addrs = RTA_DST|RTA_GATEWAY|RTA_NETMASK;

    req.dst.sin_len = sizeof(req.dst);
    req.dst.sin_family = AF_INET;
    req.dst.sin_addr.s_addr = f.net().masked_addr().addr();

    req.gateway.sa.sa_len = sizeof req.gateway;
    if (f.gateway() != IPv4::ZERO()) {
	req.gateway.gateway_in.sin_family = AF_INET;
	req.gateway.gateway_in.sin_addr.s_addr = f.gateway().addr();
    } else {
	req.gateway.gateway_dl.sdl_family = AF_LINK;
	req.gateway.gateway_dl.sdl_index = ifindex(f.vifname());
    }

    req.netmask.sin_len = sizeof(req.netmask);
    req.netmask.sin_family = AF_INET;
    req.netmask.sin_addr.s_addr = f.net().netmask().addr();

    RoutingSocket& rs = routing_socket();
    if (sizeof(req) != rs.write(&req, sizeof(req))) {
	XLOG_ERROR("write failed %s Attempting to add %s", strerror(errno),
	      f.str().c_str());
	return false;
    }
    /* XXX We could use trawl routing socket output to check on success. */
    return true;
}

bool
RoutingSocketFti::delete_entry4(const Fte4& f)
{
    if (in_configuration() == false)
	return false;

    /* XXX
    ** It is a requirement that all the sockaddr's are on a four byte
    ** boundary. We happen to know that sockaddr_in's are a multiple
    ** of four bytes. If this ever changes this code will break.
    */
    struct request {
	struct rt_msghdr hdr;
	struct sockaddr_in dst;
	struct sockaddr_in netmask;
    } req;

    size_t reqsz = sizeof(req);
    memset(&req, 0, reqsz);

    if (f.is_host_route()) {
	/* No netmask */
	reqsz -= sizeof(req.netmask);
    } else {
	req.hdr.rtm_addrs |= RTA_NETMASK;
	req.netmask.sin_len = sizeof(req.netmask);
	req.netmask.sin_family = AF_INET;
	req.netmask.sin_addr.s_addr = f.net().netmask().addr();
    }
    req.hdr.rtm_msglen = reqsz;
    req.hdr.rtm_version = RTM_VERSION;
    req.hdr.rtm_type = RTM_DELETE;
    req.hdr.rtm_addrs = RTA_DST;
    req.dst.sin_len = sizeof(req.dst);
    req.dst.sin_family = AF_INET;
    req.dst.sin_addr.s_addr = f.net().masked_addr().addr();

    RoutingSocket& rs = routing_socket();

    if ((ssize_t)reqsz != rs.write(&req, reqsz)) {
	XLOG_ERROR("write failed %s Attempting to delete %s",
		   strerror(errno),
	      f.str().c_str());
	return false;
    }
    return true;
}

bool
RoutingSocketFti::delete_all_entries4()
{
    if (in_configuration() == false)
	return false;
    
    list<Fte4> rt;

    make_routing_list(rt);
    for (list<Fte4>::iterator e = rt.begin(); e != rt.end(); e++) {
	delete_entry4(*e);
    }
    return true;
}

bool
RoutingSocketFti::lookup_route4(IPv4 addr, Fte4& fte)
{
    RoutingSocket& rs = routing_socket();

    struct request {
	struct rt_msghdr hdr;
	struct sockaddr_in dst;
    } req;

    memset(&req, 0, sizeof(req));

    req.hdr.rtm_msglen = sizeof(req);
    req.hdr.rtm_version = RTM_VERSION;
    req.hdr.rtm_type = RTM_GET;
    req.hdr.rtm_addrs = RTA_DST|RTA_IFP;	/* return the interface name */
    req.hdr.rtm_pid = rs.pid();
    req.hdr.rtm_seq = rs.seqno();
    req.dst.sin_len = sizeof(req.dst);
    req.dst.sin_family = AF_INET;
    req.dst.sin_addr.s_addr = addr.addr();

    if (sizeof(req) != rs.write(&req, sizeof(req))) {
	XLOG_ERROR("write failed %s", strerror(errno));
	return false;
    }

    /* We expect kernel to give us something back.  Force read until
     * data ripples up via rtsock_data() that corresponds to expected
     * sequence number and process id.
     */
    _cache_seqno = req.hdr.rtm_seq;
    _cache_valid = false;
    while (_cache_valid == false) {
	rs.force_read();
    }

    rtp rtp;
    init_table(rtp, rtp::DONT_CALL_DELETE,
	       reinterpret_cast<caddr_t>(&_cache_rtdata[0]),
	       _cache_rtdata.size());

    struct rt_msghdr *hdr;
    caddr_t end;

    if (!next_route(rtp, hdr, end)) {
	XLOG_ERROR("no route!!!");
	return false;
    }

    return parse_route(hdr, end, fte, RTF_UP, 0);
}

bool
RoutingSocketFti::lookup_entry4(const IPv4Net& net, Fte4& fte)
{
    RoutingSocket& rs = routing_socket();

    struct request {
	struct rt_msghdr hdr;
	struct sockaddr_in dst;
	struct sockaddr_in mask;
    } req;

    memset(&req, 0, sizeof(req));

    req.hdr.rtm_msglen = sizeof(req);
    req.hdr.rtm_version = RTM_VERSION;
    req.hdr.rtm_type = RTM_GET;
    req.hdr.rtm_addrs = RTA_DST|RTA_IFP;	/* return the interface name */
    req.hdr.rtm_pid = rs.pid();
    req.hdr.rtm_seq = rs.seqno();
    req.dst.sin_len = sizeof(req.dst);
    req.dst.sin_family = AF_INET;
    req.dst.sin_addr.s_addr = net.masked_addr().addr();
    req.mask.sin_family = AF_INET;
    req.dst.sin_addr.s_addr = net.netmask().addr();

    if (sizeof(req) != rs.write(&req, sizeof(req))) {
	XLOG_ERROR("write failed %s", strerror(errno));
	return false;
    }

    /* We expect kernel to give us something back.  Force read until
     * data ripples up via rtsock_data() that corresponds to expected
     * sequence number and process id.
     */
    _cache_seqno = req.hdr.rtm_seq;
    _cache_valid = false;
    while (_cache_valid == false) {
	rs.force_read();
    }

    rtp rtp;
    init_table(rtp, rtp::DONT_CALL_DELETE,
	       reinterpret_cast<caddr_t>(&_cache_rtdata[0]),
	       _cache_rtdata.size());

    struct rt_msghdr *hdr;
    caddr_t end;

    if (!next_route(rtp, hdr, end)) {
	XLOG_ERROR("no route!!!");
	return false;
    }

    return parse_route(hdr, end, fte, RTF_UP, 0);
}

bool
RoutingSocketFti::add_entry6(const Fte6& /* f */)
{
    if (in_configuration() == false)
	return false;

    return false;
}

bool
RoutingSocketFti::delete_entry6(const Fte6& )
{
    if (in_configuration() == false)
	return false;

    return false;
}

bool
RoutingSocketFti::delete_all_entries6()
{
    if (in_configuration() == false)
	return false;

    return false;
}

bool
RoutingSocketFti::lookup_route6(const IPv6&, Fte6& )
{
    return false;
}

bool
RoutingSocketFti::lookup_entry6(const IPv6Net&, Fte6& )
{
    return false;
}

/*
** Everything below here is a private method.
*/
RoutingSocketFti::rtp::rtp()
{
    /*
    ** Only call delete if "_table" was dynamically allocated.
    */
    _dyn = DONT_CALL_DELETE;
}

void
RoutingSocketFti::rtp::init(dyn dyn, caddr_t table, caddr_t current, caddr_t end)
{
    _dyn = dyn;
    _table = table;
    _current = current;
    _end = end;
}

RoutingSocketFti::rtp::~rtp()
{
    if (CALL_DELETE == _dyn && 0 != _table)
	delete _table;
}

bool
RoutingSocketFti::make_routing_list(list<Fte4>& rt)
{
    rtp rtp;

    if (!read_routing_table(rtp))
	return false;

    struct rt_msghdr *hdr;
    caddr_t end;
    Fte4 fte;

    while (next_route(rtp, hdr, end)) {
	if (parse_route(hdr, end, fte, RTF_UP, RTF_WASCLONED))
	    rt.push_back(fte);
    }

    return true;
}

bool
RoutingSocketFti::read_routing_table(rtp& rtp)
{
    int mib[6];
    size_t required = 0;
    caddr_t buf;

    /*
    ** Get the routing table
    */

    mib[0] = CTL_NET;
    mib[1] = PF_ROUTE;
    mib[2] = 0;			/* protocol number - always 0 */
    mib[3] = 0;			/* All protocols */
    mib[4] = NET_RT_DUMP;
    mib[5] = 0;			/* no flags */

    // estimate the size of the table by passing a null pointer
    if (0 != sysctl(mib, 6, NULL, &required, NULL , 0)) {
	XLOG_ERROR("sysctl failed: %s", strerror(errno));

	return false;
    }

    buf = new char[required];

    if (NULL == buf) {
	XLOG_ERROR("new(%d) failed: %s", required, strerror(errno));
	return false;
    }

    if (0 != sysctl(mib, 6, buf, &required, NULL , 0)) {
	XLOG_ERROR("sysctl failed: %s", strerror(errno));
	return false;
    }

    init_table(rtp, rtp::CALL_DELETE, buf, required);

    return true;
}

void
RoutingSocketFti::init_table(rtp& rtp, rtp::dyn dyn, caddr_t start, int size)
{
    rtp.init(dyn, start, start, start + size);

    init_sock();
}

bool
RoutingSocketFti::next_route(rtp& rtp, struct rt_msghdr *& hdr, caddr_t& end)
{
    if (rtp._current >= rtp._end) {
	debug_msg("next_route false\n");
	return false;
    }

    hdr = (struct rt_msghdr *)rtp._current;
    rtp._current = end = rtp._current + hdr->rtm_msglen;

    if (RTM_VERSION != hdr->rtm_version) {
		XLOG_ERROR("Version mismatch expected %d got %d",
			   RTM_VERSION,
		hdr->rtm_version);
	return false;
    }

    init_sock();

    debug_msg("true\n");

    return true;
}

bool
RoutingSocketFti::parse_route(struct rt_msghdr* hdr,
			      caddr_t		/*end*/,
			      Fte4&		fte,
			      int 		flags_on,
			      int		flags_off)
{
    if (RTM_GET != hdr->rtm_type) {
	XLOG_ERROR("Unexpected routing message %d", hdr->rtm_type);
	return false;
    }

    debug_msg("rtm_addrs %#x\n", hdr->rtm_addrs);

    /*
    ** No sockaddr structures. Nothing useful for us here.
    */
    if (0 == hdr->rtm_addrs)
	return false;

    fte.zero();

    int bits;
    struct sockaddr *sock;

    IPv4 dst, netmask, gateway;
    string name;

    while (0 != (sock = next_sock(hdr, bits))) {

	debug_msg("bits %#x\n", bits);
	debug_msg("flags %#x\n", hdr->rtm_flags);

	/*
	** Only interested if these flags are enabled.
	*/
	if (flags_on != (flags_on &  hdr->rtm_flags)) {
	    debug_msg("Wanted %#x got %#x\n",
		    flags_on, hdr->rtm_flags);
	    return false;
	}

	/*
	** Only interested if these flags are disabled.
	*/
	if ((0 != flags_off) && (flags_off == (flags_off & hdr->rtm_flags))) {
	    debug_msg("Didn't Want %#x got %#x\n",
		    flags_off, hdr->rtm_flags);
	    return false;
	}

	/*
	** If its a host route patch up the netmask
	*/
	if (RTF_HOST == (RTF_HOST & hdr->rtm_flags)) {
	    debug_msg("RTF_HOST\n");
	    netmask = IPv4::ALL_ONES();
	}

	/*
	** For the time being ignore IPv6
	*/
	if (AF_INET6 == sock->sa_family) {
	    debug_msg("AF_INET6\n");
	    return false;
	}

	/*
	** Destination should be IPv4
	*/
	if ((AF_INET != sock->sa_family) && (RTA_DST == bits)) {
	    XLOG_ERROR("Not AF_INET %d", sock->sa_family);
	    return false;
	}

	switch (bits) {
	case RTA_DST:
	    sockaddr(sock, dst);
	    break;
	case RTA_GATEWAY:
	    /*
	    ** The gateway has a link address this is not interesting.
	    */
	    if (AF_LINK == sock->sa_family) {
		debug_msg("Its an AF_LINK\n");
		break;
	    }
	    sockaddr(sock, gateway);
	    break;
	case RTA_NETMASK:
		/* XXX
		** There are a bunch of anomolies that I don't follow.
		** From observation a family of AF_UNSPEC seems to mean
		** netmask == 0. A family of 255 seems to mean
		** interpret the address????
		*/
		switch (sock->sa_family) {
		case AF_UNSPEC:
		    netmask.set_addr(0);
		    break;
		case 255:
		    sock->sa_family = AF_INET;
		    /* FALLTHROUGH */
		default:
		    sockaddr(sock, netmask);
		    break;
		}
		break;
	case RTA_IFP:
	    {
		struct sockaddr_dl *s = (struct sockaddr_dl *)sock;
		debug_msg("RTA_IFP len = %d\n", s->sdl_len);
		debug_msg("RTA_IFP family = %d\n", s->sdl_family);
		debug_msg("RTA_IFP nlen %d\n", s->sdl_nlen);
		debug_msg("RTA_IFP ifname %s\n", (char *)
			((struct sockaddr_dl *)(sock))->sdl_data);

		char tmpbuf[100];
		strncpy(&tmpbuf[0],
			(char *)((struct sockaddr_dl *)(sock))->sdl_data,
			sizeof(tmpbuf));
		tmpbuf[s->sdl_nlen] = 0;
		name = tmpbuf;
		break;
	    }
	case RTA_IFA:
	    debug_msg("RTA_IFA\n");
	    break;
	default:
	    XLOG_ERROR("strange bits %#x", bits);
	    break;
	}
    }

    /*
    ** We have finished processing the sockaddrs, so get the
    ** interface associated with this route.
    */

    if (name.empty())
	name = ifname(hdr->rtm_index);
    
    //
    // TODO: define default routing metric and admin distance instead of ~0
    //
    fte = Fte4(IPv4Net(dst, netmask.prefix_length()), gateway, name, name,
	       ~0, ~0);

    debug_msg("sock processing %s\n", fte.vifname().c_str());

    return true;
}

// init_sock() and next_sock() navigate through the list of addresses
// supplied by the sysctl. you first call init_sock(), then subsequent
// calls to next_sock() return a pointer to the sockaddr.
// "bits" indicates the position of the address you are looking for.

void
RoutingSocketFti::init_sock()
{
    _sock = 0;
    _addrs = _bit_count = 0;
}

struct sockaddr *
RoutingSocketFti::next_sock(struct rt_msghdr *hdr, int& bits)
{
    bits = 0;
    if (0 == _sock) {
	// first time, just position after the header and copy flags.
	_addrs = hdr->rtm_addrs;
	if (0 == _addrs)
	    return 0;
	_sock = (struct sockaddr *)((hdr + 1));
	_bit_count = -1;
    } else {
	if (0 == _addrs)
	    return 0;

	_sock = next_sa(_sock);
    }

    int res;

    do {
	_bit_count++;
	res = _addrs & 1;
	_addrs >>= 1;
    } while (0 == res);

    bits = 1 << _bit_count;

    return _sock;
}

bool
RoutingSocketFti::sockaddr(struct sockaddr *sock, IPv4& ipv4)
{
    switch (sock->sa_family) {
    case AF_INET:
	ipv4 = IPv4(*sock);
	{
	    struct in_addr in;
	    in.s_addr = ipv4.addr();

	    debug_msg("address = %s\n", inet_ntoa(in));

	}
	break;
    default:
	XLOG_ERROR("Address family is %d not %d", sock->sa_family, AF_INET);
	for (int i = 0; i < 14; i++)
	    XLOG_ERROR("%#x ", sock->sa_data[i]);
	break;
    }

    return true;
}

void
RoutingSocketFti::rtsock_data(const uint8_t* data, size_t nbytes)
{
    /*
     * Copy data that has been requested to be cached by setting _cache_seqno.
     */
    size_t d = 0;
    pid_t pid = routing_socket().pid();

    while (d < nbytes) {
	const rt_msghdr* rh = reinterpret_cast<const rt_msghdr*>(data + d);
	if (rh->rtm_pid == pid && rh->rtm_seq == (signed)_cache_seqno) {
#if 0
	    assert(_cache_valid == false); /* Do not overwrite cache data. */
#endif
	    _cache_rtdata.resize(rh->rtm_msglen);
	    memcpy(&_cache_rtdata[0], rh, rh->rtm_msglen);
	    _cache_valid = true;
	    return;
	}
	d += rh->rtm_msglen;
    }
}
