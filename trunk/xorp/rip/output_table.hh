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

// $XORP: xorp/rip/output_table.hh,v 1.2 2003/08/01 17:10:44 hodson Exp $

#ifndef __RIP_OUTPUT_TABLE_HH__
#define __RIP_OUTPUT_TABLE_HH__

#include "rip_module.h"
#include "libxorp/xlog.h"

#include "output.hh"
#include "route_db.hh"

/**
 * @short Route Table Output class.
 *
 * The OutputTable class produces an asynchronous RIP table dump. It's
 * intended use is for solicited and unsolicited routing table.
 *
 * Specialized implementations exist for IPv4 and IPv6.
 */
template <typename A>
class OutputTable : public OutputBase<A>
{
public:
    OutputTable(EventLoop&	e,
		Port<A>&	port,
		PacketQueue<A>&	pkt_queue,
		RouteDB<A>&	rdb,
		const A&	ip_addr = RIP_AF_CONSTANTS<A>::IP_GROUP(),
		uint16_t	ip_port = RIP_AF_CONSTANTS<A>::IP_PORT)
	: OutputBase<A>(e, port, pkt_queue, ip_addr, ip_port),
	  _rw(rdb), _rw_valid(false)
    {}

protected:
    void output_packet();

    void start_output_processing();

    void stop_output_processing();

private:
    OutputTable(const OutputTable<A>& o);		// Not implemented
    OutputTable<A>& operator=(const OutputTable<A>& o);	// Not implemented

private:
    RouteWalker<A>	_rw;		// RouteWalker
    bool		_rw_valid;	// RouteWalker is valid (no reset req).
};

#endif // __RIP_OUTPUT_TABLE_HH__
