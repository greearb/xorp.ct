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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_entry_get_routing_socket.cc,v 1.14 2008/01/04 03:15:57 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvxnet.hh"

#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif

#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif

#include "fea/fibconfig.hh"
#include "fea/fibconfig_entry_get.hh"

#include "fibconfig_entry_get_routing_socket.hh"


//
// Get single-entry information from the unicast forwarding table.
//
// The mechanism to obtain the information is routing sockets.
//

#ifdef HAVE_ROUTING_SOCKETS

FibConfigEntryGetRoutingSocket::FibConfigEntryGetRoutingSocket(FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigEntryGet(fea_data_plane_manager),
      RoutingSocket(fea_data_plane_manager.eventloop()),
      _rs_reader(*(RoutingSocket *)this)
{
}

FibConfigEntryGetRoutingSocket::~FibConfigEntryGetRoutingSocket()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the routing sockets mechanism to get "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigEntryGetRoutingSocket::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (RoutingSocket::start(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}

int
FibConfigEntryGetRoutingSocket::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (RoutingSocket::stop(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

int
FibConfigEntryGetRoutingSocket::lookup_route_by_dest4(const IPv4& dst,
						      Fte4& fte)
{
    FteX ftex(dst.af());
    int ret_value = XORP_ERROR;

    ret_value = lookup_route_by_dest(IPvX(dst), ftex);
    
    fte = ftex.get_fte4();
    
    return (ret_value);
}

int
FibConfigEntryGetRoutingSocket::lookup_route_by_network4(const IPv4Net& dst,
							 Fte4& fte)
{
    FteX ftex(dst.af());
    int ret_value = XORP_ERROR;

    ret_value = lookup_route_by_network(IPvXNet(dst), ftex);
    
    fte = ftex.get_fte4();
    
    return (ret_value);
}

int
FibConfigEntryGetRoutingSocket::lookup_route_by_dest6(const IPv6& dst,
						      Fte6& fte)
{
    FteX ftex(dst.af());
    int ret_value = XORP_ERROR;

    ret_value = lookup_route_by_dest(IPvX(dst), ftex);
    
    fte = ftex.get_fte6();
    
    return (ret_value);
}

int
FibConfigEntryGetRoutingSocket::lookup_route_by_network6(const IPv6Net& dst,
							 Fte6& fte)
{ 
    FteX ftex(dst.af());
    int ret_value = XORP_ERROR;

    ret_value = lookup_route_by_network(IPvXNet(dst), ftex);
    
    fte = ftex.get_fte6();
    
    return (ret_value);
}

int
FibConfigEntryGetRoutingSocket::lookup_route_by_dest(const IPvX& dst,
						     FteX& fte)
{
    static const size_t	buffer_size = sizeof(struct rt_msghdr) + 512;
    union {
	uint8_t		data[buffer_size];
	struct rt_msghdr rtm;
    } buffer;
    struct rt_msghdr*	rtm = &buffer.rtm;
    struct sockaddr_in*	sin;
    RoutingSocket&	rs = *this;
    
    // Zero the return information
    fte.zero();

    // Check that the family is supported
    do {
	if (dst.is_ipv4()) {
	    if (! fea_data_plane_manager().have_ipv4())
		return (XORP_ERROR);
	    break;
	}
	if (dst.is_ipv6()) {
	    if (! fea_data_plane_manager().have_ipv6())
		return (XORP_ERROR);
	    break;
	}
	break;
    } while (false);
    
    // Check that the destination address is valid
    if (! dst.is_unicast()) {
	return (XORP_ERROR);
    }
    
    //
    // Set the request
    //
    memset(&buffer, 0, sizeof(buffer));
    switch (dst.af()) {
    case AF_INET:
	rtm->rtm_msglen = sizeof(*rtm) + sizeof(struct sockaddr_in);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	rtm->rtm_msglen = sizeof(*rtm) + sizeof(struct sockaddr_in6);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_type = RTM_GET;
    rtm->rtm_addrs = (RTA_DST | RTA_IFP);
    rtm->rtm_flags = RTF_UP;
    rtm->rtm_pid = rs.pid();
    rtm->rtm_seq = rs.seqno();
    
    // Copy the destination address
    sin = reinterpret_cast<struct sockaddr_in*>(rtm + 1);
    dst.copy_out(*sin);

    //
    // Add extra space for sockaddr_dl that corresponds to the RTA_IFP flag.
    // Required if the OS is very strict in the arguments checking
    // (e.g., NetBSD).
    //
#ifdef AF_LINK
    do {
	// Set the data-link socket
	struct sockaddr_dl* sdl;

	rtm->rtm_msglen += sizeof(struct sockaddr_dl);
	switch (dst.af()) {
	case AF_INET:
	    sdl = ADD_POINTER(sin, sizeof(struct sockaddr_in),
			      struct sockaddr_dl*);
	    break;
#ifdef HAVE_IPV6
	case AF_INET6:
	    sdl = ADD_POINTER(sin, sizeof(struct sockaddr_in6),
			      struct sockaddr_dl*);
	    break;
#endif // HAVE_IPV6
	default:
	    XLOG_UNREACHABLE();
	    break;
	}
	sdl->sdl_family = AF_LINK;
#ifdef HAVE_STRUCT_SOCKADDR_DL_SDL_LEN
	sdl->sdl_len = sizeof(struct sockaddr_dl);
#endif
    } while (false);
#endif // AF_LINK

    if (rs.write(rtm, rtm->rtm_msglen) != rtm->rtm_msglen) {
	XLOG_ERROR("Error writing to routing socket: %s", strerror(errno));
	return (XORP_ERROR);
    }

    //
    // Force to receive data from the kernel, and then parse it
    //
    string error_msg;
    if (_rs_reader.receive_data(rs, rtm->rtm_seq, error_msg) != XORP_OK) {
	XLOG_ERROR("Error reading from routing socket: %s", error_msg.c_str());
	return (XORP_ERROR);
    }
    if (parse_buffer_routing_socket(fibconfig().system_config_iftree(), fte,
				    _rs_reader.buffer(), FibMsg::GETS)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfigEntryGetRoutingSocket::lookup_route_by_network(const IPvXNet& dst,
							FteX& fte)
{
    static const size_t	buffer_size = sizeof(struct rt_msghdr) + 512;
    union {
	uint8_t		data[buffer_size];
	struct rt_msghdr rtm;
    } buffer;
    struct rt_msghdr*	rtm = &buffer.rtm;
    struct sockaddr_in*	sin;
    RoutingSocket&	rs = *this;
    
    // Zero the return information
    fte.zero();

    // Check that the family is supported
    do {
	if (dst.is_ipv4()) {
	    if (! fea_data_plane_manager().have_ipv4())
		return (XORP_ERROR);
	    break;
	}
	if (dst.is_ipv6()) {
	    if (! fea_data_plane_manager().have_ipv6())
		return (XORP_ERROR);
	    break;
	}
	break;
    } while (false);

    // Check that the destination address is valid    
    if (! dst.is_unicast()) {
	return (XORP_ERROR);
    }

    //
    // Set the request
    //
    memset(&buffer, 0, sizeof(buffer));
    switch (dst.af()) {
    case AF_INET:
	rtm->rtm_msglen = sizeof(*rtm) + 2 * sizeof(struct sockaddr_in);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	rtm->rtm_msglen = sizeof(*rtm) + 2 * sizeof(struct sockaddr_in6);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_type = RTM_GET;
    rtm->rtm_addrs = (RTA_DST | RTA_NETMASK | RTA_IFP);
    rtm->rtm_flags = RTF_UP;
    rtm->rtm_pid = rs.pid();
    rtm->rtm_seq = rs.seqno();

    // Copy the destination address    
    sin = reinterpret_cast<struct sockaddr_in*>(rtm + 1);
    dst.masked_addr().copy_out(*sin);
    
    // Copy the network mask
    switch (dst.af()) {
    case AF_INET:
	sin = ADD_POINTER(sin, sizeof(struct sockaddr_in),
			  struct sockaddr_in*);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	sin = ADD_POINTER(sin, sizeof(struct sockaddr_in6),
			  struct sockaddr_in*);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
    IPvX netmask = IPvX::make_prefix(dst.af(), dst.prefix_len());
    netmask.copy_out(*sin);

    //
    // Add extra space for sockaddr_dl that corresponds to the RTA_IFP flag.
    // Required if the OS is very strict in the arguments checking
    // (e.g., NetBSD).
    //
#ifdef AF_LINK
    do {
	// Set the data-link socket
	struct sockaddr_dl* sdl;

	rtm->rtm_msglen += sizeof(struct sockaddr_dl);
	switch (dst.af()) {
	case AF_INET:
	    sdl = ADD_POINTER(sin, sizeof(struct sockaddr_in),
			      struct sockaddr_dl*);
	    break;
#ifdef HAVE_IPV6
	case AF_INET6:
	    sdl = ADD_POINTER(sin, sizeof(struct sockaddr_in6),
			      struct sockaddr_dl*);
	    break;
#endif // HAVE_IPV6
	default:
	    XLOG_UNREACHABLE();
	    break;
	}
	sdl->sdl_family = AF_LINK;
#ifdef HAVE_STRUCT_SOCKADDR_DL_SDL_LEN
	sdl->sdl_len = sizeof(struct sockaddr_dl);
#endif
    } while (false);
#endif // AF_LINK

    if (rs.write(rtm, rtm->rtm_msglen) != rtm->rtm_msglen) {
	XLOG_ERROR("Error writing to routing socket: %s", strerror(errno));
	return (XORP_ERROR);
    }
    
    //
    // Force to receive data from the kernel, and then parse it
    //
    string error_msg;
    if (_rs_reader.receive_data(rs, rtm->rtm_seq, error_msg) != XORP_OK) {
	XLOG_ERROR("Error reading from routing socket: %s", error_msg.c_str());
	return (XORP_ERROR);
    }
    if (parse_buffer_routing_socket(fibconfig().system_config_iftree(), fte,
				    _rs_reader.buffer(), FibMsg::GETS)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

#endif // HAVE_ROUTING_SOCKETS
