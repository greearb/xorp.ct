// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ident "$XORP: xorp/rip/tools/ripng_announcer.cc,v 1.15 2008/07/23 05:11:40 pavlin Exp $"

#include "rip/rip_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include <vector>
#include <fstream>

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

#include "libxorp/utils.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include "libcomm/comm_api.h"

#include "rip/auth.hh"
#include "rip/packet_assembly.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

template <typename A>
struct RipRoute {
    RipRoute() {}
    RipRoute(const IPNet<A>& 	a_net,
	     const A& 		a_nh,
	     uint16_t 		a_cost,
	     uint16_t 		a_tag)
	: net(a_net), nh(a_nh), cost(a_cost), tag(a_tag)
    {}

    IPNet<A> net;
    A        nh;
    uint16_t cost;
    uint16_t tag;
};

static bool
announce_routes(XorpFd fd, vector<RipRoute<IPv6> >* my_routes)
{
    PacketAssemblerSpecState<IPv6>	sp;
    ResponsePacketAssembler<IPv6> 	rpa(sp);
    vector<RipRoute<IPv6> >& 		rts = *my_routes;

    size_t n = my_routes->size();
    size_t i = 0;
    while (i != n) {
	RipPacket<IPv6> pkt(IPv6::RIP2_ROUTERS(), 521);
	rpa.packet_start(&pkt);
	while (i != n && rpa.packet_full() == false) {
	    rpa.packet_add_route(rts[i].net, rts[i].nh, rts[i].cost, rts[i].tag);
	    i++;
	}

	list<RipPacket<IPv6>*> auth_packets;
	if (rpa.packet_finish(auth_packets) != true)
	    break;
	if (pkt.data_bytes() == 0)
	    break;

	list<RipPacket<IPv6>*>::iterator iter;
	for (iter = auth_packets.begin(); iter != auth_packets.end(); ++iter) {
	    RipPacket<IPv6>* auth_pkt = *iter;
	    sockaddr_in6 sai;
	    auth_pkt->address().copy_out(sai);
	    sai.sin6_port = htons(auth_pkt->port());
	    if (sendto(fd, XORP_BUF_CAST(auth_pkt->data_ptr()),
		       auth_pkt->data_bytes(), 0,
		       reinterpret_cast<const sockaddr*>(&sai),
		       sizeof(sai)) < 0) {
		cerr << "Write failed: " << strerror(errno) << endl;
		return false;
	    } else {
		cout << "Packet sent" << endl;
	    }
	}
	delete_pointers_list(auth_packets);
    }
    return true;
}

static void
rip_announce(XorpFd fd, vector<RipRoute<IPv6> >& my_routes)
{
    EventLoop e;
    XorpTimer t = e.new_periodic_ms(30 * 1000,
				    callback(announce_routes, fd, &my_routes));

    announce_routes(fd, &my_routes);
    while (t.scheduled()) {
	e.run();
    }
}

static void
originate_routes_from_file(const char* 			file,
			   vector<RipRoute<IPv6> >& 	my_routes,
			   IPv6 			nh,
			   uint16_t 			cost,
			   uint16_t 			tag)
{
    ifstream fin(file);
    if (!fin) {
        cerr << "Could not open nets file " << file << endl;
	return;
    }

    string l;
    while (fin >> l) {
	try {
	    IPv6Net net(l.c_str());
	    my_routes.push_back(RipRoute<IPv6>(net, nh, cost, tag));
	} catch (...) {
	}
    }
    fin.close();
}

static XorpFd
init_rip_socket(int if_num)
{
#ifdef HAVE_IPV6
    in6_addr grp_addr;
    IPv6::RIP2_ROUTERS().copy_out(grp_addr);
    XorpFd fd = comm_bind_join_udp6(&grp_addr, if_num, htons(521),
				 COMM_SOCK_ADDR_PORT_REUSE,
				 COMM_SOCK_NONBLOCKING);
    if (!fd.is_valid()) {
	cerr << "comm_bind_join_udp6 failed" << endl;
    }
    if (comm_set_iface6(fd, if_num) != XORP_OK) {
	cerr << "comm_set_iface6 failed" << endl;
	comm_close(fd);
	fd.clear();
	return fd;
    }
     if (comm_set_multicast_ttl(fd, 255) != XORP_OK) {
	 cerr << "comm_set_multicast_ttl failed" << endl;
	 comm_close(fd);
	 fd.clear();
	 return fd;
    }
    if (comm_set_loopback(fd, 0) != XORP_OK) {
	cerr << "comm_set_loopback failed" << endl;
	comm_close(fd);
	fd.clear();
	return fd;
    }
    return fd;
#else
    cerr << "IPv6 support not found during build." << endl;
    XorpFd fd;
    return fd;
    UNUSED(if_num);
#endif
}

static void
short_usage()
{
    cerr << "Use -h for more details. " << endl;
}

static void
usage()
{
    cerr << xlog_process_name() << " [options] -i <ifname> -o <netsfile>"
	 << endl;
    cerr << "Options:" << endl;
    cerr << "  -c <cost>        specify cost for nets in next <netsfile>."
	 << endl;
    cerr << "  -i <ifname>      specify outbound interface." << endl;
    cerr << "  -n <nexthop>     specify nexthop for nets in next <netsfile>."
	 << endl;
    cerr << "  -o <netsfile>    specify file containing list of nets for announcement." << endl;

    cerr << "  -t <tag>         specify tag for nets in next <netsfile>."
	 << endl;
    cerr << "  -h               show this information." << endl;
}

int
main(int argc, char* const argv[])
{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);         // Least verbose messages
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    comm_init();

    try {
	vector<RipRoute<IPv6> > my_routes;
	const char* if_name = "";

	bool do_run = true;
	
	// Defaults
	uint16_t tag	= 0;
	uint16_t cost	= 1;
	IPv6	 nh;
	int	 if_num = -1;

	int ch;
	while ((ch = getopt(argc, argv, "c:n:i:I:o:t:h")) != -1) {
	    switch(ch) {
	    case 'c':
		cost = atoi(optarg);
		break;
	    case 'i':
		if_name = optarg;
		break;
	    case 'I':
		if_num = atoi(optarg);
		break;
	    case 'n':
		nh = IPv6(optarg);
		break;
	    case 'o':
		originate_routes_from_file(optarg, my_routes, nh, cost, tag);
		break;
	    case 't':
		tag = atoi(optarg);
		break;
	    case 'h':
	    default:
		usage();
		do_run = false;
	    }
	}

	if (do_run) {
#ifdef HAVE_IF_NAMETOINDEX
	    if_num = if_nametoindex(if_name);
#endif
	    if (if_num <= 0) {
		cerr << "Must specify a valid interface name with -i."
		     << endl;
		short_usage();
	    } else if (my_routes.empty()) {
		cerr << "No routes to originate." << endl;
		short_usage();
	    } else {
		XorpFd fd = init_rip_socket(if_num);
		if (!fd.is_valid()) {
		    rip_announce(fd, my_routes);
		    comm_close(fd);
		}
	    }
	}
    } catch (...) {
	xorp_print_standard_exceptions();
    }

    comm_exit();

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
