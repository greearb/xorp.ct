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

#ident "$XORP: xorp/rib/test_register_xrls.cc,v 1.12 2003/04/02 22:58:58 hodson Exp $"

#include "rib_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "libxipc/finder_server.hh"
#include "libxipc/xrl_std_router.hh"

#include "parser.hh"
#include "parser_direct_cmds.hh"
#include "parser_xrl_cmds.hh"
#include "xrl_target.hh"
#include "rib_client.hh"
#include "register_server.hh"
#include "xrl/targets/ribclient_base.hh"

bool callback_flag;

class RibClientTarget : public XrlRibclientTargetBase {
public:
    RibClientTarget(XrlRouter *r) : XrlRibclientTargetBase(r) {}

    XrlCmdError rib_client_0_1_route_info_changed4(
	// Input values, 
	const IPv4&	addr, 
	const uint32_t&	prefix_len, 
	const IPv4&	nexthop, 
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	protocol_origin) {
	IPv4Net net(addr, prefix_len);
	printf("route_info_changed4: net:%s, new nexthop: %s, new metric: %d new admin_distance: %d new protocol_origin: %s\n",
	       net.str().c_str(), nexthop.str().c_str(), metric,
	       admin_distance, protocol_origin.c_str());
	string s;
	s = net.str() + " " + nexthop.str();
	s += " " + c_format("%d", metric);
	s += " " + c_format("%d", admin_distance);
	s += " " + c_format("%s", protocol_origin.c_str());
	_changed.insert(s);
	callback_flag = true;
	return XrlCmdError::OKAY();
    }

    XrlCmdError rib_client_0_1_route_info_changed6(
	// Input values, 
        const IPv6&	/* addr */, 
	const uint32_t&	/* prefix_len */, 
	const IPv6&	/* nexthop */, 
	const uint32_t&	/* metric */,
	const uint32_t&	/* admin_distance */,
	const string&	/* protocol_origin */) {
	return XrlCmdError::OKAY();
    }

    XrlCmdError rib_client_0_1_route_info_invalid4(
	// Input values, 
	const IPv4&	addr, 
	const uint32_t&	prefix_len) 
    {
	IPv4Net net(addr, prefix_len);
	printf("route_info_invalid4: net:%s\n", net.str().c_str());
	string s;
	s = net.str();
	_invalidated.insert(s);
	callback_flag = true;
	return XrlCmdError::OKAY();
    }

    XrlCmdError rib_client_0_1_route_info_invalid6(
	// Input values,
        const IPv6&	/* addr */, 
	const uint32_t&	/* prefix_len */) {
	return XrlCmdError::OKAY();
    }
    bool verify_invalidated(const string& invalid);
    bool verify_changed(const string& changed);
    bool verify_no_info();

private:
    set <string> _invalidated;
    set <string> _changed;
};

bool 
RibClientTarget::verify_invalidated(const string& invalid)
{
    set <string>::iterator i;
    
    i = _invalidated.find(invalid);
    if (i == _invalidated.end()) {
	printf("EXPECTED: >%s<\n", invalid.c_str());
	for (i = _invalidated.begin(); i !=  _invalidated.end(); ++i)
	    printf("INVALIDATED: >%s<\n", i->c_str());
	return false;
    }
    _invalidated.erase(i);
    return true;
}

bool 
RibClientTarget::verify_changed(const string& changed)
{
    set <string>::iterator i;
    
    i = _changed.find(changed);
    if (i == _changed.end()) {
	printf("EXPECTED: >%s<\n", changed.c_str());
	for (i = _invalidated.begin(); i !=  _invalidated.end(); ++i)
	    printf("CHANGED: >%s<\n", i->c_str());
	return false;
    }
    _changed.erase(i);
    return true;
}

bool RibClientTarget::verify_no_info()
{
    if (_changed.empty() && _invalidated.empty())
	return true;
    return false;
}


bool xrl_done_flag = false;

void
xrl_done(const XrlError& e)
{
    if (e == XrlCmdError::OKAY()) {
	xrl_done_flag = true;
	return;
    }
    abort();
}

int
add_igp_table(XrlRibV0p1Client& client, 
	      EventLoop& loop, 
	      const string& tablename)
{
    XorpCallback1<void, const XrlError&>::RefPtr cb;
    cb = callback(xrl_done);
    client.send_add_igp_table4("rib", tablename, true, false, cb);

    xrl_done_flag = false;
    while (xrl_done_flag == false) {
	loop.run();
    }
    return 0;
}

int
add_egp_table(XrlRibV0p1Client& client, 
	      EventLoop& loop, 
	      const string& tablename)
{
    XorpCallback1<void, const XrlError&>::RefPtr cb;
    cb = callback(xrl_done);
    client.send_add_egp_table4("rib", tablename, true, false, cb);

    xrl_done_flag = false;
    while (xrl_done_flag == false) {
	loop.run();
    }
    return 0;
}

int
add_vif(XrlRibV0p1Client& client, 
	EventLoop& loop, 
	const string& vifname, 
	IPv4 myaddr, IPNet<IPv4> net)
{
    XorpCallback1<void, const XrlError&>::RefPtr cb;
    cb = callback(xrl_done);
    client.send_new_vif("rib", vifname, cb);

    xrl_done_flag = false;
    while (xrl_done_flag == false) {
	loop.run();
    }

    cb = callback(xrl_done);
    client.send_add_vif_addr4("rib", vifname, myaddr, net, cb);

    xrl_done_flag = false;
    while (xrl_done_flag == false) {
	loop.run();
    }
    return 0;
}

int
add_route(XrlRibV0p1Client& client, 
	  EventLoop& loop, 
	  const string& protocol, 
	  IPNet<IPv4> net, IPv4 nexthop, uint32_t metric)
{
    XorpCallback1<void, const XrlError&>::RefPtr cb;
    cb = callback(xrl_done);
    client.send_add_route4("rib", protocol, true, false, 
			   net, nexthop, metric, cb);

    xrl_done_flag = false;
    while (xrl_done_flag == false) {
	loop.run();
    }
    return 0;
}

int
delete_route(XrlRibV0p1Client& client, 
	     EventLoop& loop, 
	     const string& protocol, 
	     IPNet<IPv4> net)
{
    XorpCallback1<void, const XrlError&>::RefPtr cb;
    cb = callback(xrl_done);
    client.send_delete_route4("rib", protocol, true, false, 
			      net, cb);

    xrl_done_flag = false;
    while (xrl_done_flag == false) {
	loop.run();
    }
    return 0;
}

void
register_done(const XrlError& e, 
	      const bool* resolves, 
	      const IPv4* base_addr, 
	      const uint32_t* prefix, 
	      const uint32_t* /* realprefix */, 
	      const IPv4* nexthop, 
	      const uint32_t* metric, 
	      bool expected_resolves,
	      IPv4Net expected_net, 
	      IPv4 expected_nexthop,
	      uint32_t expected_metric)
{
    assert(e == XrlCmdError::OKAY());
    if (expected_resolves) {
	assert(*resolves);
	IPv4Net net(*base_addr, *prefix);
	assert(net == expected_net);
	if (*metric != expected_metric) {
	    fprintf(stderr, "Expected metric %d, got %d\n", 
		    expected_metric, *metric);
	    abort();
	}
	assert(*nexthop == expected_nexthop);
    } else {
	assert(!(*resolves));
    }
    xrl_done_flag = true;
}

int
register_interest(XrlRibV0p1Client& client, 
		  EventLoop& loop, 
		  const IPv4& addr,
		  bool expected_resolves,
		  const IPNet<IPv4>& expected_net,
		  const IPv4& expected_nexthop,
		  uint32_t expected_metric)
{
    XorpCallback7<void, const XrlError&, const bool*, const IPv4*, 
	const uint32_t*, const uint32_t*, const IPv4*, 
	const uint32_t*>::RefPtr cb;
    cb = callback(register_done, expected_resolves, 
		  expected_net, expected_nexthop, expected_metric);
    client.send_register_interest4("rib", "ribclient", addr, cb);

    xrl_done_flag = false;
    while (xrl_done_flag == false) {
	loop.run();
    }
    return 0;
}


int
main(int /* argc */, char *argv[])
{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    EventLoop eventloop;

    // Finder Server
    FinderNGServer fs(eventloop);

    // Rib Server component
    XrlStdRouter xrl_router(eventloop, "rib");
    RibClient rib_client(xrl_router, "fea");

    // Rib Client component
    XrlStdRouter client_xrl_router(eventloop, "ribclient");
    RibClientTarget ribclienttarget(&client_xrl_router);

    // RIB Instantiations for XrlRibTarget
    RIB<IPv4> urib4(UNICAST);
    RegisterServer regserv(&xrl_router);
    urib4.initialize_register(&regserv);
    if (urib4.add_igp_table("connected") < 0) {
	XLOG_ERROR("Could not add igp table \"connected\" for urib4");
	abort();
    }

    // Instantiated but not used
    RIB<IPv4> mrib4(MULTICAST);
    mrib4.add_igp_table("connected");
    RIB<IPv6> urib6(UNICAST);
    urib6.add_igp_table("connected");
    RIB<IPv6> mrib6(MULTICAST);
    mrib6.add_igp_table("connected");

    VifManager vif_manager(xrl_router, eventloop, NULL);
    vif_manager.enable();
    vif_manager.start();
    XrlRibTarget xrt(&xrl_router, urib4, mrib4, urib6, mrib6, vif_manager, NULL);

    XrlRibV0p1Client xc(&xrl_router);

    add_igp_table(xc, eventloop, "ospf");
    add_egp_table(xc, eventloop, "ebgp");

    urib4.print_rib();

    add_vif(xc, eventloop, "xl0", IPv4("1.0.0.1"), IPv4Net("1.0.0.0/24"));
    add_vif(xc, eventloop, "xl1", IPv4("1.0.1.1"), IPv4Net("1.0.1.0/24"));
    
    add_route(xc, eventloop, "ospf",
	      IPv4Net("9.0.0.0/24"), IPv4("1.0.0.2"), 7);

    printf("====================================================\n");

    register_interest(xc, eventloop, IPv4("9.0.0.1"), 
		      true, IPv4Net("9.0.0.0/24"), IPv4("1.0.0.2"), 7);

    register_interest(xc, eventloop, IPv4("9.0.1.1"), 
		      false, IPv4Net("0.0.0.0/0"), IPv4("0.0.0.0"), 0);

    printf("====================================================\n");

    callback_flag = false;
    delete_route(xc, eventloop, "ospf", IPv4Net("9.0.0.0/24"));

    //wait for a callback
    while (callback_flag == false) {
	eventloop.run();
    }

    ribclienttarget.verify_invalidated("9.0.0.0/24");
    ribclienttarget.verify_no_info();

    printf("====================================================\n");

    callback_flag = false;
    add_route(xc, eventloop, "ospf", 
	      IPv4Net("9.0.1.128/25"),  IPv4("1.0.0.2"), 7);
    //wait for a callback
    while (callback_flag == false) {
	eventloop.run();
    }

    ribclienttarget.verify_invalidated("9.0.1.0/24");
    ribclienttarget.verify_no_info();

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    
    exit (0);
}
