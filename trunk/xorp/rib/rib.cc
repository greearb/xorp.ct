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

#ident "$XORP: xorp/rib/rib.cc,v 1.1.1.1 2002/12/11 23:56:13 hodson Exp $"

#include "config.h"
#include "urib_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "register_server.hh"
#include "rib.hh"

//#define DEBUG_LOGGING

// ----------------------------------------------------------------------------
// Inline table utility methods

template<class A> inline RouteTable<A>*
RIB<A>::find_table(const string& tablename)
{
    typename map<string, RouteTable<A>*>::iterator mi = _tables.find(tablename);
    if (mi == _tables.end()) {
	return 0;
    }
    return mi->second;
}

template<class A> inline int
RIB<A>::add_table(const string& tablename, RouteTable<A>* table)
{
    if (find_table(tablename) != 0) {
	XLOG_WARNING("add_table: table %s already exists\n", tablename.c_str());
	return -1;
    }
    _tables[tablename] = table;
    return 0;
}

template<class A> inline int
RIB<A>::remove_table(const string& tablename)
{
    typename map<string, RouteTable<A>*>::iterator mi = _tables.find(tablename);
    if (mi == _tables.end()) {
	XLOG_WARNING("remove_table: table %s doesn't exist\n", 
		     tablename.c_str());
	return -1;
    }
    _tables.erase(mi);
    return 0;
}

template<class A> inline int
RIB<A>::admin_distance(const string& tablename)
{
    map<const string, int>::iterator mi = _admin_distances.find(tablename);
    if (mi == _admin_distances.end()) {
	XLOG_ERROR("Administrative distance of \"%s\" unknown.",
		   tablename.c_str());
	return _admin_distances["unknown"];
    }
    return mi->second;
}

template<class A> inline Vif*
RIB<A>::find_vif(const A& addr)
{
    map<const string, Vif>::iterator mi;
    for(mi = _vifs.begin(); mi != _vifs.end(); mi++) {
	Vif& v = mi->second;
	if (v.is_my_addr(addr)) 
	    return &v;
    }
    return 0;
}

template<class A> inline IPExternalNextHop<A>*
RIB<A>::find_external_nexthop(const A& addr)
{
    typename map<const A, IPExternalNextHop<A> >::iterator mi;
    mi = _external_nexthops.find(addr);
    if (mi == _external_nexthops.end())
	return 0;
    return &mi->second;
}

template<class A> inline IPPeerNextHop<A>*
RIB<A>::find_peer_nexthop(const A& addr)
{
    typename map<const A, IPPeerNextHop<A> >::iterator mi;
    mi = _peer_nexthops.find(addr);
    if (mi == _peer_nexthops.end())
	return 0;
    return &mi->second;
}

template<class A> inline IPExternalNextHop<A>*
RIB<A>::find_or_create_external_nexthop(const A& addr)
{
    IPExternalNextHop<A>* nh = find_external_nexthop(addr);
    if (nh) return nh;
    typedef map<const A,IPExternalNextHop<A> > C;	// ugly, but convenient
    typename C::value_type vt(addr, IPExternalNextHop<A>(addr));
    typename C::iterator i = _external_nexthops.insert(_external_nexthops.end(), vt);
    return &i->second;
}

template<class A> inline IPPeerNextHop<A>*
RIB<A>::find_or_create_peer_nexthop(const A& addr)
{
    IPPeerNextHop<A>* nh = find_peer_nexthop(addr);
    if (nh) return nh;
    typedef map<const A,IPPeerNextHop<A> > C;		// ugly, but convenient
    typename C::value_type vt(addr, addr);
    typename C::iterator i = _peer_nexthops.insert(_peer_nexthops.end(), vt);
    return &i->second;
}

// ----------------------------------------------------------------------------
// Naming utility methods

static inline string
redist_tablename(const string& fromtable, const string& totable)
{
    return "Redist:" + fromtable + "->" + totable;
}

static inline string
extint_tablename(const string& ext_tablename, const string& int_tablename)
{
    return "Ext:(" + ext_tablename + ")Int:(" + int_tablename + ")";
}

static inline string
merge_tablename(const string& table_a, const string& table_b)
{
    return "Merge:(" + table_a + ")+(" + table_b + ")";
}

// ----------------------------------------------------------------------------
// RIB class 

template<class A>
RIB<A>::RIB(RibTransportType t) : _final_table(0), _register_table(0) 
{
    if (t == UNICAST) {
	_mcast = false;
    } else if (t == MULTICAST) {
	_mcast = true;
    } else {
	XLOG_FATAL("Unknown RibTransportType.");
    }
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
    _admin_distances["unknown"] =        255;
}

template<class A> int
RIB<A>::initialize_export(FeaClient *fea) 
{
    ExportTable<A>* et = new ExportTable<A>("ExportToFEA", 0, fea);
    if (add_table("ExportToFEA", et) != 0) {
	XLOG_FATAL("Export already initialized.");
	//delete et;
	//return -1;
    }
    _final_table = et;
    return 0;
}

template<class A>
RIB<A>::~RIB<A>()
{
    while(_tables.size() != 0) {
	delete _tables.begin()->second;
	_tables.erase(_tables.begin());
    }
}

template<class A> int
RIB<A>::initialize_register(RegisterServer *regserv) 
{
    if (_register_table) {
	XLOG_WARNING("Register table already initialized.");
	return -1;
    }

    RegisterTable<A>* rt 
	= new RegisterTable<A>("RegisterTable", regserv, _mcast);
    if (add_table("RegisterTable", rt) != 0) {
	// Hopefully never reached.
	XLOG_WARNING("Add RegisterTable failed.");
	delete rt;
	return -1;
    }
    _register_table = rt;

    if (find_table("ExportToFEA") == 0) {
	//No ExportTable<A> - perhaps we're an MRIB.
	if (_final_table != 0) {
	    XLOG_FATAL("No export table when initializing register table");
	}
	_final_table = _register_table;
    } else {
	_final_table->replumb(0, _register_table);
	_register_table->set_next_table(_final_table);
    }
    return 0;
}

template<class A> int
RIB<A>::new_origin_table(const string&	tablename, 
			 int		admin_distance,
			 int		igp)
{
    OriginTable<A>* ot = new OriginTable<A>(tablename, admin_distance, igp);
    if (ot == 0 || add_table(tablename, ot) != 0) {
	XLOG_WARNING("Could not add origin table %s", tablename.c_str());
	delete ot;
	return -1;
    } else if (_final_table == 0) {
	_final_table = ot;
    }
    return 0;
}

template<class A> int
RIB<A>::new_merged_table(const string& tablename, 
			 const string& ta,
			 const string& tb) 
{
    RouteTable<A>* table_a = find_table(ta);
    if (0 == table_a) {
	XLOG_WARNING("Attempted to create merged table \"%s\" from not existent table \"%s\"", tablename.c_str(), ta.c_str());
	return -1;
    }

    RouteTable<A>* table_b = find_table(tb);
    if (0 == table_b) {
	XLOG_WARNING("Attempted to create merged table \"%s\" from not existent table \"%s\"", tablename.c_str(), tb.c_str());
	return -1; 
    }

    MergedTable<A>* mt = new MergedTable<A>(tablename, table_a, table_b);
    if (mt == 0 || add_table(tablename, mt) != 0) {
	XLOG_WARNING("Could not add merge table \"%s\"", tablename.c_str());
	delete mt;
	return -1;
    }
    _final_table = mt;
    return 0;
}

template<class A>
int 
RIB<A>::new_extint_table(const string& tablename, 
			 const string& t_ext,
			 const string& t_int) 
{
    RouteTable<A>* table_ext = find_table(t_ext);
    if (0 == table_ext) {
	XLOG_WARNING("Could not create extint table \"%s\" as external table "
		     "\"%s\" does not exist", 
		     tablename.c_str(), t_ext.c_str());
	return -1;
    }

    RouteTable<A>* table_int = find_table(t_int);
    if (0 == table_int) {
	XLOG_WARNING("Could not create extint table \"%s\" as internal table "
		     "\"%s\" does not exist",
		     tablename.c_str(), t_int.c_str());
	return -1;
    }

    ExtIntTable<A>* eit = new ExtIntTable<A>(tablename, table_ext, table_int);
    if (eit == 0 || add_table(tablename, eit) != 0) {
	XLOG_WARNING("Could not add extint table \"%s\"", tablename.c_str());
	delete eit;
	return -1;
    } 
    _final_table = eit;
    return 0;
}

template<class A> 
int
RIB<A>::new_vif(const string& vifname, const Vif& vif) 
{
    debug_msg("RIB::new_vif: %s\n", vifname.c_str());
    if (_vifs.find(vifname) == _vifs.end()) {
	// Can't use _vifs[vifname] = vif because no Vif() constructor
	map<const string, Vif>::value_type v(vifname, vif);
	_vifs.insert(_vifs.end(), v);
	return 0;
    }
    return -1;
}

template<class A> 
int
RIB<A>::delete_vif(const string& vifname) 
{
    debug_msg("RIB::delete_vif: %s\n", vifname.c_str());
    map<const string, Vif>::iterator i;
    i = _vifs.find(vifname);
    if (i == _vifs.end()) {
	return -1;
    }
    list<VifAddr>::const_iterator vai;
    for (vai = i->second.addr_list().begin(); 
	 vai != i->second.addr_list().end();
	 vai++) {
	//delete the directly connected routes associated with this VIF
	IPvXNet subnetvX = vai->subnet_addr();
	IPNet<A> subnet;
	subnetvX.get(subnet);
	delete_route("connected", subnet);
    }

    _vifs.erase(i);
    return 0;
}

template<class A> int
RIB<A>::add_vif_address(const string&	vifname, 
			const A&	addr, 
			const IPNet<A>&	subnet) 
{
    map<const string, Vif>::iterator vi = _vifs.find(vifname);
    if (vi == _vifs.end()) {
	XLOG_ERROR("Attempting to add address to non-existant Vif \"%s\"",
		   vifname.c_str());
	return -1;
    }
    vi->second.add_address(VifAddr(addr, subnet, A::ZERO(), A::ZERO()));
    //add a route for this subnet
    add_route("connected", subnet, addr, /*best possible metric*/0);
    return 0;
}

template<class A> int
RIB<A>::delete_vif_address(const string& vifname, 
			   const A& addr) 
{
    map<const string, Vif>::iterator vi = _vifs.find(vifname);
    if (vi == _vifs.end()) {
	XLOG_ERROR("Attempting to delete address from non-existant Vif \"%s\"",
		   vifname.c_str());
	return -1;
    }
    list<VifAddr>::const_iterator vai;
    for (vai = vi->second.addr_list().begin(); 
	 vai != vi->second.addr_list().end();
	 vai++) {
	IPvX addrvX = vai->addr();
	A this_addr;
	addrvX.get(this_addr);
	if (addr == this_addr) {
	    IPvXNet subnetvX = vai->subnet_addr();
	    IPNet<A> subnet;
	    subnetvX.get(subnet);
	    delete_route("connected", subnet);
	    return 0;
	}
    }
    return -1;
}

template<class A>
int RIB<A>::add_route(const string& tablename, 
		      const IPNet<A>& net, 
		      const A& nexthop_addr,
		      uint32_t metric) 
{
    RouteTable<A>* rt = find_table(tablename);
    if (rt == NULL) {
	XLOG_ERROR("Attempting to add route to unknown table \"%s\".",
		   tablename.c_str());
	abort();
	return -1;
    }

    OriginTable<A>* ot = dynamic_cast<OriginTable<A>* >(rt);
    if (ot == NULL) {
	XLOG_ERROR("Attempting to add route to table \"%s\" that is not "
		   "an origin table.", tablename.c_str());
	return -1;
    }

    //Find the vif so we can see if the nexthop is a peer.  first
    //lookup the nexthop addr, and see it's the subnet is a directly
    //conencted one.
    const IPRouteEntry<A>* re = _final_table->lookup_route(nexthop_addr);
    if (re) {
	//We found a route.  Is the subnet directly connected?
	if (re->directly_connected()) {
	    debug_msg("**directly connected route found for nexthop\n");
	    IPNextHop<A>* nh = find_or_create_peer_nexthop(nexthop_addr);
	    ot->add_route(IPRouteEntry<A>(net, re->vif(), nh, NULL, metric));
	    flush();
	    return 0;
	} else {
	    debug_msg("**not directly connected route found for nexthop\n");
	    IPNextHop<A>* nh = find_or_create_external_nexthop(nexthop_addr);
	    ot->add_route(IPRouteEntry<A>(net, /*No vif*/NULL, nh, 
					  NULL, metric));
	    flush();
	    return 0;
	}
    }

    debug_msg("** no route\n");
    //We failed to find a route.  One final possibility is that this
    //route is the route for a directly connected subnet, so we need
    //to test all the Vifs to see if this is the case.
    Vif* vif = find_vif(nexthop_addr);
    debug_msg("Vif %p\n", vif);
    IPNextHop<A>* nh;
    if (vif) {
	nh = find_or_create_peer_nexthop(nexthop_addr);
    } else {
	nh = find_or_create_external_nexthop(nexthop_addr);
    }
    assert(nh->addr() == nexthop_addr);

    //only accept the least significant 16 bits of metric
    if (metric > 0xffff) {
	XLOG_WARNING(c_format("IGP metric value %d is greater than 2^16\n", 
			      metric).c_str());
	metric &= 0xffff;
    }

    ot->add_route(IPRouteEntry<A>(net, vif, nh, NULL, metric));

    flush();
    return 0;
}

template<class A>
int 
RIB<A>::replace_route(const string& tablename, 
		      const IPNet<A>& net, 
		      const A& nexthop_addr,
		      uint32_t metric) 
{
    RouteTable<A>* rt = find_table(tablename);
    if (0 == rt) return -1; /* Table does not exist */

    OriginTable<A>* ot = dynamic_cast<OriginTable<A>*>(rt);
    if (0 == ot) return -1; /* Table is not an origin table */

    int response = ot->delete_route(net);

    if (response!=0) {
	return response;
    }
    response = add_route(tablename, net, nexthop_addr, metric);

    //no need to flush here, as add_route will do it for us

    return response;
}

template<class A> 
int 
RIB<A>::delete_route(const string& tablename, const IPNet<A>& net) 
{
    RouteTable<A>* rt = find_table(tablename);
    if (0 == rt) return -1; /* Table does not exist */

    OriginTable<A>* ot = dynamic_cast<OriginTable<A>*>(rt);
    if (0 == ot) return -1; /* Table is not an origin table */

    int result = ot->delete_route(net);
    flush();
    return result;
}

template<class A> 
void
RIB<A>::flush()
{
    if (_register_table != NULL)
	_register_table->flush();
    if (_final_table != NULL && _final_table != _register_table)
	_final_table->flush();
}

template<class A> 
int
RIB<A>::verify_route(const A& lookup_addr, 
		     const string& ifname, 
		     const A& nh_addr,
		     uint32_t metric) 
{
    const IPRouteEntry<A> *re;

    re = _final_table->lookup_route(lookup_addr);
    if (re == 0 || re->vif() == 0) {
	if (ifname == "discard") {
	    debug_msg("****ROUTE FAILURE SUCCESSFULLY VERIFIED****\n");
	    return 0;
	} 
	if (re == 0) {
	    debug_msg("RouteVerify: Route Lookup failed\n");
	} else {
	    debug_msg("Route lookup returned NULL vif: %s\n", 
		      re->str().c_str());
	}
	return -1;
    }

    IPNextHop<A>* route_nh = dynamic_cast<IPNextHop<A>*>(re->nexthop());
    if (route_nh == 0) {
	debug_msg("Next hop is not an IPNextHop\n");
	return -1;
    } else if ((nh_addr != route_nh->addr())) {
	debug_msg("NextHop: Exp: %s != Got: %s\n",
		  nh_addr.str().c_str(),
		  route_nh->addr().str().c_str());
	return -1;
    } else {
	debug_msg("NextHop: Exp: %s != Got: %s\n",
		  nh_addr.str().c_str(),
		  route_nh->addr().str().c_str());
    }
    if (ifname != re->vif()->name()) {
	XLOG_ERROR("Interface \"%s\" does not match expected \"%s\".", 
		   re->vif()->str().c_str(), ifname.c_str());
	return -1;
    } else {
	debug_msg("Ifname: Exp: %s == Got: %s\n", 
		  ifname.c_str(),
		  re->vif()->name().c_str());
    }
    if (metric != re->metric()) {
	XLOG_ERROR("Metric \"%d\" does not match expected \"%d\".", 
		   re->metric(), metric);
	return -1;
    } else {
	debug_msg("Metric: Exp: %d == Got: %d\n", metric, 
		  re->metric());
    }
    debug_msg("****ROUTE SUCCESSFULLY VERIFIED****\n");
    return 0;
}

template<class A>
inline static const A& addr_zero(const A&)
{
    return A::ZERO();
}

template<>
inline static const IPv4& addr_zero(const IPv4&)
{
    static IPv4 z = IPv4::ZERO(); /* Needs an instance to return a temporary */
    return z;
}

template<class A> const A&
RIB<A>::lookup_route(const A& lookupaddr) 
{
    debug_msg("looking up %s\n", lookupaddr.str().c_str());

    const IPRouteEntry<A>* re = _final_table->lookup_route(lookupaddr);
    if (re == 0 || re->vif() == 0) {
	return addr_zero(lookupaddr);
    }

    IPNextHop<A>* route_nh = static_cast<IPNextHop<A>*>(re->nexthop());
    return route_nh->addr();
}

template<class A> RouteRange<A>*
RIB<A>::route_range_lookup(const A& lookupaddr) {
    return _final_table->lookup_route_range( lookupaddr );
}

template<class A> RouteRegister<A> *
RIB<A>::route_register(const A& lookupaddr, const string& module) {
    debug_msg("registering %s\n", lookupaddr.str().c_str());
    return _register_table->register_route_range(lookupaddr, module);
}

template<class A> 
bool
RIB<A>::route_deregister(const IPNet<A>& subnet, const string &module) {
    debug_msg("deregistering %s\n", subnet.str().c_str());
    return _register_table->deregister_route_range(subnet, module);
}

template<class A> 
int
RIB<A>::redist_enable(const string& fromtable, const string& totable) {
    // We can redistribute from any RouteTable<A>;
    RouteTable<A>* fromtab = find_table(fromtable);
    if (fromtab == 0) {
	XLOG_ERROR("Attempt to redistribute from non-existent table \"%s\".",
		   fromtable.c_str());
	return -1;
    }

    // Find totable
    RouteTable<A>* trt = find_table(totable);
    if (trt == 0) {
	XLOG_ERROR("Attempt to redistribute to non-existent table \"%s\".",
		   totable.c_str());
	return -1;
    }

    // We can only redistribute to an OriginTable
    OriginTable<A>* totab = dynamic_cast<OriginTable<A>*>(trt);
    if (totab == 0) {
	XLOG_ERROR("Redistribution failed \"%s\" is not an origin table.",
		   totable.c_str());
	return -1;
    }

    string tablename = redist_tablename(fromtable, totable);
    RedistTable<A>* rdt = new RedistTable<A>(tablename, fromtab, totab);
    if (add_table(tablename, rdt) != 0) {
	XLOG_WARNING("Redistribution failed because redist table \"%s\" "
		     "already exists", tablename.c_str());
	delete rdt;
	return -1;
    }
    return 0;
}

template<class A> 
int
RIB<A>::redist_disable(const string& fromtable, const string& totable) 
{
    string tname = redist_tablename(fromtable, totable);

    // XXX This routine currently trawls tables map twice.
    RedistTable<A>* rdt = dynamic_cast<RedistTable<A>*>(find_table(tname));
    if (rdt == 0) {
	XLOG_WARNING("Attempt to disable redistribution \"%s\" when "
		     "redistribution is not enabled.", tname.c_str());
	return -1;
    }
    remove_table(tname);
    return 0;
}

template<class A> 
int 
RIB<A>::add_igp_table(const string& tablename) {
    debug_msg("add_igp_table %s\n", tablename.c_str());
    return add_origin_table(tablename, IGP);
}

template<class A> 
int
RIB<A>::add_egp_table(const string& tablename) {
    debug_msg("add_egp_table %s\n", tablename.c_str());
    return add_origin_table(tablename, EGP);
}

/* All the magic is in add_origin_table - XXX split into smaller units (?) */

template<class A> 
int
RIB<A>::add_origin_table(const string& tablename, int type) {
    debug_msg("add_origin_table %s type: %dn\n",
	      tablename.c_str(), type);

    /* Check if table exists and check type if so */
    RouteTable<A>* rt = find_table(tablename);
    if (rt) {
	if (dynamic_cast<OriginTable<A>*>(rt) == 0) {
	    XLOG_ERROR("add_origin_table: table \"%s\" already exists, but is "
		       "not is an OriginTable.", tablename.c_str());
	    return -1;
	} else {
	    return 0; 	    /* table already exists, use that */
	}
    }

    if (new_origin_table(tablename, admin_distance(tablename), type) < 0) {
	debug_msg("new_origin_table failed\n");
	return -1;
    }

    OriginTable<A>* new_table = static_cast<OriginTable<A>*>(find_table(tablename));
    if (_final_table == new_table) {
	/* this is the first table, so no need to plumb anything*/
	/* this won't occur if there's an ExportTable<A>, because that
           would have been created already*/
	debug_msg("first table\n");
	return 0;
    } 

    /* There are existing tables, so we need to do some plumbing */
    /* Three possibilities:
     *  1. there are existing EGP tables but no existing IGP table
     *  2. there are both existing EGP and existing IGP tables
     *  3. there are existing IGP tables but no existing EGP table
     *
     * If we're adding an IGP table:
     * we can handle 2 and 3 the same, adding a MergedTable, but 1
     * requires we construct an ExtInt table instead.  
     *
     * If we're adding an EGP table:
     * we can handle 1 and 2 the same, adding a MergedTable, but 3
     * requires we construct an ExtInt table instead.  
     */

    /* First step: find out what tables already exist */
    RouteTable<A>* igp_table = 0;
    RouteTable<A>* egp_table = 0;
    ExtIntTable<A>* ei_table = 0;

    typedef typename map<string, RouteTable<A> *>::iterator Iter;
    Iter rtpair = _tables.begin();
    for (rtpair = _tables.begin(); rtpair != _tables.end(); ++rtpair) {
	/*skip the new table!*/
	if (rtpair->second == new_table) {
	    continue;
	}

	OriginTable<A>* ot = dynamic_cast<OriginTable<A>*>(rtpair->second);
	if (ot) {
	    /* XXX igp() as a method name (?) */
	    if (ot->igp() == IGP) {
		igp_table = ot;
	    } else if (ot->igp() == EGP) {
		egp_table = ot; 
	    } else {
		abort();
	    }
	    continue;
	}
	
	if (ei_table == 0)
	    ei_table = dynamic_cast<ExtIntTable<A>*>(rtpair->second);
    }


    /* depending on which tables already exist, we may need to create
       a MergedTable or an ExtInt table */
    if (((igp_table == 0) && (type == IGP)) || 
	((egp_table == 0) && (type == EGP))) {
	/*we're going to need to create an ExtInt table*/

	if (ei_table) {
	    /* sanity check: we've found an ExtIntTable, when there
	      weren't both IGP and EGP tables*/
	    abort();
	}

	if ((egp_table == 0) && (igp_table == 0)) {
	    /* There are tables, but neither IGP or EGP origin tables */
	    /* Therefore the final table must be an ExportTable<A> or 
	       a RegisterTable (MRIB's have no ExportTable<A>) */
	    if (_final_table->type() != EXPORT_TABLE 
		&& _final_table->type() != REGISTER_TABLE) {
		abort();
	    }

	    /* there may be existing single-parent tables before the
               ExportTable such as RegisterTable - track back to be
               first of them. */
	    RouteTable<A>* rt = track_back(_final_table, 
					   EXPORT_TABLE | REGISTER_TABLE);

	    /* new plumb our new table in ahead of the first
               single-parent table */
	    rt->replumb(0, new_table);
	    new_table->set_next_table(rt);
	    //	    print_rib();
	    return 0;
	}

	/* Find the appropriate existng table to be a parent of the
           ExtIntTable */
	RouteTable<A>* next_table = track_back(_final_table,
					       EXPORT_TABLE | REGISTER_TABLE);
	RouteTable<A>* existing_table = next_table->parent();
	string einame;
	if (type == IGP) {
	    einame = extint_tablename(existing_table->tablename(), tablename);
	    ei_table = new ExtIntTable<A>(einame, existing_table, new_table);
	} else {
	    einame = extint_tablename(tablename, existing_table->tablename());
	    ei_table = new ExtIntTable<A>(einame, new_table, existing_table);
	}

	/* XXX Added table to list of resources (was not done previously) */
	if (ei_table == 0) {
	    XLOG_ERROR("Failed to create ExtIntTable \"%s\".", einame.c_str());
	    return -1;
	} else if (add_table(ei_table->tablename(), ei_table) != 0) {
	    XLOG_ERROR("Failed to add ExtIntTable \"%s\".", einame.c_str());
	    delete ei_table;
	    return -1;
	}

	if (_final_table->type() & (EXPORT_TABLE | REGISTER_TABLE)) {
	    ei_table->set_next_table(next_table);
	    next_table->replumb(existing_table, ei_table);
	} else {
	    _final_table = ei_table;
	}

	//print_rib();
	return 0;
    }

    /* We're going to need to create a MergedTable */

    RouteTable<A>* existing_table = (type == IGP) ? igp_table : egp_table;
    RouteTable<A>* next_table = existing_table->next_table();
    RouteTable<A>* new_prev_table = track_forward(existing_table, 
						  REDIST_TABLE);

    /* Skip past any RedistTables */
    if (new_prev_table != existing_table) {
	existing_table = new_prev_table;
	next_table = existing_table->next_table();
    }

    string mtname = merge_tablename(existing_table->tablename(),
				    tablename);
    MergedTable<A>* merged_table = new MergedTable<A>(mtname,
						      existing_table,
						      new_table);
    if (merged_table == 0 || add_table(mtname, merged_table)) {
	delete merged_table;
	return -1;
    }

    merged_table->set_next_table(next_table);
    if (next_table != 0)
	next_table->replumb(existing_table, merged_table);

    /* It's possible existing_table was the last table - if so, then it
     * isn't anymore. 
     */
    if (_final_table == existing_table)
	_final_table = merged_table;

    //print_rib();
    return 0;
}

template<class A> int
RIB<A>::delete_igp_table(const string& tablename) 
{
    return delete_origin_table(tablename);
}

template<class A> int
RIB<A>::delete_egp_table(const string& tablename)
{
    return delete_origin_table(tablename);
}

template <class A> int
RIB<A>::delete_origin_table(const string& tablename) 
{
    OriginTable<A>* ot = dynamic_cast<OriginTable<A>*>(find_table(tablename));
    if (0 == ot) return -1;

    /* Remove all the routes this table used to originate, but keep table */
    ot->delete_all_routes();
    return 0;
}

/*
 * Given a single-parent table, track back to the last matching table
 * before this one 
 */

template<class A>
RouteTable<A> *RIB<A>::track_back(RouteTable<A> *rt, int typemask) const {

    if (rt == 0 || (rt->type() & typemask) == 0) {
	return rt;
    }

    for (RouteTable<A>* parent = rt->parent(); 
	 parent && (parent->type() & typemask);
	 parent = rt->parent()) {
	rt = parent;
    }
    return rt;
}

/*
 * track forward to the last matching table, or return this table if
 * the next table doesn't match */

template<class A>
RouteTable<A> *RIB<A>::track_forward(RouteTable<A> *rt, int typemask) const {
    debug_msg("here1\n");
    RouteTable<A>* next;
    if (0 == rt) {
	/* XXX not the same test as track back (deliberate ?) */
	return rt;
    }
    next = rt->next_table();
    debug_msg("here2\n");
    while (next != 0) {
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


template<class A>
void RIB<A>::print_rib() const {
#ifdef DEBUG_LOGGING
    typedef map<string, RouteTable<A> *>::const_iterator CI;
    CI pair;
    pair = _tables.begin(); 
    //this is printf not debug_msg for a reason.
    printf("==============================================================\n");
    while (pair != _tables.end()) {
	RouteTable<A> *rt = pair->second;
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
#endif /* DEBUG_LOGGING */
}



template class RIB<IPv4>;
template class RIB<IPv6>;
