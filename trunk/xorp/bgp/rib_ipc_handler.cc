// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/rib_ipc_handler.cc,v 1.3 2003/01/15 19:56:48 atanu Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

// XXX - As soon as this works remove the define and make the code unconditonal
#define FLOW_CONTROL

#include "bgp_module.h"
#include "rib_ipc_handler.hh"

RibIpcHandler::RibIpcHandler(XrlStdRouter *xrl_router) 
    : PeerHandler("RIBIpcHandler", NULL, NULL),
       _ribname(""),
      _xrl_router(xrl_router),
      _v4_queue(this, xrl_router),
      _v6_queue(this, xrl_router)
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
			    ::callback(this, 
				       &RibIpcHandler::callback,"add_table"));
    //ibgp - v4
    //name - "ibgp"
    //unicast - true
    //multicast - false
    rib.send_add_egp_table4(_ribname.c_str(),
			    "ibgp", true, false,
			    ::callback(this, 
				       &RibIpcHandler::callback,"add_table"));

    //create our tables
    //ebgp - v6
    //name - "ebgp"
    //unicast - true
    //multicast - false
//     rib.send_add_egp_table6(_ribname.c_str(),
// 			    "ebgp", true, false,
// 			    ::callback(this, 
// 				       &RibIpcHandler::callback,"add_table"));
    //ibgp - v6
    //name - "ibgp"
    //unicast - true
    //multicast - false
//     rib.send_add_igp_table6(_ribname.c_str(),
// 			    "ibgp", true, false,
// 			    ::callback(this, 
// 				       &RibIpcHandler::callback,"add_table"));

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
			       ::callback(this,
					  &RibIpcHandler::callback,
					  "delete_table"));
    //ibgp - v4
    //name - "ibgp"
    //unicast - true
    //multicast - false
    rib.send_delete_igp_table4(_ribname.c_str(),
			       "ibgp", true, false,
			       ::callback(this,
					  &RibIpcHandler::callback,
					  "delete_table"));

    //create our tables
    //ebgp - v6
    //name - "ebgp"
    //unicast - true
    //multicast - false
//     rib.send_delete_egp_table6(_ribname.c_str(),
// 			       "ebgp", true, false,
// 			       ::callback(this,
// 					  &RibIpcHandler::callback,
// 					  "delete_table"));

    //ibgp - v6
    //name - "ibgp"
    //unicast - true
    //multicast - false
//     rib.send_delete_igp_table6(_ribname.c_str(),
// 			       "ibgp", true, false,
// 			       ::callback(this,
// 					  &RibIpcHandler::callback,
// 					  "delete_table"));

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
RibIpcHandler::callback(const XrlError& error, const char *comment)
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
    SubnetRoute<IPv4> msg_route(nlri, &pa_list);
    
    /*
    ** Make an internal message.
    */
    InternalMessage<IPv4> msg(&msg_route, this, GENID_UNKNOWN);

    /*
    ** Inject the message into the plumbing.
    */
    _plumbing->add_route(msg, this);
    _plumbing->push(this);

    return true;
}

bool 
RibIpcHandler::delete_static_route(const IPNet<IPv4>& nlri)
{
    debug_msg("delete_static_route: nlri %s\n", nlri.str().c_str());

    /*
    ** Create a subnet route
    */
    SubnetRoute<IPv4> msg_route(nlri, 0);

    /*
    ** Make an internal message.
    */
    InternalMessage<IPv4> msg(&msg_route, this, GENID_UNKNOWN);

    /*
    ** Inject the message into the plumbing.
    */
    _plumbing->delete_route(msg, this);
    _plumbing->push(this);

    return true;
}

template<class A>
XrlQueue<A>::XrlQueue(RibIpcHandler *rib_ipc_handler,
		      XrlStdRouter *xrl_router) 
    : _rib_ipc_handler(rib_ipc_handler),
    _xrl_router(xrl_router),
    _flying(0)
{
}

template<class A>
void
XrlQueue<A>::queue_add_route(string ribname, bool ibgp, const IPNet<A>& net,
			 const A& nexthop)
{
    Queued q;

    q.add = true;
    q.ribname = ribname;
    q.ibgp = ibgp;
    q.net = net;
    q.nexthop = nexthop;

    _xrl_queue.push(q);

    sendit();
}

template<class A>
void
XrlQueue<A>::queue_delete_route(string ribname, bool ibgp, const IPNet<A>& net)
{
    Queued q;

    q.add = false;
    q.ribname = ribname;
    q.ibgp = ibgp;
    q.net = net;

    _xrl_queue.push(q);

    sendit();
}

template<class A>
bool
XrlQueue<A>::busy()
{
    return 0 != _flying;
}

template<>
void
XrlQueue<IPv4>::sendit()
{
    debug_msg("queue length %u\n", (uint32_t)_xrl_queue.size());

    if(_flying >= FLYING_LIMIT)
	return;

    if(_xrl_queue.empty()) {
#ifdef	FLOW_CONTROL
	debug_msg("Output no longer busy\n");
	_rib_ipc_handler->output_no_longer_busy();
#endif	
	return;
    }

    Queued q = _xrl_queue.front();
    _xrl_queue.pop();

    XrlRibV0p1Client rib(_xrl_router);

    const char *bgp = q.ibgp ? "ibgp" : "ebgp";

    if(q.add) {
	debug_msg("adding route from %s peer to rib\n", bgp);
	rib.send_add_route4(q.ribname.c_str(),
			    bgp,
			    true, false,
			    q.net, q.nexthop, /*metric*/0, 
			    ::callback(this, &XrlQueue::callback,
				       "add_route"));
    } else {
	debug_msg("deleting route from %s peer to rib\n", bgp);
	rib.send_delete_route4(q.ribname.c_str(),
			       bgp,
			       true, false,
			       q.net,
			       ::callback(this, &XrlQueue::callback,
					  "delete_route"));
    }
    _flying++;
}

template<>
void
XrlQueue<IPv6>::sendit()
{
    if(_flying >= FLYING_LIMIT)
	return;

    if(_xrl_queue.empty()) {
// 	debug_msg("Output no longer busy\n");
// 	_rib_ipc_handler->output_no_longer_busy();
	return;
    }

    Queued q = _xrl_queue.front();
    _xrl_queue.pop();

    XrlRibV0p1Client rib(_xrl_router);

    const char *bgp = q.ibgp ? "ibgp" : "ebgp";

    if(q.add) {
	debug_msg("adding route from %s peer to rib\n", bgp);
	rib.send_add_route6(q.ribname.c_str(),
			    bgp,
			    true, false,
			    q.net, q.nexthop, /*metric*/0, 
			    ::callback(this, &XrlQueue::callback,
				       "add_route"));
    } else {
	debug_msg("deleting route from %s peer to rib\n", bgp);
	rib.send_delete_route6(q.ribname.c_str(),
			       bgp,
			       true, false,
			       q.net,
			       ::callback(this, &XrlQueue::callback,
					  "delete_route"));
    }
    _flying++;
}

template<class A>
void
XrlQueue<A>::callback(const XrlError& error, const char *comment)
{
    debug_msg("callback %s %s\n", comment, error.str().c_str());
    if(XrlError::OKAY() != error) {
	XLOG_WARNING("callback: %s %s",  comment, error.str().c_str());
    }

    _flying--;
    sendit();
}
