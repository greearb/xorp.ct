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

// $XORP: xorp/fea/fti_rtsock.hh,v 1.1.1.1 2002/12/11 23:56:02 hodson Exp $

#ifndef	__FEA_FTI_RTSOCK_HH__
#define __FEA_FTI_RTSOCK_HH__

#include <list>
#include "libxorp/xorp.h"
#include "rtsock.hh"

class RoutingSocketFti : public Fti, public RoutingSocketObserver {
public:
    RoutingSocketFti(RoutingSocket& rs);
    ~RoutingSocketFti();

    bool start_configuration();
    bool end_configuration();

    /* IPv4 Methods */
    bool add_entry4(const Fte4& fte);
    bool delete_entry4(const Fte4& fte);
    bool delete_all_entries4();

    bool lookup_route4(IPv4 addr, Fte4& fte);
    bool lookup_entry4(const IPv4Net& netmask, Fte4& fte);

    /* IPv6 Methods */
    bool add_entry6(const Fte6& fte);
    bool delete_entry6(const Fte6& fte);
    bool delete_all_entries6();

    bool lookup_route6(const IPv6& addr, Fte6& fte);
    bool lookup_entry6(const IPv6Net& netmask, Fte6& fte);

    void rtsock_data(const uint8_t*, size_t bytes);

private:
    struct rtp {
	enum dyn { CALL_DELETE, DONT_CALL_DELETE };
	dyn _dyn;

	caddr_t _table;		/* Routing table */
	caddr_t _current;	/* Current pointer into the routing table */
	caddr_t _end;		/* End of routing table */

	rtp();
	void init(dyn dyn, caddr_t table, caddr_t current, caddr_t end);
	~rtp();
    };

    bool make_routing_list(list<Fte4>& rt);
    bool read_routing_table(rtp& rtp);
    void init_table(rtp& rtp, rtp::dyn dyn, caddr_t start, int size);
    bool next_route(rtp& rtp, struct rt_msghdr *& hdr, char *& end);
    bool parse_route(struct rt_msghdr *hdr, caddr_t end, Fte4& fte,
		     int flags_on, int flags_off);

    void init_sock();

    struct sockaddr *next_sock(struct rt_msghdr *hdr, int& bits);
    bool sockaddr(struct sockaddr *sock, IPv4& addr);

protected:
    // Not implemented
    RoutingSocketFti(const RoutingSocketFti&);
    RoutingSocketFti& operator=(const RoutingSocketFti&);

protected:
    bool _in_configuration;

    list<Fte4> _routing_table;
    list<Fte4>::iterator _current_entry;

    struct sockaddr *_sock;
    int _addrs;
    int _bit_count;

    uint32_t	    _cache_seqno;	/* Seqno of routing socket data to
					 * cache so route lookup via routing
					 * socket can appear synchronous. */
    bool	    _cache_valid;	/* Cache data arrived. */
    vector<uint8_t> _cache_rtdata;	/* Cached routing socket data
					 * route lookup uses to determine
					 * response. */
};

#endif	// __FEA_FTI_RTSOCK_HH__
