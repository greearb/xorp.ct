// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP$

#ifndef __FEA_NEXTHOP_PORT_MAPPER_HH__
#define __FEA_NEXTHOP_PORT_MAPPER_HH__

#include <list>
#include <map>


class NexthopPortMapperObserver;

/**
 * @short A class for to keep the mapping between next-hop information
 * and a port number.
 *
 * The next-hop information can be one of the following: network interface,
 * IP host address (local or peer address on point-to-point links), or
 * IP subnet address (of the directly connected subnet for an interface).
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
    void notify_observers();

    map<pair<string, string>, int>	_interface_map;
    map<IPv4, int>			_ipv4_map;
    map<IPv6, int>			_ipv6_map;
    map<IPv4Net, int>			_ipv4net_map;
    map<IPv6Net, int>			_ipv6net_map;
    list<NexthopPortMapperObserver *>	_observers;
};

/**
 * @short A class for observing changes to a @see NetlinkPortMapper object.
 */
class NexthopPortMapperObserver {
public:
    virtual ~NexthopPortMapperObserver() = 0;
    virtual void nexthop_port_mapper_event() = 0;
};

#endif // __FEA_NEXTHOP_PORT_MAPPER_HH__
