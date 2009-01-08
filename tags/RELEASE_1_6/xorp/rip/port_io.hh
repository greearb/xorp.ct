// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/rip/port_io.hh,v 1.16 2008/10/02 21:58:17 bms Exp $

#ifndef __RIP_PORT_IO_HH__
#define __RIP_PORT_IO_HH__

#include "constants.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

template <typename A>
class PortIOUserBase;

/**
 * Base class for RIP Port I/O classes.
 *
 * RIP Port I/O classes provide support for reading and writing to RIP
 * Ports.
 */
template <typename A>
class PortIOBase
{
public:
    typedef A			Addr;
    typedef PortIOUserBase<A>	PortIOUser;
public:
    /**
     * Constructor associating a Port I/O User object with I/O object.
     */
    PortIOBase(PortIOUser&	user,
	       const string&	ifname,
	       const string&	vifname,
	       const Addr&	address,
	       bool		enabled = true);

    virtual ~PortIOBase();

    /**
     * Send RIP packet to Port.
     *
     * The invoker of this method is expected to be the associated
     * Port I/O User instance.  The invoker is responsible for the
     * packet data in the send request and should not decommission the
     * data until it receives the @ref
     * PortIOUserBase<A>::send_completion callback.  If the request to
     * send fails immediately the send_completion callback is not
     * called for the associated data.
     *
     * @param dst_addr address to send packet.
     * @param dst_port port to send packet to.
     * @param rip_packet vector containing rip packet to be sent.
     *
     * @return false on immediately detectable failure, true otherwise.
     */
    virtual bool send(const Addr&		dst_addr,
		      uint16_t			dst_port,
		      const vector<uint8_t>&	rip_packet) = 0;

    /**
     * Check if send request is pending.
     * @return true if a send request is pending, false otherwise.
     */
    virtual bool pending() const = 0;

    /**
     * Get Interface name associated with I/O.
     */
    const string& ifname() const { return _ifname; }

    /**
     * Get Virtual Interface name associated with I/O.
     */
    const string& vifname() const { return _vifname; }

    /**
     * Get associated address.
     */
    const Addr& address() const { return _addr; }

    /**
     * Get the maximum number of route entries in a packet.
     */
    size_t max_route_entries_per_packet() const { return _max_rte_pp; }

    /**
     * Set the maximum number of route entries in a packet.
     * @return true on success, false if route entries per packet is fixed.
     */
    bool set_max_route_entries_per_packet(size_t max_entries);

    /**
     * Get enabled status of I/O system.
     */
    bool enabled() const { return _en; }

    /**
     * Set enabled status of I/O system.  The user object associated with
     * the I/O system will be notified through
     * @ref PortIOBase<A>::port_io_enabled_change() if the enabled state
     * changes.
     *
     * @param en new enable state.
     */
    void set_enabled(bool en);

protected:
    PortIOUser&	_user;
    string	_ifname;
    string	_vifname;
    Addr	_addr;
    size_t	_max_rte_pp;
    bool	_en;
};


/**
 * @short Base class for users of Port I/O classes.
 */
template <typename A>
class PortIOUserBase {
public:
    typedef PortIOBase<A> PortIO;
public:
    PortIOUserBase() : _pio(0) {}

    virtual ~PortIOUserBase();

    virtual void port_io_send_completion(bool		success) = 0;

    virtual void port_io_receive(const A&	src_addr,
				 uint16_t	src_port,
				 const uint8_t* rip_packet,
				 size_t 	rip_packet_bytes) = 0;

    virtual void port_io_enabled_change(bool en) = 0;

    bool set_io_handler(PortIO* pio, bool set_owner);

    PortIO* io_handler();

    const PortIO* io_handler() const;

    bool port_io_enabled() const;

protected:
    PortIO*	_pio;
    bool	_pio_owner;
};


// ----------------------------------------------------------------------------
// Inline PortIOBase Methods
//

template <typename A>
PortIOBase<A>::PortIOBase(PortIOUser&	user,
			  const string&	ifname,
			  const string&	vifname,
			  const Addr&	addr,
			  bool		en)
    : _user(user), _ifname(ifname), _vifname(vifname), _addr(addr),
      _max_rte_pp(RIPv2_ROUTES_PER_PACKET), _en(en)
{}

template <typename A>
PortIOBase<A>::~PortIOBase()
{}

template <>
inline bool
PortIOBase<IPv4>::set_max_route_entries_per_packet(size_t)
{
    return false;
}

template <>
inline bool
PortIOBase<IPv6>::set_max_route_entries_per_packet(size_t max_entries)
{
    _max_rte_pp = max_entries;
    return true;
}

template <typename A>
inline void
PortIOBase<A>::set_enabled(bool en)
{
    if (en != _en) {
	_en = en;
	_user.port_io_enabled_change(en);
    }
}


// ----------------------------------------------------------------------------
// Inline PortIOUserBase Methods
//

template <typename A>
PortIOUserBase<A>::~PortIOUserBase()
{
    if (_pio && _pio_owner)
	delete _pio;
    _pio = 0;
}

template <typename A>
inline bool
PortIOUserBase<A>::set_io_handler(PortIO* pio, bool set_owner)
{
    if (_pio == 0) {
	_pio = pio;
	_pio_owner = set_owner;
	return true;
    }
    return false;
}

template <typename A>
inline PortIOBase<A>*
PortIOUserBase<A>::io_handler()
{
    return _pio;
}

template <typename A>
inline const PortIOBase<A>*
PortIOUserBase<A>::io_handler() const
{
    return _pio;
}

template <typename A>
inline bool
PortIOUserBase<A>::port_io_enabled() const
{
    if (_pio)
	return _pio->enabled();
    return false;
}

#endif // __RIP_PEER_IO_HH__
