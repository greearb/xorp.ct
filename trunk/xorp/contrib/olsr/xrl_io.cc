// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/contrib/olsr/xrl_io.cc,v 1.1 2008/04/24 15:19:56 bms Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include <list>
#include <set>

#include "libxipc/xrl_router.hh"

#include "xrl/interfaces/rib_xif.hh"
#include "xrl/interfaces/socket4_xif.hh"

#include "libfeaclient/ifmgr_xrl_mirror.hh"
#include "policy/backend/policytags.hh"

#include "olsr.hh"
#include "io.hh"
#include "xrl_io.hh"
#include "xrl_port.hh"

#define OLSR_ADMIN_DISTANCE 230	    // XXX hardcoded

// --------------------------------------------------------------------------
// Utility methods

/**
 * Query whether an address exists on given interface and vif path,
 * and that all items on path are enabled.
 */
static bool
address_exists(const IfMgrIfTree&	iftree,
	       const string&		ifname,
	       const string&		vifname,
	       const IPv4&		addr)
{
    debug_msg("Looking for %s/%s/%s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str());

    const IfMgrIfAtom* ia = iftree.find_interface(ifname);
    if (ia == NULL)
	return false;

    const IfMgrVifAtom* va = ia->find_vif(vifname);
    if (va == NULL)
	return false;

    const IfMgrIPv4Atom* aa = va->find_addr(addr);
    if (aa == NULL)
	return false;

    return true;
}

/**
 * Query whether an address exists on given interface and vif path.
 * and that all items on path are enabled.
 */
static bool
address_enabled(const IfMgrIfTree&	iftree,
		const string&		ifname,
		const string&		vifname,
		const IPv4&		addr)
{
    debug_msg("Looking for %s/%s/%s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str());

    const IfMgrIfAtom* ia = iftree.find_interface(ifname);
    if (ia == 0 || ia->enabled() == false || ia->no_carrier()) {
	debug_msg("if %s exists ? %d ?\n", ifname.c_str(), (ia ? 1 : 0));
	return false;
    }

    const IfMgrVifAtom* va = ia->find_vif(vifname);
    if (va == 0 || va->enabled() == false) {
	debug_msg("vif %s exists ? %d?\n", vifname.c_str(), (va ? 1: 0));
	return false;
    }

    const IfMgrIPv4Atom* aa = va->find_addr(addr);
    if (aa == 0 || aa->enabled() == false) {
	debug_msg("addr %s exists ? %d?\n", addr.str().c_str(), (aa ? 1: 0));
	return false;
    }
    debug_msg("Found\n");
    return true;
}

// --------------------------------------------------------------------------
// Predicate functors.

/**
 * @short Functor to test whether a particular local address is
 * associated with an XrlPort.
 */
struct port_has_local_address {
    port_has_local_address(const IPv4& addr) : _addr(addr) {}
    bool operator() (const XrlPort* xp) {
	return xp && xp->local_address() == _addr;
    }
private:
    IPv4 _addr;
};

/**
 * @short Functor to test whether a particular interface/vif is
 * associated with an XrlPort.
 */
struct port_has_interface_vif {
    port_has_interface_vif(const string& interface,
			   const string& vif)
     : _interface(interface), _vif(vif) {}
    bool operator() (const XrlPort* xp) {
	return xp && xp->ifname() == _interface && xp->vifname() == _vif;
    }
private:
    string _interface;
    string _vif;
};

/**
 * @short Functor to test whether a particular XrlPort
 * is in a given service state.
 */
struct port_has_io_in_state {
    port_has_io_in_state(ServiceStatus st) : _st(st) {}

    bool operator() (const XrlPort* xp) const
    {
	return xp && xp->status() == _st;
    }
protected:
    ServiceStatus _st;
};

/**
 * @short Functor to test whether a particular OLSR port is appropriate
 * for packet arriving on socket.
 *
 * NB At a future date we might want to track socket id to XrlPortIO
 * mappings.  This would be more efficient.
 */
struct is_port_for {
    is_port_for(const string* sockid, const string* ifname,
		const string* vifname, const IPv4* addr,
		IfMgrXrlMirror* im)
	: _psid(sockid),
	  _ifname(ifname),
	  _vifname(vifname),
	  _pa(addr),
	  _pim(im)
    {}

    bool operator() (XrlPort*& xp);

protected:
    bool link_addr_valid() const;

private:
    const string* 	_psid;
    const string*	_ifname;
    const string*	_vifname;
    const IPv4* 	_pa;
    IfMgrXrlMirror* 	_pim;
};

inline bool
is_port_for::link_addr_valid() const
{
    return true;
}

bool
is_port_for::operator() (XrlPort*& xp)
{
    //
    // Perform address family specific check for source address being
    // link-local.  For IPv4 the concept does not exist, for IPv6
    // check if origin is link local.
    //
    if (link_addr_valid() == false) {
	return false;
    }

    if (xp == 0)
	return false;

    // If another socket, ignore
    if (xp->sockid() != *_psid)
	return false;

    // If our packet, ignore
    if (xp->local_address() == *_pa)
	return false;

    // Check the incoming interface and vif name (if known)
    if ((! _ifname->empty()) && (! _vifname->empty())) {
	if (xp->ifname() != *_ifname)
	    return false;
	if (xp->vifname() != *_vifname)
	    return false;
    }

    //
    // Packet has arrived on multicast socket and is not one of ours.
    //
    // Check source address to find originating neighbour on local nets
    // or p2p link.
    //
    const IfMgrIPv4Atom* ifa;
    ifa = _pim->iftree().find_addr(xp->ifname(),
				   xp->vifname(),
				   xp->local_address());

    if (ifa == 0) {
	return false;
    }
    if (ifa->has_endpoint()) {
	return ifa->endpoint_addr() == *_pa;
    }

    IPv4Net n(ifa->addr(), ifa->prefix_len());
    return n.contains(*_pa);
}

// --------------------------------------------------------------------------
// XrlIO

XrlIO::XrlIO(EventLoop& eventloop, XrlRouter& xrl_router,
	     const string& feaname, const string& ribname)
    : _eventloop(eventloop),
      _xrl_router(xrl_router),
      _feaname(feaname),
      _ribname(ribname),
      _component_count(0),
      _ifmgr(eventloop, feaname.c_str(), _xrl_router.finder_address(),
	     _xrl_router.finder_port()),
      _rib_queue(eventloop, xrl_router)
{
    _ifmgr.set_observer(this);
    _ifmgr.attach_hint_observer(this);
    _rib_queue.set_io(this);
}

XrlIO::~XrlIO()
{
    _ifmgr.detach_hint_observer(this);
    _ifmgr.unset_observer(this);

    while (! _dead_ports.empty()) {
	XrlDeadPortMap::iterator ii = _dead_ports.begin();
	XrlPort* xp = (*ii).second;
	delete xp;
	_dead_ports.erase(ii);
    }
}

int
XrlIO::startup()
{
    ServiceBase::set_status(SERVICE_STARTING);
    //
    // XXX: when the startup is completed,
    // IfMgrHintObserver::tree_complete() will be called.
    //
    if (_ifmgr.startup() != XORP_OK) {
	ServiceBase::set_status(SERVICE_FAILED);
	return (XORP_ERROR);
    }

    register_rib();
    component_up("startup");

    return (XORP_OK);
}

int
XrlIO::shutdown()
{
    //
    // XXX: when the shutdown is completed, XrlIO::status_change()
    // will be called.
    //
    ServiceBase::set_status(SERVICE_SHUTTING_DOWN);

    //
    // XXX Walk ports and shut them down.  Only when they are all
    // shutdown should we consider ourselves shutdown.
    //
    debug_msg("XXX XrlIO::shutdown (%p)\n", this);
    debug_msg("XXX n_ports = %u n_dead_ports %u\n",
	      XORP_UINT_CAST(_ports.size()),
	      XORP_UINT_CAST(_dead_ports.size()));

    while (! _ports.empty()) {
	XrlPortList::iterator ii = _ports.begin();
	XrlPort* xp = (*ii);
	debug_msg("   XXX killing port %p\n", xp);
	xp->shutdown();
	_ports.erase(ii);
	_dead_ports.insert(make_pair(dynamic_cast<ServiceBase*>(xp), xp));
    }

    unregister_rib();

    // XXX something is keeping the components up.
    component_down("shutdown");

    return (_ifmgr.shutdown());
}

void
XrlIO::component_up(string name)
{
    XLOG_ASSERT(name != "OlsrXrlPort");

    debug_msg("component_up '%s'\n", name.c_str());
    _component_count++;

    debug_msg("component_count %u\n", XORP_UINT_CAST(_component_count));

    // XXX - Should really get every component to register at
    // initialisation time and track the individual
    // status. Simpler to uncomment the printfs and track the count.
    if (3 == _component_count)
	ServiceBase::set_status(SERVICE_RUNNING);

    UNUSED(name);
}

void
XrlIO::component_down(string name)
{
    XLOG_ASSERT(name != "OlsrXrlPort");

    debug_msg("component_down '%s'\n", name.c_str());
    _component_count--;

    debug_msg("component_count %u\n", XORP_UINT_CAST(_component_count));

    if (0 == _component_count)
	ServiceBase::set_status(SERVICE_SHUTDOWN);
    else
	ServiceBase::set_status(SERVICE_SHUTTING_DOWN);

    UNUSED(name);
}

void
XrlIO::status_change(ServiceBase*  service,
		     ServiceStatus old_status,
		     ServiceStatus new_status)
{
    if (service->service_name() == "OlsrXrlPort") {
	try_start_next_port();

	if (new_status != SERVICE_SHUTDOWN)
	    return;
#if 0
	// Resolve ServiceBase* to XrlPort*.
	// Note: this assertion can fail if we never detected the failure
	// of the socket to start up.
	XrlDeadPortMap::iterator ii = _dead_ports.find(service);
	XLOG_ASSERT(ii != _dead_ports.end());
#endif

	return;
    }

    if (old_status == new_status)
	return;
    if (SERVICE_RUNNING == new_status)
	component_up(service->service_name());
    if (SERVICE_SHUTDOWN == new_status)
	component_down(service->service_name());
}

//
// This method is specific to XrlIO.
//
// Note: socket4_user.xif does not provide the destination address
// or port at this time, therefore it is impossible to verify that
// the traffic has been sent to a broadcast or multicast group.
//
void
XrlIO::receive(const string& sockid,
	       const string& interface,
	       const string& vif,
	       const IPv4& src,
	       const uint16_t& sport,
	       const vector<uint8_t>& payload)
{
    debug_msg("receive(%s, %s, %s, %s, %u, %u)\n",
	sockid.c_str(), interface.c_str(), vif.c_str(),
	cstring(src), XORP_UINT_CAST(sport),
	XORP_UINT_CAST(payload.size()));

    // Do I have an XrlPort on this interface where this packet
    // could actually have been received?
    // XXX: This can't be relied upon on all platforms, in particular
    // it may break on Windows; it is totally necessary for BSD derived
    // platforms, because if the socket(s) are bound to interface addresses,
    // they will not see any broadcast traffic ever.
    // TODO: Use sockid for a quicker lookup.
    XrlPortList& xpl = this->ports();
    XrlPortList::iterator xpi;
    xpi = find_if(xpl.begin(), xpl.end(),
		  port_has_interface_vif(interface, vif));
    if (xpi == xpl.end()) {
	XLOG_ERROR("No socket exists for interface/vif %s/%s",
		   interface.c_str(), vif.c_str());
	return;
    }

    // Did the upper layer register a receive callback?
    if (IO::_receive_cb.is_empty())
	return;

    //
    // XXX: create a copy of the payload, because the callback's argument
    // is not const-ified.
    // XXX: 'dst' and 'dport' cannot be learned from the socket4_user
    // XRL target, therefore empty addresses are passed to the upper layer.
    //
    vector<uint8_t> payload_copy(payload);
    IO::_receive_cb->dispatch(interface, vif, IPv4::ZERO(), 0, src, sport,
			      &payload_copy[0], payload_copy.size());
}

bool
XrlIO::send(const string& interface, const string& vif,
	    const IPv4& src, const uint16_t& sport,
	    const IPv4& dst, const uint16_t& dport,
	    uint8_t* data, const uint32_t& len)
{
    debug_msg("send(%s, %s, %s, %u, %s, %u, %p, %u)\n",
	      interface.c_str(), vif.c_str(),
	      cstring(src), XORP_UINT_CAST(dport),
	      cstring(dst), XORP_UINT_CAST(sport),
	      data, XORP_UINT_CAST(len));

    // Do I have an XrlPort on this interface to send from?
    XrlPortList& xpl = this->ports();
    XrlPortList::iterator xpi;
    xpi = find_if(xpl.begin(), xpl.end(), port_has_local_address(src));
    if (xpi == xpl.end()) {
	XLOG_ERROR("No socket exists for address %s/%s/%s:%u",
		   interface.c_str(), vif.c_str(),
		   cstring(src), XORP_UINT_CAST(sport));
	return false;
    }

    // Copy the payload, as the XRL send happens asynchronously.
    vector<uint8_t> payload(len);
    memcpy(&payload[0], data, len);

    bool result = (*xpi)->send_to(dst, dport, payload);

    return result;
}

bool
XrlIO::enable_address(const string& interface, const string& vif,
		      const IPv4& address, const uint16_t& port,
		      const IPv4& all_nodes_address)
{
    debug_msg("Interface %s Vif %s Address %s Port %u AllNodesAddr %s\n",
	      interface.c_str(), vif.c_str(),
	      cstring(address), XORP_UINT_CAST(port),
	      cstring(all_nodes_address));

    if (! address_exists(_iftree, interface, vif, address)) {
	XLOG_WARNING("%s/%s/%s:%u does not exist",
	    interface.c_str(), vif.c_str(),
	    cstring(address), XORP_UINT_CAST(port));
	return false;
    }

    // Check if port already exists.
    XrlPortList::const_iterator xpi;
    xpi = find_if(this->ports().begin(), this->ports().end(),
		  port_has_local_address(address));
    if (xpi != this->ports().end()) {
	XLOG_WARNING("Socket already exists for address %s/%s/%s:%u",
		     interface.c_str(), vif.c_str(),
		     cstring(address), XORP_UINT_CAST(port));
	return true;
    }

    // Create XrlPort.
    XrlPort* xp = new XrlPort(this,
			      _eventloop,
			      _xrl_router,
			      _feaname,
			      interface,
			      vif,
			      address,
			      port,
			      all_nodes_address);
    this->ports().push_back(xp);

    // Add self to observers of the XrlPort's status.
    xp->set_observer(this);

    // Start next XrlPort if no others are starting up already.
    try_start_next_port();

    return true;
}

bool
XrlIO::disable_address(const string& interface, const string& vif,
		       const IPv4& address, const uint16_t& port)
{
    debug_msg("Interface %s Vif %s Address %s Port %u\n",
	      interface.c_str(), vif.c_str(),
	      cstring(address), XORP_UINT_CAST(port));

    XrlPortList& xpl = this->ports();
    XrlPortList::iterator xpi;
    xpi = find_if(xpl.begin(), xpl.end(), port_has_local_address(address));
    if (xpi != xpl.end()) {
	XrlPort* xp = *xpi;
	if (xp) {
	    _dead_ports.insert(make_pair(dynamic_cast<ServiceBase*>(xp), xp));
	    xp->shutdown();
	}
	xpl.erase(xpi);
    }

    return true;
}

bool
XrlIO::is_vif_broadcast_capable(const string& interface, const string& vif)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    if (! is_interface_enabled(interface))
	return false;

    const IfMgrVifAtom* fv = ifmgr_iftree().find_vif(interface, vif);
    if (fv == NULL)
	return false;

    return (fv->broadcast_capable());
}

bool
XrlIO::is_vif_multicast_capable(const string& interface, const string& vif)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    if (! is_interface_enabled(interface))
	return false;

    const IfMgrVifAtom* fv = ifmgr_iftree().find_vif(interface, vif);
    if (fv == NULL)
	return false;

    return (fv->multicast_capable());
}

bool
XrlIO::is_vif_loopback(const string& interface, const string& vif)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    if (! is_interface_enabled(interface))
	return false;

    const IfMgrVifAtom* fv = ifmgr_iftree().find_vif(interface, vif);
    if (fv == NULL)
	return false;

    return (fv->loopback());
}

bool
XrlIO::get_broadcast_address(const string& interface,
			     const string& vif,
			     const IPv4& address,
			     IPv4& bcast_address) const
{
    debug_msg("Interface %s Vif %s Address %s\n", interface.c_str(),
	      vif.c_str(), cstring(address));

    if (! is_vif_enabled(interface, vif))
	return false;

    const IfMgrIPv4Atom* fa = ifmgr_iftree().find_addr(interface,
						       vif,
						       address);
    if (fa == NULL)
	return false;

    if (! fa->has_broadcast()) {
	debug_msg("%s/%s/%s doesn't have a broadcast address.\n",
		  interface.c_str(), vif.c_str(), cstring(address));
	return false;
    }

    bcast_address = fa->broadcast_addr();

    return true;
}

bool
XrlIO::is_interface_enabled(const string& interface) const
{
    debug_msg("Interface %s\n", interface.c_str());

    const IfMgrIfAtom* fi = ifmgr_iftree().find_interface(interface);
    if (fi == NULL)
	return false;

    return (fi->enabled() && (! fi->no_carrier()));
}

bool
XrlIO::is_vif_enabled(const string& interface, const string& vif) const
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    if (! is_interface_enabled(interface))
	return false;

    const IfMgrVifAtom* fv = ifmgr_iftree().find_vif(interface, vif);
    if (fv == NULL)
	return false;

    return (fv->enabled());
}

bool
XrlIO::is_address_enabled(const string& interface, const string& vif,
			  const IPv4& address) const
{
    return address_enabled(ifmgr_iftree(), interface, vif, address);
}

bool
XrlIO::get_addresses(const string& interface, const string& vif,
			   list<IPv4>& addresses) const
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    const IfMgrVifAtom* fv = ifmgr_iftree().find_vif(interface, vif);
    if (fv == NULL)
	return false;

    IfMgrVifAtom::IPv4Map::const_iterator i;
    for (i = fv->ipv4addrs().begin(); i != fv->ipv4addrs().end(); i++)
	addresses.push_back(i->second.addr());

    return true;
}

bool
XrlIO::get_interface_id(const string& interface, uint32_t& interface_id)
{
    debug_msg("Interface %s\n", interface.c_str());

    const IfMgrIfAtom* fi = ifmgr_iftree().find_interface(interface);
    if (fi == NULL)
	return false;

    interface_id = fi->pif_index();

    return true;
}

uint32_t
XrlIO::get_prefix_length(const string& interface, const string& vif,
			       IPv4 address)
{
    debug_msg("Interface %s Vif %s Address %s\n", interface.c_str(),
	      vif.c_str(), cstring(address));

    const IfMgrIPv4Atom* fa = ifmgr_iftree().find_addr(interface,
						       vif,
						       address);
    if (fa == NULL)
	return 0;

    return (fa->prefix_len());
}

uint32_t
XrlIO::get_mtu(const string& interface)
{
    debug_msg("Interface %s\n", interface.c_str());

    const IfMgrIfAtom* fi = ifmgr_iftree().find_interface(interface);
    if (fi == NULL)
	return 0;

    return (fi->mtu());
}

void
XrlIO::register_rib()
{
    XrlRibV0p1Client rib(&_xrl_router);

    // Register the protocol admin distance as configured.
    // XXX: Don't forget to add the protocol admin distance to every
    // RIB for which we plan to provide an origin table.
    if (! rib.send_set_protocol_admin_distance(
	    _ribname.c_str(),
	    "olsr",	// protocol
	    true,	// ipv4
	    false,	// ipv6
	    true,	// unicast
	    false,	// multicast
	    OLSR_ADMIN_DISTANCE,	// admin_distance
	    callback(this,
		     &XrlIO::rib_command_done,
		     true,
		     "set_protocol_admin_distance"))) {
	XLOG_WARNING("Failed to set OLSR admin distance in RIB");
    }

    if (! rib.send_add_igp_table4(
	    _ribname.c_str(),			// RIB target name
	    "olsr",				// protocol name
	    _xrl_router.class_name(),		// our class
	    _xrl_router.instance_name(),	// our instance
	    true,				// unicast
	    false,				// multicast
	    callback(this,
		     &XrlIO::rib_command_done,
		     true,
		     "add_igp_table4"))) {
	XLOG_FATAL("Failed to add OLSR table(s) to IPv4 RIB");
    }
}

void
XrlIO::unregister_rib()
{
    XrlRibV0p1Client rib(&_xrl_router);

    if (! rib.send_delete_igp_table4(
	    _ribname.c_str(),			// RIB target name
	    "olsr",				// protocol name
	    _xrl_router.class_name(),		// our class
	    _xrl_router.instance_name(),	// our instance
	    true,				// unicast
	    false,				// multicast
	    callback(this,
		     &XrlIO::rib_command_done,
		     false,
		     "delete_igp_table4"))) {
	XLOG_FATAL("Failed to delete OLSR table(s) from IPv4 RIB");
    }
}

void
XrlIO::rib_command_done(const XrlError& error, bool up,
			const char *comment)
{
    debug_msg("callback %s %s\n", comment, cstring(error));

    switch (error.error_code()) {
    case OKAY:
	break;
    case REPLY_TIMED_OUT:
	// We should really be using a reliable transport where
	// this error cannot happen. But it has so lets retry if we can.
	XLOG_ERROR("callback: %s %s",  comment, cstring(error));
	break;
    case RESOLVE_FAILED:
    case SEND_FAILED:
    case SEND_FAILED_TRANSIENT:
    case NO_SUCH_METHOD:
	XLOG_ERROR("callback: %s %s",  comment, cstring(error));
	break;
    case NO_FINDER:
	// XXX - Temporarily code dump if this condition occurs.
	XLOG_FATAL("NO FINDER");
	break;
    case BAD_ARGS:
    case COMMAND_FAILED:
    case INTERNAL_ERROR:
	XLOG_FATAL("callback: %s %s",  comment, cstring(error));
	break;
    }

    if (0 == strcasecmp(comment, "set_protocol_admin_distance"))
	return;

    if (up) {
	component_up(c_format("rib %s", comment));
    } else {
	component_down(c_format("rib %s", comment));
    }
}

bool
XrlIO::add_route(IPv4Net net,
		 IPv4 nexthop,
		 uint32_t nexthop_id,
		 uint32_t metric,
		 const PolicyTags& policytags)
{
    debug_msg("Net %s Nexthop %s metric %d policy %s\n",
	      cstring(net),
	      cstring(nexthop),
	      metric,
	      cstring(policytags));

    _rib_queue.queue_add_route(_ribname, net, nexthop, nexthop_id, metric,
			       policytags);

    return true;
}

bool
XrlIO::replace_route(IPv4Net net, IPv4 nexthop, uint32_t nexthop_id,
			uint32_t metric,
			const PolicyTags& policytags)
{
    debug_msg("Net %s Nexthop %s metric %d policy %s\n",
	      cstring(net),
	      cstring(nexthop),
	      metric,
	      cstring(policytags));

    _rib_queue.queue_delete_route(_ribname, net);
    _rib_queue.queue_add_route(_ribname, net, nexthop, nexthop_id, metric,
			       policytags);

    return true;
}

bool
XrlIO::delete_route(IPv4Net net)
{
    debug_msg("Net %s\n", cstring(net));

    _rib_queue.queue_delete_route(_ribname, net);

    return true;
}

void
XrlIO::tree_complete()
{
    //
    // XXX: we use same actions when the tree is completed or updates are made
    //
    updates_made();
}

void
XrlIO::updates_made()
{
    IfMgrIfTree::IfMap::const_iterator ii;
    IfMgrIfAtom::VifMap::const_iterator vi;
    IfMgrVifAtom::IPv4Map::const_iterator ai;
    const IfMgrIfAtom* if_atom;
    const IfMgrIfAtom* other_if_atom;
    const IfMgrVifAtom* vif_atom;
    const IfMgrVifAtom* other_vif_atom;
    const IfMgrIPv4Atom* addr_atom;
    const IfMgrIPv4Atom* other_addr_atom;

    //
    // Check whether the old interfaces, vifs and addresses are still there
    //
    for (ii = _iftree.interfaces().begin();
	 ii != _iftree.interfaces().end();
	 ++ii)
    {
	bool is_old_interface_enabled = false;
	bool is_new_interface_enabled = false;
	bool is_old_vif_enabled = false;
	bool is_new_vif_enabled = false;
	bool is_old_address_enabled = false;
	bool is_new_address_enabled = false;

	if_atom = &ii->second;
	is_old_interface_enabled = if_atom->enabled();
	is_old_interface_enabled &= (! if_atom->no_carrier());

	// Check the interface
	other_if_atom = ifmgr_iftree().find_interface(if_atom->name());
	if (other_if_atom == NULL) {
	    // The interface has disappeared
	    is_new_interface_enabled = false;
	} else {
	    is_new_interface_enabled = other_if_atom->enabled();
	    is_new_interface_enabled &= (! other_if_atom->no_carrier());
	}

	if ((is_old_interface_enabled != is_new_interface_enabled)
	    && (! _interface_status_cb.is_empty())) {
	    // The interface's enabled flag has changed
	    _interface_status_cb->dispatch(if_atom->name(),
					   is_new_interface_enabled);
	}

	for (vi = if_atom->vifs().begin(); vi != if_atom->vifs().end(); ++vi) {
	    vif_atom = &vi->second;
	    is_old_vif_enabled = vif_atom->enabled();
	    is_old_vif_enabled &= is_old_interface_enabled;

	    // Check the vif
	    other_vif_atom = ifmgr_iftree().find_vif(if_atom->name(),
						     vif_atom->name());
	    if (other_vif_atom == NULL) {
		// The vif has disappeared
		is_new_vif_enabled = false;
	    } else {
		is_new_vif_enabled = other_vif_atom->enabled();
	    }
	    is_new_vif_enabled &= is_new_interface_enabled;

	    if ((is_old_vif_enabled != is_new_vif_enabled)
		&& (! _vif_status_cb.is_empty())) {
		// The vif's enabled flag has changed
		_vif_status_cb->dispatch(if_atom->name(),
					 vif_atom->name(),
					 is_new_vif_enabled);
	    }

	    for (ai = vif_atom->ipv4addrs().begin();
		 ai != vif_atom->ipv4addrs().end();
		 ++ai) {
		addr_atom = &ai->second;
		is_old_address_enabled = addr_atom->enabled();
		is_old_address_enabled &= is_old_vif_enabled;

		// Check the address
		other_addr_atom = ifmgr_iftree().find_addr(if_atom->name(),
							   vif_atom->name(),
							   addr_atom->addr());
		if (other_addr_atom == NULL) {
		    // The address has disappeared
		    is_new_address_enabled = false;
		} else {
		    is_new_address_enabled = other_addr_atom->enabled();
		}
		is_new_address_enabled &= is_new_vif_enabled;

		if ((is_old_address_enabled != is_new_address_enabled)
		    && (! _address_status_cb.is_empty())) {
		    // The address's enabled flag has changed
		    _address_status_cb->dispatch(if_atom->name(),
						 vif_atom->name(),
						 addr_atom->addr(),
						 is_new_address_enabled);
		}
	    }
	}
    }

    //
    // Check for new interfaces, vifs and addresses
    //
    for (ii = ifmgr_iftree().interfaces().begin();
	 ii != ifmgr_iftree().interfaces().end();
	 ++ii) {
	if_atom = &ii->second;

	// Check the interface
	other_if_atom = _iftree.find_interface(if_atom->name());
	if (other_if_atom == NULL) {
	    // A new interface
	    if (if_atom->enabled()
		&& (! if_atom->no_carrier())
		&& (! _interface_status_cb.is_empty())) {
		_interface_status_cb->dispatch(if_atom->name(), true);
	    }
	}

	for (vi = if_atom->vifs().begin(); vi != if_atom->vifs().end(); ++vi) {
	    vif_atom = &vi->second;

	    // Check the vif
	    other_vif_atom = _iftree.find_vif(if_atom->name(),
					      vif_atom->name());
	    if (other_vif_atom == NULL) {
		// A new vif
		if (if_atom->enabled()
		    && (! if_atom->no_carrier())
		    && (vif_atom->enabled())
		    && (! _vif_status_cb.is_empty())) {
		    _vif_status_cb->dispatch(if_atom->name(), vif_atom->name(),
					     true);
		}
	    }

	    for (ai = vif_atom->ipv4addrs().begin();
		 ai != vif_atom->ipv4addrs().end();
		 ++ai) {
		addr_atom = &ai->second;

		// Check the address
		other_addr_atom = _iftree.find_addr(if_atom->name(),
						    vif_atom->name(),
						    addr_atom->addr());
		if (other_addr_atom == NULL) {
		    // A new address
		    if (if_atom->enabled()
			&& (! if_atom->no_carrier())
			&& (vif_atom->enabled())
			&& (addr_atom->enabled())
			&& (! _address_status_cb.is_empty())) {
			_address_status_cb->dispatch(if_atom->name(),
						     vif_atom->name(),
						     addr_atom->addr(),
						     true);
		    }
		}
	    }
	}
    }

    //
    // Update the local copy of the interface tree
    //
    _iftree = ifmgr_iftree();
}

// Gradually start each XrlPort to avoid races with the FEA.
void
XrlIO::try_start_next_port()
{
    // If there are any ports currently starting up,
    // do not try to start another.
    XrlPortList::const_iterator cpi;
    cpi = find_if(_ports.begin(), _ports.end(),
		  port_has_io_in_state(SERVICE_STARTING));
    if (cpi != _ports.end()) {
	debug_msg("doing nothing (there is a port %p in SERVICE_STARTING "
		  "state).\n", (*cpi));
	return;
    }

    // Look for the first port that needs to be started up, and try
    // to start it up.
    XrlPortList::iterator xpi = _ports.begin();
    XrlPort* xp = 0;
    while (xp == 0) {
	xpi = find_if(xpi, _ports.end(),
		      port_has_io_in_state(SERVICE_READY));
	if (xpi == _ports.end()) {
	    debug_msg("reached end of ports in SERVICE_READY state\n");
	    return;
	}
	xp = (*xpi);	// XXX
	xpi++;
    }

    debug_msg("starting up port %p\n", xp);
    xp->startup();
}

XrlPort*
XrlIO::find_port(const string&	ifname,
		 const string&	vifname,
		 const IPv4&	addr)
{
    XrlPortList::iterator xpi;
    xpi = find_if(this->ports().begin(), this->ports().end(),
		  port_has_local_address(addr));
    if (xpi == this->ports().end()) {
	return 0;
    }

    XrlPort* xp = (*xpi);
    if (xp->ifname() != ifname || xp->vifname() != vifname) {
	return 0;
    }
    return xp;
}

const XrlPort*
XrlIO::find_port(const string&	ifname,
		 const string&	vifname,
		 const IPv4&	addr) const
{
    XrlPortList::const_iterator xpi;
    xpi = find_if(this->ports().begin(), this->ports().end(),
		  port_has_local_address(addr));
    if (xpi == this->ports().end()) {
	return 0;
    }

    const XrlPort* xp = (*xpi);
    if (xp->ifname() != ifname || xp->vifname() != vifname) {
	return 0;
    }
    return xp;
}
