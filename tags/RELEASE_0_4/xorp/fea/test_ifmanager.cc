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

#ident "$XORP: xorp/fea/test_ifmanager.cc,v 1.2 2003/03/10 23:20:17 hodson Exp $"

#include <algorithm>
#include <functional>
#include <list>
#include <iterator>
#include <string>

#include "fea_module.h"
#include "config.h"

#include "libxorp/debug.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include <libxorp/ipvx.hh>
#include "libxorp/mac.hh"
#include "ifmanager.hh"
#include "ifconfig.hh"

const char ipv6_address[] = "::280:c8ff:feb9:3f5d";


/* Terminate method */
static void
fail(const char* file, int line, const char* func, const string& reason)
{
    cerr << "Failed in " << func << " (" << file << " line " << line << ")"
	 << endl;
    cerr << reason << endl;
    exit(-1);
}

/* Macro to provide most of arguments for fail() */
#define FAIL(x)	fail(__FILE__, __LINE__, __FUNCTION__, x)

/*
** Drive the device code through its paces.
** If "real" is set to true then a real interface will be
** used. Remember to provide a real interface.
*/
static void
test1_actual(const char *interface_name, IfConfig& ifconfig)
{
    InterfaceManager ifmgr(&ifconfig);

    /* Create and enable interface */
    if (ifmgr.create_interface(interface_name) == false)
	FAIL(c_format("Could not create interface named: %s", interface_name));

    Interface* i = ifmgr.find_interface_by_name(interface_name);

    if (i == 0)
	FAIL(c_format("Could not get interface named: %s", interface_name));

    if (i->set_enabled(true) == false)
	FAIL(c_format("Could not enable interface: %s", interface_name));

    /* Test mac get and set */
    Mac mac;

    if (i->get_mac(mac) == false)
	FAIL(c_format("Could not get Mac on interface: %s",
		      i->name().c_str()));

    if (i->set_mac(mac) == false)
	FAIL(c_format("Could not set Mac on interface: %s",
		      i->name().c_str()));

    cout << "Mac Address: " << mac.str() << endl;

    /* Test mtu get and set */
    int mtu;

    if (i->get_mtu(mtu) == false)
	FAIL(c_format("Could not get mtu on interface: %s",
		      i->name().c_str()));

    if (i->set_mtu(mtu) == false)
	FAIL(c_format("Could not set mtu on interface: %s",
		      i->name().c_str()));

    cout << "MTU: " << mtu << endl;

    /* Create and enable a vif */
    if (i->create_vif(interface_name) == false)
	FAIL(c_format("Could not create vif named: %s", interface_name));

    IfTreeVif* v = i->find_vif_by_name(interface_name);
    if (v == 0)
	FAIL(c_format("Could not get vif named: %s", interface_name));

    v->set_enabled(true);

    v->create_address(IPvX(IPv4("128.64.16.64")));
    v->create_address(IPvX(IPv6(ipv6_address)));

    IfTreeVifAddress *va = v->find_address(IPvX(IPv4("128.64.16.64")));
    va->set_enabled(true);

    const int* prefix =  va->prefix();
    if (prefix == 0)
	FAIL("Could not get prefix");

    va->set_prefix(*prefix);
    cout << "prefix: " << *prefix << endl;

    cout << "address admin enabled? " << va->admin_enabled() << endl;

    const IPvX* broadcast = va->broadcast();
    if (broadcast == 0)
	FAIL("Could not get broadcast address");

    va->set_broadcast(*broadcast);
    cout << "broadcast: " << broadcast->str() << endl;

    va = v->find_address(IPvX(IPv6(ipv6_address)));
    prefix = va->prefix();
    if (prefix == 0)
	FAIL("Could not get prefix");

    va->set_prefix(*prefix);
    cout << "prefix: " << *prefix << endl;

    v->create_address(IPv4("10.10.10.1"));
    v->create_address(IPv4("198.0.0.1"));
    v->delete_address(IPv4("10.10.10.1"));
    v->delete_address(IPv4("128.64.16.64"));
    v->delete_address(IPv4("198.0.0.1"));

    v->set_enabled(false);
    v->set_enabled(true);

    i->delete_vif(v->name());

    i->set_enabled(false);
    ifmgr.delete_interface(i->name());
}

void test1(const char* interface_name, bool real)
{
    if (real) {
	EventLoop e;
	RoutingSocket _rs(e);
	Iflist ifl(_rs);
	IfconfigFreeBSD ifc(ifl);
	test1_actual(interface_name, ifc);
    } else {
	IfconfigDummy ifc;
	test1_actual(interface_name, ifc);
    }
}

/*
** Set an IPv6 address on a real interface.
*/
void
test2(const char *interface_name)
{
    EventLoop e;
    RoutingSocket _rs(e);
    Iflist ifl(_rs);
    IfconfigFreeBSD bsd_ifconfig(ifl);

    bsd_ifconfig.add_address(interface_name,
			     IPvX(IPv6(ipv6_address)));
}

/*
** Delete an IPv6 address on a real interface.
*/
void
test3(const char *interface_name)
{
    EventLoop e;
    RoutingSocket _rs(e);
    Iflist ifl(_rs);
    IfconfigFreeBSD bsd_ifconfig(ifl);

    bsd_ifconfig.delete_address(interface_name, IPvX(IPv6(ipv6_address)));
}

void
usage(char *name)
{
    fprintf(stderr, "usage: %s -t num  -i interface -r\n",
	    name);
    exit(-1);
}

int
main(int argc, char *argv[])
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    /*
    ** Initialize and start xlog
    */
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    int c;
    int test_num = 1;
    const char *interface = "interface0";
    bool real_interface = false;

    while((c = getopt (argc, argv, "t:i:r")) != EOF) {
	switch (c) {
	case 't':
	    test_num = atoi(optarg);
	    break;
	case 'i':
	    interface = optarg;
	    break;
	case 'r':
	    real_interface = true;
	    break;
	    break;
	case '?':
	    usage(argv[0]);
	}
    }

    try {
	switch (test_num) {
	case 1:
	    test1(interface, real_interface);
	    break;
	case 2:
	    test2(interface);
	    break;
	case 3:
	    test3(interface);
	    break;
	default:
	    cerr << "Unknown test: " << test_num << endl;
	}
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    /*
    ** Gracefully stop and exit xlog
    */
    xlog_stop();
    xlog_exit();

    return 0;
}
