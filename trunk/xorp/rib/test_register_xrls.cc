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

#ident "$XORP: xorp/rib/test_register_xrls.cc,v 1.30 2004/09/17 14:00:05 abittau Exp $"

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "libxipc/finder_server.hh"
#include "libxipc/xrl_std_router.hh"

#include "xrl/targets/ribclient_base.hh"

#include "parser.hh"
#include "parser_direct_cmds.hh"
#include "parser_xrl_cmds.hh"
#include "register_server.hh"
#include "rib_manager.hh"
#include "xrl_target.hh"

bool callback_flag;

class RibClientTarget : public XrlRibclientTargetBase {
public:
    RibClientTarget(XrlRouter* r) : XrlRibclientTargetBase(r) {}

    XrlCmdError rib_client_0_1_route_info_changed4(
	// Input values,
	const IPv4&	addr,
	const uint32_t&	prefix_len,
	const IPv4&	nexthop,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	protocol_origin)
    {
	IPv4Net net(addr, prefix_len);
	printf("route_info_changed4: net:%s, new nexthop: %s, new metric: %d "
	       "new admin_distance: %d new protocol_origin: %s\n",
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
	const string&	/* protocol_origin */)
    {
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
	const uint32_t&	/* prefix_len */)
    {
	return XrlCmdError::OKAY();
    }

    bool verify_invalidated(const string& invalid);
    bool verify_changed(const string& changed);
    bool verify_no_info();

private:
    set<string> _invalidated;
    set<string> _changed;
};

bool
RibClientTarget::verify_invalidated(const string& invalid)
{
    set<string>::iterator iter;

    iter = _invalidated.find(invalid);
    if (iter == _invalidated.end()) {
	printf("EXPECTED: >%s<\n", invalid.c_str());
	for (iter = _invalidated.begin(); iter !=  _invalidated.end(); ++iter)
	    printf("INVALIDATED: >%s<\n", iter->c_str());
	return false;
    }
    _invalidated.erase(iter);
    return true;
}

bool
RibClientTarget::verify_changed(const string& changed)
{
    set<string>::iterator iter;

    iter = _changed.find(changed);
    if (iter == _changed.end()) {
	printf("EXPECTED: >%s<\n", changed.c_str());
	for (iter = _invalidated.begin(); iter !=  _invalidated.end(); ++iter)
	    printf("CHANGED: >%s<\n", iter->c_str());
	return false;
    }
    _changed.erase(iter);
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
    client.send_add_igp_table4("rib", tablename, "", "", true, false, cb);

    xrl_done_flag = false;
    while (xrl_done_flag == false) {
	loop.run();
    }
    return XORP_OK;
}

int
add_egp_table(XrlRibV0p1Client& client,
	      EventLoop& loop,
	      const string& tablename)
{
    XorpCallback1<void, const XrlError&>::RefPtr cb;
    cb = callback(xrl_done);
    client.send_add_egp_table4("rib", tablename, "", "", true, false, cb);

    xrl_done_flag = false;
    while (xrl_done_flag == false) {
	loop.run();
    }
    return XORP_OK;
}

int
add_vif(XrlRibV0p1Client& client,
	EventLoop& loop,
	const string& vifname,
	IPv4 myaddr, IPv4Net net)
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
    return XORP_OK;
}

int
add_route(XrlRibV0p1Client& client,
	  EventLoop& loop,
	  const string& protocol,
	  IPv4Net net, IPv4 nexthop, uint32_t metric)
{
    XorpCallback1<void, const XrlError&>::RefPtr cb;
    cb = callback(xrl_done);
    client.send_add_route4("rib", protocol, true, false,
			   net, nexthop, metric, XrlAtomList(), cb);

    xrl_done_flag = false;
    while (xrl_done_flag == false) {
	loop.run();
    }
    return XORP_OK;
}

int
delete_route(XrlRibV0p1Client& client,
	     EventLoop& loop,
	     const string& protocol,
	     IPv4Net net)
{
    XorpCallback1<void, const XrlError&>::RefPtr cb;
    cb = callback(xrl_done);
    client.send_delete_route4("rib", protocol, true, false,
			      net, cb);

    xrl_done_flag = false;
    while (xrl_done_flag == false) {
	loop.run();
    }
    return XORP_OK;
}

void
register_done(const XrlError& e,
	      const bool* resolves,
	      const IPv4* base_addr,
	      const uint32_t* prefix_len,
	      const uint32_t* /* real_prefix_len */,
	      const IPv4* nexthop,
	      const uint32_t* metric,
	      bool expected_resolves,
	      IPv4Net expected_net,
	      IPv4 expected_nexthop,
	      uint32_t expected_metric)
{
    XLOG_ASSERT(e == XrlCmdError::OKAY());

    if (expected_resolves) {
	XLOG_ASSERT(*resolves == true);
	IPv4Net net(*base_addr, *prefix_len);
	XLOG_ASSERT(net == expected_net);
	if (*metric != expected_metric) {
	    fprintf(stderr, "Expected metric %d, got %d\n",
		    expected_metric, *metric);
	    abort();
	}
	XLOG_ASSERT(*nexthop == expected_nexthop);
    } else {
	XLOG_ASSERT(*resolves == false);
    }
    xrl_done_flag = true;
}

int
register_interest(XrlRibV0p1Client& client,
		  EventLoop& loop,
		  const IPv4& addr,
		  bool expected_resolves,
		  const IPv4Net& expected_net,
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
    return XORP_OK;
}


int
main(int /* argc */, char* argv[])
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
    FinderServer fs(eventloop);

    // Rib Server component
    XrlStdRouter xrl_std_router_rib(eventloop, "rib");

    // Rib Client component
    XrlStdRouter client_xrl_router(eventloop, "ribclient");
    RibClientTarget ribclienttarget(&client_xrl_router);

    RibManager rib_manager(eventloop, xrl_std_router_rib, "fea");

    // RIB Instantiations for XrlRibTarget
    RIB<IPv4> urib4(UNICAST, rib_manager, eventloop);
    RegisterServer register_server(&xrl_std_router_rib);
    urib4.initialize(register_server);
    if (urib4.add_igp_table("connected", "", "") != XORP_OK) {
	XLOG_ERROR("Could not add igp table \"connected\" for urib4");
	abort();
    }

    // Instantiated but not used
    RIB<IPv4> mrib4(MULTICAST, rib_manager, eventloop);
    mrib4.add_igp_table("connected", "", "");
    RIB<IPv6> urib6(UNICAST, rib_manager, eventloop);
    urib6.add_igp_table("connected", "", "");
    RIB<IPv6> mrib6(MULTICAST, rib_manager, eventloop);
    mrib6.add_igp_table("connected", "", "");

    VifManager vif_manager(xrl_std_router_rib, eventloop, NULL, "fea");
    vif_manager.enable();
    vif_manager.start();
    XrlRibTarget xrt(&xrl_std_router_rib, urib4, mrib4, urib6, mrib6,
		     vif_manager, NULL);

    XrlRibV0p1Client xc(&xrl_std_router_rib);

    wait_until_xrl_router_is_ready(eventloop, xrl_std_router_rib);

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

    // Wait for a callback
    while (callback_flag == false) {
	eventloop.run();
    }

    ribclienttarget.verify_invalidated("9.0.0.0/24");
    ribclienttarget.verify_no_info();

    printf("====================================================\n");

    callback_flag = false;
    add_route(xc, eventloop, "ospf", IPv4Net("9.0.1.128/25"),
	      IPv4("1.0.0.2"), 7);
    // Wait for a callback
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
