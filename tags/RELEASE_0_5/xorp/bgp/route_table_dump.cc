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

#ident "$XORP: xorp/bgp/route_table_dump.cc,v 1.10 2003/11/04 02:27:19 mjh Exp $"

//#define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "route_table_dump.hh"
#include "route_table_fanout.hh"

//#define DEBUG_CODEPATH
#ifdef DEBUG_CODEPATH
#define cp(x) printf("DumpCodePath: %2d\n", x)
#else
#define cp(x) {}
#endif

template<class A>
DumpTable<A>::DumpTable(string table_name,
			const PeerHandler *peer,
			const list <const PeerHandler*>& peer_list,
			BGPRouteTable<A> *parent_table)
    : BGPRouteTable<A>("DumpTable-" + table_name),
    _dump_iter(peer, peer_list)
{
    cp(1);
    _parent = parent_table;
    _peer = peer;
    _next_table = 0;
    _output_busy = false;
    _waiting_for_deletion_completion = false;
}

template<class A>
int
DumpTable<A>::add_route(const InternalMessage<A> &rtmsg,
			BGPRouteTable<A> *caller) 
{
    debug_msg("DumpTable<A>::add_route %x on %s\n",
	      (u_int)(&rtmsg), tablename().c_str());
    assert(caller == _parent);
    assert(_next_table != NULL);
    cp(1);
    if (_dump_iter.route_change_is_valid(rtmsg.origin_peer(),
					 rtmsg.net(),
					 rtmsg.genid(),
					 RTQUEUE_OP_ADD)) {
	cp(2);
	return _next_table->add_route(rtmsg, (BGPRouteTable<A>*)this);
    } else {
	cp(3);
	return ADD_UNUSED;
    }
}

/*
 * We can see a replace_route during a route dump because routes we've
 * already dumped may then change before the dump has completed */

template<class A>
int
DumpTable<A>::replace_route(const InternalMessage<A> &old_rtmsg,
			    const InternalMessage<A> &new_rtmsg,
			    BGPRouteTable<A> *caller) 
{
    debug_msg("DumpTable<A>::replace_route %x -> %x on %s\n",
	      (u_int)(&old_rtmsg), (u_int)(&new_rtmsg), tablename().c_str());
    assert(caller == _parent);
    assert(_next_table != NULL);
    assert(old_rtmsg.net() == new_rtmsg.net());
    cp(4);

    bool old_is_valid =
	_dump_iter.route_change_is_valid(old_rtmsg.origin_peer(),
					 old_rtmsg.net(),
					 old_rtmsg.genid(),
					 RTQUEUE_OP_REPLACE_OLD);
    bool new_is_valid =
	_dump_iter.route_change_is_valid(new_rtmsg.origin_peer(),
					 new_rtmsg.net(),
					 new_rtmsg.genid(),
					 RTQUEUE_OP_REPLACE_NEW);

    /* I'm not sure that all these combinations can happen */
    if (old_is_valid && new_is_valid) {
	cp(5);
	return _next_table->replace_route(old_rtmsg, new_rtmsg,
					  (BGPRouteTable<A>*)this);
    } else if (new_is_valid) {
	cp(6);
	return _next_table->add_route(new_rtmsg,
				      (BGPRouteTable<A>*)this);
    } else if (old_is_valid) {
	cp(7);
	return _next_table->delete_route(new_rtmsg,
					 (BGPRouteTable<A>*)this);
    } else {
	cp(8);
	return ADD_UNUSED;
    }
}

template<class A>
int
DumpTable<A>::route_dump(const InternalMessage<A> &rtmsg,
			 BGPRouteTable<A> *caller,
			 const PeerHandler* dump_peer) 
{
    cp(9);
    assert(caller == _parent);
    assert(_next_table != NULL);
    assert(dump_peer == _peer);
    debug_msg("DumpTable<A>::route_dump %s\n", rtmsg.net().str().c_str());
    /* turn the route_dump into a route_add */
    _dump_iter.route_dump(rtmsg);
    _dumped++;
    int result = _next_table->add_route(rtmsg, (BGPRouteTable<A>*)this);
    _next_table->push((BGPRouteTable<A>*)this);
    return result;
}

/*
 * We can see a delete_route during a route dump because routes we've
 * already dumped may then be deleted before the dump has completed */

template<class A>
int
DumpTable<A>::delete_route(const InternalMessage<A> &rtmsg,
			   BGPRouteTable<A> *caller) 
{
    debug_msg("DumpTable<A>::delete_route %x on %s\n",
	      (u_int)(&rtmsg), tablename().c_str());
    assert(caller == _parent);
    assert(_next_table != NULL);

    if (_dump_iter.route_change_is_valid(rtmsg.origin_peer(),
					 rtmsg.net(),
					 rtmsg.genid(),
					 RTQUEUE_OP_DELETE)) {
	cp(10);
	return _next_table->delete_route(rtmsg, (BGPRouteTable<A>*)this);
    } else {
	cp(11);
	return 0;
    }
}

template<class A>
int
DumpTable<A>::push(BGPRouteTable<A> *caller) 
{
    assert(caller == _parent);
    cp(12);
    return _next_table->push((BGPRouteTable<A>*)this);
}

template<class A>
const SubnetRoute<A>*
DumpTable<A>::lookup_route(const IPNet<A> &net) const 
{
    cp(13);
    return _parent->lookup_route(net);
}

template<class A>
void
DumpTable<A>::route_used(const SubnetRoute<A>* rt, bool in_use)
{
    cp(14);
    _parent->route_used(rt, in_use);
}

template<class A>
string
DumpTable<A>::str() const 
{
    string s = "DumpTable<A>" + tablename();
    return s;
}

template<class A>
void
DumpTable<A>::initiate_background_dump() 
{
    assert(_next_table != NULL);
    cp(15);

    _dumped = 0;
    _dump_active = true;
    _dump_timer = eventloop().
	new_oneoff_after_ms(0 /*call back immediately, but after
				network events or expired timers */,
			    callback(this,
				     &DumpTable<A>::do_next_route_dump));
}

template<class A>
void
DumpTable<A>::suspend_dump() 
{
    if (_dump_active == false) return;
    cp(16);
    _dump_active = false;
    _dump_timer.unschedule();

    // suspend is being called because the fanout table is unplumbing us.
    _next_table->set_parent(NULL);

    // ensure we can't continue to operate
    _next_table = (BGPRouteTable<A>*)0xd0d0;
    _parent = (BGPRouteTable<A>*)0xd0d0;
    delete this;
}

template<class A>
void
DumpTable<A>::do_next_route_dump() 
{
    if (_output_busy) {
	cp(17);
	// we can't really do anything yet - wait to be triggered by
	// the output state changing
	debug_msg("Dump: output busy\n");
	return;
    }

    // process any queue that built up
    while(_parent->get_next_message(this) == true) {
	cp(18);
	debug_msg("DumpTable<A>::do_next_route_dump processed queued message\n");
	if (_output_busy) {
	    cp(19);
	    return;
	}
    };

    // if we get here, the output is not busy and there's no queue of
    // changes upstream of us, so it's time to do more of the route
    // dump...
    debug_msg("dumped %d routes\n", _dumped);
    if (_dump_iter.is_valid() == false) {
	cp(20);
	if (_dump_iter.waiting_for_deletion_completion()) {
	    cp(21);
	    // go into final wait state
	    _waiting_for_deletion_completion = true;
	    // re-enable normal flow control
	    _parent->output_state(_output_busy, this);
	} else {
	    cp(22);
	    unplumb_self();
	    delete this;
	}
	cp(23);
	return;
    } else {
	cp(24);
    }

    //    if (_dump_iter.route_iterator_is_valid())
    //	debug_msg("dump route with net %p\n", _dump_iter.net().str().c_str());
    if (_parent->dump_next_route(_dump_iter) == false) {
	cp(25);
	if (_dump_iter.next_peer() == false) {
	    cp(26);
	    if (_dump_iter.waiting_for_deletion_completion()) {
		cp(27);
		// go into final wait state
		_waiting_for_deletion_completion = true;
		// re-enable normal flow control
		_parent->output_state(_output_busy, this);
	    } else {
		cp(28);
		unplumb_self();
		delete this;
	    }
	    return;
	} else {
	    cp(29);
	}
    } else {
	cp(30);
    }

    _dump_timer = eventloop().
	new_oneoff_after_ms(0 /*call back immediately, but after
				network events or expired timers */,
			    callback(this,
				     &DumpTable<A>::do_next_route_dump));
}

template<class A>
void
DumpTable<A>::output_state(bool busy, BGPRouteTable<A> *next_table) 
{
    assert(next_table == _next_table);
    if (busy)
	debug_msg("Dump: output state busy\n");
    else
	debug_msg("Dump: output state not busy\n");

    if (_waiting_for_deletion_completion) {
	cp(31);
	// we're in the final waiting phase, so just stay out of the way.
	_parent->output_state(busy, next_table);
    } else {
	cp(32);
	if (_output_busy == true && busy == false) {
	    cp(33);
	    _output_busy = false;
	    debug_msg("Dump: output went idle so dump next\n");
	    do_next_route_dump();
	} else {
	    cp(34);
	    _output_busy = busy;
	}
    }
}

template<class A>
bool
DumpTable<A>::get_next_message(BGPRouteTable<A> *next_table) 
{
    assert(next_table == _next_table);
    if (_waiting_for_deletion_completion) {
	cp(35);
	return _parent->get_next_message(this);
    } else {
	cp(36);
	debug_msg("Dump: get_next_message\n");
	/* get_next_message is normally used so that the RibOut can cause
	   the fanout table queue to drain clocked by transmit complete
	   interrupts.  But with a dump table, it's not the RibOut that
	   manages the draining of the upstream queue, so we simply return
	   false, pretending there's no queue.  If the output is no longer
	   busy, then we'll get our output_state method called, which we
	   can use to trigger the next batch of work */
	return false;
    }
}

template<class A>
void
DumpTable<A>::peering_went_down(const PeerHandler *peer, uint32_t genid,
				BGPRouteTable<A> *caller) 
{
    cp(37);
    XLOG_ASSERT(_parent == caller);
    XLOG_ASSERT(_next_table != NULL);
    _dump_iter.peering_went_down(peer, genid);
    _next_table->peering_went_down(peer, genid, this);
}

template<class A>
void
DumpTable<A>::peering_down_complete(const PeerHandler *peer, uint32_t genid,
				    BGPRouteTable<A> *caller) 
{
    cp(38);
    XLOG_ASSERT(_parent == caller);
    XLOG_ASSERT(_next_table != NULL);
    _next_table->peering_down_complete(peer, genid, this);
    _dump_iter.peering_down_complete(peer, genid);

    // If we were waiting for this signal, we can now unplumb ourselves.
    if (_waiting_for_deletion_completion
	&& (_dump_iter.waiting_for_deletion_completion() == false)) {
	cp(39);
	unplumb_self();
	delete this;
    } else {
	cp(40);
    }
}



template<class A>
void
DumpTable<A>::unplumb_self() 
{
    debug_msg("unplumbing self\n");
    assert(_next_table != NULL);
    assert(_parent != NULL || (_parent == NULL && _dump_active == false));
    _dump_active = false;

#ifdef NOTDEF
    // if the output isn't busy, tell the fanout table so it won't
    // queue changes anymore
    if (_parent != NULL) {
	_parent->output_state(_output_busy, (BGPRouteTable<A>*)this);
	assert(_parent->type() == FANOUT_TABLE);
    }
#endif
    _next_table->set_parent(_parent);
    cp(41);
    if (_parent != NULL) {
	cp(42);
	((FanoutTable<A>*)_parent)->
	    remove_next_table((BGPRouteTable<A>*)this);
	((FanoutTable<A>*)_parent)->add_next_table(_next_table, _peer);
    }
    // ensure we can't continue to operate
    _next_table = (BGPRouteTable<A>*)0xd0d0;
    _parent = (BGPRouteTable<A>*)0xd0d0;
}

template class DumpTable<IPv4>;
template class DumpTable<IPv6>;




