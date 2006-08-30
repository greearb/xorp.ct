// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2006 International Computer Science Institute
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

#ident "$XORP: xorp/fea/test_rawsock4.cc,v 1.20 2006/08/23 17:35:17 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/xorp.h"
#include "libxorp/ipvx.hh"

#include "libcomm/comm_api.h"

#include "libproto/checksum.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif

#include <iostream>

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_SYSEXITS_H
#include <sysexits.h>
#endif
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

/* Windows has no 'struct ip', so ship one. */
#ifdef HOST_OS_WINDOWS
#include "libxorp/win_io.h"
#include "ip.h"
#endif

#include "ifconfig.hh"
#include "nexthop_port_mapper.hh"
#include "rawsock4.hh"

/* ------------------------------------------------------------------------- */
/* Verbose output control */

static bool s_verbose = false;

inline bool verbose()           { return s_verbose; }
inline void set_verbose(bool v) { s_verbose = v; }

#define verbose_log(x...)                                                     \
do {                                                                          \
    if (verbose()) {                                                          \
        printf("From %s:%d: ", __FILE__, __LINE__);                           \
        printf(x);                                                            \
    }                                                                         \
} while(0)

/* ------------------------------------------------------------------------- */
/* IcmpEchoHeader */

struct IcmpEchoHeader {
    enum { TYPE_REQUEST = 8, TYPE_REPLY = 0 };

    uint8_t type() const { return _type; }
    uint8_t code() const { return _code; }
    uint16_t id() const { return ntohs(_id); }
    uint16_t seq() const { return ntohs(_seq); }
    uint16_t cksum() const { return _cksum; }

    IcmpEchoHeader(uint8_t ht, uint16_t id, uint16_t seq) :
	_type(ht), _code(0)
    {
	_id = htons(id);
	_seq = htons(seq);
	_cksum = 0;
	_cksum = inet_checksum(&_type, 8);
    }

    IcmpEchoHeader(const uint8_t* buf, size_t bufbytes)
    {
	XLOG_ASSERT(bufbytes >= 8);
	memcpy(this,  buf, 8);
    }

private:
    uint8_t  _type;
    uint8_t  _code;
    uint16_t _cksum;
    uint16_t _id;
    uint16_t _seq;
};

/* ------------------------------------------------------------------------- */
/*
 * Pinger.  Highly simplified "ping" class.
 *
 * This class is a child of FilterRawSocket4 and is able to send and parse
 * ICMP echo request and responses.  The class is clocked off a single
 * timeout timer.  When the timer expires it schedules the sending of
 * the next ICMP echo request.  If the last ICMP packet request has
 * not been answered when the timer expires the bad count is incremented.
 *
 * The pinger stop sending pings when the sum of the good and bad responses
 * equals the constructor argument "count".
 */
class Pinger : public FilterRawSocket4 {
public:
    Pinger(EventLoop& e, const IfTree& iftree, const IPv4& src,
	   const IPv4& dst, size_t count = 3)
	: FilterRawSocket4(e, IPPROTO_ICMP, iftree),
	  _iftree(iftree), _src(src), _dst(dst),
	  _sent(0), _good(0), _bad(0), _count(count)
    {
	_id = getpid();
	_seq = 0;
	_waiting = false;
	_timeout = e.new_oneoff_after_ms(0,
					 callback(this, &Pinger::timeout));
    }

    size_t good() const { return _good; }
    size_t bad() const  { return _bad; }
    size_t sent() const { return _sent; }
    size_t done() const { return _good + _bad == _count; }

protected:

    bool find_outgoing_interface(const IPvX& src, const IPvX& dst,
				 string& ifname, string& vifname) const {
	const IfTreeInterface* iftree_if = NULL;
	const IfTreeVif* iftree_vif = NULL;

	UNUSED(dst);
	ifname = "";
	vifname = "";

	if (find_interface_vif_by_addr(src, iftree_if, iftree_vif) != true)
	    return (false);

	// Interface found
	XLOG_ASSERT(iftree_if != NULL);
	XLOG_ASSERT(iftree_vif != NULL);
	ifname = iftree_if->ifname();
	vifname = iftree_vif->vifname();

	return (true);
    }

    bool send() {
	string ifname, vifname;

	XLOG_ASSERT(_waiting == false);
	if (_sent == _count) {
	    return false;
	}

	if (find_outgoing_interface(IPvX(_src), IPvX(_dst), ifname, vifname)
	    != true) {
	    fprintf(stderr, "Cannot find outgoing interface for destination "
		    "%s\n", _src.str().c_str());
	    exit(EXIT_FAILURE);
	}

	_timeout.schedule_after_ms(1000);
	_seq++;

	IcmpEchoHeader icmp_hdr(IcmpEchoHeader::TYPE_REQUEST, _id, _seq);

	vector<uint8_t> payload(sizeof(IcmpEchoHeader));
	memcpy(&payload[0], &icmp_hdr, sizeof(IcmpEchoHeader));

	verbose_log("Sending id %d seqno %d cksum 0x%04x\n",
		    icmp_hdr.id(), icmp_hdr.seq(), icmp_hdr.cksum());

	string error_msg;
	int32_t ip_ttl = 64;
	int32_t ip_tos = -1;
	if (proto_socket_write(ifname, vifname,
			       _src, _dst, ip_ttl, ip_tos,
			       false, payload, error_msg)
	    
	    != XORP_OK) {
	    fprintf(stderr, "error: %s\n", error_msg.c_str());
	    exit(EXIT_FAILURE);
	}

	_sent++;
	_waiting = true;
	return true;
    }

    void process_recv_data(const string&	if_name,
			   const string&	vif_name,
			   const IPvX&		src_address,
			   const IPvX&		dst_address,
			   int32_t		ip_ttl,
			   int32_t		ip_tos,
			   bool			ip_router_alert,
			   const vector<uint8_t>& ext_headers_type,
			   const vector<vector<uint8_t> >& ext_headers_payload,
			   const vector<uint8_t>& payload)
    {
	UNUSED(if_name);
	UNUSED(vif_name);
	UNUSED(src_address);
	UNUSED(dst_address);
	UNUSED(ip_ttl);
	UNUSED(ip_tos);
	UNUSED(ip_router_alert);
	UNUSED(ext_headers_type);
	UNUSED(ext_headers_payload);

	if (payload.size() < sizeof(IcmpEchoHeader)) {
	    verbose_log("Ignoring small ICMP packet (payload = %u bytes).\n",
			XORP_UINT_CAST(payload.size()));
	    return;
	}

	IcmpEchoHeader eh(&payload[0], payload.size());

	if (eh.id() != _id) {
	    verbose_log("Ignoring packet with id %d (!= %d)\n", eh.id(), _id);
	    return;
	}

	if (eh.seq() != _seq) {
	    verbose_log("Unexpected seqno %d (!= %d)\n", eh.seq(), _seq);
	    return;
	}

	if (eh.code() != 0) {
	    verbose_log("Unexpected code %d (!= %d)\n", eh.code(), 0);
	    return;
	}

	if (eh.type() == IcmpEchoHeader::TYPE_REQUEST) {
	    verbose_log("Ignoring my outbound ICMP packet\n");
	    return;
	} else if (eh.type() != IcmpEchoHeader::TYPE_REPLY) {
	    verbose_log("Ignoring ICMP packet type %d\n", eh.type());
	    return;
	}

	if (_waiting) {
	    printf("Sequence number %d received.\n", _seq);
	    _good++;
	    _waiting = false;
	}
	verbose_log("Receiving id %d, seqno %d\n", eh.id(), eh.seq());
    }

    void timeout() {
	if (_waiting) {
	    printf("Sequence number %d timed out.\n", _seq);
	    _bad++;
	}
	_waiting = false;
	send();
    }

protected:
    const IfTree&	_iftree;
    XorpTimer		_timeout;
    IPv4		_src;
    IPv4		_dst;
    uint16_t		_seq;
    uint16_t		_id;

    bool		_waiting; // waiting for packet

    size_t		_sent;
    size_t		_good;
    size_t		_bad;
    const size_t	_count;
};

/* ------------------------------------------------------------------------- */
/* Utility resolver function */

IPv4
lookup4(const char* addr)
{
    const struct hostent* he = gethostbyname(addr);
    if (he == 0 || he->h_length < 4) {
	fprintf(stderr, "gethostbyname failed: %s %d\n",
#ifdef HAVE_HSTRERROR
		hstrerror(h_errno),
#elif HOST_OS_WINDOWS
		win_strerror(WSAGetLastError()),
#else
		"",
#endif
#ifdef HOST_OS_WINDOWS
		WSAGetLastError()
#else
		h_errno
#endif
	);
	exit(EXIT_FAILURE);
    }
    struct in_addr ia;
    memcpy(&ia, he->h_addr_list[0], sizeof(ia));
    return IPv4(ia.s_addr);
}

/* ------------------------------------------------------------------------- */

#ifndef EX_USAGE
#define EX_USAGE 1
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

static void
usage()
{
    fprintf(stderr, "usage: test_rawsock [-v] [-c count] [dst [src]]\n");
    fprintf(stderr,
	    "pings dst from address src.  Addresses default to localhost\n"
	    );
    exit(EX_USAGE);
}

/* ------------------------------------------------------------------------- */

int
main(int argc, char* const* argv)
{
#ifndef HOST_OS_WINDOWS
    //
    // Root test, can't run if not root.
    //
    if (geteuid() != 0) {
	fprintf(stderr, "%s\n*\n", string(79, '*').c_str());
	fprintf(stderr, "* This program needs root privileges.\n");
	fprintf(stderr, "* Returning success even though not run.\n");
	fprintf(stderr, "*\n%s\n", string(79, '*').c_str());
	// Return 0 because if part of 'make check' we don't want this
	// to count as a failure.
	return EXIT_SUCCESS;
    }
#endif

    comm_init();

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);         // Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    int count = 3;

    int ch;
    while ((ch = getopt(argc, argv, "c:hv")) != -1) {
	switch (ch) {
	case 'c':
	    count = atoi(optarg);
	    break;
	case 'v':
	    set_verbose(true);
	    break;
	default:
	    usage();
	    break;
	}
    }
    argc -= optind;
    argv += optind;

    if (argc > 2) {
	usage();
    }

    if (count <= 0) count = 3;

    IPv4 dst, src;
    switch (argc) {
    case 0:
	src = IPv4::LOOPBACK();
	dst = IPv4::LOOPBACK();
	break;
    case 1:
	{
	    char hostname[MAXHOSTNAMELEN];
	    if (gethostname(hostname, sizeof(hostname))) {
		fprintf(stderr, "gethostname() failed: %s", strerror(errno));
		exit(EXIT_FAILURE);
	    }
	    hostname[sizeof(hostname) - 1] = '\0';
	    src = lookup4(hostname);
	    dst = lookup4(argv[0]);
	}
	break;
    case 2:
	src = lookup4(argv[1]);
	dst = lookup4(argv[0]);
	break;
    }

    printf("Running \"ping\" from %s to %s\n",
	   src.str().c_str(), dst.str().c_str());

    try {
	EventLoop eventloop;
	string error_msg;

	//
	// Create the interface tree
	//
	NexthopPortMapper nexthop_port_mapper;
	IfConfigUpdateReplicator ifc_repl;
	IfConfigErrorReporter if_err;

	IfConfig ifconfig(eventloop, ifc_repl, if_err, nexthop_port_mapper);
	if (ifconfig.start(error_msg) != XORP_OK) {
	    XLOG_FATAL("Cannot start IfConfig: %s", error_msg.c_str());
	}
	const IfTree& iftree = ifconfig.pull_config();

	Pinger pinger(eventloop, iftree, src, dst, count);
	if (pinger.start(error_msg) != XORP_OK) {
	    fprintf(stderr, "Cannot start pinger: %s", error_msg.c_str());
	    exit(EXIT_FAILURE);
	}
	
	while (pinger.done() == false)
	    eventloop.run();

	printf("Received %u good responses and %u bad responses.\n",
	       XORP_UINT_CAST(pinger.good()), XORP_UINT_CAST(pinger.bad()));
	if (pinger.good() == 0) {
	    exit(EXIT_FAILURE);
	}
    } catch (...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    comm_exit();

    return EXIT_SUCCESS;
}
