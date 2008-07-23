// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/ospf/test_routing_interactive.cc,v 1.10 2008/01/04 03:16:58 pavlin Exp $"

#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/test_main.hh"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"

#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include "libproto/spt.hh"

#include "ospf.hh"
#include "delay_queue.hh"
#include "vertex.hh"
#include "area_router.hh"
#include "debug_io.hh"

#include "test_args.hh"
#include "test_build_lsa.hh"

template <typename A>
class Routing {
 public:
    Routing(OspfTypes::Version version, bool verbose, int verbose_level);

    /**
     * Send a command to route tester.
     */
    bool cmd(Args& args) throw(InvalidString);


 private:
    OspfTypes::Version _version;
    EventLoop _eventloop;
    TestInfo _info;
    DebugIO<A> _io;
    Ospf<A> _ospf;

    OspfTypes::AreaID _selected_area;

    /**
     * Return a pointer to the selected area router.
     */
    AreaRouter<A> *get_area_router(Args& args) {
	PeerManager<A>& pm = _ospf.get_peer_manager();
	AreaRouter<A> *area_router = pm.get_area_router(_selected_area);
	if (0 == area_router)
	    xorp_throw(InvalidString,
		       c_format("Invalid area <%s> [%s]",
				pr_id(_selected_area).c_str(),
				args.original_line().c_str()));
	return area_router;
    }
};

template <typename A>
Routing<A>::Routing(OspfTypes::Version version,
		    bool verbose, int verbose_level)
    :  _version(version),
       _info("routing", verbose, verbose_level, cout),
       _io(_info, version, _eventloop),
       _ospf(version, _eventloop, &_io)
{
    _io.startup();
    _ospf.trace().all(verbose);
    _ospf.set_testing(true);
}

template <typename A>
bool 
Routing<A>::cmd(Args& args) throw(InvalidString)
{
    string word;
    while (args.get_next(word)) {
	if (_info.verbose()) {
	    cout << "[" << word << "]" << endl;
	    cout << args.original_line() << endl;
	}
	if ("#" == word.substr(0,1)) {
	    // CMD: #
	    return true;
	} else if ("set_router_id" == word) {
	    // CMD: set_router_id <router id>
	    _ospf.set_router_id(set_id(get_next_word(args, "set_router_id").
				       c_str()));
	} else if ("create" == word) {
	    // CMD: create <area id> <area type>
	    OspfTypes::AreaID area =
		set_id(get_next_word(args, "create").c_str());
	    PeerManager<A>& pm = _ospf.get_peer_manager();
	    bool okay;
	    word = get_next_word(args, "area_type");
	    OspfTypes::AreaType area_type =
		from_string_to_area_type(word, okay);
	    if (!okay)
		xorp_throw(InvalidString,
			   c_format("<%s> is not a valid area type",
				    word.c_str()));
	    if (!pm.create_area_router(area, area_type,
				       false /* !permissive*/))
		xorp_throw(InvalidString,
			   c_format("Failed to create area <%s> [%s]",
				    pr_id(area).c_str(),
				    args.original_line().c_str()));
	    
	} else if ("select" == word) {
	    // CMD: select <area id>
	    OspfTypes::AreaID area = 
		set_id(get_next_word(args, "select").c_str());
	    PeerManager<A>& pm = _ospf.get_peer_manager();
	    if (0 == pm.get_area_router(area))
		xorp_throw(InvalidString,
			   c_format("Invalid area <%s> [%s]",
				    pr_id(area).c_str(),
				    args.original_line().c_str()));
	    _selected_area = area;
	} else if ("replace" == word) {
	    // CMD: replace <LSA>
	    // Replace this routers RouterLSA.
	    AreaRouter<A> *area_router = get_area_router(args);
	    BuildLsa blsa(_version);
	    Lsa *lsa = blsa.generate(args);
	    if (0 == lsa)
		xorp_throw(InvalidString,
			   c_format("Couldn't form a LSA [%s]",
				    args.original_line().c_str()));
	    Lsa::LsaRef lsar(lsa);
	    lsar->set_self_originating(true);
	    area_router->testing_replace_router_lsa(lsar);
	} else if ("add" == word) {
	    // CMD: add <LSA>
	    AreaRouter<A> *area_router = get_area_router(args);
	    BuildLsa blsa(_version);
	    Lsa *lsa = blsa.generate(args);
	    if (0 == lsa)
		xorp_throw(InvalidString,
			   c_format("Couldn't form a LSA [%s]",
				    args.original_line().c_str()));
	    Lsa::LsaRef lsar(lsa);
	    area_router->testing_add_lsa(lsar);
	} else if ("compute" == word) {
	    // CMD: compute <area id>
	    OspfTypes::AreaID area = 
		set_id(get_next_word(args, "compute").c_str());
	    PeerManager<A>& pm = _ospf.get_peer_manager();
	    AreaRouter<A> *area_router = pm.get_area_router(area);
	    if (0 == area_router)
		xorp_throw(InvalidString,
			   c_format("Invalid area <%s> [%s]",
				    pr_id(area).c_str(),
				    args.original_line().c_str()));
	    if (_info.verbose())
		area_router->testing_print_link_state_database();
 	    area_router->testing_routing_total_recompute();
	} else if ("destroy" == word) {
	    // CMD: destroy <area id>
	    OspfTypes::AreaID area =
		set_id(get_next_word(args, "destroy").c_str());
	    PeerManager<A>& pm = _ospf.get_peer_manager();
	    if (!pm.destroy_area_router(area))
		xorp_throw(InvalidString,
			   c_format("Failed to delete area <%s> [%s]",
				    pr_id(area).c_str(),
				    args.original_line().c_str()));
	} else if ("verify_routing_table_size" == word) {
	    // CMD: verify_routing_table_size <count>
	    uint32_t expected_count = 
		get_next_number(args, "verify_routing_table_size");
	    uint32_t actual_count = _io.routing_table_size();
	    if (expected_count != actual_count)
		xorp_throw(InvalidString,
			   c_format("Routing table size expected %d actual %d"
				    " [%s]",
				    expected_count, actual_count,
				    args.original_line().c_str()));
	} else if ("verify_routing_entry" == word) {
	    // CMD: verify_routing_entry <net> <nexthop> <metric> <equal> <discard>
	    IPNet<A> net(get_next_word(args, "verify_routing_entry").c_str());
	    A nexthop(get_next_word(args, "verify_routing_entry").c_str());
	    uint32_t metric = get_next_number(args, "verify_routing_entry");
	    bool equal = get_next_word(args, "verify_routing_entry") 
		== "true" ? true : false;
	    bool discard = get_next_word(args, "verify_routing_entry") == 
		"true" ? true : false;
	    if (!_io.routing_table_verify(net, nexthop, metric, equal,discard))
		xorp_throw(InvalidString,
			   c_format("Matching routing table entry not found"
				    " [%s]",
				    args.original_line().c_str()));
	} else {
	    xorp_throw(InvalidString, c_format("Unknown command <%s>. [%s]",
					   word.c_str(),
					   args.original_line().c_str()))
	}
    }

    return true;
}

template <typename A>
int
go(OspfTypes::Version version, bool verbose, int verbose_level)
{
    Routing<A> routing(version, verbose, verbose_level);

    try {
	for(;;) {
	    string line;
	    if (!getline(cin, line))
		return 0;
	    Args args(line);
	    if (!routing.cmd(args))
		return -1;
	}
    } catch(...) {
	xorp_print_standard_exceptions();
	return -1;
    }
}

int
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    TestMain t(argc, argv);

    bool v2 = t.get_optional_flag("-2", "--OSPFv2", "OSPFv2");
    bool v3 = t.get_optional_flag("-3", "--OSPFv3", "OSPFv3");
    t.complete_args_parsing();

    if (0 != t.exit())
	return t.exit();

    OspfTypes::Version version;
    if (v2 == v3 || v2) {
	version = OspfTypes::V2;
	return go<IPv4>(version, t.get_verbose(), t.get_verbose_level());
    } else {
	version = OspfTypes::V3;
	return go<IPv6>(version, t.get_verbose(), t.get_verbose_level());
    }

    xlog_stop();
    xlog_exit();
    
    return 0;
}
