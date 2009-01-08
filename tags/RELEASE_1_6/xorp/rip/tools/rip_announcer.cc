// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

#ident "$XORP: xorp/rip/tools/rip_announcer.cc,v 1.17 2008/10/02 21:58:21 bms Exp $"

#include <vector>
#include <fstream>

#include "rip/rip_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

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

bool
announce_routes(XorpFd fd, vector<RipRoute<IPv4> >* my_routes)
{
    NullAuthHandler			nah;
    PacketAssemblerSpecState<IPv4>	sp(nah);
    ResponsePacketAssembler<IPv4> 	rpa(sp);
    vector<RipRoute<IPv4> >& 		rts = *my_routes;

    size_t n = my_routes->size();
    size_t i = 0;
    while (i != n) {
	RipPacket<IPv4> pkt(IPv4::RIP2_ROUTERS(), 520);
	rpa.packet_start(&pkt);
	while (i != n && rpa.packet_full() == false) {
	    rpa.packet_add_route(rts[i].net, rts[i].nh, rts[i].cost, rts[i].tag);
	    i++;
	}

	list<RipPacket<IPv4>*> auth_packets;
	if (rpa.packet_finish(auth_packets) != true)
	    break;
	if (pkt.data_bytes() == 0)
	    break;

	list<RipPacket<IPv4>*>::iterator iter;
	for (iter = auth_packets.begin(); iter != auth_packets.end(); ++iter) {
	    RipPacket<IPv4>* auth_pkt = *iter;
	    sockaddr_in sai;
	    auth_pkt->address().copy_out(sai);
	    sai.sin_port = htons(auth_pkt->port());
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
fake_peer(XorpFd fd, uint32_t period, vector<RipRoute<IPv4> >& my_routes)
{
    EventLoop e;
    XorpTimer t = e.new_periodic_ms(period * 1000,
				    callback(announce_routes, fd, &my_routes));

    announce_routes(fd, &my_routes);
    while (t.scheduled()) {
	e.run();
    }
}

static void
originate_routes_from_file(const char* 			file,
			   vector<RipRoute<IPv4> >& 	my_routes,
			   IPv4 			nh,
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
	    IPv4Net net(l.c_str());
	    my_routes.push_back(RipRoute<IPv4>(net, nh, cost, tag));
	} catch (...) {
	}
    }
    fin.close();
}

static XorpFd
init_rip_socket(IPv4 if_addr)
{
    in_addr mcast_addr, join_if_addr;

    IPv4::RIP2_ROUTERS().copy_out(mcast_addr);
    if_addr.copy_out(join_if_addr);
    XorpFd fd = comm_bind_join_udp4(&mcast_addr, &join_if_addr, htons(520),
				 COMM_SOCK_ADDR_PORT_DONTREUSE,
				 COMM_SOCK_NONBLOCKING);
    if (!fd.is_valid()) {
	cerr << "Could not instantiate socket" << endl;
    } else if (comm_set_iface4(fd, &join_if_addr) != XORP_OK) {
	cerr << "comm_set_iface4 failed" << endl;
	comm_close(fd);
	fd.clear();
    } else if (comm_set_multicast_ttl(fd, 1) != XORP_OK) {
	cerr << "comm_set_multicast_ttl failed" << endl;
	comm_close(fd);
	fd.clear();
    }
    return fd;
}

static void
short_usage()
{
    cerr << "Use -h for more details. " << endl;
}

static void
usage()
{
    cerr << xlog_process_name() << " [options] -i <ifaddr> -o <netsfile>"
	 << endl;
    cerr << "Options:" << endl;
    cerr << "  -c <cost>        specify cost for nets in next <netsfile>."
	 << endl;
    cerr << "  -i <ifaddr>      specify outbound interface." << endl;
    cerr << "  -n <nexthop>     specify nexthop for nets in next <netsfile>."
	 << endl;
    cerr << "  -o <netsfile>    specify file containing list of nets for announcement." << endl;
    cerr << "  -p <period>	specify announcement period in seconds (default = 30)." << endl;
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
	vector<RipRoute<IPv4> > my_routes;
	IPv4	 if_addr;

	bool do_run = true;

	// Defaults
	uint16_t tag	= 0;
	uint16_t cost	= 1;
	uint32_t period = 30;
	IPv4	 nh;

	int ch;
	while ((ch = getopt(argc, argv, "c:n:i:o:p:t:h")) != -1) {
	    switch(ch) {
	    case 'c':
		cost = atoi(optarg);
		break;
	    case 'i':
		if_addr = IPv4(optarg);
		break;
	    case 'n':
		nh = IPv4(optarg);
		break;
	    case 'o':
		originate_routes_from_file(optarg, my_routes, nh, cost, tag);
		break;
	    case 'p':
		period = strtoul(optarg, NULL, 10);
		if (period == 0) {
		    period = 30;
		}
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
	    if (if_addr == IPv4::ZERO()) {
		cerr << "Must specify a valid interface address with -i."
		     << endl;
		short_usage();
	    } else if (my_routes.empty()) {
		cerr << "No routes to originate." << endl;
		short_usage();
	    } else {
		XorpFd fd = init_rip_socket(if_addr);
		if (!fd.is_valid()) {
		    fake_peer(fd, period, my_routes);
		    comm_close(fd);
		    fd.clear();
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
