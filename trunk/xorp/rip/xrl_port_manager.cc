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

#ident "$XORP: xorp/rip/xrl_port_manager.cc,v 1.13 2004/05/03 23:08:09 hodson Exp $"

// #define DEBUG_LOGGING

#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/debug.h"

#include "libxipc/xrl_router.hh"

#include "libfeaclient/ifmgr_atoms.hh"

#include "port.hh"
#include "xrl_port_manager.hh"
#include "xrl_port_io.hh"

// ----------------------------------------------------------------------------
// Utility methods

/**
 * Query whether an address exists on given interface and vif path and all
 * items on path are enabled.
 */
template <typename A>
static bool
address_enabled(const IfMgrIfTree&	iftree,
		const string&		ifname,
		const string&		vifname,
		const A&		addr)
{
    debug_msg("Looking for %s/%s/%s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str());

    const IfMgrIfAtom* ia = iftree.find_if(ifname);
    if (ia == 0 || ia->enabled() == false) {
	debug_msg("if %s exists ? %d ?\n", ifname.c_str(), (ia ? 1 : 0));
	return false;
    }

    const IfMgrVifAtom* va = ia->find_vif(vifname);
    if (va == 0 || va->enabled() == false) {
	debug_msg("vif %s exists ? %d?\n", vifname.c_str(), (va ? 1: 0));
	return false;
    }

    const typename IfMgrIP<A>::Atom* aa = va->find_addr(addr);
    if (aa == 0 || aa->enabled() == false) {
	debug_msg("addr %s exists ? %d?\n", addr.str().c_str(), (aa ? 1: 0));
	return false;
    }
    debug_msg("Found\n");
    return true;
}

/**
 * Unary function object to test whether a particular address is
 * associated with a RIP port.
 */
template <typename A>
struct port_has_address {
    inline port_has_address(const A& addr) : _addr(addr) {}
    inline bool operator() (const Port<A>* p) {
	const PortIOBase<A>* io = p->io_handler();
	return io && io->address() == _addr;
    }
private:
    A _addr;
};

/**
 * Unary function object to test whether a particular port's io
 * system is in a given state.
 */
template <typename A>
struct port_has_io_in_state {
    inline port_has_io_in_state(ServiceStatus st) : _st(st) {}

    bool operator() (const Port<A>* p) const
    {
	const PortIOBase<A>* 	io  = p->io_handler();
	const XrlPortIO<A>* 	xio = dynamic_cast<const XrlPortIO<A>*>(io);
	if (xio == 0)
	    return false;
	return xio->status() == _st;
    }
protected:
    ServiceStatus _st;
};

/**
 * Unary function object to test whether a particular RIP port is appropriate
 * for packet arriving on socket.
 *
 * NB At a future date we might want to track socket id to XrlPortIO
 * mappings.  This would be more efficient.
 */
template <typename A>
struct is_port_for {
    inline is_port_for(const string* sockid, const A* addr, IfMgrXrlMirror* im)
	: _psid(sockid), _pa(addr), _pim(im)
    {}

    inline bool operator() (Port<A>*& p);

protected:
    inline bool link_addr_valid() const;

private:
    const string* 	_psid;
    const A* 		_pa;
    IfMgrXrlMirror* 	_pim;
};

template <>
inline bool
is_port_for<IPv4>::link_addr_valid() const
{
    return true;
}

template <>
inline bool
is_port_for<IPv6>::link_addr_valid() const
{
    return _pa->is_linklocal_unicast();
}

template <typename A>
bool
is_port_for<A>::operator() (Port<A>*& p)
{
    //
    // Perform address family specific check for source address being
    // link-local.  For IPv4 the concept does not exist, for IPv6
    // check if origin is link local.
    //
    if (link_addr_valid() == false) {
	return false;
    }

    PortIOBase<A>* 	io  = p->io_handler();
    XrlPortIO<A>* 	xio = dynamic_cast<XrlPortIO<A>*>(io);
    if (xio == 0)
	return false;

    // If another socket, ignore
    if (xio->socket_id() != *_psid)
	return false;

    // If our packet, ignore
    if (xio->address() == *_pa)
	return false;

    //
    // Packet has arrived on multicast socket and is not one of ours.
    //
    // Check source address to find originating neighbour on local nets
    // or p2p link.
    //
    const typename IfMgrIP<A>::Atom* ifa;
    ifa = _pim->iftree().find_addr(xio->ifname(),
				   xio->vifname(),
				   xio->address());

    if (ifa == 0) {
	return false;
    }
    if (ifa->has_endpoint()) {
	return ifa->endpoint_addr() == *_pa;
    }

    IPNet<A> n(ifa->addr(), ifa->prefix_len());
    return n.contains(*_pa);
}


// ----------------------------------------------------------------------------
// XrlPortManager

template <typename A>
bool
XrlPortManager<A>::startup()
{
    set_status(STARTING);
    // Transition to RUNNING occurs when tree_complete() is called, ie
    // we have interface/vif/address state available.

    return true;
}

template <typename A>
bool
XrlPortManager<A>::shutdown()
{
    set_status(SHUTTING_DOWN);

    typename PortManagerBase<A>::PortList& pl = this->ports();
    typename PortManagerBase<A>::PortList::iterator i = pl.begin();

    // XXX Walk ports and shut them down.  Only when they are all
    // shutdown should we consider ourselves shutdown.

    debug_msg("XXX XrlPortManager<A>::shutdown (%p)\n", this);
    debug_msg("XXX n_ports = %u n_dead_ports %u\n",
	      uint32_t(this->ports().size()),
	      uint32_t(_dead_ports.size()));

    while (i != pl.end()) {
	Port<A>* p = *i;
	XrlPortIO<A>* xio = dynamic_cast<XrlPortIO<A>*>(p->io_handler());
	if (xio) {
	    _dead_ports.insert(make_pair(xio, p));
	    xio->shutdown();
	    pl.erase(i++);
	    debug_msg("   XXX killing port %p\n", p);
	} else {
	    i++;
	    debug_msg("   XXX skipping port %p\n", p);
	}
    }

    return true;
}

template <typename A>
void
XrlPortManager<A>::tree_complete()
{
    debug_msg("XrlPortManager<IPv%d>::tree_complete notification\n",
	      A::ip_version());
    set_status(RUNNING);
}

template <typename A>
void
XrlPortManager<A>::updates_made()
{
    debug_msg("XrlPortManager<IPv%d>::updates_made notification\n",
	      A::ip_version());
}

template <typename A>
bool
XrlPortManager<A>::add_rip_address(const string& ifname,
				   const string& vifname,
				   const A&	 addr)
{
    if (status() != RUNNING) {
	debug_msg("add_rip_address failed: not running.\n");
	return false;
    }

    // Check whether address exists and is enabled, fail if not.
    const IfMgrIfTree& iftree = _ifm.iftree();
    if (address_enabled(iftree, ifname, vifname, addr) == false) {
	debug_msg("add_rip_address failed: address does not exist or is not enabled.\n");
	return false;
    }

    // Check if port already exists
    typename PortManagerBase<A>::PortList::const_iterator pi;
    pi = find_if(this->ports().begin(), this->ports().end(),
		 port_has_address<A>(addr));
    if (pi != this->ports().end())
	return true;

    // Create port
    Port<A>* p = new Port<A>(*this);
    this->ports().push_back(p);

    // Create XrlPortIO object for port
    XrlPortIO<A>* io = new XrlPortIO<A>(_xr, *p, ifname, vifname, addr);

    // Bind io to port
    p->set_io_handler(io, false);

    // Add self to observers of io objects status
    io->set_observer(this);

    // Start port's I/O handler if no others are starting up already
    try_start_next_io_handler();

    return true;
}

template <typename A>
bool
XrlPortManager<A>::remove_rip_address(const string& 	/* ifname */,
				      const string&	/* vifname */,
				      const A&	addr)
{
    typename PortManagerBase<A>::PortList& pl = this->ports();
    typename PortManagerBase<A>::PortList::iterator i;
    i = find_if(pl.begin(), pl.end(), port_has_address<A>(addr));
    if (i != pl.end()) {
	Port<A>* p = *i;
	XrlPortIO<A>* xio = dynamic_cast<XrlPortIO<A>*>(p->io_handler());
	if (xio) {
	    _dead_ports.insert(make_pair(xio, p));
	    xio->shutdown();
	}
	pl.erase(i);
    }
    return true;
}

template <typename A>
bool
XrlPortManager<A>::deliver_packet(const string& 		sockid,
				  const A& 			src_addr,
				  uint16_t 			src_port,
				  const vector<uint8_t>& 	pdata)
{
    typename PortManagerBase<A>::PortList& pl = this->ports();
    typename PortManagerBase<A>::PortList::iterator i;

    debug_msg("Packet on %s from %s/%u %u bytes\n",
	      sockid.c_str(), src_addr.str().c_str(), src_port,
	      static_cast<uint32_t>(pdata.size()));

    i = find_if(pl.begin(), pl.end(),
		is_port_for<A>(&sockid, &src_addr, &_ifm));

    if (i == this->ports().end()) {
	debug_msg("Discarding packet %s/%u %u bytes\n",
		  src_addr.str().c_str(), src_port,
		  static_cast<uint32_t>(pdata.size()));
	return false;
    }
    Port<A>* p = *i;
    p->port_io_receive(src_addr, src_port, &pdata[0], pdata.size());

    XLOG_ASSERT(find_if(++i, pl.end(),
			is_port_for<A>(&sockid, &src_addr, &_ifm))
		== pl.end());

    return true;
}

template <typename A>
Port<A>*
XrlPortManager<A>::find_port(const string& 	ifname,
			     const string& 	vifname,
			     const A&		addr)
{
    typename PortManagerBase<A>::PortList::iterator pi;
    pi = find_if(this->ports().begin(), this->ports().end(),
		 port_has_address<A>(addr));
    if (pi == this->ports().end()) {
	return 0;
    }

    Port<A>* port = *pi;
    PortIOBase<A>* port_io = port->io_handler();
    if (port_io->ifname() != ifname || port_io->vifname() != vifname) {
	return 0;
    }
    return port;
}

template <typename A>
const Port<A>*
XrlPortManager<A>::find_port(const string& 	ifname,
			     const string& 	vifname,
			     const A&		addr) const
{
    typename PortManagerBase<A>::PortList::const_iterator pi;
    pi = find_if(this->ports().begin(), this->ports().end(),
		 port_has_address<A>(addr));
    if (pi == this->ports().end()) {
	return 0;
    }

    const Port<A>* port = *pi;
    const PortIOBase<A>* port_io = port->io_handler();
    if (port_io->ifname() != ifname || port_io->vifname() != vifname) {
	return 0;
    }
    return port;
}

template <typename A>
bool
XrlPortManager<A>::underlying_rip_address_up(const string&	ifname,
					     const string&	vifname,
					     const A&		addr) const
{
    return address_enabled(_ifm.iftree(), ifname, vifname, addr);
}

template <typename A>
bool
XrlPortManager<A>::underlying_rip_address_exists(const string&	ifname,
						 const string&	vifname,
						 const A&	addr) const
{
    return _ifm.iftree().find_addr(ifname, vifname, addr) != 0;
}

template <typename A>
void
XrlPortManager<A>::status_change(ServiceBase* 	service,
				 ServiceStatus 	/* old_status */,
				 ServiceStatus 	new_status)
{
    debug_msg("XXX %p status -> %s\n",
	      service, service_status_name(new_status));

    try_start_next_io_handler();

    if (new_status != SHUTDOWN)
	return;

    typename map<ServiceBase*, Port<A>*>::iterator i;
    i = _dead_ports.find(service);
    XLOG_ASSERT(i != _dead_ports.end());
    //    delete i->second;
    //    _dead_ports.erase(i);
}

template <typename A>
XrlPortManager<A>::~XrlPortManager()
{
    _ifm.detach_hint_observer(this);

    while (_dead_ports.empty() == false) {
	Port<A>* p = _dead_ports.begin()->second;
	PortIOBase<A>* io = p->io_handler();
	delete io;
	delete p;
	_dead_ports.erase(_dead_ports.begin());
    }
}

template <typename A>
void
XrlPortManager<A>::try_start_next_io_handler()
{
    typename PortManagerBase<A>::PortList::const_iterator cpi;
    cpi = find_if(this->ports().begin(), this->ports().end(),
		  port_has_io_in_state<A>(STARTING));
    if (cpi != this->ports().end()) {
	return;
    }

    typename PortManagerBase<A>::PortList::iterator pi = this->ports().begin();
    XrlPortIO<A>* xio = 0;
    while (xio == 0) {
	pi = find_if(pi, this->ports().end(),
		     port_has_io_in_state<A>(READY));
	if (pi == this->ports().end()) {
	    return;
	}
	Port<A>* p = (*pi);
	xio = dynamic_cast<XrlPortIO<A>*>(p->io_handler());
	pi++;
    }
    xio->startup();
}

#ifdef INSTANTIATE_IPV4
template class XrlPortManager<IPv4>;
#endif

#ifdef INSTANTIATE_IPV6
template class XrlPortManager<IPv6>;
#endif
