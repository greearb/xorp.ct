// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/fea/data_plane/fibconfig/fibconfig_entry_get_routing_socket.hh,v 1.5 2008/01/03 22:59:36 pavlin Exp $

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
     * Lookup a route by destination address.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int lookup_route_by_dest4(const IPv4& dst, Fte4& fte);

    /**
     * Lookup route by network address.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int lookup_route_by_network4(const IPv4Net& dst, Fte4& fte);

    /**
     * Lookup a route by destination address.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int lookup_route_by_dest6(const IPv6& dst, Fte6& fte);

    /**
     * Lookup route by network address.
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
     * Parse information about routing entry information received from
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
     * Lookup route by network address.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int lookup_route_by_network(const IPvXNet& dst, FteX& fte);

    RoutingSocketReader _rs_reader;
};

#endif // __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_ENTRY_GET_ROUTING_SOCKET_HH__
