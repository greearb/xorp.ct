// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/devnotes/template.cc,v 1.2 2003/01/16 19:08:48 mjh Exp $"

#include <vector>
#include <fstream>

#include "rip/rip_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

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

bool
announce_routes(int fd, vector<RipRoute<IPv4> >* my_routes)
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
	rpa.packet_finish();
	if (pkt.data_bytes() == 0) {
	    break;
	}

	sockaddr_in sai;
	pkt.address().copy_out(sai);
	sai.sin_port = htons(pkt.port());
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
fake_peer(int fd, vector<RipRoute<IPv4> >& my_routes)
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

static int
init_rip_socket(IPv4 if_addr)
{
    in_addr mcast_addr, join_if_addr;

    IPv4::RIP2_ROUTERS().copy_out(mcast_addr);
    if_addr.copy_out(join_if_addr);
    int fd = comm_bind_join_udp4(&mcast_addr, &join_if_addr,
				 htons(520), 0);
    if (fd < 0) {
	cerr << "Could not instantiate socket" << endl;
	fd = -1;
    } else if (comm_set_iface4(fd, &join_if_addr) < 0) {
	cerr << "comm_set_iface4 failed" << endl;
	close(fd);
	fd = -1;
    } else if (comm_set_ttl(fd, 1) < 0) {
	cerr << "comm_set_ttl failed" << endl;
	close(fd);
	fd = -1;
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
	vector<RipRoute<IPv4> > my_routes;
	IPv4	 if_addr;

	bool do_run = true;

	// Defaults
	uint16_t tag	= 0;
	uint16_t cost	= 1;
	IPv4	 nh;

	int ch;
	while ((ch = getopt(argc, argv, "c:n:i:o:t:h")) != -1) {
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
		int fd = init_rip_socket(if_addr);
		if (fd > 0) {
		    fake_peer(fd, my_routes);
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
