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

#ident "$XORP: xorp/rip/output_updates.cc,v 1.10 2004/09/17 13:57:14 abittau Exp $"

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
}

template <typename A>
OutputUpdates<A>::~OutputUpdates()
{
    stop_output_processing();
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
    ResponsePacketAssembler<A> rpa(this->_port);
    RipPacket<A>* pkt = new RipPacket<A>(this->ip_addr(), this->ip_port());
    rpa.packet_start(pkt);

    uint32_t done = 0;
    const RouteEntry<A>* r = 0;
    for (r = _uq.get(_uq_iter); r != 0; r = _uq.next(_uq_iter)) {
	pair<A,uint16_t> p = this->_port.route_policy(*r);

	if (p.second > RIP_INFINITY)
	    continue;

	RouteEntryOrigin<A>* origin = NULL;
	RouteEntry<A>* copy = new RouteEntry<A>(r->net(),p.first,p.second,
						origin,r->tag(),r->policytags());

	bool accepted = do_filtering(copy);
	if(!accepted) {
	    delete copy;
	    continue;
	}

	rpa.packet_add_route(copy->net(), copy->nexthop(), copy->cost(), r->tag());

	delete copy;

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
	this->_pkt_queue.enqueue_packet(pkt);
	this->_port.push_packets();
	this->_port.counters().incr_triggered_updates();
	this->incr_packets_sent();
    }

    if (r != 0) {
	// Not finished with updates so set time to reschedule self
	this->_op_timer 
	    = this->_e.new_oneoff_after_ms(this->interpacket_gap_ms(),
			callback(this, &OutputUpdates<A>::output_packet));
    } else {
	// Finished with updates for this run.  Do not set timer.
    }
}

template <typename A>
void OutputUpdates<A>::start_output_processing()
{
    if (_uq.reader_valid(_uq_iter) == false) {
	_uq_iter = _uq.create_reader();
    }
    output_packet();
}

template <typename A>
void OutputUpdates<A>::stop_output_processing()
{
    _uq.destroy_reader(_uq_iter);
    this->_op_timer.unschedule();
}


// ----------------------------------------------------------------------------
// Instantiations

#ifdef INSTANTIATE_IPV4
template class OutputUpdates<IPv4>;
#endif

#ifdef INSTANTIATE_IPV6
template class OutputUpdates<IPv6>;
#endif
