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

// You may also (at your option) redistribute this software and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or any later version.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#ident "$XORP: xorp/contrib/ospfd/xorp/xrl_target.C,v 1.7 2004/04/02 10:33:14 mjh Exp $"

#include <time.h>
#include <vector>

#include "ospf_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/status_codes.h"

#include "ospfinc.h"
#include "tcppkt.h"
#include "system.h"
#include "os-instance.h"
#include "ospfd_xorp.h"
#include "xrl_target.h"

// ----------------------------------------------------------------------------
// Utility methods for handling named enumerations

struct NamedEnumItem {
public:
    NamedEnumItem(const char* n, int32_t v) : _name(n), _value(v) {}
    inline const char* name() const { return _name; }
    inline int32_t value() const { return _value; }
protected:
    const char* 	_name;
    int32_t	 	_value;
};

class NamedEnumBase {
public:
    NamedEnumBase() {}
    virtual ~NamedEnumBase() {}
    const NamedEnumItem* item_by_name(const char* name) const {
	for (uint32_t i = 0; i < _items.size(); i++) {
	    if (strcasecmp(name, _items[i].name()) == 0) {
		return &_items[i];
	    }
	}
	return 0;
    }
    const uint32_t item_count() const { 
	return _items.size();
    }
    const NamedEnumItem* item(uint32_t index) const {
	if (index < _items.size()) {
	    return &_items[index];
	}
	return 0;
    }
    uint32_t index_by_name(const char* name) const {
	const NamedEnumItem* r = item_by_name(name);
	if (0 == r) {
	    return _items.size();
	}
	return r - &_items[0];
    }
    string names() const {
	string s = c_format("\"%s\"", item(0)->name());
	for (uint32_t i = 1; i < item_count(); i++) {
	    s += c_format(", \"%s\"", item(i)->name());
	}
	return s;
    }
    string values() const {
	string s = c_format("\"%d\"", item(0)->value());
	for (uint32_t i = 1; i < item_count(); i++) {
	    s += c_format(", \"%d\"", item(i)->value());
	}
	return s;
    }
protected:
    vector<NamedEnumItem> _items;
};

static struct CryptEnum : public NamedEnumBase {
    CryptEnum() {
	_items.push_back(NamedEnumItem("none", AUT_NONE));
	_items.push_back(NamedEnumItem("cleartext", AUT_PASSWD));
	_items.push_back(NamedEnumItem("md5", AUT_CRYPT));
    }
} crypt_enum;

static struct InterfaceCryptEnum : public NamedEnumBase {
    InterfaceCryptEnum() {
	_items.push_back(NamedEnumItem("none", AUT_NONE));
	_items.push_back(NamedEnumItem("cleartext", AUT_PASSWD));
    }
} if_crypt_enum;

static struct IfTypeEnum : public NamedEnumBase {
    IfTypeEnum() {
	_items.push_back(NamedEnumItem("broadcast", IFT_BROADCAST));
	_items.push_back(NamedEnumItem("pp", IFT_PP));
	_items.push_back(NamedEnumItem("nbma", IFT_NBMA));
	_items.push_back(NamedEnumItem("p2mp", IFT_P2MP));
	_items.push_back(NamedEnumItem("vl", IFT_VL));
	_items.push_back(NamedEnumItem("loopback", IFT_LOOPBK));
    }
} if_type_enum;

static struct MulticastForwardingEnum : public NamedEnumBase {
    MulticastForwardingEnum() {
	_items.push_back(NamedEnumItem("blocked", IF_MCFWD_BLOCKED));
	_items.push_back(NamedEnumItem("multicast", IF_MCFWD_MC));
	_items.push_back(NamedEnumItem("unicast", IF_MCFWD_UNI));
    }
} if_multicast_enum;

// ----------------------------------------------------------------------------
// Common Interface Methods 

XrlCmdError 
XrlOspfTarget::common_0_1_get_target_name(
					  // Output values, 
					  string& name)
{
    name = XrlOspfTargetBase::name();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::common_0_1_get_version(
				      // Output values, 
				      string& version)
{
    version = c_format("OSPFD v%d.%d", (int)OSPF::vmajor, (int)OSPF::vminor);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::common_0_1_get_status(// Output values, 
				     uint32_t& status,
				     string& reason)
{
    if (ospf()->shutting_down()) {
	status = PROC_SHUTDOWN;
	reason = "Shutting down";
    } else {
	status = PROC_READY;
	reason = "Ready";
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::common_0_1_shutdown()
{
    //give ospf 30 seconds to shut down
    ospf()->shutdown(30);
    return XrlCmdError::OKAY();
}


// ----------------------------------------------------------------------------
// Macros to help OSPF Global Config Methods

#define FAST_FAIL_ON_NO_OSPF() 						\
   if (ospf_ready() == false)						\
	return XrlCmdError::COMMAND_FAILED("Awaiting router id.");

#define OSPF_GLOBAL_SET_FIELD(field, val)				\
    CfgGen cg; 								\
    ospf()->qryOspf(cg); 				/* Get */	\
    cg.field = val;		 			/* Modify */	\
    ospf()->cfgOspf(&cg); 				/* Set */	\
    CfgGen cg_after;							\
    ospf()->qryOspf(cg_after); 				/* Get again */	\
    if (cg.field != cg_after.field)			/* Verify */	\
	return XrlCmdError::COMMAND_FAILED("Set failed.");	

#define OSPF_GLOBAL_GET_FIELD(var, field) 				\
    CfgGen cg; 								\
    ospf()->qryOspf(cg); 						\
    const CfgGen& ccg = cg;						\
    var = ccg.field;		 					

// ----------------------------------------------------------------------------
// OSPF Global Config Methods

XrlCmdError 
XrlOspfTarget::ospf_0_1_set_router_id(
				      // Output values,
				      const uint32_t& id)
{
    if (ospf() && ospf()->my_id() == id) {
	// Nothing to be done
	string reason = c_format("Router ID already set to %u.", id); 

	return XrlCmdError::COMMAND_FAILED(reason);
    }

    // This is the only place we deal with _pp_ospf.  Everything else
    // in this file uses XrlOspfTarget::opsf() to get a pointer to the
    // OSPF object. (NB ospf() == *_pp_ospf).
    delete *_pp_ospf;
    *_pp_ospf = new OSPF(id, sys_etime);

    // Set defaults
    CfgGen initial_config;
    initial_config.set_defaults();
    ospf()->cfgOspf(&initial_config);

    return XrlCmdError::OKAY(); 
}

XrlCmdError
XrlOspfTarget::ospf_0_1_get_router_id(
				      // Output values, 
				      uint32_t&	id)
{
    FAST_FAIL_ON_NO_OSPF();
    id = ospf()->my_id();
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_set_lsdb_limit(
				       // Output values, 
				       const int32_t& limit)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_SET_FIELD(lsdb_limit, limit);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_get_lsdb_limit(
				       // Output values, 
				       int32_t&	limit)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_GET_FIELD(limit, lsdb_limit);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_set_mospf(
				  // Input values, 
				  const bool&	enabled)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_SET_FIELD(mospf_enabled, enabled);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_get_mospf(
				  // Input values, 
				  bool& enabled)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_GET_FIELD(enabled, mospf_enabled);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_set_interarea_mc(
					 // Input values, 
					 const bool&	enabled)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_SET_FIELD(inter_area_mc, enabled);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_get_interarea_mc(
					 // Output values, 
					 bool&	enabled)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_GET_FIELD(enabled, inter_area_mc);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_set_overflow_interval(
					      // Input values, 
					      const int32_t&	ovfl_int)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_SET_FIELD(ovfl_int, ovfl_int);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_get_overflow_interval(
					      // Output values, 
					      int32_t&	ovfl_int)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_GET_FIELD(ovfl_int, ovfl_int);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_set_flood_rate(
				       // Input values, 
				       const int32_t&	rate)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_SET_FIELD(new_flood_rate, rate);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_get_flood_rate(
				       // Output values, 
				       int32_t&	rate)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_GET_FIELD(rate, new_flood_rate);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_set_max_rxmt_window(
					    // Input values, 
					    const uint32_t&	window)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_SET_FIELD(max_rxmt_window, window);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_get_max_rxmt_window(
					    // Output values, 
					    uint32_t&	window)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_GET_FIELD(window, max_rxmt_window);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_set_max_dds(
				    // Input values, 
				    const uint32_t&	max_dds)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_SET_FIELD(max_dds, max_dds);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_get_max_dds(
				    // Output values, 
				    uint32_t&	max_dds)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_GET_FIELD(max_dds, max_dds);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_set_lsa_refresh_rate(
					     // Input values, 
					     const uint32_t&	rate)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_SET_FIELD(refresh_rate, rate);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_get_lsa_refresh_rate(
					     // Output values, 
					     uint32_t&	rate)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_GET_FIELD(rate, refresh_rate);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_set_p2p_adj_limit(
					  // Input values, 
					  const uint32_t&	max_adj)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_SET_FIELD(PPAdjLimit, max_adj);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_get_p2p_adj_limit(
					  // Output values, 
					  uint32_t&	max_adj)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_GET_FIELD(max_adj, PPAdjLimit);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_set_random_refresh(
					   // Input values, 
					   const bool&	enabled)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_SET_FIELD(random_refresh, enabled);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_get_random_refresh(
					   // Output values, 
					   bool&	enabled)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_GLOBAL_GET_FIELD(enabled, random_refresh);
    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// Area configuration methods

XrlCmdError
XrlOspfTarget::ospf_0_1_add_or_configure_area(
					      // Input values, 
					      const uint32_t&	area_id, 
					      const bool&	is_stub, 
					      const uint32_t&	default_cost, 
					      const bool&	import_summary_routes) 
{
    FAST_FAIL_ON_NO_OSPF();
    CfgArea ca;
    ca.area_id		= area_id;
    ca.stub 		= is_stub;
    ca.dflt_cost 	= default_cost;
    ca.import_summs	= import_summary_routes;
    ospf()->cfgArea(&ca, ADD_ITEM);

    CfgArea sca;
    ospf()->qryArea(sca, area_id);
    if (memcmp(&sca, &ca, sizeof(sca))) {
	return XrlCmdError::COMMAND_FAILED("Failed to set one or more "
					   "attributes");
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_delete_area(
				    // Input values, 
				    const uint32_t&	area_id) 
{
    FAST_FAIL_ON_NO_OSPF();
    CfgArea ca;
    ca.area_id = area_id;
    ospf()->cfgArea(&ca, DELETE_ITEM);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_query_area(
				   // Input values, 
				   const uint32_t&	area_id, 
				   // Output values, 
				   bool&	is_stub, 
				   uint32_t&	default_cost, 
				   bool&	import_summary_routes) 
{
    FAST_FAIL_ON_NO_OSPF();
    CfgArea ca;
    if (ospf()->qryArea(ca, area_id) == false) {
	return XrlCmdError::COMMAND_FAILED("No area matching area_id");
    }

    is_stub	 	  = ca.stub;
    default_cost 	  = ca.dflt_cost;
    import_summary_routes = ca.import_summs;

    return XrlCmdError::OKAY();
}


XrlCmdError
XrlOspfTarget::ospf_0_1_list_area_ids(
				      // Output values, 
				      XrlAtomList&	area_ids) 
{	
    FAST_FAIL_ON_NO_OSPF();
    list<CfgArea> lca;
    ospf()->getAreas(lca);

    list<CfgArea>::const_iterator ci = lca.begin();
    while (ci != lca.end()) {
	area_ids.append(XrlAtom(ci->area_id));
	ci++;
    }
    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// Aggregate configuration methods

XrlCmdError
XrlOspfTarget::ospf_0_1_add_or_configure_aggregate(
	// Input values, 
	const uint32_t&	area_id, 
	const IPv4&	network, 
	const IPv4&	netmask, 
	const bool&	suppress_advertisement)
{
    FAST_FAIL_ON_NO_OSPF();
    CfgRnge cr;
    cr.area_id = area_id;
    cr.net     = network.addr();
    cr.mask    = netmask.addr();
    cr.no_adv  = suppress_advertisement;
    ospf()->cfgRnge(&cr, ADD_ITEM);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_delete_aggregate(
	// Input values, 
	const uint32_t&	area_id, 
	const IPv4&	network, 
	const IPv4&	netmask)
{
    FAST_FAIL_ON_NO_OSPF();
    CfgRnge cr;
    cr.area_id = area_id;
    cr.net     = network.addr();
    cr.mask    = netmask.addr();
    ospf()->cfgRnge(&cr, DELETE_ITEM);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_query_aggregate(
	// Input values, 
	const uint32_t&	area_id, 
	const IPv4&	network, 
	const IPv4&	netmask, 
	// Output values, 
	bool&	suppress_advertisement)
{
    FAST_FAIL_ON_NO_OSPF();
    CfgRnge cr;
    cr.area_id = area_id;
    cr.net     = network.addr();
    cr.mask    = netmask.addr();
    if (ospf()->qryRnge(cr, cr.area_id, cr.net, cr.mask)) {
	return XrlCmdError::COMMAND_FAILED("Aggregate does not exist.");
    }
    suppress_advertisement = cr.no_adv;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_list_aggregates(
	// Input values, 
	const uint32_t&	area_id, 
	// Output values, 
	XrlAtomList&	networks, 
	XrlAtomList&	netmasks)
{
    FAST_FAIL_ON_NO_OSPF();

    list<CfgRnge> lcr;
    ospf()->getRnges(lcr, area_id);

    list<CfgRnge>::const_iterator ci = lcr.begin();
    while (ci != lcr.end()) {
	IPv4 network(ci->net);
	networks.append(XrlAtom(network));
	
	IPv4 netmask(ci->mask);
	netmasks.append(XrlAtom(netmask));

	ci++;
    }
    
    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// Host configuration methods

XrlCmdError
XrlOspfTarget::ospf_0_1_add_or_configure_host(
	// Input values, 
	const IPv4&	network, 
	const IPv4&	netmask, 
	const uint32_t&	area_id, 
	const uint32_t&	cost)
{
    FAST_FAIL_ON_NO_OSPF();

    if (cost > 65535) {
	return XrlCmdError::COMMAND_FAILED("Cost must be within the range "
					   "0-65535.");
    }
    
    CfgArea ca;
    if (ospf()->qryArea(ca, area_id) == false) {
	return XrlCmdError::COMMAND_FAILED("No area matching area_id");
    }

    CfgHost ch;
    ch.net     = network.addr();
    ch.mask    = netmask.addr();
    ch.area_id = area_id;
    ch.cost    = (uns16)cost;
    ospf()->cfgHost(&ch, ADD_ITEM);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_delete_host(
	// Input values, 
	const IPv4&	network, 
	const IPv4&	netmask, 
	const uint32_t&	area_id)
{
    FAST_FAIL_ON_NO_OSPF();

    CfgHost ch;
    ch.net     = network.addr();
    ch.mask    = netmask.addr();
    ch.area_id = area_id;
    ospf()->cfgHost(&ch, DELETE_ITEM);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_query_host(
	// Input values, 
	const IPv4&	network, 
	const IPv4&	netmask, 
	const uint32_t&	area_id, 
	// Output values, 
	uint32_t&	cost)
{
    FAST_FAIL_ON_NO_OSPF();

    CfgHost ch;
    if (ospf()->qryHost(ch, area_id, 
			network.addr(), netmask.addr()) == false) {
	return XrlCmdError::COMMAND_FAILED("Host not found inside area");
    }

    cost = ch.cost;

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_list_hosts(
	// Input values, 
	const uint32_t&	area_id, 
	// Output values, 
	XrlAtomList&	networks, 
	XrlAtomList&	netmasks)
{
    FAST_FAIL_ON_NO_OSPF();

    list<CfgHost> lch;
    ospf()->getHosts(lch, area_id);

    list<CfgHost>::const_iterator ci = lch.begin();
    while (ci != lch.end()) {
	IPv4 network(ci->net);
	networks.append(XrlAtom(network));
	
	IPv4 netmask(ci->mask);
	netmasks.append(XrlAtom(netmask));

	ci++;
    }

    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// Virtual Link configuration methods

/* Accessor helper macros */

#define OSPF_VLINK_SET_FIELD(desc, trans_id, nbr_id, param, field, max_val)   \
    CfgVL cv;								      \
    if (ospf()->qryVL(cv, trans_id, nbr_id) == false) {			      \
	return XrlCmdError::COMMAND_FAILED("No virtual link matching transit" \
					   "id and neighbor id");	      \
    }									      \
    if (param > max_val) {						      \
	return XrlCmdError::COMMAND_FAILED("Parameter " #desc " should be in" \
				           " range 0 to " #max_val ".");      \
    }									      \
    cv.field = param;							      \
    ospf()->cfgVL(&cv, ADD_ITEM);

#define OSPF_VLINK_GET_FIELD(desc, trans_id, nbr_id, param, field)	      \
    CfgVL cv;								      \
    if (ospf()->qryVL(cv, trans_id, nbr_id) == false) {			      \
	return XrlCmdError::COMMAND_FAILED("No virtual link matching transit" \
			       	       	   "id and neighbor id");	      \
    }									      \
    param = cv.field;							      \

XrlCmdError
XrlOspfTarget::ospf_0_1_add_vlink(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id)
{
    FAST_FAIL_ON_NO_OSPF();

    CfgVL cv;
    cv.transit_area = transit_area;
    cv.nbr_id       = neighbor_id;

    // Fill in defaults
    cv.xmt_dly	    = 5;
    cv.rxmt_int	    = 10;
    cv.hello_int    = 30;
    cv.dead_int     = 180;
    cv.auth_type    = AUT_NONE;
    memset(cv.auth_key, 0, sizeof(cv.auth_key));
    ospf()->cfgVL(&cv, ADD_ITEM);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_delete_vlink(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id)
{
    FAST_FAIL_ON_NO_OSPF();
    CfgVL cv;
    cv.transit_area = transit_area;
    cv.nbr_id       = neighbor_id;
    ospf()->cfgVL(&cv, DELETE_ITEM);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_vlink_set_transmit_delay(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	const uint32_t&	delay_secs)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_VLINK_SET_FIELD("transmit delay", transit_area, neighbor_id, 
			 delay_secs, xmt_dly, 255);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_vlink_get_transmit_delay(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	// Output values, 
	uint32_t&	delay_secs)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_VLINK_GET_FIELD("transmit delay", transit_area, neighbor_id, 
			 delay_secs, xmt_dly);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_vlink_set_retransmit_interval(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	const uint32_t&	interval_secs)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_VLINK_SET_FIELD("retransmit interval", transit_area, neighbor_id, 
			 interval_secs, rxmt_int, 255);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_vlink_get_retransmit_interval(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	// Output values, 
	uint32_t&	interval_secs)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_VLINK_GET_FIELD("retransmit interval", transit_area, neighbor_id, 
			 interval_secs, rxmt_int);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_vlink_set_hello_interval(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	const uint32_t&	interval_secs)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_VLINK_SET_FIELD("hello interval", transit_area, neighbor_id, 
			 interval_secs, hello_int, 65535);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_vlink_get_hello_interval(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	// Output values, 
	uint32_t&	interval_secs)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_VLINK_GET_FIELD("hello interval", transit_area, neighbor_id, 
			 interval_secs, hello_int);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_vlink_set_router_dead_interval(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	const uint32_t&	interval_secs)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_VLINK_SET_FIELD("dead interval", transit_area, neighbor_id, 
			 interval_secs, dead_int, 65535);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_vlink_get_router_dead_interval(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	// Output values, 
	uint32_t&	interval_secs)
{
    FAST_FAIL_ON_NO_OSPF();
    OSPF_VLINK_GET_FIELD("dead interval", transit_area, neighbor_id, 
			 interval_secs, dead_int);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_vlink_set_authentication(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	const string&	type, 
	const string&	key)
{
    FAST_FAIL_ON_NO_OSPF();

    CfgVL cv;
    if (ospf()->qryVL(cv, transit_area, neighbor_id) == false) {
	return XrlCmdError::COMMAND_FAILED("No virtual link matching transit "
					   "id and neighbor id");
    }

    const NamedEnumItem* nei = crypt_enum.item_by_name(type.c_str());
    if (nei == 0) {
	string oi = "Authentication type should be either ";	
	oi += crypt_enum.names();
	return XrlCmdError::COMMAND_FAILED(oi);
    }

    cv.auth_type = nei->value();
    for (uint32_t i = 0; i < sizeof(cv.auth_key); i++) {
	cv.auth_key[i] = (i < key.size()) ? key[i] : 0;
    }
    ospf()->cfgVL(&cv, ADD_ITEM);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_vlink_get_authentication(
	// Input values, 
	const uint32_t&	transit_area, 
	const uint32_t&	neighbor_id, 
	// Output values, 
	string&	type, 
	string&	key)
{
    FAST_FAIL_ON_NO_OSPF();
    CfgVL cv;
    if (ospf()->qryVL(cv, transit_area, neighbor_id) == false) {
	return XrlCmdError::COMMAND_FAILED("No virtual link matching transit "
					   "id and neighbor id");
    }

    const NamedEnumItem* nei = crypt_enum.item(cv.auth_type);
    if (0 == nei) {
	string oops = c_format("Internal value \"%d\" is not in known "
			       "enumerated values ", cv.auth_type);
	oops += crypt_enum.values();
	return XrlCmdError::COMMAND_FAILED(oops);
    }
    type = nei->name();

    char s[sizeof(cv.auth_key) + 1];
    memcpy(s, cv.auth_key, sizeof(cv.auth_key));
    s[sizeof(cv.auth_key)] = 0;
    key = s;

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_list_vlinks(
	// Input values, 
	const uint32_t&	transit_id, 
	// Output values, 
	XrlAtomList&	neighbor_ids)
{
    FAST_FAIL_ON_NO_OSPF();

    list<CfgVL> lcv;
    ospf()->getVLs(lcv, transit_id);
    for (list<CfgVL>::iterator i = lcv.begin(); i != lcv.end(); i++) {
	neighbor_ids.append(XrlAtom(i->nbr_id));
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_add_or_configure_external_route(
	// Input values, 
	const IPv4Net&	network, 
	const IPv4&	gateway, 
	const uint32_t&	type, 
	const uint32_t&	cost, 
	const bool&	multicast, 
	const uint32_t&	external_route_tag) 
{
    FAST_FAIL_ON_NO_OSPF();

    if (type != 1 || type != 2) {
	return XrlCmdError::COMMAND_FAILED("External route type must be "
					   "1 or 2");
    }
    if (cost > 65535) {
	return XrlCmdError::COMMAND_FAILED("External route cost must be "
					   "between 0 and 65535");
    }
    CfgExRt ce;
    ce.net	= network.masked_addr().addr();
    ce.mask	= IPv4::make_prefix(network.prefix_len()).addr();
    ce.type2	= (type == 2) ? true : false;
    ce.mc	= multicast;
    ce.direct	= 0;
    ce.noadv	= 0;
    ce.cost	= cost;
    ce.gw	= gateway.addr();
    ce.phyint	= _xorp_ospfd.get_phyint(ce.gw);
    ce.tag	= external_route_tag;
    ospf()->cfgExRt(&ce, ADD_ITEM);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_delete_external_route(
	// Input values, 
	const IPv4Net&	network, 
	const IPv4&	gateway) 
{
    FAST_FAIL_ON_NO_OSPF();
    CfgExRt ce;
    ce.net	= network.masked_addr().addr();
    ce.mask	= IPv4::make_prefix(network.prefix_len()).addr();
    ce.gw	= gateway.addr();
    ce.phyint	= _xorp_ospfd.get_phyint(ce.gw);
    ospf()->cfgExRt(&ce, DELETE_ITEM);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfTarget::ospf_0_1_query_external_route(
	// Input values, 
	const IPv4Net&	network, 
	const IPv4&	gateway, 
	// Output values, 
	uint32_t&	type, 
	uint32_t&	cost, 
	bool&		multicast, 
	uint32_t&	external_route_tag) 
{
    FAST_FAIL_ON_NO_OSPF();
    CfgExRt ce;
    ce.net	= network.masked_addr().addr();
    ce.mask	= IPv4::make_prefix(network.prefix_len()).addr();
    ce.gw	= gateway.addr();
    ce.phyint	= _xorp_ospfd.get_phyint(ce.gw);
    if (ospf()->qryExRt(ce, ce.net, ce.mask, ce.gw, ce.phyint) == false) {
	return XrlCmdError::COMMAND_FAILED("External route not found.");
    }
    type	       = (ce.type2) ? 2 : 1;
    cost	       = ce.cost;
    multicast	       = ce.mc;
    external_route_tag = ce.tag;
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_list_external_routes(
	// Input values, 
	const IPv4Net&	network, 
	// Output values, 
	XrlAtomList&	gateways)
{
    FAST_FAIL_ON_NO_OSPF();

    InAddr net  = network.masked_addr().addr();
    InMask mask = IPv4::make_prefix(network.prefix_len()).addr();

    list<CfgExRt> l;
    ospf()->getExRts(l, net, mask);
    for (list<CfgExRt>::const_iterator ci = l.begin(); ci != l.end(); ci++) {
	gateways.append(XrlAtom(IPv4(ci->gw)));
    }
    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// OSPF Interface configuration

#define FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, bsdphyint)	      \
    in_addr addr;							      \
    BSDPhyInt* bsdphyint;						      \
    if (!_xorp_ospfd.parse_interface(identifier.c_str(), addr, bsdphyint)) {  \
	string r = c_format("Interface %s not known.", identifier.c_str());   \
	return XrlCmdError::COMMAND_FAILED(r);				      \
    }

#define OSPF_INTERFACE_GET_FIELD(bsdphyint, var, field)			      \
    CfgIfc m;								      \
    if (!ospf()->qryIfc(m, bsdphyint->get_addr(),bsdphyint->get_phyint())) {  \
	return XrlCmdError::COMMAND_FAILED("Failed to get config");	      \
    }									      \
    var = m.field;

#define OSPF_INTERFACE_SET_FIELD(bsdphyint, field, var)			      \
    CfgIfc m;								      \
    if (!ospf()->qryIfc(m, bsdphyint->get_addr(),bsdphyint->get_phyint())) {  \
	return XrlCmdError::COMMAND_FAILED("Failed to get config");	      \
    }									      \
    m.field = var;							      \
    ospf()->cfgIfc(&m, ADD_ITEM);					      \
    if (!ospf()->qryIfc(m, bsdphyint->get_addr(),bsdphyint->get_phyint())) {  \
	return XrlCmdError::COMMAND_FAILED("Failed to get config (2)");	      \
    }									      \
    if (var != m.field)							      \
	return XrlCmdError::COMMAND_FAILED("Set failed.");

XrlCmdError 
XrlOspfTarget::ospf_0_1_add_interface(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	if_index, 
	const uint32_t&	area_id, 
	const uint32_t&	cost, 
	const uint32_t&	mtu, 
	const string&	type, 
	const bool&	on_demand, 
	const bool&	passive)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);

    const NamedEnumItem* nei = if_type_enum.item_by_name(type.c_str());
    if (0 == nei) {
	string hey = "Interface type should be either ";
	hey += if_type_enum.names();
	return XrlCmdError::COMMAND_FAILED(hey);
    }

    CfgIfc m;
    m.address 	= phyp->get_addr();
    m.phyint  	= phyp->get_phyint();
    m.mask    	= phyp->get_mask();
    m.mtu     	= (mtu) ? mtu : phyp->get_mtu();
    m.IfIndex 	= if_index;
    m.area_id 	= area_id;
    m.IfType  	= nei->value();
    m.demand  	= on_demand;
    m.passive 	= passive;
    m.if_cost	= cost;

    // Values initial defaults set for (hopefully sane)
    m.dr_pri	= 1;
    m.xmt_dly	= 5;
    m.rxmt_int	= 10;
    m.hello_int	= 30;
    m.dead_int	= 180;
    m.auth_type = AUT_NONE;
    memset(m.auth_key, 0, sizeof(m.auth_key));
    m.mc_fwd	= IF_MCFWD_UNI;
    m.igmp	= 0;
    ospf()->cfgIfc(&m, ADD_ITEM);

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_set_if_index(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	index)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_SET_FIELD(phyp, IfIndex, index);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_get_if_index(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	uint32_t&	index)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_GET_FIELD(phyp, index, IfIndex);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_set_area_id(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	area_id)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_SET_FIELD(phyp, area_id, area_id);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_get_area_id(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	uint32_t&	area_id)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_GET_FIELD(phyp, area_id, area_id);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_set_cost(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	cost)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_SET_FIELD(phyp, if_cost, cost);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_get_cost(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	uint32_t&	cost)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_GET_FIELD(phyp, cost, if_cost);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_set_mtu(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	mtu)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_SET_FIELD(phyp, mtu, mtu);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_get_mtu(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	uint32_t&	mtu)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_GET_FIELD(phyp, mtu, mtu);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_set_type(
	// Input values, 
	const string&	identifier, 
	const string&	type)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);

    const NamedEnumItem* nei = if_type_enum.item_by_name(type.c_str());
    if (0 == nei) {
	string hey = "Interface type should be either ";
	hey += if_type_enum.names();
	return XrlCmdError::COMMAND_FAILED(hey);
    }

    OSPF_INTERFACE_SET_FIELD(phyp, IfType, nei->value());
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_get_type(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	string&	type)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);

    CfgIfc m;  
    if (!ospf()->qryIfc(m, phyp->get_addr(), phyp->get_phyint())) {
	return XrlCmdError::COMMAND_FAILED("Failed to get config");
    }

    const NamedEnumItem* nei = if_type_enum.item(m.IfType);
    if (0 == nei) {
	string oops = c_format("Internal value \"%d\" is not in known "
			       "enumerated values ", m.IfType);
	oops += if_type_enum.values();
	return XrlCmdError::COMMAND_FAILED(oops);
    }

    type = nei->name();
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_set_dr_priority(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	dr_priority)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_SET_FIELD(phyp, dr_pri, dr_priority);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_get_dr_priority(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	uint32_t&	dr_priority)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_GET_FIELD(phyp, dr_priority, dr_pri);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_set_transit_delay(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	delay_secs)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_SET_FIELD(phyp, xmt_dly, delay_secs);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_get_transit_delay(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	uint32_t&	delay_secs)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_GET_FIELD(phyp, delay_secs, xmt_dly);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_set_retransmit_interval(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	interval_secs)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_SET_FIELD(phyp, rxmt_int, interval_secs);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_get_retransmit_interval(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	uint32_t&	interval_secs)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_GET_FIELD(phyp, interval_secs, rxmt_int);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_set_router_dead_interval(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	interval_secs)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_SET_FIELD(phyp, dead_int, interval_secs);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_get_router_dead_interval(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	uint32_t&	interval_secs)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_GET_FIELD(phyp, interval_secs, dead_int);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_set_poll_interval(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	interval_secs)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_SET_FIELD(phyp, poll_int, interval_secs);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_get_poll_interval(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	uint32_t&	interval_secs)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_GET_FIELD(phyp, interval_secs, poll_int);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_set_authentication(
	// Input values, 
	const string&	identifier, 
	const string&	type, 
	const string&	key)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);

    const NamedEnumItem* nei = if_crypt_enum.item_by_name(type.c_str());
    if (0 == nei) {
	string oi = "Authentication type should be either ";	
	oi += if_crypt_enum.names();
	return XrlCmdError::COMMAND_FAILED(oi);
    }

    CfgIfc m;  
    if (!ospf()->qryIfc(m, phyp->get_addr(), phyp->get_phyint())) {
	return XrlCmdError::COMMAND_FAILED("Failed to get config");
    }
    m.auth_type = nei->value();
    for (uint32_t i = 0; i < sizeof(m.auth_key); i++) {
	m.auth_key[i] = (i < key.size()) ? key[i] : 0;
    }
    ospf()->cfgIfc(&m, ADD_ITEM);

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_get_authentication(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	string&	type, 
	string&	key)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);

    CfgIfc m;  
    if (!ospf()->qryIfc(m, phyp->get_addr(), phyp->get_phyint())) {
	return XrlCmdError::COMMAND_FAILED("Failed to get config");
    }
    const NamedEnumItem* nei = if_crypt_enum.item(m.auth_type);
    if (0 == nei) {
	string oops = c_format("Internal value \"%d\" is not in known "
			       "enumerated values ", m.auth_type);
	oops += if_crypt_enum.values();
	return XrlCmdError::COMMAND_FAILED(oops);
    }
    type = nei->name();

    char s[sizeof(m.auth_key) + 1];
    memcpy(s, m.auth_key, sizeof(m.auth_key));
    s[sizeof(m.auth_key)] = 0;
    key = s;

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_set_multicast_forwarding(
	// Input values, 
	const string&	identifier, 
	const string&	type)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);

    const NamedEnumItem* nei = if_multicast_enum.item_by_name(type.c_str());
    if (0 == nei) {
	string oi = "Multicast forwarding type should be either ";	
	oi += if_multicast_enum.names();
	return XrlCmdError::COMMAND_FAILED(oi);
    }    

    CfgIfc m;  
    if (!ospf()->qryIfc(m, phyp->get_addr(), phyp->get_phyint())) {
	return XrlCmdError::COMMAND_FAILED("Failed to get config");
    }
    m.mc_fwd = nei->value();
    ospf()->cfgIfc(&m, ADD_ITEM);

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_get_multicast_forwarding(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	string&		type)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);

    CfgIfc m;  
    if (!ospf()->qryIfc(m, phyp->get_addr(), phyp->get_phyint())) {
	return XrlCmdError::COMMAND_FAILED("Failed to get config");
    }
    const NamedEnumItem* nei = if_multicast_enum.item(m.mc_fwd);
    if (0 == nei) {
	string oops = c_format("Internal value \"%d\" is not in known "
			       "enumerated values ", m.mc_fwd);
	oops += if_multicast_enum.values();
	return XrlCmdError::COMMAND_FAILED(oops);
    }
    type = nei->name();
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_set_on_demand(
	// Input values, 
	const string&	identifier, 
	const bool&	on_demand)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_SET_FIELD(phyp, demand, on_demand);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_get_on_demand(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	bool&	on_demand)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_GET_FIELD(phyp, on_demand, demand);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_set_passive(
	// Input values, 
	const string&	identifier, 
	const bool&	passive)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_SET_FIELD(phyp, passive, passive);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_get_passive(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	bool&	passive)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_GET_FIELD(phyp, passive, passive);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_set_igmp(
	// Input values, 
	const string&	identifier, 
	const bool&	enabled)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_SET_FIELD(phyp, igmp, enabled);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_get_igmp(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	bool&	enabled)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);
    OSPF_INTERFACE_GET_FIELD(phyp, enabled, igmp);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_delete_interface(
	// Input values, 
	const string&	identifier)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);

    CfgIfc m;  
    if (!ospf()->qryIfc(m, phyp->get_addr(), phyp->get_phyint())) {
	return XrlCmdError::COMMAND_FAILED("Failed to get config");
    }
    ospf()->cfgIfc(&m, DELETE_ITEM);

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_list_interfaces(
	// Output values, 
	XrlAtomList&	identifiers)
{
    FAST_FAIL_ON_NO_OSPF();

    list<CfgIfc> lci;
    ospf()->getIfcs(lci);
    for (list<CfgIfc>::const_iterator i = lci.begin(); i != lci.end(); i++) {
	identifiers.append(XrlAtom(IPv4(i->address)));
    }

    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// Interface MD5 Key configuration

static const char OSPF_TIME_DATE_FMT[] = "%Y-%m-%d %H:%M:%S";

static inline bool
parse_start_time(const string& trep, SPFtime& spf)
{
    tm tmstr;
    if (strptime(trep.c_str(), OSPF_TIME_DATE_FMT, &tmstr)) {
	spf = SPFtime(mktime(&tmstr), 0);
	return true;
    } else {
	timeval now;
	gettimeofday(&now, 0);
	spf = SPFtime(now);
	return false;
    }
}

static inline bool
parse_stop_time(const string& trep, SPFtime& spf, bool& time_set_flag)
{
    tm tmstr;
    if (strptime(trep.c_str(), OSPF_TIME_DATE_FMT, &tmstr)) {
	spf = SPFtime(mktime(&tmstr), 0);
	time_set_flag = true;
	return true;
    } else {
	time_set_flag = false;
	return false;
    }
}

string format_spftime(const SPFtime& spf, bool is_specified)
{
    if (is_specified == false) {
	return "Never";
    } else {
	char s[128];
	time_t t = spf.sec;
	struct tm* lt = localtime(&t);
	strftime(s, sizeof(s), OSPF_TIME_DATE_FMT, lt);
	return s;
    }
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_add_md5_key(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	key_id, 
	const string&	md5key, 
	const string&	start_receive, 
	const string&	stop_receive, 
	const string&	start_transmit, 
	const string&	stop_transmit)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);

    if (key_id > 255) {
	return XrlCmdError::COMMAND_FAILED("Key id must be between 0 and "
					   "255.");
    }

    CfgAuKey k;
    memset(&k, 0, sizeof(k));
    k.address = phyp->get_addr();
    k.phyint  = phyp->get_phyint();
    k.key_id  = key_id;
    for (size_t i = 0; i < sizeof(k.auth_key); i++)
	k.auth_key[i] = (i < md5key.size()) ? md5key[i] : '0';
    parse_start_time(start_receive, k.start_accept);
    parse_stop_time(stop_receive, k.stop_accept, k.stop_accept_specified);
    parse_start_time(start_transmit, k.start_generate);
    parse_stop_time(stop_transmit, k.stop_generate, k.stop_generate_specified);
    
    ospf()->cfgAuKey(&k, ADD_ITEM);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_get_md5_key(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	key_id, 
	// Output values, 
	string&	md5key, 
	string&	start_receive, 
	string&	stop_receive, 
	string&	start_transmit, 
	string&	stop_transmit)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);

    if (key_id > 255) {
	return XrlCmdError::COMMAND_FAILED("Key id must be between 0 and "
					   "255.");
    }
    CfgAuKey k;
    if (ospf()->qryAuKey(k, key_id, phyp->get_addr(), 
			 phyp->get_phyint()) == false) {
	return XrlCmdError::COMMAND_FAILED("Unknown key requested.");
    }
    md5key = string((const char*)k.auth_key, sizeof(k.auth_key));
    start_receive = format_spftime(k.start_accept, true);
    stop_receive = format_spftime(k.stop_accept, k.stop_accept_specified);
    start_transmit = format_spftime(k.start_generate, true);
    stop_transmit = format_spftime(k.stop_generate, k.stop_generate_specified);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_delete_md5_key(
	// Input values, 
	const string&	identifier, 
	const uint32_t&	key_id)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);

    if (key_id > 255) {
	return XrlCmdError::COMMAND_FAILED("Key id must be between 0 and "
					   "255.");
    }
    CfgAuKey k;
    memset(&k, 0, sizeof(k));
    k.address = phyp->get_addr();
    k.phyint  = phyp->get_phyint();
    k.key_id  = key_id;

    ospf()->cfgAuKey(&k, DELETE_ITEM);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_interface_list_md5_keys(
	// Input values, 
	const string&	identifier, 
	// Output values, 
	XrlAtomList&	key_ids)
{
    FAST_FAIL_ON_NO_OSPF();
    FAST_FAIL_ON_UNKNOWN_INTERFACE(identifier, addr, phyp);

    list<CfgAuKey> keys;
    ospf()->getAuKeys(keys, phyp->get_addr(), phyp->get_phyint());
    for(list<CfgAuKey>::const_iterator k = keys.begin();
	k != keys.end(); k++) {
	key_ids.append(XrlAtom(k->key_id));
    }

    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// Static neighbor related

XrlCmdError 
XrlOspfTarget::ospf_0_1_add_neighbor(
	// Input values, 
	const IPv4&	nbr_addr, 
	const bool&	dr_eligible)
{
    FAST_FAIL_ON_NO_OSPF();
    CfgNbr cn;
    cn.nbr_addr	   = nbr_addr.addr();
    cn.dr_eligible = dr_eligible;
    ospf()->cfgNbr(&cn, ADD_ITEM);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_get_neighbor(
	// Input values, 
	const IPv4&	nbr_addr, 
	// Output values, 
	bool&	dr_eligible)
{
    FAST_FAIL_ON_NO_OSPF();
    CfgNbr cn;
    ospf()->qryNbr(cn, nbr_addr.addr());
    dr_eligible = cn.dr_eligible;
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_delete_neighbor(
	// Input values, 
	const IPv4&	nbr_addr)
{
    FAST_FAIL_ON_NO_OSPF();
    CfgNbr cn;
    cn.nbr_addr	   = nbr_addr.addr();
    ospf()->cfgNbr(&cn, DELETE_ITEM);
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfTarget::ospf_0_1_list_neighbors(
	// Output values, 
	XrlAtomList&	nbr_addrs)
{
    FAST_FAIL_ON_NO_OSPF();
    list<CfgNbr> lcn;
    ospf()->getNbrs(lcn);
    for(list<CfgNbr>::const_iterator i = lcn.begin(); i != lcn.end(); i++) {
	nbr_addrs.append(XrlAtom(IPv4(i->nbr_addr)));
    }
    return XrlCmdError::OKAY();
}
