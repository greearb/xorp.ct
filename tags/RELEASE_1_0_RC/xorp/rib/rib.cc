// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/rib/rib.cc,v 1.32 2004/06/07 22:59:13 hodson Exp $"

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/c_format.hh"

#include "register_server.hh"
#include "rib_manager.hh"
#include "rib.hh"

// ----------------------------------------------------------------------------
// Inline table utility methods

template <typename A>
struct table_has_name {
    inline table_has_name(const string& name) : _n(name) {}
    inline bool operator() (const RouteTable<A>* rt) const {
	return rt->tablename() == _n;
    }
private:
    const string& _n;
};

template <typename A, typename T>
struct table_has_name_and_type {
    inline table_has_name_and_type(const string& name) : _n(name) {}
    inline bool operator() (const RouteTable<A>* rt) const {
	const T* t = dynamic_cast<const T*>(rt);
	return (t != 0) && (rt->tablename() == _n);
    }
private:
    const string& _n;
};

template <typename A>
inline RouteTable<A>*
RIB<A>::find_table(const string& tablename)
{
    typename list<RouteTable<A>* >::iterator li;

    li = find_if(_tables.begin(), _tables.end(), table_has_name<A>(tablename));
    if (li == _tables.end()) {
	return NULL;
    }
    return *li;
}

template <typename A>
inline OriginTable<A>*
RIB<A>::find_table_by_instance(const string& tablename,
			       const string& target_class,
			       const string& target_instance)
{
    typename map<string, OriginTable<A>* >::iterator mi;

    mi = _routing_protocol_instances.find(tablename + " "
					  + target_class + " "
					  + target_instance);
    if (mi == _routing_protocol_instances.end()) {
	return NULL;
    }
    return mi->second;
}

template <typename A>
Protocol*
RIB<A>::find_protocol(const string& protocol)
{
    typename map<string, Protocol* >::iterator mi = _protocols.find(protocol);

    if (mi == _protocols.end()) {
	return NULL;
    }
    return mi->second;
}

template <typename A>
inline int
RIB<A>::add_table(RouteTable<A>* table)
{
    const string& tablename = table->tablename();
    if (find_table(tablename) != NULL) {
	XLOG_WARNING("add_table: table %s already exists", tablename.c_str());
	return XORP_ERROR;
    }
    _tables.push_back(table);
    return XORP_OK;
}

template <typename A>
inline int
RIB<A>::remove_table(const string& tablename)
{
    typename list<RouteTable<A>* >::iterator li;

    li = find_if(_tables.begin(), _tables.end(), table_has_name<A>(tablename));
    if (li == _tables.end()) {
	XLOG_WARNING("remove_table: table %s doesn't exist",
		     tablename.c_str());
	return XORP_ERROR;
    }
    _tables.erase(li);
    return XORP_OK;
}

template <typename A>
inline int
RIB<A>::admin_distance(const string& tablename)
{
    map<const string, int>::iterator mi;

    mi = _admin_distances.find(tablename);
    if (mi == _admin_distances.end()) {
	XLOG_ERROR("Administrative distance of \"%s\" unknown.",
		   tablename.c_str());
	return _admin_distances["unknown"];
    }
    return mi->second;
}

template <typename A>
inline Vif*
RIB<A>::find_vif(const A& addr)
{
    map<const string, Vif>::iterator mi;

    for (mi = _vifs.begin(); mi != _vifs.end(); ++mi) {
	Vif& v = mi->second;
	if (v.is_my_addr(addr))
	    return &v;
    }
    return NULL;
}

template <typename A>
inline IPExternalNextHop<A>*
RIB<A>::find_external_nexthop(const A& addr)
{
    typename map<const A, IPExternalNextHop<A> >::iterator mi;

    mi = _external_nexthops.find(addr);
    if (mi == _external_nexthops.end())
	return NULL;
    return &mi->second;
}

template <typename A>
inline IPPeerNextHop<A>*
RIB<A>::find_peer_nexthop(const A& addr)
{
    typename map<const A, IPPeerNextHop<A> >::iterator mi;

    mi = _peer_nexthops.find(addr);
    if (mi == _peer_nexthops.end())
	return NULL;
    return &mi->second;
}

template <typename A>
inline IPExternalNextHop<A>*
RIB<A>::find_or_create_external_nexthop(const A& addr)
{
    IPExternalNextHop<A>* nexthop = find_external_nexthop(addr);
    if (nexthop != NULL)
	return nexthop;

    typedef map<const A,IPExternalNextHop<A> > C;	// for convenience
    typename C::value_type vt(addr, IPExternalNextHop<A>(addr));
    typename C::iterator iter;
    iter = _external_nexthops.insert(_external_nexthops.end(), vt);
    return &iter->second;
}

template <typename A>
inline IPPeerNextHop<A>*
RIB<A>::find_or_create_peer_nexthop(const A& addr)
{
    IPPeerNextHop<A>* nexthop = find_peer_nexthop(addr);
    if (nexthop != NULL)
	return nexthop;

    typedef map<const A,IPPeerNextHop<A> > C;		// for convenience
    typename C::value_type vt(addr, addr);
    typename C::iterator iter;
    iter = _peer_nexthops.insert(_peer_nexthops.end(), vt);
    return &iter->second;
}

// ----------------------------------------------------------------------------
// Naming utility methods

static inline const char*
export_tablename()
{
    return "ExportToRibClients";
}

static inline string
redist_tablename(const string& from_table)
{
    return "Redist:" + from_table;
}

// ----------------------------------------------------------------------------
// RIB class

template <typename A>
RIB<A>::RIB(RibTransportType t, RibManager& rib_manager, EventLoop& eventloop)
    : _rib_manager(rib_manager),
      _eventloop(eventloop),
      _final_table(NULL),
      _register_table(NULL),
      _errors_are_fatal(false)
{
    if (t == UNICAST) {
	_multicast = false;
    } else if (t == MULTICAST) {
	_multicast = true;
    } else {
	XLOG_FATAL("Unknown RibTransportType.");
    }

    // TODO: XXX: don't use hard-coded values below!
    _admin_distances["connected"] =        0;
    _admin_distances["static"] =           1;
    _admin_distances["eigrp-summary"] =    5;
    _admin_distances["ebgp"] =            20;
    _admin_distances["eigrp-internal"] =  90;
    _admin_distances["igrp"] =           100;
    _admin_distances["ospf"] =           110;
    _admin_distances["is-is"] =          115;
    _admin_distances["rip"] =            120;
    _admin_distances["eigrp-external"] = 170;
    _admin_distances["ibgp"] =           200;
    _admin_distances["fib2mrib"] =	 254;
    _admin_distances["unknown"] =        255;
}

template <typename A>
RIB<A>::~RIB()
{
    while (_tables.empty() == false) {
	delete _tables.front();
	_tables.pop_front();
    }
}

template <typename A>
int
RIB<A>::initialize_export(list<RibClient* >* rib_clients_list)
{
    ExportTable<A>* et = new ExportTable<A>(export_tablename(), 0,
					    rib_clients_list);

    if (add_table(et) != XORP_OK) {
	XLOG_FATAL("Export already initialized.");
	//delete et;
	//return XORP_ERROR;
    }
    _final_table = et;
    return XORP_OK;
}

template <typename A>
int
RIB<A>::initialize_redist_all(const string& all)
{
    if (_register_table == NULL) {
	XLOG_ERROR("Register table is not yet initialized");
	return XORP_ERROR;
    }

    if (find_table(redist_tablename(all)) != NULL) {
	return XORP_OK;		// RedistTable already exists, no sweat
    }

    RedistTable<A>* r;
    r = new RedistTable<A>(redist_tablename(all), _register_table);
    if (add_table(r) != XORP_OK) {
	delete r;
	return XORP_ERROR;
    }

    return XORP_OK;
}

template <typename A>
int
RIB<A>::initialize_register(RegisterServer* regserv)
{
    if (_register_table != NULL) {
	XLOG_WARNING("Register table already initialized.");
	return XORP_ERROR;
    }

    RegisterTable<A>* rt;
    rt = new RegisterTable<A>("RegisterTable", regserv, _multicast);
    if (add_table(rt) != XORP_OK) {
	XLOG_WARNING("Add RegisterTable failed.");
	delete rt;
	return XORP_ERROR;
    }
    _register_table = rt;

    if (find_table(export_tablename()) == NULL) {
	// No ExportTable<A> - perhaps we're an MRIB.
	if (_final_table != NULL) {
	    XLOG_FATAL("No export table when initializing register table");
	}
	_final_table = _register_table;
    } else {
	_final_table->replumb(NULL, _register_table);
	_register_table->set_next_table(_final_table);
    }
    return XORP_OK;
}

template <typename A>
int
RIB<A>::new_origin_table(const string&	tablename,
			 const string&	target_class,
			 const string&	target_instance,
			 int		admin_distance,
			 ProtocolType	protocol_type)
{
    OriginTable<A>* ot = new OriginTable<A>(tablename, admin_distance,
					    protocol_type, _eventloop);
    if (ot == NULL || add_table(ot) != XORP_OK) {
	XLOG_WARNING("Could not add origin table %s", tablename.c_str());
	delete ot;
	return XORP_ERROR;
    } else if (_final_table == NULL) {
	_final_table = ot;
    }

    //
    // Store the XRL target instance, so we know which OriginTable to
    // shutdown if the routing protocol dies.
    //
    if (!target_instance.empty()) {
	_rib_manager.register_interest_in_target(target_class);
	_routing_protocol_instances[tablename + " "
				   + target_class + " "
				   + target_instance] = ot;
    }
    return XORP_OK;
}

template<>
int
RIB<IPv4>::new_vif(const string& vifname, const Vif& vif)
{
    debug_msg("RIB::new_vif: %s\n", vifname.c_str());
    if (_vifs.find(vifname) == _vifs.end()) {
	// Can't use _vifs[vifname] = vif because no Vif() constructor
	map<const string, Vif>::value_type v(vifname, vif);
	_vifs.insert(_vifs.end(), v);

	// We need to add the routes from the VIF to the connected table
	map<const string, Vif>::iterator iter = _vifs.find(vifname);
	XLOG_ASSERT(iter != _vifs.end());
	Vif* new_vif = &(iter->second);
	XLOG_ASSERT(new_vif != NULL);
	list<VifAddr>::const_iterator ai;
	for (ai = new_vif->addr_list().begin();
	     ai != new_vif->addr_list().end();
	     ai++) {
	    if (ai->addr().is_ipv4()) {
		add_route("connected", ai->subnet_addr().get_ipv4net(),
			  ai->addr().get_ipv4(), "", "", 0);
	    }
	}

	return XORP_OK;
    }
    return XORP_ERROR;
}

template<>
int
RIB<IPv6>::new_vif(const string& vifname, const Vif& vif)
{
    debug_msg("RIB::new_vif: %s\n", vifname.c_str());
    if (_vifs.find(vifname) == _vifs.end()) {
	// Can't use _vifs[vifname] = vif because no Vif() constructor
	map<const string, Vif>::value_type v(vifname, vif);
	_vifs.insert(_vifs.end(), v);

	// We need to add the routes from the VIF to the connected table
	map<const string, Vif>::iterator iter = _vifs.find(vifname);
	XLOG_ASSERT(iter != _vifs.end());
	Vif* new_vif = &(iter->second);
	XLOG_ASSERT(new_vif != NULL);
	list<VifAddr>::const_iterator ai;
	for (ai = new_vif->addr_list().begin();
	     ai != new_vif->addr_list().end();
	     ai++) {
	    if (ai->addr().is_ipv6()) {
		add_route("connected", ai->subnet_addr().get_ipv6net(),
			  ai->addr().get_ipv6(), "", "", 0);
	    }
	}

	return XORP_OK;
    }
    return XORP_ERROR;
}

template <typename A>
int
RIB<A>::delete_vif(const string& vifname)
{
    debug_msg("RIB::delete_vif: %s\n", vifname.c_str());
    map<const string, Vif>::iterator iter;
    iter = _vifs.find(vifname);
    if (iter == _vifs.end()) {
	return XORP_ERROR;
    }
    list<VifAddr>::const_iterator vai;
    for (vai = iter->second.addr_list().begin();
	 vai != iter->second.addr_list().end();
	 ++vai) {
	try {
	    // Delete the directly connected routes associated with this VIF
	    IPvXNet subnetvX = vai->subnet_addr();
	    IPNet<A> subnet;
	    subnetvX.get(subnet);
	    delete_route("connected", subnet);
	} catch (const InvalidCast&) {
	    debug_msg("Invalid cast of %s in RIB<%s>",
		      vai->subnet_addr().str().c_str(),
		      A::ip_version_str().c_str());
	}
    }

    _vifs.erase(iter);
    return XORP_OK;
}

template <typename A>
int
RIB<A>::set_vif_flags(const string& vifname,
		      bool is_p2p,
		      bool is_loopback,
		      bool is_multicast,
		      bool is_broadcast,
		      bool is_up)
{
    map<const string, Vif>::iterator vi = _vifs.find(vifname);
    if (vi == _vifs.end()) {
	XLOG_ERROR("Attempting to set flags to non-existant Vif \"%s\"",
		   vifname.c_str());
	return XORP_ERROR;
    }

    Vif& vif = vi->second;
    vif.set_p2p(is_p2p);
    vif.set_loopback(is_loopback);
    vif.set_multicast_capable(is_multicast);
    vif.set_broadcast_capable(is_broadcast);
    vif.set_underlying_vif_up(is_up);

    return XORP_OK;
}

template <typename A>
int
RIB<A>::add_vif_address(const string&	vifname,
			const A&	addr,
			const IPNet<A>&	subnet,
			const A&	broadcast_addr,
			const A&	peer_addr)
{
    map<const string, Vif>::iterator vi = _vifs.find(vifname);
    if (vi == _vifs.end()) {
	XLOG_ERROR("Attempting to add address to non-existant Vif \"%s\"",
		   vifname.c_str());
	return XORP_ERROR;
    }
    vi->second.add_address(VifAddr(addr, subnet, broadcast_addr, peer_addr));
    // Add a route for this subnet
    add_route("connected", subnet, addr, "", "", /* best possible metric */ 0);
    return XORP_OK;
}

template <typename A>
int
RIB<A>::delete_vif_address(const string& vifname,
			   const A& addr)
{
    map<const string, Vif>::iterator vi = _vifs.find(vifname);
    if (vi == _vifs.end()) {
	XLOG_ERROR("Attempting to delete address from non-existant Vif \"%s\"",
		   vifname.c_str());
	return XORP_ERROR;
    }
    list<VifAddr>::const_iterator vai;
    for (vai = vi->second.addr_list().begin();
	 vai != vi->second.addr_list().end();
	 ++vai) {
	IPvX addrvX = vai->addr();
	try {
	    A this_addr;
	    addrvX.get(this_addr);
	    if (addr == this_addr) {
		IPvXNet subnetvX = vai->subnet_addr();
		IPNet<A> subnet;
		subnetvX.get(subnet);
		delete_route("connected", subnet);
		return XORP_OK;
	    }
	} catch (const InvalidCast&) {
	    debug_msg("Invalid cast of %s in RIB<%s>\n",
		      addrvX.str().c_str(), A::ip_version_str().c_str());
	}
    }
    return XORP_ERROR;
}

template <typename A>
int
RIB<A>::add_route(const string&		tablename,
		  const IPNet<A>&	net,
		  const A&		nexthop_addr,
		  const string&		ifname,
		  const string&		vifname,
		  uint32_t		metric)
{
    RouteTable<A>* rt = find_table(tablename);
    if (rt == NULL) {
	if (_errors_are_fatal) {
	    XLOG_FATAL("Attempting to add route to unknown table \"%s\".",
		       tablename.c_str());
	} else {
	    XLOG_ERROR("Attempting to add route to unknown table \"%s\".",
		       tablename.c_str());
	    return XORP_ERROR;
	}
    }

    Protocol* protocol = find_protocol(tablename);
    if (protocol == NULL) {
	if (_errors_are_fatal) {
	    XLOG_FATAL("Attempting to add route with unknown protocol \"%s\".",
		       tablename.c_str());
	} else {
	    XLOG_ERROR("Attempting to add route with unknown protocol \"%s\".",
		       tablename.c_str());
	    return XORP_ERROR;
	}
    }

    OriginTable<A>* ot = dynamic_cast<OriginTable<A>* >(rt);
    if (ot == NULL) {
	if (_errors_are_fatal) {
	    XLOG_FATAL("Attempting to add route to table \"%s\" that is not "
		       "an origin table.", tablename.c_str());
	} else {
	    XLOG_ERROR("Attempting to add route to table \"%s\" that is not "
		       "an origin table.", tablename.c_str());
	    return XORP_ERROR;
	}
    }

    if (! vifname.empty()) {
	//
	// Add a route with explicitly specified network interface
	//
	map<const string, Vif>::iterator iter = _vifs.find(vifname);
	if (iter == _vifs.end()) {
	    XLOG_ERROR("Attempting to add route to table \"%s\" "
		       "(prefix %s next-hop %s ifname %s vifname %s): "
		       "no such network interface",
		       tablename.c_str(), net.str().c_str(),
		       nexthop_addr.str().c_str(),
		       ifname.c_str(), vifname.c_str());
	    return XORP_ERROR;
	}
	Vif* vif = &iter->second;
	IPNextHop<A>* nexthop = find_or_create_peer_nexthop(nexthop_addr);
	ot->add_route(IPRouteEntry<A>(net, vif, nexthop, *protocol, metric));
	flush();
	return XORP_OK;
    }

    //
    // Find the vif so we can see if the nexthop is a peer.  First
    // lookup the nexthop addr, and see it's the subnet is a directly
    // connected one.
    //
    const IPRouteEntry<A>* re = _final_table->lookup_route(nexthop_addr);
    if (re != NULL) {
	// We found a route.  Is the subnet directly connected?
	Vif* vif = re->vif();
	if ((vif != NULL)
	    && (vif->is_same_subnet(IPvXNet(re->net()))
		|| vif->is_same_p2p(IPvX(nexthop_addr)))) {
	    debug_msg("**directly connected route found for nexthop\n");
	    IPNextHop<A>* nexthop = find_or_create_peer_nexthop(nexthop_addr);
	    ot->add_route(IPRouteEntry<A>(net, re->vif(), nexthop,
					  *protocol, metric));
	    flush();
	    return XORP_OK;
	} else {
	    debug_msg("**not directly connected route found for nexthop\n");
	    //
	    // XXX: If the route came from an IGP, then we must have
	    // a directly-connected interface toward the next-hop router
	    //
	    if (protocol->protocol_type() == IGP) {
		XLOG_ERROR("Attempting to add IGP route to table \"%s\" "
			   "(prefix %s next-hop %s): no directly connected "
			   "interface toward the next-hop router",
			   tablename.c_str(), net.str().c_str(),
			   nexthop_addr.str().c_str());
		return XORP_ERROR;
	    }

	    IPNextHop<A>* nexthop = find_or_create_external_nexthop(nexthop_addr);
	    ot->add_route(IPRouteEntry<A>(net, /* No vif */ NULL, nexthop,
					  *protocol, metric));
	    flush();
	    return XORP_OK;
	}
    }

    debug_msg("** no route\n");
    //
    // We failed to find a route.  One final possibility is that this
    // route is the route for a directly connected subnet, so we need
    // to test all the Vifs to see if this is the case.
    //
    Vif* vif = find_vif(nexthop_addr);
    debug_msg("Vif %p\n", vif);
    IPNextHop<A>* nexthop;
    if (vif != NULL) {
	nexthop = find_or_create_peer_nexthop(nexthop_addr);
    } else {
	//
	// XXX: If the route came from an IGP, then we must have
	// a directly-connected interface toward the next-hop router
	//
	if (protocol->protocol_type() == IGP) {
	    XLOG_ERROR("Attempting to add IGP route to table \"%s\" "
		       "(prefix %s next-hop %s): no directly connected "
		       "interface toward the next-hop router",
		       tablename.c_str(), net.str().c_str(),
		       nexthop_addr.str().c_str());
	    return XORP_ERROR;
	}

	nexthop = find_or_create_external_nexthop(nexthop_addr);
    }
    XLOG_ASSERT(nexthop->addr() == nexthop_addr);

    // Only accept the least significant 16 bits of metric.
    if (metric > 0xffff) {
	XLOG_WARNING("IGP metric value %d is greater than 0xffff", metric);
	metric &= 0xffff;
    }

    ot->add_route(IPRouteEntry<A>(net, vif, nexthop, *protocol, metric));

    flush();
    return XORP_OK;
}

template <typename A>
int
RIB<A>::replace_route(const string&	tablename,
		      const IPNet<A>&	net,
		      const A&		nexthop_addr,
		      const string&	ifname,
		      const string&	vifname,
		      uint32_t		metric)
{
    RouteTable<A>* rt = find_table(tablename);
    if (NULL == rt)
	return XORP_ERROR; // Table does not exist

    OriginTable<A>* ot = dynamic_cast<OriginTable<A>* >(rt);
    if (NULL == ot)
	return XORP_ERROR; // Table is not an origin table

    int response = ot->delete_route(net);
    if (response != XORP_OK)
	return response;

    response = add_route(tablename, net, nexthop_addr, ifname, vifname,
			 metric);

    // No need to flush here, as add_route will do it for us.

    return response;
}

template <typename A>
int
RIB<A>::delete_route(const string& tablename, const IPNet<A>& net)
{
    RouteTable<A>* rt = find_table(tablename);
    if (NULL == rt)
	return XORP_ERROR; // Table does not exist

    OriginTable<A>* ot = dynamic_cast<OriginTable<A>* >(rt);
    if (NULL == ot)
	return XORP_ERROR; // Table is not an origin table

    int result = ot->delete_route(net);
    flush();
    return result;
}

template <typename A>
void
RIB<A>::flush()
{
    if (_register_table != NULL)
	_register_table->flush();
    if (_final_table != NULL && _final_table != _register_table)
	_final_table->flush();
}

template <typename A>
int
RIB<A>::verify_route(const A& lookup_addr,
		     const string& ifname,
		     const A& nexthop_addr,
		     uint32_t metric)
{
    const IPRouteEntry<A>* re;

    re = _final_table->lookup_route(lookup_addr);
    if (re == NULL || re->vif() == NULL) {
	// TODO: XXX: hard-coded interface name!!
	if (ifname == "discard") {
	    debug_msg("****ROUTE FAILURE SUCCESSFULLY VERIFIED****\n");
	    return XORP_OK;
	}
	if (re == NULL) {
	    debug_msg("RouteVerify: Route Lookup failed\n");
	} else {
	    debug_msg("Route lookup returned NULL vif: %s\n",
		      re->str().c_str());
	}
	return XORP_ERROR;
    }

    IPNextHop<A>* route_nexthop = dynamic_cast<IPNextHop<A>* >(re->nexthop());
    if (route_nexthop == NULL) {
	debug_msg("Next hop is not an IPNextHop\n");
	return XORP_ERROR;
    } else if ((nexthop_addr != route_nexthop->addr())) {
	debug_msg("NextHop: Exp: %s != Got: %s\n",
		  nexthop_addr.str().c_str(),
		  route_nexthop->addr().str().c_str());
	return XORP_ERROR;
    } else {
	debug_msg("NextHop: Exp: %s != Got: %s\n",
		  nexthop_addr.str().c_str(),
		  route_nexthop->addr().str().c_str());
    }
    if (ifname != re->vif()->name()) {
	XLOG_ERROR("Interface \"%s\" does not match expected \"%s\".",
		   re->vif()->str().c_str(), ifname.c_str());
	return XORP_ERROR;
    } else {
	debug_msg("Ifname: Exp: %s == Got: %s\n",
		  ifname.c_str(),
		  re->vif()->name().c_str());
    }
    if (metric != re->metric()) {
	XLOG_ERROR("Metric \"%d\" does not match expected \"%d\".",
		   re->metric(), metric);
	return XORP_ERROR;
    } else {
	debug_msg("Metric: Exp: %d == Got: %d\n", metric,
		  re->metric());
    }
    debug_msg("****ROUTE SUCCESSFULLY VERIFIED****\n");
    return XORP_OK;
}

template <typename A>
const A&
RIB<A>::lookup_route(const A& lookupaddr)
{
    debug_msg("looking up %s\n", lookupaddr.str().c_str());

    const IPRouteEntry<A>* re = _final_table->lookup_route(lookupaddr);
    if (re == NULL || re->vif() == NULL) {
	return A::ZERO();
    }

    IPNextHop<A>* route_nexthop = static_cast<IPNextHop<A>* >(re->nexthop());
    return route_nexthop->addr();
}

template <typename A>
RouteRange<A>*
RIB<A>::route_range_lookup(const A& lookupaddr)
{
    return _final_table->lookup_route_range(lookupaddr);
}

template <typename A>
RouteRegister<A>*
RIB<A>::route_register(const A& lookupaddr, const string& module)
{
    debug_msg("registering %s\n", lookupaddr.str().c_str());
    return _register_table->register_route_range(lookupaddr, module);
}

template <typename A>
int
RIB<A>::route_deregister(const IPNet<A>& subnet, const string& module)
{
    debug_msg("deregistering %s\n", subnet.str().c_str());
    return _register_table->deregister_route_range(subnet, module);
}

template <typename A>
RedistTable<A>*
RIB<A>::protocol_redist_table(const string& protocol)
{
    RouteTable<A>* rt = find_table(redist_tablename(protocol));
    if (rt) {
	return dynamic_cast<RedistTable<A>*>(rt);
    }
    return 0;
}

template <typename A>
int
RIB<A>::add_igp_table(const string& tablename,
		      const string& target_class,
		      const string& target_instance)
{
    debug_msg("add_igp_table %s\n", tablename.c_str());
    int r = add_origin_table(tablename, target_class, target_instance, IGP);
    if (r != XORP_OK)
	return r;

    // XXX For now we unconditionally plumb a RedistTable behind each
    // OriginTable.  We do this because the RedistTable needs to track
    // the routes within the OriginTable in order to be able to render
    // a dump when another protocol requests redistribution.
    r = add_redist_table(tablename);
    if (r != XORP_OK) {
	delete_origin_table(tablename, target_class, target_instance);
    }

    return r;
}

template <typename A>
int
RIB<A>::add_egp_table(const string& tablename,
		      const string& target_class,
		      const string& target_instance)
{
    debug_msg("add_egp_table %s\n", tablename.c_str());
    int r = add_origin_table(tablename, target_class, target_instance, EGP);
    if (r != XORP_OK) {
	return r;
    }

#if 0
    // XXX For now we unconditionally plumb a RedistTable behind each
    // OriginTable.  We need this because the RedistTable needs to
    // track the routes within the OriginTable in order to be able to
    // render a dump when another protocol requests redistribution.
    r = add_redist_table(tablename);
    if (r != XORP_OK) {
	delete_origin_table(tablename, target_class, target_instance);
    }
#endif
    return r;
}

template <typename A>
int
RIB<A>::add_redist_table(const string& parent_tablename)
{
    RouteTable<A>* p = find_table(parent_tablename);
    if (p == NULL) {
	XLOG_WARNING("add_redist_table: parent table %s does not exist",
		     parent_tablename.c_str());
	return XORP_ERROR;
    }

    if (find_table(redist_tablename(parent_tablename)) != 0) {
	return XORP_OK;		// RedistTable already exists, no sweat
    }

    RedistTable<A>* r;
    r = new RedistTable<A>(redist_tablename(parent_tablename), p);
    if (add_table(r) != XORP_OK) {
	delete r;
	return XORP_ERROR;
    }

    return XORP_OK;
}

//
// All the magic is in add_origin_table.
// TODO: XXX: split into smaller units (??)

template <typename A>
int
RIB<A>::add_origin_table(const string& tablename,
			 const string& target_class,
			 const string& target_instance,
			 ProtocolType protocol_type)
{
    debug_msg("add_origin_table %s type: %dn\n",
	      tablename.c_str(), protocol_type);

    Protocol* protocol = find_protocol(tablename);
    if (protocol == NULL) {
	protocol = new Protocol(tablename, protocol_type, 0);
	_protocols[tablename] = protocol;
    } else {
	protocol->increment_genid();
    }

    // Check if table exists and check type if so
    RouteTable<A>* rt = find_table(tablename);
    if (rt != NULL) {
	if (dynamic_cast<OriginTable<A>* >(rt) == NULL) {
	    XLOG_ERROR("add_origin_table: table \"%s\" already exists, but is "
		       "not is an OriginTable.", tablename.c_str());
	    return XORP_ERROR;
	} else {
	    return XORP_OK; 	    // table already exists, use that
	}
    }

    if (new_origin_table(tablename, target_class, target_instance,
			 admin_distance(tablename), protocol_type)
	!= XORP_OK) {
	debug_msg("new_origin_table failed\n");
	return XORP_ERROR;
    }

    OriginTable<A>* new_table = static_cast<OriginTable<A>* >(find_table(tablename));
    if (_final_table == new_table) {
	//
	// This is the first table, so no need to plumb anything
	// this won't occur if there's an ExportTable<A>, because that
	// would have been created already.
	//
	debug_msg("first table\n");
	return XORP_OK;
    }

    //
    // There are existing tables, so we need to do some plumbing
    //
    // Three possibilities:
    //  1. there are existing EGP tables but no existing IGP table
    //  2. there are both existing EGP and existing IGP tables
    //  3. there are existing IGP tables but no existing EGP table
    //
    // If we're adding an IGP table:
    // we can handle 2 and 3 the same, adding a MergedTable, but 1
    // requires we construct an ExtInt table instead.
    //
    // If we're adding an EGP table:
    // we can handle 1 and 2 the same, adding a MergedTable, but 3
    // requires we construct an ExtInt table instead.
    //

    // First step: find out what tables already exist
    RouteTable<A>* 	igp_table = NULL;
    RouteTable<A>* 	egp_table = NULL;
    ExtIntTable<A>* 	ei_table  = NULL;

    typename list<RouteTable<A>* >::iterator li;
    for (li = _tables.begin(); li != _tables.end(); ++li) {
	// XXX: skip the new table!
	RouteTable<A>* current = *li;
	if (current == new_table) {
	    continue;
	}

	OriginTable<A>* ot = dynamic_cast<OriginTable<A>* >(current);
	if (ot != NULL) {
	    if (ot->protocol_type() == IGP) {
		igp_table = ot;
	    } else if (ot->protocol_type() == EGP) {
		egp_table = ot;
	    } else {
		XLOG_UNREACHABLE();
	    }
	    continue;
	} else {
	    if (ei_table == NULL)
		ei_table = dynamic_cast<ExtIntTable<A>* >(current);
	}
    }

    //
    // Depending on which tables already exist, we may need to create
    // a MergedTable or an ExtInt table.
    //
    if (((igp_table == NULL) && (protocol_type == IGP)) ||
	((egp_table == NULL) && (protocol_type == EGP))) {
	//
	// Sanity check: we've found an ExtIntTable, when there
	// weren't both IGP and EGP tables.
	//
	XLOG_ASSERT(ei_table == NULL);

	if ((egp_table == NULL) && (igp_table == NULL)) {
	    //
	    // There are tables, but neither IGP or EGP origin tables
	    // Therefore the final table must be an ExportTable<A> or
	    // a RegisterTable (MRIB's have no ExportTable<A>).
	    //
	    if (_final_table->type() != EXPORT_TABLE
		&& _final_table->type() != REGISTER_TABLE) {
		XLOG_UNREACHABLE();
	    }

	    //
	    // There may be existing single-parent tables before the
	    // ExportTable such as RegisterTable - track back to be
	    // first of them.
	    //
	    RouteTable<A>* rt = track_back(_final_table,
					   EXPORT_TABLE
					   | REDIST_TABLE
					   | REGISTER_TABLE);

	    //
	    // Plumb our new table in ahead of the first single-parent
	    // table.
	    //
	    rt->replumb(NULL, new_table);
	    new_table->set_next_table(rt);
	    return XORP_OK;
	}

	//
	// We are going to need to create an ExtInt table.
	//
	// Find the appropriate existng table to be a parent
	// of the ExtIntTable.
	//
	RouteTable<A>* next_table = track_back(_final_table,
					       EXPORT_TABLE
					       | REDIST_TABLE
					       | REGISTER_TABLE);
	RouteTable<A>* existing_table = next_table->parent();
	if (protocol_type == IGP) {
	    ei_table = new ExtIntTable<A>(existing_table, new_table);
	} else {
	    ei_table = new ExtIntTable<A>(new_table, existing_table);
	}

	// XXX: Added table to list of resources (was not done previously)
	if (add_table(ei_table) != XORP_OK) {
	    XLOG_ERROR("Failed to add ExtIntTable \"%s\".",
		       ei_table->tablename().c_str());
	    delete ei_table;
	    return XORP_ERROR;
	}

	if (_final_table->type() & (EXPORT_TABLE | REGISTER_TABLE)) {
	    ei_table->set_next_table(next_table);
	    next_table->replumb(existing_table, ei_table);
	} else {
	    _final_table = ei_table;
	}
	return XORP_OK;
    }

    //
    // We're going to need to create a MergedTable
    //
    RouteTable<A>* existing_table = (protocol_type == IGP) ? igp_table : egp_table;
    RouteTable<A>* next_table = existing_table->next_table();

    // Skip past any RedistTables
    RouteTable<A>* new_prev_table = track_forward(existing_table,
						  REDIST_TABLE);
    if (new_prev_table != existing_table) {
	existing_table = new_prev_table;
	next_table = existing_table->next_table();
    }

    MergedTable<A>* merged_table = new MergedTable<A>(existing_table,
						      new_table);
    if (merged_table == NULL || add_table(merged_table) != XORP_OK) {
	delete merged_table;
	return XORP_ERROR;
    }

    merged_table->set_next_table(next_table);
    if (next_table != NULL)
	next_table->replumb(existing_table, merged_table);

    //
    // It's possible existing_table was the last table - if so, then it
    // isn't anymore.
    //
    if (_final_table == existing_table)
	_final_table = merged_table;

    return XORP_OK;
}

template <typename A>
int
RIB<A>::delete_igp_table(const string& tablename,
			 const string& target_class,
			 const string& target_instance)
{
    return delete_origin_table(tablename, target_class, target_instance);
}

template <typename A>
int
RIB<A>::delete_egp_table(const string& tablename,
			 const string& target_class,
			 const string& target_instance)
{
    return delete_origin_table(tablename, target_class, target_instance);
}

template <class A>
int
RIB<A>::delete_origin_table(const string& tablename,
			    const string& target_class,
			    const string& target_instance)
{
    OriginTable<A>* ot = dynamic_cast<OriginTable<A>* >(find_table(tablename));
    if (NULL == ot)
	return XORP_ERROR;

    if (!target_instance.empty()) {
	if (find_table_by_instance(tablename, target_class, target_instance)
	    != ot) {
	    XLOG_ERROR("Got delete_origin_table for wrong target name\n");
	    return XORP_ERROR;
	} else {
	    _routing_protocol_instances.erase(tablename + " "
					      + target_class + " "
					      + target_instance);
	}
    }

    // Remove all the routes this table used to originate, but keep table
    ot->routing_protocol_shutdown();
    return XORP_OK;
}

template <class A>
void
RIB<A>::target_death(const string& target_class,
		     const string& target_instance)
{
    string s = " " + target_class + " " + target_instance;
    typename map<const string, OriginTable<A>* >::iterator iter;
    for (iter = _routing_protocol_instances.begin();
	 iter != _routing_protocol_instances.end();
	 ++iter) {
	if (iter->first.find(s) != string::npos) {
	    // We've found the target.
	    iter->second->routing_protocol_shutdown();
	    _routing_protocol_instances.erase(iter);

	    // No need to go any further.
	    return;
	}
    }
}

//
// Given a single-parent table, track back to the last matching table
// before this one.
//
template <typename A>
RouteTable<A>*
RIB<A>::track_back(RouteTable<A>* rt, int typemask) const
{
    if (rt == NULL || (rt->type() & typemask) == 0) {
	return rt;
    }

    for (RouteTable<A>* parent = rt->parent();
	 parent && (parent->type() & typemask);
	 parent = rt->parent()) {
	rt = parent;
    }
    return rt;
}

//
// Track forward to the last matching table, or return this table if
// the next table doesn't match.
//
template <typename A>
RouteTable<A>*
RIB<A>::track_forward(RouteTable<A>* rt, int typemask) const
{
    debug_msg("here1\n");
    RouteTable<A>* next;
    if (NULL == rt) {
	// XXX: not the same test as track back (deliberate ?)
	return rt;
    }
    next = rt->next_table();
    debug_msg("here2\n");
    while (next != NULL) {
	debug_msg("here3\n");
	if ((next->type() & typemask) != 0) {
	    debug_msg("here4 next->type()= %d, typemask=%x\n",
		      next->type(), typemask);
	    rt = next;
	    next = rt->next_table();
	} else {
	    debug_msg("here5\n");
	    return rt;
	}
    }
    debug_msg("here6\n");
    return rt;
}

template <typename A>
void
RIB<A>::print_rib() const
{
#ifdef DEBUG_LOGGING
    typename map<string, RouteTable<A>* >::const_iterator pair;
    pair = _tables.begin();
    // XXX: this is printf not debug_msg for a reason.
    printf("==============================================================\n");
    while (pair != _tables.end()) {
	RouteTable<A>* rt = pair->second;
	printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
	printf(rt->str().c_str());
	rt = rt->next_table();
	while (rt != NULL) {
	    printf(rt->str().c_str());
	    rt = rt->next_table();
	}
	++pair;
    }
    printf("==============================================================\n");
#endif // DEBUG_LOGGING
}

template <typename A>
string
RIB<A>::name() const
{
    return c_format("%s %s RIB",
		    (_multicast) ? "Multicast" : "Unicast",
		    A::ip_version_str().c_str());
}

template class RIB<IPv4>;
template class RIB<IPv6>;
