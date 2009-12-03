// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/rip/output_table.hh,v 1.10 2008/10/02 21:58:16 bms Exp $

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
 * Non-copyable due to inheritance from OutputBase<A>.
 */
template <typename A>
class OutputTable :
    public OutputBase<A>
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
    RouteWalker<A>	_rw;		// RouteWalker
    bool		_rw_valid;	// RouteWalker is valid (no reset req).
};

#endif // __RIP_OUTPUT_TABLE_HH__
