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

// You may also (at your option) redistribute this software and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or any later version.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#ident "$XORP: xorp/ospfd/xorp/ospf_config.C,v 1.1.1.1 2002/12/11 23:56:10 hodson Exp $"

#if defined(OLD_CONFIG_CODE)

#include <sys/types.h>
#include <sys/socket.h> 
#include <net/if.h> 

#include "ospf_module.h"
#include "ospfinc.h"
#include "system.h"

#include "tcppkt.h"
#include "os-instance.h"
#include "ospf_config.h"
#include "ospfd_xorp.h"

bool get_prefix(const char *prefix, InAddr &net, InMask &mask); 

GlobalConfig::GlobalConfig() {

    _m.lsdb_limit = 0;
    _m.mospf_enabled = false;
    _m.inter_area_mc = true;
    _m.ovfl_int = 300;
    _m.new_flood_rate = 200;
    _m.max_rxmt_window = 8;
    _m.max_dds = 4;
    _m.log_priority = 4;
    _m.host_mode = false;
    _m.refresh_rate = 0;
    _m.PPAdjLimit = 0;
    _m.random_refresh = false;

    _config_changed = 0;
    _config_done = 0;

    _interfaces = 0;
}

int
GlobalConfig::add_area(const IPv4 &area_id) {
    AreaConfig *ac;
    ac = new AreaConfig(area_id);
    _areas[area_id] = ac;
    _config_changed = 1;
    _config_done = 0;
    return 0;
}

AreaConfig *
GlobalConfig::find_area(const IPv4 &area_id) {
    typedef map<IPv4,AreaConfig*>::iterator Iterator;
    Iterator iter = _areas.find(area_id);
    if (iter == _areas.end()) {
	fprintf(stderr, "Attempt to find unknown area: %s\n",
		string(area_id).c_str());
	return NULL;
    }
    return iter->second;
}

int
GlobalConfig::add_route(const IPv4Net &prefix, const IPv4 &nexthop, 
	  int type, int cost) {
    RouteConfig *rc;
    try {
	rc = new RouteConfig(prefix, nexthop, type, cost);
    } catch (ConfigError ce) {
	fprintf(stderr, "new RouteConfig threw ConfigError\n");
	return -1;
    }
    _routes[prefix] = rc;
    _config_changed = 1;
    _config_done = 0;
    return 0;
}

int 
GlobalConfig::add_interface(const IPv4 &area_id, const string &identifier,
			    int cost) {
    AreaConfig *ac = find_area(area_id);
    if (ac == NULL) return -1;
    _config_changed = 1;
    _config_done = 0;
    return ac->add_interface(identifier, ++_interfaces, cost);
}

int 
GlobalConfig::add_aggregate(const IPv4 &area_id, const IPv4Net &prefix) {
    AreaConfig *ac = find_area(area_id);
    if (ac == NULL) return -1;
    _config_changed = 1;
    _config_done = 0;
    return ac->add_aggregate(prefix);
}


int 
GlobalConfig::add_host(const IPv4 &area_id, const IPv4Net &prefix, 
		       int cost) {
    AreaConfig *ac = find_area(area_id);
    if (ac == NULL) return -1;
    _config_changed = 1;
    _config_done = 0;
    return ac->add_host(prefix, cost);
}

int 
GlobalConfig::add_vlink(const IPv4 &area_id, const IPv4 &endpoint) {
    AreaConfig *ac = find_area(area_id);
    if (ac == NULL) return -1;
    _config_changed = 1;
    _config_done = 0;
    return ac->add_vlink(endpoint);
}

int 
GlobalConfig::add_neighbor(const IPv4 &area_id, const string &identifier,
			   const IPv4 &address, int priority) {
    AreaConfig *ac = find_area(area_id);
    if (ac == NULL) return -1;
    _config_changed = 1;
    _config_done = 0;
    return ac->add_neighbor(identifier, address, priority);
}

int 
GlobalConfig::add_key(const IPv4 &area_id, const string &identifier,
		      int keyid, char *key, int keylen) {
    AreaConfig *ac = find_area(area_id);
    if (ac == NULL) return -1;
    _config_changed = 1;
    _config_done = 0;
    return ac->add_key(identifier, keyid, key, keylen);
}

int 
GlobalConfig::set_param(const ConfigParam &param) {
    
    string name = param.paramname();

    debug_msg("set_param %s", name.c_str());

    _config_done = 0;

    if (name == "router_id")
	_router_id = param;
    else if (name == "ospf_ext_lsdb_limit")
	_m.lsdb_limit = param;
    else if (name == "ospf_exit_overflow_interval")
	_m.ovfl_int = param;
    else if (name == "ase_orig_rate")
	_m.new_flood_rate = param;
    else if (name == "lsu_rxmt_window")
	_m.max_rxmt_window = param;
    else if (name == "dd_sessions")
	_m.max_dds = param;
    else if (name == "log_level")
	_m.log_priority = param;
    else if (name == "refresh_rate")
	_m.refresh_rate = param;
    else if (name == "pp_adj_limit")
	_m.PPAdjLimit = param;
    else if (name == "host_mode")
	_m.host_mode = bool(param);
    else if (name == "mospf")
	_m.mospf_enabled = param;
    else if (name == "no_inter_area_mc")
	_m.inter_area_mc = !param;
    else if (name == "done") 
	_config_done = true;
    else {
	fprintf(stderr, "Unknown parameter: %s\n", name.c_str());
	return -1;
    }
    _config_changed = 1;
    return 0;
}

int 
GlobalConfig::set_area_param(const IPv4 &area_id, const ConfigParam &param) {
    AreaConfig *ac = find_area(area_id);
    if (ac == NULL) return -1;
    _config_changed = 1;
    _config_done = 0;
    return ac->set_param(param);
}

int 
GlobalConfig::set_interface_param(const IPv4 &area_id, 
				  const string &identifier, 
				  const ConfigParam &param) {
    AreaConfig *ac = find_area(area_id);
    if (ac == NULL) return -1;
    _config_changed = 1;
    _config_done = 0;
    return ac->set_interface_param(identifier, param);
}

int 
GlobalConfig::set_aggregate_param(const IPv4 &area_id, 
				  const IPv4Net &prefix,
				  const ConfigParam &param) {
    AreaConfig *ac = find_area(area_id);
    if (ac == NULL) return -1;
    _config_changed = 1;
    _config_done = 0;
    return ac->set_aggregate_param(prefix, param);
}

int 
GlobalConfig::set_host_param(const IPv4 &area_id, 
			     const IPv4Net &prefix, 
			     const ConfigParam &param) {
    AreaConfig *ac = find_area(area_id);
    if (ac == NULL) return -1;
    _config_changed = 1;
    _config_done = 0;
    return ac->set_host_param(prefix, param);
}

int 
GlobalConfig::set_vlink_param(const IPv4 &area_id, 
			      const IPv4 &endpoint,	
			      const ConfigParam &param) {
    AreaConfig *ac = find_area(area_id);
    if (ac == NULL) return -1;
    _config_changed = 1;
    _config_done = 0;
    return ac->set_vlink_param(endpoint, param);
}

int 
GlobalConfig::set_neighbor_param(const IPv4 &area_id, 
				 const string &identifier,
				 const IPv4 &address,
				 const ConfigParam &param) {
    AreaConfig *ac = find_area(area_id);
    if (ac == NULL) return -1;
    _config_changed = 1;
    _config_done = 0;
    return ac->set_neighbor_param(identifier, address.addr(), param);
}

int 
GlobalConfig::set_key_param(const IPv4 &area_id, 
			    const string &identifier,
			    int keyid,
			    const ConfigParam &param) {
    AreaConfig *ac = find_area(area_id);
    if (ac == NULL) return -1;
    _config_changed = 1;
    _config_done = 0;
    return ac->set_key_param(identifier, keyid, param);
}

int 
GlobalConfig::set_route_param(const IPv4Net &prefix,
			      const ConfigParam &param) {
    typedef map<IPv4Net,RouteConfig*>::iterator Iterator;
    Iterator iter = _routes.find(prefix);
    if (iter == _routes.end()) {
	fprintf(stderr, "Attempt to find unknown route: %s\n",
		string(prefix).c_str());
	return -1;
    }
    _config_changed = 1;
    _config_done = 0;
    return iter->second->set_param(param);
}

int GlobalConfig::activate() {
    rtid_t new_router_id = ntoh32(_router_id.addr());
    if (!ospf)
	ospf = new OSPF(new_router_id);

    ospf->cfgStart();
    
    ospf->cfgOspf(&_m);

    typedef map<IPv4,AreaConfig*>::iterator AreaIterator;
    AreaIterator a_iter = _areas.begin();
    while (a_iter != _areas.end()) {
	a_iter->second->activate();
	++a_iter;
    }

    typedef map<IPv4Net,RouteConfig*>::iterator RouteIterator;
    RouteIterator r_iter = _routes.begin();
    while (r_iter != _routes.end()) {
	r_iter->second->activate();
	++r_iter;
    }

    ospf->cfgDone();
    return 0;
}

AreaConfig::AreaConfig(const IPv4 &area_id) :
    _area_id(area_id)
{
    _m.area_id = ntoh32(area_id.addr());
    _m.stub = false;
    _m.dflt_cost = 0;
    _m.import_summs = true;
}

int 
AreaConfig::set_param(const ConfigParam &param) {
    string name = param.paramname();
    if (name == "stub")
	_m.stub = param;
    else if (name == "stub_default_cost")
	_m.dflt_cost = param;
    else if (name == "no_summaries")
	_m.import_summs = !bool(param);
    else {
	fprintf(stderr, "Unknown parameter: %s\n", name.c_str());
	return -1;
    }
    return 0;
}

int AreaConfig::add_interface(const string &identifier, 
			      int ifnum, int cost) {
    InterfaceConfig *ic;
    try {
	ic = new InterfaceConfig(identifier, _area_id, ifnum, cost);
    } catch (ConfigError ce) {
	fprintf(stderr, "new InterfaceConfig threw ConfigError\n");
	return -1;
    }
    _interfaces[identifier] = ic;
    return 0;
}

InterfaceConfig *
AreaConfig::find_interface(const string &identifier) {
    typedef map<string,InterfaceConfig*>::iterator Iterator;
    printf("AreaConfig::find_interface\n");
    Iterator iter = _interfaces.find(identifier);
    if (iter == _interfaces.end()) {
	fprintf(stderr, "Attempt to find unknown interface: %s\n",
		identifier.c_str());
	return NULL;
    }
    return iter->second;
}



int
AreaConfig::add_aggregate(const IPv4Net &prefix) {
    AggregateConfig *ac;
    try {
	ac = new AggregateConfig(prefix, _area_id);
    } catch (ConfigError ce) {
	fprintf(stderr, "new AggregateConfig threw ConfigError\n");
	return -1;
    }
    _aggregates[prefix] = ac;
    return 0;
}

int
AreaConfig::add_host(const IPv4Net &prefix, int cost) {
    HostConfig *hc;
    try {
	hc = new HostConfig(prefix, _area_id, cost);
    } catch (ConfigError ce) {
	fprintf(stderr, "new HostConfig threw ConfigError\n");
	return -1;
    }
    _hosts[prefix] = hc;
    return 0;
}

int 
AreaConfig::add_vlink(const IPv4 &endpoint) {
    VlinkConfig *vc;
    vc = new VlinkConfig(endpoint, _area_id);
    _vls[endpoint] = vc;
    return 0;
}

int 
AreaConfig::add_neighbor(const string &identifier,
			 const IPv4 &address, int priority) {
    InterfaceConfig *ic = find_interface(identifier);
    if (ic == NULL) return -1;
    return ic->add_neighbor(address, priority);
}

int 
AreaConfig::add_key(const string &identifier,
		    int keyid, char *key, int keylen) {
    InterfaceConfig *ic = find_interface(identifier);
    if (ic == NULL) return -1;
    return ic->add_key(keyid, key, keylen);
}

int 
AreaConfig::set_interface_param(const string &identifier, 
				const ConfigParam &param) {
    printf("AreaConfig::set_interface_param\n");
    InterfaceConfig *ic = find_interface(identifier);
    if (ic == NULL) return -1;
    return ic->set_param(param);
}

int 
AreaConfig::set_aggregate_param(const IPv4Net &prefix,
				const ConfigParam &param) {
    typedef map<IPv4Net,AggregateConfig*>::iterator Iterator;
    Iterator iter = _aggregates.find(prefix);
    if (iter == _aggregates.end()) {
	fprintf(stderr, "Attempt to find unknown aggregate: %s\n",
		string(prefix).c_str());
	return -1;
    }
    return iter->second->set_param(param);
}

int 
AreaConfig::set_host_param(const IPv4Net &prefix, 
			   const ConfigParam &param) {
    typedef map<IPv4Net,HostConfig*>::iterator Iterator;
    Iterator iter = _hosts.find(prefix);
    if (iter == _hosts.end()) {
	fprintf(stderr, "Attempt to find unknown host: %s\n",
		string(prefix).c_str());
	return -1;
    }
    return iter->second->set_param(param);
}

int 
AreaConfig::set_vlink_param(const IPv4 &endpoint,
			    const ConfigParam &param) {
    typedef map<IPv4,VlinkConfig*>::iterator Iterator;
    Iterator iter = _vls.find(endpoint);
    if (iter == _vls.end()) {
	fprintf(stderr, "Attempt to find unknown vlink: %s\n",
		string(endpoint).c_str());
	return -1;
    }
    return iter->second->set_param(param);
}

int 
AreaConfig::set_neighbor_param(const string &identifier,
			       const IPv4 &address, 
			       const ConfigParam &param) {
    InterfaceConfig *ic = find_interface(identifier);
    if (ic == NULL) return -1;
    return ic->set_neighbor_param(address, param);
}

int 
AreaConfig::set_key_param(const string &identifier, int keyid,
			  const ConfigParam &param) {
    InterfaceConfig *ic = find_interface(identifier);
    if (ic == NULL) return -1;
    return ic->set_key_param(keyid, param);
}

int
AreaConfig::activate() {
    ospf->cfgArea(&_m, ADD_ITEM);

    typedef map<string,InterfaceConfig*>::iterator InterfaceIterator;
    InterfaceIterator i_iter = _interfaces.begin();
    while (i_iter != _interfaces.end()) {
	i_iter->second->activate();
	++i_iter;
    }

    typedef map<IPv4Net,AggregateConfig*>::iterator AggregateIterator;
    AggregateIterator a_iter = _aggregates.begin();
    while (a_iter != _aggregates.end()) {
	a_iter->second->activate();
	++a_iter;
    }

    typedef map<IPv4Net,HostConfig*>::iterator HostIterator;
    HostIterator h_iter = _hosts.begin();
    while (h_iter != _hosts.end()) {
	h_iter->second->activate();
	++h_iter;
    }

    typedef map<IPv4,VlinkConfig*>::iterator VlinkIterator;
    VlinkIterator v_iter = _vls.begin();
    while (v_iter != _vls.end()) {
	v_iter->second->activate();
	++v_iter;
    }
    
    return 0;
}

InterfaceConfig::InterfaceConfig(const string &identifier, 
				 const IPv4 &area_id,
				 int ifnum, int cost) 
    throw (ConfigError)
    : _identifier(identifier), _cost(cost)
{
    in_addr addr;
    BSDPhyInt *phyp;
    printf("SendInterface\n");

    if (!sys->parse_interface(identifier.c_str(), addr, phyp))
	throw ConfigError();

    _m.address = phyp->addr;
    _m.phyint = phyp->phyint;
    _m.mask = phyp->mask;
    _m.mtu = phyp->mtu;
    _m.IfIndex = ifnum;
    _m.area_id = ntoh32(area_id.addr());

    if ((phyp->flags & IFF_BROADCAST) != 0)
	_m.IfType = IFT_BROADCAST;
    else if ((phyp->flags & IFF_POINTOPOINT) != 0)
	_m.IfType = IFT_PP;
    else
	_m.IfType = IFT_NBMA;

    _m.dr_pri = 1;
    _m.xmt_dly = 1;
    _m.rxmt_int = 5;
    _m.hello_int = 10;
    _m.dead_int = 40;
    _m.poll_int = 120;
    _m.auth_type = 0;
    memset(_m.auth_key, 0, 8);
    _m.mc_fwd = true;
    _m.demand = false;
    _m.passive = false;
}

int
InterfaceConfig::add_neighbor(const IPv4 &address, int priority) {
    NeighborConfig *nc = new NeighborConfig(address, priority);
    _nbrs[address] = nc;
    return 0;
}

int
InterfaceConfig::add_key(int keyid, char *key, int keylen) {
    KeyConfig *kc;
    try {
	kc = new KeyConfig(_identifier, keyid, key, keylen);
    } catch (ConfigError ce) {
	fprintf(stderr, "new KeyConfig threw ConfigError\n");
	return -1;
    }
    _keys[keyid] = kc;
    return 0;
}

int 
InterfaceConfig::set_param(const ConfigParam &param) {
    string name = param.paramname();
    if (name == "mtu")
	_m.mtu = param;
    else if (name == "if_index")
	_m.IfIndex = param;
    else if (name == "nbma")
	_m.IfType = IFT_NBMA;
    else if (name == "ptmp")
	_m.IfType = IFT_P2MP;
    else if (name == "ospf_if_rtr_priority")
	_m.dr_pri = param;
    else if (name == "ospf_if_transit_delay")
	_m.xmt_dly = param;
    else if (name == "ospf_if_retrans_interval")
	_m.rxmt_int = param;
    else if (name == "ospf_if_hello_interval")
	_m.hello_int = param;
    else if (name == "ospf_if_rtr_dead_interval")
	_m.dead_int = param;
    else if (name == "ospf_if_poll_interval")
	_m.poll_int = param;
    else if (name == "ospf_if_auth_type")
	_m.auth_type = param;
    else if (name == "ospf_if_auth_key") {
	memset(_m.auth_key, 0, 8);
	strncpy((char *)(_m.auth_key), string(param).c_str(), 8);
    } else if (name == "ospf_if_multicast_forwarding")
	_m.mc_fwd = param;
    else if (name == "on-demand")
	_m.demand = param;
    else if (name == "passive")
	_m.passive = param;
    else {
	fprintf(stderr, "Unknown parameter: %s\n", name.c_str());
	return -1;
    }
    return 0;
}

int 
InterfaceConfig::set_neighbor_param(const IPv4 &address, 
				    const ConfigParam &param) {
    typedef map<IPv4,NeighborConfig*>::iterator Iterator;
    Iterator iter = _nbrs.find(address);
    if (iter == _nbrs.end()) {
	fprintf(stderr, "Attempt to find unknown neighbor: %s\n",
		string(address).c_str());
	return -1;
    }
    return iter->second->set_param(param);
}

int 
InterfaceConfig::set_key_param(int keyid, const ConfigParam &param) {
    typedef map<int,KeyConfig*>::iterator Iterator;
    Iterator iter = _keys.find(keyid);
    if (iter == _keys.end()) {
	fprintf(stderr, "Attempt to find unknown keyid: %d\n",
		keyid);
	return -1;
    }
    return iter->second->set_param(param);
}

int
InterfaceConfig::activate() {
    ospf->cfgIfc(&_m, ADD_ITEM);

    typedef map<IPv4,NeighborConfig*>::iterator NeighborIterator;
    NeighborIterator n_iter = _nbrs.begin();
    while (n_iter != _nbrs.end()) {
	n_iter->second->activate();
	++n_iter;
    }

    typedef map<int,KeyConfig*>::iterator KeyIterator;
    KeyIterator k_iter = _keys.begin();
    while (k_iter != _keys.end()) {
	k_iter->second->activate();
	++k_iter;
    }

    return 0;
}


AggregateConfig::AggregateConfig(const IPv4Net &prefix, const IPv4 &area_id) 
    throw (ConfigError)
    : _prefix(prefix)
{
    InAddr net;
    InAddr mask;

    if (get_prefix(string(prefix).c_str(), net, mask)) {
	_m.net = net;
	_m.mask = mask;
	_m.area_id = ntoh32(area_id.addr());
	_m.no_adv = 0;
    } else {
	throw ConfigError();
    }
}

int
AggregateConfig::set_param(const ConfigParam &param) {
    string name = param.paramname();
    if (name == "suppress")
	_m.no_adv = param;
    else {
	fprintf(stderr, "Unknown parameter: %s\n", name.c_str());
	return -1;
    }
    return 0;
}

int
AggregateConfig::activate() {
    ospf->cfgRnge(&_m, ADD_ITEM);
    return 0;
}

HostConfig::HostConfig(const IPv4Net &prefix, const IPv4 &area_id, int cost) 
    throw (ConfigError)
    : _prefix(prefix)
{
    InAddr net;
    InAddr mask;

    if (get_prefix(string(prefix).c_str(), net, mask)) {
	_m.net = net;
	_m.mask = mask;
	_m.area_id = ntoh32(area_id.addr());
	_m.cost = cost;
    } else {
	throw ConfigError();
    }
}

int
HostConfig::set_param(const ConfigParam &param) {
    string name = param.paramname();
    if (name == "cost")
	_m.cost = param;
    else {
	fprintf(stderr, "Unknown parameter: %s\n", name.c_str());
	return -1;
    }
    return 0;
}

int 
HostConfig::activate() {
    ospf->cfgHost(&_m, ADD_ITEM);
    return 0;
}

VlinkConfig::VlinkConfig(const IPv4 &endpoint, const IPv4 &area_id) :
    _endpoint(endpoint)
{
    _m.nbr_id = ntoh32(endpoint.addr());
    _m.transit_area = ntoh32(area_id.addr());
    _m.xmt_dly = 1;
    _m.rxmt_int = 5;
    _m.hello_int = 10;
    _m.dead_int = 40;
    _m.auth_type = 0;
    memset(_m.auth_key, 0, 8);
}

int
VlinkConfig::set_param(const ConfigParam &param) {
    string name = param.paramname();
    if (name == "ospf_virt_if_transit_delay")
	_m.xmt_dly = param;
    else if (name == "ospf_virt_if_retrans_interval")
	_m.rxmt_int = param;
    else if (name == "ospf_virt_if_hello_interval")
	_m.hello_int = param;
    else if (name == "ospf_virt_if_rtr_dead_interval")
	_m.dead_int = param;
    else if (name == "ospf_virt_if_auth_type")
	_m.auth_type = param;
    else if (name == "ospf_virt_if_auth_key") {
	memset(_m.auth_key, 0, 8);
	strncpy((char *)(_m.auth_key), string(param).c_str(), 8);
    } else {
	fprintf(stderr, "Unknown parameter: %s\n", name.c_str());
	return -1;
    }
    return 0;
}

int 
VlinkConfig::activate() {
    ospf->cfgVL(&_m, ADD_ITEM);
    return 0;
}

NeighborConfig::NeighborConfig(const IPv4 &address, int priority) :
    _address(address)
{
    _m.nbr_addr = ntoh32(address.addr());
    _m.dr_eligible = priority;
}

int
NeighborConfig::set_param(const ConfigParam &param) {
    string name = param.paramname();
    if (name == "priority")
	_m.dr_eligible = param;
    else {
	fprintf(stderr, "Unknown parameter: %s\n", name.c_str());
	return -1;
    }
    return 0;
}

int 
NeighborConfig::activate() {
    ospf->cfgNbr(&_m, ADD_ITEM);
    return 0;
}

RouteConfig::RouteConfig(const IPv4Net &prefix, const IPv4 &nexthop, 
			 int type, int cost) 
    throw (ConfigError)
    : _prefix(prefix)
{
    InAddr net;
    InMask mask;

    if (get_prefix(string(prefix).c_str(), net, mask)) {
	_m.net = net;
	_m.mask = mask;
	_m.type2 = (type == 2);
	_m.mc = 0;
	_m.direct = 0;
	_m.noadv = 0;
	_m.cost = cost;
	_m.gw = ntoh32(nexthop.addr());
	_m.phyint = sys->get_phyint(_m.gw);
	_m.tag = 0;
    } else {
	throw ConfigError();
    }
}

int 
RouteConfig::set_param(const ConfigParam &param) {
    string name = param.paramname();
    if (name == "mcsource")
	_m.mc = param;
    else if (name == "tag")
	_m.tag = param;
    else if (name == "nexthop") {
	IPv4 gw = param;
	_m.gw = ntoh32(gw.addr());
	_m.phyint = sys->get_phyint(_m.gw);
    } else if (name == "type")
	_m.type2 = (int(param) == 2);
    else if (name == "cost")
	_m.cost = param;
    else {
	fprintf(stderr, "Unknown parameter: %s\n", name.c_str());
	return -1;
    }
    return 0;
}

int 
RouteConfig::activate() {
    ospf->cfgExRt(&_m, ADD_ITEM);
    return 0;
}


KeyConfig::KeyConfig(const string &identifier, int keyid, 
		     char *key, int keylen) 
    throw (ConfigError)
{
    in_addr addr;
    BSDPhyInt *phyp;

    if ((keyid < 0) || (keyid > 255))
	fprintf(stderr, "Bad Key ID: %d\n", keyid);
    _m.key_id = keyid;
    if ((keylen < 0) || (keyid > 15)) {
	fprintf(stderr, "Bad Key Length: %d\n", keyid);
	return;
    }
    memset(_m.auth_key, 0, 16);
    memcpy(_m.auth_key, key, keylen);

    if (!sys->parse_interface(identifier.c_str(), addr, phyp))
	throw ConfigError();

    _m.address = phyp->addr;
    _m.phyint = phyp->phyint;
    _m.start_accept = 0;
    _m.start_generate = 0;
    _m.stop_generate = 0;
    _m.stop_accept = 0;
}

int 
KeyConfig::set_param(const ConfigParam &param) {
    string name = param.paramname();
    if (name == "startaccept")
	_startacc = param;
    else if (name == "startgenerate")
	_startgen = param;
    else if (name == "stopgenerate")
	_stopgen = param;
    else if (name == "stopaccept")
	_stopacc = param;
    else {
	fprintf(stderr, "Unknown parameter: %s\n", name.c_str());
	return -1;
    }
    return 0;
}

int 
KeyConfig::activate() {
    timeval now;
    tm tmstr;
    gettimeofday(&now, 0);
    if (strptime(_startacc.c_str(), "%D@%T", &tmstr))
	_m.start_accept = mktime(&tmstr) - now.tv_sec;
    if (strptime(_startgen.c_str(), "%D@%T", &tmstr))
	_m.start_generate = mktime(&tmstr) - now.tv_sec;
    if (strptime(_stopgen.c_str(), "%D@%T", &tmstr))
	_m.stop_generate = mktime(&tmstr) - now.tv_sec;
    if (strptime(_stopacc.c_str(), "%D@%T", &tmstr))
	_m.stop_accept = mktime(&tmstr) - now.tv_sec;
    ospf->cfgAuKey(&_m, ADD_ITEM);
    return 0;
}

ConfigParam::ConfigParam(const string &paramname, int value) :
    _paramname(paramname)
{
    _type = inttype;
    _intvalue = value;
}

ConfigParam::ConfigParam(const string &paramname, bool value) :
    _paramname(paramname)
{
    _type = booltype;
    _boolvalue = value;
}

ConfigParam::ConfigParam(const string &paramname, string value) :
    _paramname(paramname)
{
    _type = stringtype;
    _stringvalue = value;
}

ConfigParam::ConfigParam(const string &paramname, IPv4 value) :
    _paramname(paramname)
{
    _type = ipv4type;
    _ipv4value = value;
}

ConfigParam::ConfigParam(const XrlAtom *atom) 
    throw (ConfigError)
{
 
    XrlAtomType type = atom->type();

    _paramname = atom->name();

    if (type == xrlatom_int32) {
	_type = inttype;
	_intvalue = atom->int32();
    } else if (type == xrlatom_uint32) {
	_type = inttype;
	_intvalue = atom->uint32();
    } else if (type == xrlatom_ipv4) {
	_type = ipv4type;
	_ipv4value = atom->ipv4();
    } else if (type == xrlatom_text) {
	_type = stringtype;
	_stringvalue = atom->text();
    } else {
        XLOG_ERROR("Unknown parameter type");
	throw ConfigError();
    }
}

ConfigParam::operator int() const {
    if (_type != inttype)
	fprintf(stderr, "type mismatch\n");
    return _intvalue;
}

ConfigParam::operator uns32() const {
    if (_type != inttype)
	fprintf(stderr, "type mismatch\n");
    return _intvalue;
}

ConfigParam::operator uns16() const {
    if (_type != inttype)
	fprintf(stderr, "type mismatch\n");
    if (_intvalue < 0 || _intvalue > 65535) {
	fprintf(stderr, "type out of range\n");
	return 0;
    }
    return _intvalue;
}

ConfigParam::operator byte() const {
    if (_type != inttype)
	fprintf(stderr, "type mismatch\n");
    if (_intvalue < 0 || _intvalue > 255) {
	fprintf(stderr, "type out of range\n");
	return 0;
    }
    return _intvalue;
}

ConfigParam::operator bool() const {
    if (_type != booltype)
	fprintf(stderr, "type mismatch\n");
    return _boolvalue;
}

ConfigParam::operator string() const {
    if (_type != stringtype)
	fprintf(stderr, "type mismatch\n");
    return _stringvalue;
}

ConfigParam::operator IPv4() const {
    if (_type != ipv4type)
	fprintf(stderr, "type mismatch\n");
    return _ipv4value;
}

#endif /* OLD_CONFIG_CODE */
