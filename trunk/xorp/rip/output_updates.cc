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

#ident "$XORP: xorp/rip/output_updates.cc,v 1.3 2003/08/04 23:41:10 hodson Exp $"

#include "output_updates.hh"
#include "packet_assembly.hh"
#include "packet_queue.hh"
#include "route_db.hh"

template <typename A>
OutputUpdates<A>::OutputUpdates(EventLoop&	e,
				Port<A>&	port,
				PacketQueue<A>&	pkt_queue,
				RouteDB<A>&	rdb,
				const A&	dst_addr,
				uint16_t	dst_port)
    : OutputBase<A>(e, port, pkt_queue, dst_addr, dst_port),
      _uq(rdb.update_queue())
{
    _uq_iter = _uq.create_reader();
}

template <typename A>
OutputUpdates<A>::~OutputUpdates()
{
    _uq.destroy_reader(_uq_iter);
}

template <typename A>
void
OutputUpdates<A>::ffwd()
{
    _uq.ffwd(_uq_iter);
}

template <typename A>
void
OutputUpdates<A>::output_packet()
{
    ResponsePacketAssembler<A> rpa(_port);
    RipPacket<A>* pkt = new RipPacket<A>(ip_addr(), ip_port());
    rpa.packet_start(pkt);

    uint32_t done = 0;
    const RouteEntry<A>* r = 0;
    for (r = _uq.get(_uq_iter); r != 0; r = _uq.next(_uq_iter)) {
	pair<A,uint16_t> p = _port.route_policy(*r);

	if (p.second > RIP_INFINITY)
	    continue;

	rpa.packet_add_route(r->net(), p.first, p.second, r->tag());

	done++;
	if (rpa.packet_full()) {
	    _uq.next(_uq_iter);
	    break;
	}
    }

    if (done == 0 || rpa.packet_finish() == false) {
	// No routes added to packet or error finishing packet off.
	delete pkt;
    } else {
	_pkt_queue.enqueue_packet(pkt);
	_port.push_packets();
    }

    if (r != 0) {
	// Not finished with updates so set time to reschedule self
	_op_timer = _e.new_oneoff_after_ms(interpacket_gap_ms(),
			callback(this, &OutputUpdates<A>::output_packet));
    } else {
	// Finished with updates for this run.  Do not set timer.
    }
}

template class OutputUpdates<IPv4>;
template class OutputUpdates<IPv6>;
