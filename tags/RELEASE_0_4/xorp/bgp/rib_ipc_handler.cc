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

#ident "$XORP: xorp/bgp/rib_ipc_handler.cc,v 1.20 2003/06/20 18:55:56 hodson Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

// XXX - As soon as this works remove the define and make the code unconditonal
#define FLOW_CONTROL

// Give every XRL in flight the opportnity to timeout
#define MAX_ERR_RETRIES FLYING_LIMIT

#include "bgp_module.h"
#include "main.hh"
#include "rib_ipc_handler.hh"

RibIpcHandler::RibIpcHandler(XrlStdRouter *xrl_router, EventLoop& eventloop, 
			     BGPMain& bgp) 
    : PeerHandler("RIBIpcHandler", NULL, NULL), 
    _ribname(""),
      _xrl_router(xrl_router), _eventloop(eventloop),
    _interface_failed(false),
    _v4_queue(this, xrl_router, &bgp),
    _v6_queue(this, xrl_router, &bgp)
{
}

RibIpcHandler::~RibIpcHandler() 
{
    if(_v4_queue.busy() || _v6_queue.busy())
	XLOG_WARNING("Deleting RibIpcHandler with callbacks pending");

    set_plumbing(NULL);

    if("" != _ribname)
	XLOG_WARNING("Deleting RibIpcHandler while still registered with RIB");
    /*
    ** If would be great to de-register from the RIB here. The problem
    ** is that if we start a de-register the callback will return to a
    ** freed data structure.
    */
}

bool
RibIpcHandler::register_ribname(const string& r)
{
    if(_ribname == r)
	return true;

    bool res;
    if("" == r) {
	res = unregister_rib();
    }

    _ribname = r;

    if("" == r) {
	return res;
    }

    XrlRibV0p1Client rib(_xrl_router);
    
    //create our tables
    //ebgp - v4
    //name - "ebgp"
    //unicast - true
    //multicast - false
    rib.send_add_egp_table4(_ribname.c_str(),
			    "ebgp", true, false,
			    callback(this, 
				     &RibIpcHandler::rib_command_done,"add_table"));
    //ibgp - v4
    //name - "ibgp"
    //unicast - true
    //multicast - false
    rib.send_add_egp_table4(_ribname.c_str(),
			    "ibgp", true, false,
			    callback(this, 
				     &RibIpcHandler::rib_command_done,"add_table"));

    //create our tables
    //ebgp - v6
    //name - "ebgp"
    //unicast - true
    //multicast - false
//     rib.send_add_egp_table6(_ribname.c_str(),
// 		"ebgp", true, false,
// 		callback(this, 
    // 		         &RibIpcHandler::rib_command_done,"add_table"));
    //ibgp - v6
    //name - "ibgp"
    //unicast - true
    //multicast - false
//     rib.send_add_igp_table6(_ribname.c_str(),
// 		  "ibgp", true, false,
// 		  callback(this,
//			   &RibIpcHandler::rib_command_done,"add_table"));

    return true;
}

bool
RibIpcHandler::unregister_rib()
{
    XrlRibV0p1Client rib(_xrl_router);
    
    //delete our tables
    //ebgp - v4
    //name - "ebgp"
    //unicast - true
    //multicast - false
    rib.send_delete_egp_table4(_ribname.c_str(),
			       "ebgp", true, false,
			       callback(this,
					&RibIpcHandler::rib_command_done,
					"delete_table"));
    //ibgp - v4
    //name - "ibgp"
    //unicast - true
    //multicast - false
    rib.send_delete_igp_table4(_ribname.c_str(),
			       "ibgp", true, false,
			       callback(this,
					&RibIpcHandler::rib_command_done,
					"delete_table"));

    //create our tables
    //ebgp - v6
    //name - "ebgp"
    //unicast - true
    //multicast - false
//     rib.send_delete_egp_table6(_ribname.c_str(),
// 			       "ebgp", true, false,
// 			       callback(this,
// 				  	&RibIpcHandler::rib_command_done,
// 					"delete_table"));

    //ibgp - v6
    //name - "ibgp"
    //unicast - true
    //multicast - false
//     rib.send_delete_igp_table6(_ribname.c_str(),
// 			       "ibgp", true, false,
// 			       callback(this,
// 					&RibIpcHandler::rib_command_done,
// 					"delete_table"));

    return true;
}

int 
RibIpcHandler::start_packet(bool ibgp) 
{
    _ibgp = ibgp;
    debug_msg("RibIpcHandler::start packet\n");
    return 0;
}

int 
RibIpcHandler::add_route(const SubnetRoute<IPv4> &rt) 
{
    debug_msg("RibIpcHandler::add_route(IPv4) %x\n", (u_int)(&rt));

    if("" == _ribname)
	return 0;

    _v4_queue.queue_add_route(_ribname, _ibgp, rt.net(), rt.nexthop());

    return 0;
}

int 
RibIpcHandler::add_route(const SubnetRoute<IPv6>& rt) {
    debug_msg("RibIpcHandler::add_route(IPv6) %p\n", &rt);

    if ("" == _ribname)
	return 0;

    _v6_queue.queue_add_route(_ribname, _ibgp, rt.net(), rt.nexthop());

    return 0;
}

int 
RibIpcHandler::replace_route(const SubnetRoute<IPv4> &old_rt,
			     const SubnetRoute<IPv4> &new_rt) {
    debug_msg("RibIpcHandler::replace_route(IPv4) %p %p\n", &old_rt, &new_rt);
    delete_route(old_rt);
    add_route(new_rt);
    return 0;
}

int 
RibIpcHandler::replace_route(const SubnetRoute<IPv6> &old_rt,
			     const SubnetRoute<IPv6> &new_rt) {
    debug_msg("RibIpcHandler::replace_route(IPv6) %p %p\n", &old_rt, &new_rt);
    delete_route(old_rt);
    add_route(new_rt);
    return 0;
}

int 
RibIpcHandler::delete_route(const SubnetRoute<IPv4> &rt)
{
    debug_msg("RibIpcHandler::delete_route(IPv4) %x\n", (u_int)(&rt));

    if("" == _ribname)
	return 0;

    _v4_queue.queue_delete_route(_ribname, _ibgp, rt.net());

    return 0;
}

int 
RibIpcHandler::delete_route(const SubnetRoute<IPv6>& rt)
{
    debug_msg("RibIpcHandler::delete_route(IPv6) %p\n", &rt);
    UNUSED(rt);
    if("" == _ribname)
	return 0;

    _v6_queue.queue_delete_route(_ribname, _ibgp, rt.net());

    return 0;
}

void
RibIpcHandler::rib_command_done(const XrlError& error, const char *comment)
{
    debug_msg("callback %s %s\n", comment, error.str().c_str());
    if(XrlError::OKAY() != error) {
	XLOG_WARNING("callback: %s %s",  comment, error.str().c_str());
    }
}

PeerOutputState
RibIpcHandler::push_packet()
{
    debug_msg("RibIpcHandler::push packet\n");

#ifdef	FLOW_CONTROL
    if(_v4_queue.busy() || _v6_queue.busy()) {
	debug_msg("busy\n");
	return PEER_OUTPUT_BUSY;
    }
#endif

    debug_msg("not busy\n");

    return  PEER_OUTPUT_OK;
}

bool 
RibIpcHandler::insert_static_route(const OriginType origin,
				   const AsPath& aspath,
				   const IPv4& next_hop,
				   const IPNet<IPv4>& nlri)
{
    debug_msg("insert_static_route: origin %d aspath %s next hop %s nlri %s\n",
	      origin, aspath.str().c_str(), next_hop.str().c_str(), 
	      nlri.str().c_str());

    /*
    ** Construct the path attribute list.
    */
    PathAttributeList<IPv4> pa_list(next_hop, aspath, origin);

    /*
    ** Create a subnet route
    */
    SubnetRoute<IPv4>* msg_route 
	= new SubnetRoute<IPv4>(nlri, &pa_list, NULL);
    
    /*
    ** Make an internal message.
    */
    InternalMessage<IPv4> msg(msg_route, this, GENID_UNKNOWN);

    /*
    ** Inject the message into the plumbing.
    */
    _plumbing->add_route(msg, this);
    _plumbing->push(this);
    msg_route->unref();

    return true;
}

bool 
RibIpcHandler::delete_static_route(const IPNet<IPv4>& nlri)
{
    debug_msg("delete_static_route: nlri %s\n", nlri.str().c_str());

    /*
    ** Create a subnet route
    */
    SubnetRoute<IPv4>* msg_route
	= new SubnetRoute<IPv4>(nlri, 0, NULL);

    /*
    ** Make an internal message.
    */
    InternalMessage<IPv4> msg(msg_route, this, GENID_UNKNOWN);

    /*
    ** Inject the message into the plumbing.
    */
    _plumbing->delete_route(msg, this);
    _plumbing->push(this);
    msg_route->unref();

    return true;
}

void 
RibIpcHandler::fatal_error(const string& reason)
{
    _interface_failed = true;
    _failure_reason = reason;
}

bool 
RibIpcHandler::status(string& reason) const
{
    if (_interface_failed) {
	reason = _failure_reason;
	return false;
    } else {
	return true;
    }
}

template<class A>
XrlQueue<A>::XrlQueue(RibIpcHandler *rib_ipc_handler,
		      XrlStdRouter *xrl_router, BGPMain *bgp)
    : _rib_ipc_handler(rib_ipc_handler),
      _xrl_router(xrl_router), _bgp(bgp),
      _flying(0), _previously_succeeded(false), _synchronous_mode(false),
      _errors(0), _interface_failed(false)
{
}

template<class A>
void
XrlQueue<A>::queue_add_route(string ribname, bool ibgp, const IPNet<A>& net,
			 const A& nexthop)
{
    if (_interface_failed)
	return;

    Queued q;

    q.add = true;
    q.ribname = ribname;
    q.ibgp = ibgp;
    q.net = net;
    q.nexthop = nexthop;
    q.id = _id++;

    _xrl_queue.push_back(q);

    sendit();
}

template<class A>
void
XrlQueue<A>::queue_delete_route(string ribname, bool ibgp, const IPNet<A>& net)
{
    if (_interface_failed)
	return;

    Queued q;

    q.add = false;
    q.ribname = ribname;
    q.ibgp = ibgp;
    q.net = net;
    q.id = _id++;

    _xrl_queue.push_back(q);

    sendit();
}

template<class A>
bool
XrlQueue<A>::busy()
{
    return 0 != _flying;
}

template<class A>
void
XrlQueue<A>::sendit()
{
    for(;;) {
	debug_msg("queue length %u\n", (uint32_t)_xrl_queue.size());

	if(_flying >= FLYING_LIMIT || (_flying >= 1 && _synchronous_mode))
	    return;

	if(_xrl_queue.empty()) {
#ifdef	FLOW_CONTROL
	    debug_msg("Output no longer busy\n");
	    _rib_ipc_handler->output_no_longer_busy();
#endif	
	    return;
	}

	if (_xrl_queue.size() <= _flying)
	    return;

	typename deque <XrlQueue<A>::Queued>::const_iterator qi;
	size_t i = 0;

	for (qi = _xrl_queue.begin(); qi != _xrl_queue.end(); qi++, i++)
	    if (i == _flying)
		break;

	XLOG_ASSERT(qi != _xrl_queue.end());

	Queued q = *qi;
	XrlRibV0p1Client rib(_xrl_router);
	const char *bgp = q.ibgp ? "ibgp" : "ebgp";
	bool sent = sendit_spec(q, rib, bgp);

	if (sent) {
	    _flying++;
	    continue;
	}

	// We expect that the send may fail if the socket buffer is full.
	// It should therefore be the case that we have some route
	// adds/deletes in flight. If _flying is zero then something
	// unexpected has happended. We have no outstanding sends and
	// still its gone to poo.

	XLOG_ASSERT(0 != _flying);

	// We failed to send the last XRL. Don't attempt to send any more.
	return;
    }
}

template<>
bool
XrlQueue<IPv4>::sendit_spec(Queued& q,  XrlRibV0p1Client& rib, const char *bgp)
{
    bool sent;

    if(q.add) {
	debug_msg("adding route from %s peer to rib\n", bgp);
	sent = rib.send_add_route4(q.ribname.c_str(),
			    bgp,
			    true, false,
			    q.net, q.nexthop, /*metric*/0, 
			    callback(this, &XrlQueue::route_command_done,
				     q.id, "add_route"));
    } else {
	debug_msg("deleting route from %s peer to rib\n", bgp);
	sent = rib.send_delete_route4(q.ribname.c_str(),
			       bgp,
			       true, false, q.net,
			       ::callback(this, &XrlQueue::route_command_done,
					  q.id, "delete_route"));
    }

    return sent;
}

template<>
bool
XrlQueue<IPv6>::sendit_spec(Queued& q, XrlRibV0p1Client& rib, const char *bgp)
{
    bool sent;

    if(q.add) {
	debug_msg("adding route from %s peer to rib\n", bgp);
	sent = rib.send_add_route6(q.ribname.c_str(),
			    bgp,
			    true, false,
			    q.net, q.nexthop, /*metric*/0, 
			    callback(this, &XrlQueue::route_command_done,
				     q.id, "add_route"));
    } else {
	debug_msg("deleting route from %s peer to rib\n", bgp);
	sent = rib.send_delete_route6(q.ribname.c_str(),
			       bgp,
			       true, false,
			       q.net,
			       callback(this, &XrlQueue::route_command_done,
					q.id, "delete_route"));
    }

    return sent;
}

template<class A>
void
XrlQueue<A>::route_command_done(const XrlError& error,
				uint32_t	sequence, 
				const char*	comment)
{
    _flying--;
    debug_msg("callback %d %s %s\n", sequence, comment, error.str().c_str());

    switch (error.error_code()) {
    case OKAY:
	{
	_errors = 0;
	_synchronous_mode = false;
	_previously_succeeded = true;

	Queued q = _xrl_queue.front();
	if (q.id == sequence) {
	    _xrl_queue.pop_front();
	} else {
	    typename deque <XrlQueue<A>::Queued>::iterator qi;

	    bool found = false;
	    for (qi = _xrl_queue.begin(); qi != _xrl_queue.end(); qi++)
		if ((*qi).id == sequence) {
 		    _xrl_queue.erase(qi);
		    found = true;
		    break;
		}
	    XLOG_ASSERT(found);
	}

	sendit();
	}
	break;

    case REPLY_TIMED_OUT:
	// We should really be using a reliable transport where
	// this error cannot happen. But it has so lets retry if we can.
	XLOG_WARNING("callback: %s %s",  comment, error.str().c_str());
	_errors++;
	_synchronous_mode = true;
	delayed_send(1000);
	break;

    case RESOLVE_FAILED:
	if (!_previously_succeeded) {
	    //give the other end time to get started.  we shouldn't
	    //really need to do this if we're started from rtrmgr, but
	    //it doesn't do any harm, and makes us more robust.
	    XLOG_WARNING("callback: %s %s",  comment, error.str().c_str());
	    _errors++;
	    _synchronous_mode = true;
	    delayed_send(1000);
	}
	/* FALLTHROUGH */
    case SEND_FAILED:
    case SEND_FAILED_TRANSIENT:
    case NO_SUCH_METHOD:
	if (_previously_succeeded) {
	    XLOG_ERROR("callback: %s %s",  comment, error.str().c_str());
	    XLOG_ERROR("Interface is now permanently disabled\n");
	    //This is fatal for this interface.  Cause the interface to
	    //fail permanently, and await notification from the finder
	    //that the RIB has really gone down.
	    _interface_failed = true;
	    _rib_ipc_handler
		->fatal_error("Fatal error talking to RIB: " + string(comment) 
			  + " " + error.str());
	}
	break;
    case NO_FINDER:
	_bgp->finder_death();
	break;
    case BAD_ARGS:
    case COMMAND_FAILED:
    case INTERNAL_ERROR:
	XLOG_FATAL("callback: %s %s",  comment, error.str().c_str());
	break;
    }

    if (_errors >= MAX_ERR_RETRIES) {
	XLOG_ERROR("callback: %s %s",  comment, error.str().c_str());
	XLOG_ERROR("Interface is now permanently disabled\n");
	//This is fatal for this interface.  Cause the interface to
	//fail permanently, and await notification from the finder
	//that the RIB has really gone down.
	_interface_failed = true;
	_rib_ipc_handler
	    ->fatal_error("Too many retrys attempting to talk to RIB");

	//Clean up the queue - we can no longer handle these requests.
	while (!_xrl_queue.empty()) {
	    _xrl_queue.pop_front();
	}
    }
}


template<class A>
void
XrlQueue<A>::delayed_send(uint32_t delay_ms) 
{
    //don't re-schedule if the timer is already running
    if (_delayed_send_timer.scheduled())
	return;

    _delayed_send_timer = eventloop().
	new_oneoff_after_ms(delay_ms, ::callback(this, 
						 &XrlQueue<A>::sendit));
}


