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

#ident "$XORP: xorp/bgp/test_packet.cc,v 1.5 2003/09/27 03:17:53 atanu Exp $"

#include "bgp_module.h"
#include "config.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "test_packet.hh"
#include "socket.hh"

/* **************** main *********************** */

int main(int /* argc */, char *argv[])
{
    BGPTestPacket tp;
    
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();
    
    int retval = tp.run_tests();
    
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    return retval;
}

BGPTestPacket::BGPTestPacket()
{
}

int
BGPTestPacket::run_tests()
{
    // Run tests
    
    struct test {
	string test_name;
	bool (BGPTestPacket::*func)();
    } tests[] = {
	{"Keep alive",	&BGPTestPacket::test_keepalive},
	{"Open", &BGPTestPacket::test_open},
	{"Update IPv4", &BGPTestPacket::test_update},
	{"Update IPv6", &BGPTestPacket::test_update_ipv6},
	{"Notification", &BGPTestPacket::test_notification},
	{"AS path", &BGPTestPacket::test_aspath},
    };

    bool failed = false;
    for(unsigned int i = 0; i < sizeof(tests) / sizeof(struct test); i++) {
	printf("%-15s", tests[i].test_name.c_str());
	if(!((*this).*tests[i].func)()) {
	    printf("FAILED\n");
	    failed = true;
	} else {
	    printf("passed\n");
	}
    }
	    
    return !failed ? 0 : -1;
}

bool BGPTestPacket::test_keepalive()
{
    // test packet writing.
    // create a KeepAlive packet and then read it back.
    bool result;

    
    debug_msg("Starting test of KeepAlivePacket\n\n");
    debug_msg("Encoding KeepAlivePacket\n\n");
    
    KeepAlivePacket *ka = create_keepalive();
    size_t len;
    const uint8_t *buf = ka->encode(len);

    debug_msg("Decoding KeepAlivePacket\n\n");
    KeepAlivePacket* keepalivepacket = new KeepAlivePacket(buf,len);
    
    debug_msg("Ending test of KeepAlivePacket\n\n");
    
    result = (*ka == *keepalivepacket);

    delete keepalivepacket;
    delete ka;
    delete[] buf;
    
    return result;
}


bool BGPTestPacket::test_open()
{
    // test packet writing.
    // create a Open packet and then read it back.
    bool result;

    debug_msg("Starting test of OpenPacket\n\n");
    debug_msg("Encoding OpenPacket\n\n");
    
    OpenPacket *op = create_open();
    size_t len;
    const uint8_t *buf = op->encode(len);
    
    debug_msg("Decoding OpenPacket\n\n");
    
    OpenPacket *openpacket;
    try {
	openpacket = new OpenPacket(buf,len);
    } catch (CorruptMessage &err) {
	debug_msg("Construction of UpdatePacket from buffer failed\n");
	delete op;
	delete[] buf;
	return false;
    }
    
    debug_msg("Ending test of OpenPacket\n\n");
    
    result = (*openpacket == *op);

    delete openpacket;
    delete op;
    delete[] buf;
    
    return result;
}

bool BGPTestPacket::test_update()
{
    // test packet writing.
    // create a Update packet and then read it back.
    bool result;

    debug_msg("Starting test of UpdatePacket\n\n");
    debug_msg("Encoding UpdatePacket\n\n");
    
    UpdatePacket *up = create_update();
    size_t len;
    const uint8_t *buf = up->encode(len);
    
    debug_msg("Decoding UpdatePacket\n\n");
    
    UpdatePacket *updatepacket;
    try {
	updatepacket = new UpdatePacket(buf,len);
    } catch (CorruptMessage &err) {
	debug_msg("Construction of UpdatePacket from buffer failed\n");
	delete up;
	delete[] buf;
	return false;
    }
    
    debug_msg("Ending test of UpdatePacket\n\n");
    
    result = (*updatepacket == *up);

    delete updatepacket;
    delete up;
    delete[] buf;

    return result;
}

bool BGPTestPacket::test_update_ipv6()
{
    // test packet writing.
    // create a Update packet and then read it back.
    bool result;

    debug_msg("Starting test of UpdatePacket IPv6\n\n");
    debug_msg("Encoding UpdatePacket IPv6\n\n");
    
    UpdatePacket *up = create_update_ipv6();
    size_t len;
    const uint8_t *buf = up->encode(len);
    
    debug_msg("Decoding UpdatePacket IPv6\n\n");
    
    UpdatePacket *updatepacket;
    try {
	updatepacket = new UpdatePacket(buf,len);
    } catch (CorruptMessage &err) {
	debug_msg("Construction of UpdatePacket IPv6 from buffer failed\n");
	delete up;
	delete[] buf;
	return false;
    }
    
    debug_msg("Ending test of UpdatePacket IPv6\n\n");
    
    result = (*updatepacket == *up);

    delete updatepacket;
    delete up;
    delete[] buf;

    return result;
}

bool BGPTestPacket::test_notification()
{
    // test packet writing.
    // create a Notification packet and then read it back.
    bool result;

    debug_msg("Starting test of NotificationPacket\n\n");
    debug_msg("Encoding NotificationPacket\n\n");
    
    NotificationPacket *np = create_notification();
    size_t len;
    const uint8_t *buf = np->encode(len);
    
    debug_msg("Decoding NotificationPacket\n\n");
    NotificationPacket *notificationpacket;
    try {
	notificationpacket = new NotificationPacket(buf,len);
    } catch (InvalidPacket& err) {
	debug_msg("Construction of NotificationPacket from buffer failed\n");
	delete np;
	delete[] buf;
	return false;
    }

    debug_msg("Ending test of NotificationPacket\n\n");
    
    result = (*notificationpacket == *np);

    delete notificationpacket;
    delete np;
    delete[] buf;
    
    return result;
}

KeepAlivePacket* BGPTestPacket::create_keepalive()
{
    KeepAlivePacket* p = new KeepAlivePacket();
    //uint8_t m[16] = {255,1,255,2,255,3,255,4,255,5,255,6,255,7,255,8};
    // p->set_marker(m); // XXX no support for random markers right now.
    return p;
}

UpdatePacket* BGPTestPacket::create_update()
{
    IPv4Net net[3];
    net[0] = IPv4Net("1.2.3.4/32");
    net[1] = IPv4Net("5.6.7.8/32");
    net[2] = IPv4Net("1.2.3.4/32");
    BGPUpdateAttrib wdr(net[2]);
	
    AsSegment as_seq;
    as_seq.set_type(AS_SEQUENCE);
    debug_msg("sequence length : %u\n", (uint32_t)as_seq.as_size());
    as_seq.add_as(AsNum(12));
    debug_msg("sequence length : %u\n", (uint32_t)as_seq.as_size());
    as_seq.add_as(AsNum(13));
    debug_msg("sequence length : %u\n", (uint32_t)as_seq.as_size());
    as_seq.add_as(AsNum(14));
    AsPath p;
    p.add_segment(as_seq);
    debug_msg("sequence length : %u\n", (uint32_t)as_seq.as_size());
    BGPUpdateAttrib nlr_0(net[0]);
    BGPUpdateAttrib nlr_1(net[1]);	
    // nlr_0.dump();
    // nlr_1.dump();
    // wdr.dump();
    ASPathAttribute path_att(p);
    UpdatePacket *bup = new UpdatePacket();
    bup->add_withdrawn(wdr);
    bup->add_pathatt(path_att);
    bup->add_nlri(nlr_0);
    bup->add_nlri(nlr_1);
    return bup;
}

UpdatePacket* BGPTestPacket::create_update_ipv6()
{
    IPv4Net net[3];
    net[0] = IPv4Net("1.2.3.4/32");
    net[1] = IPv4Net("5.6.7.8/32");
    net[2] = IPv4Net("1.2.3.4/32");
    BGPUpdateAttrib wdr(net[2]);
	
    AsSegment as_seq;
    as_seq.set_type(AS_SEQUENCE);
    debug_msg("sequence length : %u\n", (uint32_t)as_seq.as_size());
    as_seq.add_as(AsNum(12));
    debug_msg("sequence length : %u\n", (uint32_t)as_seq.as_size());
    as_seq.add_as(AsNum(13));
    debug_msg("sequence length : %u\n", (uint32_t)as_seq.as_size());
    as_seq.add_as(AsNum(14));
    AsPath p;
    p.add_segment(as_seq);
    debug_msg("sequence length : %u\n", (uint32_t)as_seq.as_size());
    BGPUpdateAttrib nlr_0(net[0]);
    BGPUpdateAttrib nlr_1(net[1]);	
    // nlr_0.dump();
    // nlr_1.dump();
    // wdr.dump();
    ASPathAttribute path_att(p);
    UpdatePacket *bup = new UpdatePacket();
    bup->add_withdrawn(wdr);
    bup->add_pathatt(path_att);
    bup->add_nlri(nlr_0);
    bup->add_nlri(nlr_1);

    MPReachNLRIAttribute<IPv6> mpreach(SAFI_UNICAST);
    mpreach.set_nexthop("20:20:20:20:20:20:20:20");
    mpreach.add_nlri("2000::/3");
    mpreach.encode();
    debug_msg("%s\n", mpreach.str().c_str());
    bup->add_pathatt(mpreach);
    debug_msg("%s\n", bup->str().c_str());

    MPUNReachNLRIAttribute<IPv6> mpunreach(SAFI_UNICAST);
    mpunreach.add_withdrawn("2000::/3");
    mpunreach.encode();
    bup->add_pathatt(mpunreach);

    return bup;
}

OpenPacket* BGPTestPacket::create_open()
{
    /* commented out since not sure if authentication data is correctly being set here */

    //uint8_t data[16] = {8,6,3,4,5,6,7,8,9,0,1,2,3,4,5,6};
    //BGPAuthParameter* p = new BGPAuthParameter();
    //p->set_length(6);
    //p->set_authcode(1);
    //p->set_authdata(data);

    //uint8_t data2[16] = {9,7,3,4,5,6,7,8,9,0,1,2,3,4,5,6};
    //BGPAuthParameter* p2 = new BGPAuthParameter();
    //p2->set_length(6);
    //p2->set_authcode(2);
    //p2->set_authdata(data2);
	
    //ld->add_parameter(p);
    //ld->add_parameter(p2);

    OpenPacket* op = new OpenPacket(AsNum(1234),
					  IPv4("192.168.1.1"),
					  20 /* holdtime */);
	
    //delete ld;
    return op;
}

NotificationPacket* BGPTestPacket::create_notification()
{
	return new NotificationPacket(1,1);
}

bool BGPTestPacket::test_aspath()
{
    AsNum* asnum1 = new AsNum(16);
    AsNum* asnum2 = new AsNum(16);
    AsNum* asnum3 = new AsNum(17); 
    AsNum* asnum4 = new AsNum((uint32_t)16);
    AsNum* asnum5 = new AsNum((uint32_t)16);

    if (*asnum1 == *asnum2)
	debug_msg("AS 16 == AS16\n");
    else
	debug_msg("AS 16 != AS16\n");

    if (*asnum1 == *asnum3)
	debug_msg("AS 16 == AS17\n");
    else
	debug_msg("AS 16 != AS17\n");

    if (*asnum1 == *asnum4)
	debug_msg("AS 16 == AS16 (extended)\n");
    else
	debug_msg("AS 16 != AS16 (extended)\n");

    if (*asnum4 == *asnum5)
	debug_msg("AS 16 (extended) == AS16 (extended)\n");
    else
	debug_msg("AS 16 (extended) != AS16 (extended)\n");

    delete asnum1;
    delete asnum2;
    delete asnum3;
    delete asnum4;
    delete asnum5;
    // dummy return value;
    return true;
}
