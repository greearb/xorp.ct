// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/rip/tools/ripng_announcer.cc,v 1.2 2004/03/11 14:39:43 hodson Exp $"

#include "rip/rip_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include <net/if.h>

#include <vector>
#include <fstream>

#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include "libcomm/comm_api.h"

#include "rip/auth.hh"
#include "rip/packet_assembly.hh"

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
announce_routes(int fd, vector<RipRoute<IPv6> >* my_routes)
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
	rpa.packet_finish();
	if (pkt.data_bytes() == 0) {
	    break;
	}

	sockaddr_in6 sai;
	pkt.address().copy_out(sai);
	sai.sin6_port = htons(pkt.port());
	if (sendto(fd,  pkt.data_ptr(), pkt.data_bytes(), 0,
		   reinterpret_cast<const sockaddr*>(&sai), sizeof(sai)) < 0) {
	    cerr << "Write failed: " << strerror(errno) << endl;
	    return false;
	} else {
	    cout << "Packet sent" << endl;
	}
    }
    return true;
}

static void
rip_announce(int fd, vector<RipRoute<IPv6> >& my_routes)
{
    EventLoop e;
    XorpTimer t = e.new_periodic(30 * 1000,
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

static int
init_rip_socket(int if_num)
{
#ifdef HAVE_IPV6
    in6_addr grp_addr;
    IPv6::RIP2_ROUTERS().copy_out(grp_addr);
    int fd = comm_bind_join_udp6(&grp_addr, if_num, htons(521), /* reuse */1);
    if (fd < 0) {
	cerr << "comm_bind_join_udp6 failed" << endl;
    }
    if (comm_set_iface6(fd, if_num) != XORP_OK) {
	cerr << "comm_set_iface6 failed" << endl;
	comm_close(fd);
	return -1;
    }
     if (comm_set_ttl(fd, 255) != XORP_OK) {
	 cerr << "comm_set_ttl failed" << endl;
	 comm_close(fd);
	 return -1;
    }
    if (comm_set_loopback(fd, 0) != XORP_OK) {
	cerr << "comm_set_loopback failed" << endl;
	return -1;
    }
    return fd;
#else
    UNUSED(if_num);
    cerr << "IPv6 support not found during build." << endl;
    return -1;
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

    try {
	vector<RipRoute<IPv6> > my_routes;
	const char* if_name = "";

	bool do_run = true;
	
	// Defaults
	uint16_t tag	= 0;
	uint16_t cost	= 1;
	IPv6	 nh;

	int ch;
	while ((ch = getopt(argc, argv, "c:n:i:o:t:h")) != -1) {
	    switch(ch) {
	    case 'c':
		cost = atoi(optarg);
		break;
	    case 'i':
		if_name = optarg;
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
	    int if_num = if_nametoindex(if_name);
	    if (if_num <= 0) {
		cerr << "Must specify a valid interface name with -i."
		     << endl;
		short_usage();
	    } else if (my_routes.empty()) {
		cerr << "No routes to originate." << endl;
		short_usage();
	    } else {
		int fd = init_rip_socket(if_num);
		if (fd > 0) {
		    rip_announce(fd, my_routes);
		    close(fd);
		}
	    }
	}
    } catch (...) {
	xorp_print_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
