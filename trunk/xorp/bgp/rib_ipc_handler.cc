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

#ident "$XORP: xorp/bgp/rib_ipc_handler.cc,v 1.47 2004/03/24 19:34:30 atanu Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "bgp.hh"
#include "rib_ipc_handler.hh"

RibIpcHandler::RibIpcHandler(XrlStdRouter *xrl_router, BGPMain& bgp) 
    : PeerHandler("RIBIpcHandler", NULL, NULL, NULL), 
      _ribname(""),
      _xrl_router(xrl_router),
      _v4_queue(this, xrl_router, &bgp),
      _v6_queue(this, xrl_router, &bgp),
      _fake_id((unsigned int)0)
{
}

RibIpcHandler::~RibIpcHandler() 
{
    if(_v4_queue.busy() || _v6_queue.busy())
	XLOG_WARNING("Deleting RibIpcHandler with callbacks pending");

    set_plumbing(NULL, NULL);

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
    if (_ribname == r)
	return true;

    if ("" == r) {
	_ribname.erase();
	return unregister_rib();
    }

    _ribname = r;

    XrlRibV0p1Client rib(_xrl_router);
    //create our tables
    //ebgp - v4
    //name - "ebgp"
    //unicast - true
    //multicast - true
    rib.send_add_egp_table4(_ribname.c_str(),
		"ebgp", _xrl_router->class_name(),
                 _xrl_router->instance_name(), true, true,
                 callback(this, 
			  &RibIpcHandler::rib_command_done,"add_table"));
    //ibgp - v4
    //name - "ibgp"
    //unicast - true
    //multicast - true
    rib.send_add_egp_table4(_ribname.c_str(),
		"ibgp", _xrl_router->class_name(),
                _xrl_router->instance_name(), true, true,
		callback(this, 
			 &RibIpcHandler::rib_command_done,"add_table"));

    //create our tables
    //ebgp - v6
    //name - "ebgp"
    //unicast - true
    //multicast - true
    rib.send_add_egp_table6(_ribname.c_str(),
                "ebgp",  _xrl_router->class_name(),
                _xrl_router->instance_name(), true, true,
		callback(this, 
 		         &RibIpcHandler::rib_command_done,"add_table"));
    //ibgp - v6
    //name - "ibgp"
    //unicast - true
    //multicast - true
    rib.send_add_egp_table6(_ribname.c_str(),
		"ibgp", _xrl_router->class_name(),
                _xrl_router->instance_name(), true, true,
		callback(this,
			 &RibIpcHandler::rib_command_done,"add_table"));

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
    //multicast - true
    rib.send_delete_egp_table4(_ribname.c_str(),
			       "ebgp", _xrl_router->class_name(),
                               _xrl_router->instance_name(),
			       true, true,
			       callback(this,
					&RibIpcHandler::rib_command_done,
					"delete_table"));
    //ibgp - v4
    //name - "ibgp"
    //unicast - true
    //multicast - true
    rib.send_delete_egp_table4(_ribname.c_str(),
			       "ibgp", _xrl_router->class_name(),
                               _xrl_router->instance_name(), 
			       true, true,
			       callback(this,
					&RibIpcHandler::rib_command_done,
					"delete_table"));

    //create our tables
    //ebgp - v6
    //name - "ebgp"
    //unicast - true
    //multicast - true
    rib.send_delete_egp_table6(_ribname.c_str(),
 			       "ebgp", _xrl_router->class_name(),
                               _xrl_router->instance_name(), 
			       true, true,
 			       callback(this,
 				  	&RibIpcHandler::rib_command_done,
 					"delete_table"));

    //ibgp - v6
    //name - "ibgp"
    //unicast - true
    //multicast - true
    rib.send_delete_egp_table6(_ribname.c_str(),
 			       "ibgp", _xrl_router->class_name(),
                               _xrl_router->instance_name(), 
			       true, true,
 			       callback(this,
 					&RibIpcHandler::rib_command_done,
 					"delete_table"));

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
RibIpcHandler::add_route(const SubnetRoute<IPv4> &rt, Safi safi)
{
    debug_msg("RibIpcHandler::add_route(IPv4) %x\n", (u_int)(&rt));

    if("" == _ribname)
	return 0;

    _v4_queue.queue_add_route(_ribname, _ibgp, safi, rt.net(), rt.nexthop());

    return 0;
}

int 
RibIpcHandler::add_route(const SubnetRoute<IPv6>& rt, Safi safi)
{
    debug_msg("RibIpcHandler::add_route(IPv6) %p\n", &rt);

    if ("" == _ribname)
	return 0;

    _v6_queue.queue_add_route(_ribname, _ibgp, safi, rt.net(), rt.nexthop());

    return 0;
}

int 
RibIpcHandler::replace_route(const SubnetRoute<IPv4> &old_rt,
			     const SubnetRoute<IPv4> &new_rt,
			     Safi safi)
{
    debug_msg("RibIpcHandler::replace_route(IPv4) %p %p\n", &old_rt, &new_rt);
    delete_route(old_rt, safi);
    add_route(new_rt, safi);
    return 0;
}

int 
RibIpcHandler::replace_route(const SubnetRoute<IPv6> &old_rt,
			     const SubnetRoute<IPv6> &new_rt,
			     Safi safi)
{
    debug_msg("RibIpcHandler::replace_route(IPv6) %p %p\n", &old_rt, &new_rt);
    delete_route(old_rt, safi);
    add_route(new_rt, safi);
    return 0;
}

int 
RibIpcHandler::delete_route(const SubnetRoute<IPv4> &rt, Safi safi)
{
    debug_msg("RibIpcHandler::delete_route(IPv4) %x\n", (u_int)(&rt));

    if("" == _ribname)
	return 0;

    _v4_queue.queue_delete_route(_ribname, _ibgp, safi, rt.net());

    return 0;
}

int 
RibIpcHandler::delete_route(const SubnetRoute<IPv6>& rt, Safi safi)
{
    debug_msg("RibIpcHandler::delete_route(IPv6) %p\n", &rt);
    UNUSED(rt);
    if("" == _ribname)
	return 0;

    _v6_queue.queue_delete_route(_ribname, _ibgp, safi, rt.net());

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

#if	0
    if(_v4_queue.busy() || _v6_queue.busy()) {
	debug_msg("busy\n");
	return PEER_OUTPUT_BUSY;
    }

    debug_msg("not busy\n");
#endif

    return  PEER_OUTPUT_OK;
}

bool 
RibIpcHandler::originate_route(const OriginType origin, const AsPath& aspath,
			       const IPv4Net& nlri, const IPv4& next_hop,
			       const bool& unicast, const bool& multicast)
{
    debug_msg("origin %d aspath %s nlri %s next hop %s unicast %d"
	      " multicast %d\n",
	      origin, aspath.str().c_str(), nlri.str().c_str(),
	      next_hop.str().c_str(), unicast, multicast);

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
    if (unicast) {
	_plumbing_unicast->add_route(msg, this);
	_plumbing_unicast->push<IPv4>(this);
    }

    if (multicast) {
	_plumbing_multicast->add_route(msg, this);
	_plumbing_multicast->push<IPv4>(this);
    }

    msg_route->unref();

    return true;
}

bool 
RibIpcHandler::originate_route(const OriginType origin, const AsPath& aspath,
			       const IPv6Net& nlri, const IPv6& next_hop,
			       const bool& unicast, const bool& multicast)
{
    debug_msg("origin %d aspath %s nlri %s next hop %s unicast %d"
	      " multicast %d\n",
	      origin, aspath.str().c_str(), nlri.str().c_str(),
	      next_hop.str().c_str(), unicast, multicast);

    /*
    ** Construct the path attribute list.
    */
    PathAttributeList<IPv6> pa_list(next_hop, aspath, origin);

    /*
    ** Create a subnet route
    */
    SubnetRoute<IPv6>* msg_route 
	= new SubnetRoute<IPv6>(nlri, &pa_list, NULL);
    
    /*
    ** Make an internal message.
    */
    InternalMessage<IPv6> msg(msg_route, this, GENID_UNKNOWN);

    /*
    ** Inject the message into the plumbing.
    */
    if (unicast) {
	_plumbing_unicast->add_route(msg, this);
	_plumbing_unicast->push<IPv6>(this);
    }

    if (multicast) {
	_plumbing_multicast->add_route(msg, this);
	_plumbing_multicast->push<IPv6>(this);
    }

    msg_route->unref();

    return true;
}

bool
RibIpcHandler::withdraw_route(const IPv4Net& nlri, const bool& unicast,
			      const bool& multicast)
{
    debug_msg("nlri %s unicast %d multicast %d\n", nlri.str().c_str(),
	      unicast, multicast);

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
    if (unicast) {
	_plumbing_unicast->delete_route(msg, this);
	_plumbing_unicast->push<IPv4>(this);
    }

    if (multicast) {
	_plumbing_multicast->delete_route(msg, this);
	_plumbing_multicast->push<IPv4>(this);
    }

    msg_route->unref();

    return true;
}

bool
RibIpcHandler::withdraw_route(const IPv6Net& nlri, const bool& unicast,
			      const bool& multicast)
{
    debug_msg("nlri %s unicast %d multicast %d\n", nlri.str().c_str(),
	      unicast, multicast);

    /*
    ** Create a subnet route
    */
    SubnetRoute<IPv6>* msg_route
	= new SubnetRoute<IPv6>(nlri, 0, NULL);

    /*
    ** Make an internal message.
    */
    InternalMessage<IPv6> msg(msg_route, this, GENID_UNKNOWN);

    /*
    ** Inject the message into the plumbing.
    */
    if (unicast) {
	_plumbing_unicast->delete_route(msg, this);
	_plumbing_unicast->push<IPv6>(this);
    }

    if (multicast) {
	_plumbing_multicast->delete_route(msg, this);
	_plumbing_multicast->push<IPv6>(this);
    }

    msg_route->unref();

    return true;
}

template<class A>
XrlQueue<A>::XrlQueue(RibIpcHandler *rib_ipc_handler,
		      XrlStdRouter *xrl_router, BGPMain *bgp)
    : _rib_ipc_handler(rib_ipc_handler),
      _xrl_router(xrl_router), _bgp(bgp),
      _flying(0)
{
}

template<class A>
EventLoop& 
XrlQueue<A>::eventloop() const 
{ 
    return _rib_ipc_handler->eventloop(); 
}

template<class A>
void
XrlQueue<A>::queue_add_route(string ribname, bool ibgp, Safi safi,
			     const IPNet<A>& net, const A& nexthop)
{
    Queued q;

    q.add = true;
    q.ribname = ribname;
    q.ibgp = ibgp;
    q.safi = safi;
    q.net = net;
    q.nexthop = nexthop;
    q.comment = 
	c_format("add_route: ribname %s %s safi %d net %s nexthop %s",
		 ribname.c_str(),
		 ibgp ? "ibgp" : "ebgp",
		 safi,
		 net.str().c_str(),
		 nexthop.str().c_str());

    _xrl_queue.push_back(q);

    start();
}

template<class A>
void
XrlQueue<A>::queue_delete_route(string ribname, bool ibgp, Safi safi,
				const IPNet<A>& net)
{
    Queued q;

    q.add = false;
    q.ribname = ribname;
    q.ibgp = ibgp;
    q.safi = safi;
    q.net = net;
    q.comment = 
	c_format("delete_route: ribname %s %s safi %d net %s",
		 ribname.c_str(),
		 ibgp ? "ibgp" : "ebgp",
		 safi,
		 net.str().c_str());

    _xrl_queue.push_back(q);

    start();
}

template<class A>
bool
XrlQueue<A>::busy()
{
    return 0 != _flying;
}

template<class A>
void
XrlQueue<A>::start()
{
    // If we are currently busy don't attempt to send any more XRLs.
    if (busy())
	return;

    // Now there are no outstanding XRLs try and send as many of the queued
    // route commands as possible as possible.

    for(;;) {
	debug_msg("queue length %u\n", (uint32_t)_xrl_queue.size());

	if(_xrl_queue.empty()) {
	    debug_msg("Output no longer busy\n");
#if	0
	    _rib_ipc_handler->output_no_longer_busy();
#endif
	    return;
	}

	typename deque<typename XrlQueue<A>::Queued>::const_iterator qi;

	qi = _xrl_queue.begin();

	XLOG_ASSERT(qi != _xrl_queue.end());

	Queued q = *qi;
	XrlRibV0p1Client rib(_xrl_router);
	const char *bgp = q.ibgp ? "ibgp" : "ebgp";
	bool sent = sendit_spec(q, rib, bgp);

	if (sent) {
	    _flying++;
	    _xrl_queue.pop_front();
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
    bool unicast = false;
    bool multicast = false;

    switch(q.safi) {
    case SAFI_UNICAST:
	unicast = true;
	break;
    case SAFI_MULTICAST:
	multicast = true;
	break;
    }

    if(q.add) {
	debug_msg("adding route from %s peer to rib\n", bgp);
	sent = rib.send_add_route4(q.ribname.c_str(),
			    bgp,
			    unicast, multicast,
			    q.net, q.nexthop, /*metric*/0, 
			    callback(this, &XrlQueue::route_command_done,
				     q.comment));
    } else {
	debug_msg("deleting route from %s peer to rib\n", bgp);
	sent = rib.send_delete_route4(q.ribname.c_str(),
				      bgp,
				      unicast, multicast,
				      q.net,
				      ::callback(this,
						 &XrlQueue::route_command_done,
						 q.comment));
    }

    return sent;
}

template<>
bool
XrlQueue<IPv6>::sendit_spec(Queued& q, XrlRibV0p1Client& rib, const char *bgp)
{
    bool sent;
    bool unicast = false;
    bool multicast = false;

    switch(q.safi) {
    case SAFI_UNICAST:
	unicast = true;
	break;
    case SAFI_MULTICAST:
	multicast = true;
	break;
    }

    if(q.add) {
	debug_msg("adding route from %s peer to rib\n", bgp);
	sent = rib.send_add_route6(q.ribname.c_str(),
			    bgp,
			    unicast, multicast,
			    q.net, q.nexthop, /*metric*/0, 
			    callback(this, &XrlQueue::route_command_done,
				     q.comment));
    } else {
	debug_msg("deleting route from %s peer to rib\n", bgp);
	sent = rib.send_delete_route6(q.ribname.c_str(),
			       bgp,
			       unicast, multicast,
			       q.net,
			       callback(this, &XrlQueue::route_command_done,
					q.comment));
    }

    return sent;
}

template<class A>
void
XrlQueue<A>::route_command_done(const XrlError& error,
				const string comment)
{
    _flying--;
    debug_msg("callback %s %s\n", comment.c_str(), error.str().c_str());

    switch (error.error_code()) {
    case OKAY:
	break;

    case REPLY_TIMED_OUT:
	// We should really be using a reliable transport where
	// this error cannot happen. But it has so lets retry if we can.
	XLOG_WARNING("callback: %s %s",  comment.c_str(), error.str().c_str());
	break;

    case RESOLVE_FAILED:
    case SEND_FAILED:
    case SEND_FAILED_TRANSIENT:
    case NO_SUCH_METHOD:
	XLOG_ERROR("callback: %s %s",  comment.c_str(), error.str().c_str());
	break;

    case NO_FINDER:
	// XXX - Temporarily code dump if this condition occurs.
	XLOG_FATAL("NO FINDER");
	_bgp->finder_death(__FILE__, __LINE__);
	break;

    case BAD_ARGS:
    case COMMAND_FAILED:
    case INTERNAL_ERROR:
	XLOG_FATAL("callback: %s %s",  comment.c_str(), error.str().c_str());
	break;
    }

    // Fire of more requests.
    start();
}

