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

// $XORP: xorp/fea/data_plane/fibconfig/fibconfig_entry_get_routing_socket.hh,v 1.9 2008/10/02 21:56:56 bms Exp $

#ifndef __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_ENTRY_GET_ROUTING_SOCKET_HH__
#define __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_ENTRY_GET_ROUTING_SOCKET_HH__

#include "fea/fibconfig_entry_get.hh"
#include "fea/data_plane/control_socket/routing_socket.hh"


class FibConfigEntryGetRoutingSocket : public FibConfigEntryGet,
				       public RoutingSocket {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    FibConfigEntryGetRoutingSocket(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~FibConfigEntryGetRoutingSocket();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

    /**
     * Lookup an IPv4 route by destination address.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int lookup_route_by_dest4(const IPv4& dst, Fte4& fte);

    /**
     * Lookup an IPv4 route by network address.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int lookup_route_by_network4(const IPv4Net& dst, Fte4& fte);

    /**
     * Lookup an IPv6 route by destination address.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int lookup_route_by_dest6(const IPv6& dst, Fte6& fte);

    /**
     * Lookup an IPv6 route by network address.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int lookup_route_by_network6(const IPv6Net& dst, Fte6& fte);

    /**
     * Flag values used to tell underlying FIB message parsing routines
     * which messages the caller is interested in.
     */
    struct FibMsg {
	enum {
	    UPDATES	= 1 << 0,
	    GETS	= 1 << 1,
	    RESOLVES	= 1 << 2
	};
    };
    typedef uint32_t FibMsgSet;

    /**
     * Parse information about forwarding entry information received from
     * the underlying system.
     * 
     * The information to parse is in RTM format
     * (e.g., obtained by routing sockets or by sysctl(3) mechanism).
     * 
     * @param iftree the interface tree to use.
     * @param fte the Fte storage to store the parsed information.
     * @param buffer the buffer with the data to parse.
     * @param filter the set of messages that caller is interested in.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     * @see FteX.
     */
    static int parse_buffer_routing_socket(const IfTree& iftree, FteX& fte,
					   const vector<uint8_t>& buffer,
					   FibMsgSet filter);
 
   /** Routing table ID that we are interested in might have changed.
     */
    virtual int notify_table_id_change(uint32_t new_tbl) {
	// Virtual routing tables not implemented on BSD
	UNUSED(new_tbl);
	return XORP_OK;
    }

private:
    /**
     * Lookup a route by destination address.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int lookup_route_by_dest(const IPvX& dst, FteX& fte);

    /**
     * Lookup a route by network address.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int lookup_route_by_network(const IPvXNet& dst, FteX& fte);

    RoutingSocketReader _rs_reader;
};

#endif // __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_ENTRY_GET_ROUTING_SOCKET_HH__
