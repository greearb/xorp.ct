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

#ident "$XORP: xorp/rip/xrl_port_manager.cc,v 1.4 2004/02/12 19:10:41 hodson Exp $"

#define DEBUG_LOGGING

#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/debug.h"

#include "libxipc/xrl_router.hh"

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
    const IfMgrIfAtom* ia = iftree.find_if(ifname);
    if (ia == 0 || ia->enabled() == false)
	return false;

    const IfMgrVifAtom* va = ia->find_vif(vifname);
    if (va == 0 || va->enabled() == false)
	return false;

    const typename IfMgrIP<A>::Atom* aa = va->find_addr(addr);
    return (aa != 0 && aa->enabled());
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
 * Unary function object to test whether a particular socket id is
 * associated with a RIP port.
 *
 * NB At a future date we might want to track socket id to XrlPortIO
 * mappings.  This would be more efficient.
 */
template <typename A>
struct port_has_sockid {
    inline port_has_sockid(const string& sockid) : _id(sockid) {}
    inline bool operator() (Port<A>*& p) {
	PortIOBase<A>* io = p->io_handler();
	XrlPortIO<A>* xio = dynamic_cast<XrlPortIO<A>*>(io);
	return xio && xio->socket_id() == _id;
    }
private:
    string _id;
};


// ----------------------------------------------------------------------------
// XrlPortManager

template <typename A>
void
XrlPortManager<A>::startup()
{
    set_status(STARTING);
    // Transition to RUNNING occurs when tree_complete() is called, ie
    // we have interface/vif/address state available.
}

template <typename A>
void
XrlPortManager<A>::shutdown()
{
    set_status(SHUTTING_DOWN);

    typename PortManagerBase<A>::PortList& pl = ports();
    typename PortManagerBase<A>::PortList::iterator i = pl.begin();

    // XXX Walk ports and shut them down.  Only when they are all
    // shutdown should we consider ourselves shutdown.

    debug_msg("XXX XrlPortManager<A>::shutdown (%p)\n", this);
    debug_msg("XXX n_ports = %u n_dead_ports %u\n",
	      ports().size(), _dead_ports.size());

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
    pi = find_if(ports().begin(), ports().end(), port_has_address<A>(addr));
    if (pi != ports().end())
	return true;

    // Create port
    Port<A>* p = new Port<A>(*this);
    ports().push_back(p);

    // Create XrlPortIO object for port
    XrlPortIO<A>* io = new XrlPortIO<A>(_xr, *p, ifname, vifname, addr);

    // Bind io to port
    p->set_io_handler(io, false);

    // Add self to observers of io objects status
    io->set_observer(this);

    // Start io object
    io->startup();

    return true;
}

template <typename A>
bool
XrlPortManager<A>::remove_rip_address(const string& 	/* ifname */,
				      const string&	/* vifname */,
				      const A&	addr)
{
    typename PortManagerBase<A>::PortList& pl = ports();
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
    typename PortManagerBase<A>::PortList& pl = ports();
    typename PortManagerBase<A>::PortList::iterator i;
    i = find_if(pl.begin(), pl.end(), port_has_sockid<A>(sockid));
    if (i == ports().end()) {
	return false;
    }
    Port<A>* p = *i;
    p->port_io_receive(src_addr, src_port, &pdata[0], pdata.size());
    return true;
}

template <typename A>
Port<A>*
XrlPortManager<A>::find_port(const string& 	ifname,
			     const string& 	vifname,
			     const A&		addr)
{
    typename PortManagerBase<A>::PortList::iterator pi;
    pi = find_if(ports().begin(), ports().end(), port_has_address<A>(addr));
    if (pi == ports().end()) {
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
    pi = find_if(ports().begin(), ports().end(), port_has_address<A>(addr));
    if (pi == ports().end()) {
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
void
XrlPortManager<A>::status_change(ServiceBase* 	service,
				 ServiceStatus 	/* old_status */,
				 ServiceStatus 	new_status)
{
    debug_msg("XXX %p status -> %s\n",
	      service, service_status_name(new_status));

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

template class XrlPortManager<IPv4>;
template class XrlPortManager<IPv6>;
