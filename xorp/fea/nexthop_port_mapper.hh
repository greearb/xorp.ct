// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/fea/nexthop_port_mapper.hh,v 1.15 2008/10/02 21:56:50 bms Exp $

#ifndef __FEA_NEXTHOP_PORT_MAPPER_HH__
#define __FEA_NEXTHOP_PORT_MAPPER_HH__




#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"


class NexthopPortMapperObserver;

/**
 * @short A class for to keep the mapping between next-hop information
 * and a port number.
 *
 * The next-hop information can be one of the following: network interface,
 * IP host address (local or peer address on point-to-point links), or
 * IP subnet address (of the directly connected subnet for an interface).
 *
 * Note that observers (@ref NexthopPortMapperObserver) can be attached
 * to monitor changes to the port mapping. The observers notification
 * is triggered by an explicit call to @ref NexthopPortMapper#notify_observers.
 */
class NexthopPortMapper {
public:
    NexthopPortMapper();
    ~NexthopPortMapper();

    /**
     * Clear all mapping.
     */
    void clear();

    /**
     * Add an observer for observing changes to the mapper.
     *
     * @param observer the observer to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_observer(NexthopPortMapperObserver* observer);

    /**
     * Delete an observer for observing changes to the mapper.
     *
     * @param observer the observer to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_observer(NexthopPortMapperObserver* observer);

    /**
     * Notify observers about any port mapping changes.
     */
    void notify_observers();

    /**
     * Lookup a next-hop interface/vif name to obtain the corresponding
     * port number.
     *
     * @param ifname the next-hop interface name to lookup.
     * @param vifname the next-hop vif name to lookup.
     * @return the port number on success, otherwise -1.
     */
    int lookup_nexthop_interface(const string& ifname,
				 const string& vifname) const;

    /**
     * Lookup a next-hop IPv4 address to obtain the corresponding port number.
     *
     * @param ipv4 the next-hop address to lookup.
     * @return the port number on success, otherwise -1.
     */
    int lookup_nexthop_ipv4(const IPv4& ipv4) const;

    /**
     * Lookup a next-hop IPv6 address to obtain the corresponding port number.
     *
     * @param ipv6 the next-hop address to lookup.
     * @return the port number on success, otherwise -1.
     */
    int lookup_nexthop_ipv6(const IPv6& ipv6) const;

    /**
     * Add an entry for an interface/vif name to port mapping.
     *
     * If the entry already exists, then the port will be updated.
     *
     * @param ifname the interface name to add.
     * @param vifname the vif name to add.
     * @param port the port number to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_interface(const string& ifname, const string& vifname, int port);

    /**
     * Delete an entry for an interface/vif name to port mapping.
     *
     * @param ifname the interface name to delete.
     * @param vifname the vif name to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_interface(const string& ifname, const string& vifname);

    /**
     * Add an entry for an IPv4 address to port mapping.
     *
     * If the entry already exists, then the port will be updated.
     *
     * @param ipv4 the address to add.
     * @param port the port number to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_ipv4(const IPv4& ipv4, int port);

    /**
     * Delete an entry for an IPv4 address to port mapping.
     *
     * @param ipv4 the address to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_ipv4(const IPv4& ipv4);

    /**
     * Add an entry for an IPv6 address to port mapping.
     *
     * If the entry already exists, then the port will be updated.
     *
     * @param ipv6 the address to add.
     * @param port the port number to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_ipv6(const IPv6& ipv6, int port);

    /**
     * Delete an entry for an IPv6 address to port mapping.
     *
     * @param ipv6 the address to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_ipv6(const IPv6& ipv6);

    /**
     * Add an entry for an IPv4 subnet to port mapping.
     *
     * If the entry already exists, then the port will be updated.
     *
     * @param ipv4net the subnet to add.
     * @param port the port number to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_ipv4net(const IPv4Net& ipv4net, int port);

    /**
     * Delete an entry for an IPv4 subnet to port mapping.
     *
     * @param ipv4net the subnet to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_ipv4net(const IPv4Net& ipv4net);

    /**
     * Add an entry for an IPv6 subnet to port mapping.
     *
     * If the entry already exists, then the port will be updated.
     *
     * @param ipv6net the subnet to add.
     * @param port the port number to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_ipv6net(const IPv6Net& ipv6net, int port);

    /**
     * Delete an entry for an IPv6 subnet to port mapping.
     *
     * @param ipv6net the subnet to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_ipv6net(const IPv6Net& ipv6net);

private:
    bool is_mapping_changed() const;

    // The maps with the lookup info
    map<pair<string, string>, int>	_interface_map;
    map<IPv4, int>			_ipv4_map;
    map<IPv6, int>			_ipv6_map;
    map<IPv4Net, int>			_ipv4net_map;
    map<IPv6Net, int>			_ipv6net_map;

    // The maps with the old info
    map<pair<string, string>, int>	_old_interface_map;
    map<IPv4, int>			_old_ipv4_map;
    map<IPv6, int>			_old_ipv6_map;
    map<IPv4Net, int>			_old_ipv4net_map;
    map<IPv6Net, int>			_old_ipv6net_map;

    list<NexthopPortMapperObserver *>	_observers;
};

/**
 * @short A class for observing changes to a @ref NetlinkPortMapper object.
 */
class NexthopPortMapperObserver {
public:
    /**
     * Empty virtual destructor
     */
    virtual ~NexthopPortMapperObserver() {}

    /**
     * The next-hop port mapping has changed.
     *
     * @param is_mapping_changed if true, the port mapping has changed,
     * otherwise it remains same (i.e., it has been only "touched").
     */
    virtual void nexthop_port_mapper_event(bool is_mapping_changed) = 0;
};

#endif // __FEA_NEXTHOP_PORT_MAPPER_HH__
