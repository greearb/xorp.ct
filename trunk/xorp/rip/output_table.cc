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

#ident "$XORP: xorp/rip/output_table.cc,v 1.5 2004/02/20 01:22:03 hodson Exp $"

#include "output_table.hh"
#include "packet_assembly.hh"
#include "packet_queue.hh"
#include "route_db.hh"

template <typename A>
void
OutputTable<A>::output_packet()
{
    if (_rw_valid == false) {
	_rw.reset();
	_rw_valid = true;
    }
    _rw.resume();

    ResponsePacketAssembler<A> rpa(_port);
    RipPacket<A>* pkt = new RipPacket<A>(ip_addr(), ip_port());
    rpa.packet_start(pkt);

    uint32_t done = 0;
    const RouteEntry<A>* r = 0;
    for (r = _rw.current_route(); r != 0; r = _rw.next_route()) {
	pair<A,uint16_t> p = _port.route_policy(*r);

	if (p.second > RIP_INFINITY) {
	    continue;
	}

	rpa.packet_add_route(r->net(), p.first, p.second, r->tag());
	done++;
	if (rpa.packet_full()) {
	    _rw.next_route();
	    break;
	}
    }

    if (done == 0 || rpa.packet_finish() == false) {
	// No routes added to packet or error finishing packet off.
	delete pkt;
    } else {
	_pkt_queue.enqueue_packet(pkt);
	_port.push_packets();
	if (ip_port() == RIP_AF_CONSTANTS<A>::IP_PORT)
	    _port.counters().incr_unsolicited_updates();
	incr_packets_sent();
    }

    if (r == 0) {
	// Reached null route so note route walker is now invalid.
	_rw_valid = false;
    } else {
	// Not finished so set time to reschedule self and pause
	// route walker.
	_op_timer = _e.new_oneoff_after_ms(interpacket_gap_ms(),
			callback(this, &OutputTable<A>::output_packet));
	_rw.pause(interpacket_gap_ms());
    }
}

template <typename A>
void
OutputTable<A>::start_output_processing()
{
    output_packet();		// starts timer
}

template <typename A>
void
OutputTable<A>::stop_output_processing()
{
    _op_timer.unschedule();	// stop timer
}


// ----------------------------------------------------------------------------
// Instantiations

#ifdef INSTANTIATE_IPV4
template class OutputTable<IPv4>;
#endif

#ifdef INSTANTIATE_IPV6
template class OutputTable<IPv6>;
#endif
