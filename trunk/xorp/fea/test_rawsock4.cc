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

#ident "$XORP: xorp/fea/test_rawsock4.cc,v 1.6 2004/06/10 22:40:57 hodson Exp $"

#include <sys/types.h>
#include <sys/uio.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#define DEBUG_LOGGING

#include <iostream>
#include <netdb.h>
#include <sysexits.h>

#include "config.h"
#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

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
/*
 * inet_cksum extracted from:
 *                      P I N G . C
 *
 * Author -
 *      Mike Muuss
 *      U. S. Army Ballistic Research Laboratory
 *      December, 1983
 * Modified at Uc Berkeley
 *
 * (ping.c) Status -
 *      Public Domain.  Distribution Unlimited.
 *
 *                      I N _ C K S U M
 *
 * Checksum routine for Internet Protocol family headers (C Version)
 *
 */
static int
inet_cksum(const u_short* addr, uint16_t len)
{
        register int nleft = (int)len;
        register const u_short *w = addr;
        u_short answer = 0;
        register int sum = 0;

        /*
         *  Our algorithm is simple, using a 32 bit accumulator (sum),
         *  we add sequential 16 bit words to it, and at the end, fold
         *  back all the carry bits from the top 16 bits into the lower
         *  16 bits.
         */
        while (nleft > 1)  {
                sum += *w++;
                nleft -= 2;
        }

        /* mop up an odd byte, if necessary */
        if (nleft == 1) {
                *(u_char *) (&answer) = *(const u_char *)w ;
                sum += answer;
        }

        /*
         * add back carry outs from top 16 bits to low 16 bits
         */
        sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
        sum += (sum >> 16);                     /* add carry */
        answer = ~sum;                          /* truncate to 16 bits */
        return (answer);
}

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
	_cksum = inet_cksum((const uint16_t*)&_type, 8);
    }

    IcmpEchoHeader(const uint8_t* buf, size_t bufbytes)
    {
	assert(bufbytes >= 8);
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
 * This class is a child of IoRawSocket and is able to send and parse
 * ICMP echo request and responses.  The class is clocked off a single
 * timeout timer.  When the timer expires it schedules the sending of
 * the next ICMP echo request.  If the last ICMP packet request has
 * not been answered when the timer expires the bad count is incremented.
 *
 * The pinger stop sending pings when the sum of the good and bad responses
 * equals the constructor argument "count".
 */

class Pinger : public IoRawSocket4 {
public:
    Pinger(EventLoop& e, IPv4 src, IPv4 dst, size_t count = 3)
	: IoRawSocket4(e, IPPROTO_ICMP),
	  _src(src), _dst(dst),
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
    bool send() {
	assert(_waiting == false);
	if (_sent == _count) {
	    return false;
	}

	_timeout.schedule_after_ms(1000);
	_seq++;

	struct ip hdr;
	static_assert(sizeof(struct ip) == 20);
	hdr.ip_v = 4;
	hdr.ip_hl = 5;
	hdr.ip_tos = 0;
	hdr.ip_len = htons(sizeof(hdr) + sizeof(IcmpEchoHeader));
	hdr.ip_id = _seq;
	hdr.ip_off = 0;
	hdr.ip_ttl = 64;
	hdr.ip_p = IPPROTO_ICMP;
	hdr.ip_sum = 0;
	hdr.ip_src.s_addr = _src.addr();
	hdr.ip_dst.s_addr = _dst.addr();
	hdr.ip_sum = inet_cksum((const uint16_t*)&hdr, sizeof(hdr));

	IcmpEchoHeader icmp_hdr(IcmpEchoHeader::TYPE_REQUEST, _id, _seq);

	size_t  tsz = sizeof(struct ip) + sizeof(IcmpEchoHeader);
	uint8_t tbuf[tsz];

	memcpy(tbuf, &hdr, sizeof(hdr));
	memcpy(tbuf + sizeof(hdr), &icmp_hdr, sizeof(IcmpEchoHeader));

	verbose_log("Sending id %d seqno %d cksum 0x%04x\n",
		    icmp_hdr.id(), icmp_hdr.seq(), icmp_hdr.cksum());

	if (write(tbuf, tsz) < 0) {
	    fprintf(stderr, "error: %d %s\n", errno, strerror(errno));
	    exit(EXIT_FAILURE);
	}

	_sent++;
	_waiting = true;
	return true;
    }

    void process_recv_data(const vector<uint8_t>& pkt)
    {
	const uint8_t* buf = &pkt[0];
	size_t bufbytes = pkt.size();

	if (bufbytes < sizeof(struct ip)) {
	    verbose_log("Ignoring short packet (%u bytes).\n",
			(uint32_t)bufbytes);
	    return;
	}

	const struct ip* hdr = reinterpret_cast<const struct ip*>(buf);
	if (hdr->ip_p != IPPROTO_ICMP) {
	    verbose_log("Ignoring non-icmp packet (ip_p = %d).\n", hdr->ip_p);
	    return;
	}

	size_t payload_bytes = ntohs(hdr->ip_len) - 4 * hdr->ip_hl;
	if (payload_bytes < sizeof(IcmpEchoHeader)) {
	    verbose_log("Ignoring small ICMP packet (payload = %u bytes).\n",
			(uint32_t)payload_bytes);
	    return;
	}

	IcmpEchoHeader eh(buf + 4 * hdr->ip_hl, payload_bytes);

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
    XorpTimer	_timeout;
    IPv4	_src;
    IPv4	_dst;
    uint16_t	_seq;
    uint16_t	_id;

    bool	_waiting; // waiting for packet

    size_t	_sent;
    size_t	_good;
    size_t	_bad;
    const size_t _count;
};

/* ------------------------------------------------------------------------- */
/* Utility resolver function */

IPv4
lookup4(const char* addr)
{
    const struct hostent* he = gethostbyname(addr);
    if (he == 0 || he->h_length < 4) {
	fprintf(stderr, "gethostbyname failed: %s\n", hstrerror(h_errno));
	exit(EXIT_FAILURE);
    }
    const in_addr* ia = reinterpret_cast<const in_addr*>(he->h_addr_list[0]);
    return IPv4(ia->s_addr);
}

/* ------------------------------------------------------------------------- */

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
    //
    // Root test, can't run if not root.
    //
    if (geteuid() != 0) {
	fprintf(stderr, "%s\n*\n", string(79, '*').c_str());
	fprintf(stderr, "* This program needs root privelages.\n");
	fprintf(stderr, "* Returning success even though not run.\n");
	fprintf(stderr, "*\n%s\n", string(79, '*').c_str());
	// Return 0 because if part of 'make check' we don't want this
	// to count as a failure.
	return EXIT_SUCCESS;
    }

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
	   dst.str().c_str(), src.str().c_str());

    try {
	EventLoop e;
	Pinger pinger(e, src, dst, count);

	while (pinger.done() == false)
	    e.run();

	printf("Received %u good responses and %u bad responses.\n",
	       (uint32_t)pinger.good(), (uint32_t)pinger.bad());
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

    return EXIT_SUCCESS;
}
