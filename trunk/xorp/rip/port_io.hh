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

// $XORP: xorp/rip/port_io.hh,v 1.6 2004/01/09 00:13:51 hodson Exp $

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
    inline const string& ifname() const { return _ifname; }

    /**
     * Get Virtual Interface name associated with I/O.
     */
    inline const string& vifname() const { return _vifname; }

    /**
     * Get associated address.
     */
    inline const Addr& address() const { return _addr; }

    /**
     * Get the maximum number of route entries in a packet.
     */
    inline size_t max_route_entries_per_packet() const { return _max_rte_pp; }

    /**
     * Set the maximum number of route entries in a packet.
     * @return true on success, false if route entries per packet is fixed.
     */
    inline bool set_max_route_entries_per_packet(size_t max_entries);

    /**
     * Get enabled status of I/O system.
     */
    inline bool	enabled() const { return _en; }

    /**
     * Set enabled status of I/O system.  The user object associated with
     * the I/O system will be notified through
     * @ref PortIOBase<A>::port_io_enable_change() if the enabled state
     * changes.
     *
     * @param en new enable state.
     */
    inline void set_enabled(bool en);

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

    inline bool set_io_handler(PortIO* pio, bool set_owner);

    inline PortIO* io_handler();

    inline const PortIO* io_handler() const;

    inline bool port_io_enabled() const;

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
