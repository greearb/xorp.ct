// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8: 

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

#ident "$XORP: xorp/rip/output_table.cc,v 1.10 2004/09/17 13:57:14 abittau Exp $"

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

    ResponsePacketAssembler<A> rpa(this->_port);
    RipPacket<A>* pkt = new RipPacket<A>(this->ip_addr(), this->ip_port());
    rpa.packet_start(pkt);

    uint32_t done = 0;
    const RouteEntry<A>* r = 0;
    for (r = _rw.current_route(); r != 0; r = _rw.next_route()) {

	// We may either "drop the packet..."
	// or set cost to infinity...
	// or depending on poison-reverse / horizon settings
	if(r->filtered()) {
	    continue;
	}    

	pair<A,uint16_t> p = this->_port.route_policy(*r);

	if (p.second > RIP_INFINITY) {
	    continue;
	}

	RouteEntryOrigin<A>* origin = NULL; // XXX
	RouteEntry<A>* copy = new RouteEntry<A>(r->net(),p.first,p.second,
					        origin,r->tag(),r->policytags());
	
	bool accepted = do_filtering(copy);
	if(!accepted) {
	    delete copy;
	    continue;
	}

	rpa.packet_add_route(copy->net(), copy->nexthop(), copy->cost(), copy->tag());

	// XXX: packet_add_route stores args
	delete copy;
	
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
	this->_pkt_queue.enqueue_packet(pkt);
	this->_port.push_packets();
	if (this->ip_port() == RIP_AF_CONSTANTS<A>::IP_PORT) {
	    this->_port.counters().incr_unsolicited_updates();
	} else {
	    this->_port.counters().incr_non_rip_updates_sent();
	}
	this->incr_packets_sent();
    }

    if (r == 0) {
	// Reached null route so note route walker is now invalid.
	_rw_valid = false;
    } else {
	// Not finished so set time to reschedule self and pause
	// route walker.
	this->_op_timer 
	    = this->_e.new_oneoff_after_ms(this->interpacket_gap_ms(),
                          callback(this, &OutputTable<A>::output_packet));
	_rw.pause(this->interpacket_gap_ms());
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
    this->_op_timer.unschedule();	// stop timer
}


// ----------------------------------------------------------------------------
// Instantiations

#ifdef INSTANTIATE_IPV4
template class OutputTable<IPv4>;
#endif

#ifdef INSTANTIATE_IPV6
template class OutputTable<IPv6>;
#endif
