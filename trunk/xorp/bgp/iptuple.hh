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

// $XORP: xorp/bgp/iptuple.hh,v 1.3 2003/03/10 23:19:58 hodson Exp $

#ifndef __BGP_IPTUPLE_HH__
#define __BGP_IPTUPLE_HH__

#include "bgp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

class UnresolvableHost : public XorpReasonedException {
public:
    UnresolvableHost(const char* file, size_t line, const string init_why = "")
 	: XorpReasonedException("UnresolvableHost", file, line, init_why) {}
};

/**
 * Store the Local Interface, Local Server Port, Peer Interface and
 * Peer Server Port tuple.
 */
class Iptuple {
public:
    Iptuple();
    Iptuple(const char *local_interface, uint16_t local_port,
	    const char *peer_interface, uint16_t peer_port)
	throw(UnresolvableHost);

    Iptuple(const IPv4& local_up,  uint16_t local_port,
	    const IPv4& peer_up, uint16_t peer_port);

    Iptuple(const Iptuple&);
    Iptuple operator=(const Iptuple&);
    void copy(const Iptuple&);

    bool operator==(const Iptuple&) const;

    static in_addr get_addr(const char *host) throw(UnresolvableHost);

    struct in_addr get_local_addr() const;
    uint16_t get_local_port() const;

    struct in_addr get_peer_addr() const;
    uint16_t get_peer_port() const;

    string str() const;
private:
    string _local_interface;	// String representation only for debugging.
    string _peer_interface;	// String representation only for debugging.

    /*
    ** All held in network byte order
    */
    struct in_addr _local;	// Local interface.
    uint16_t _local_port;
    struct in_addr _peer;	// Peer interface.
    uint16_t _peer_port;
};

#endif // __BGP_IPTUPLE_HH__
