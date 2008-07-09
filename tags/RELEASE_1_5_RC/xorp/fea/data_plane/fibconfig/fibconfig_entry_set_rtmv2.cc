// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/fea/data_plane/fibconfig/fibconfig_entry_set_rtmv2.cc,v 1.18 2008/01/04 03:16:00 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HOST_OS_WINDOWS
#include "fea/data_plane/control_socket/windows_routing_socket.h"
#endif
#ifdef HAVE_IPHLPAPI_H
#include <iphlpapi.h>
#endif
#ifdef HAVE_ROUTPROT_H
#include <routprot.h>
#endif

#include "fea/fibconfig.hh"
#include "fea/fibconfig_entry_set.hh"
#ifdef HOST_OS_WINDOWS
#include "fea/data_plane/control_socket/windows_rtm_pipe.hh"
#include "fea/data_plane/control_socket/windows_rras_support.hh"
#endif

#include "fibconfig_entry_set_rtmv2.hh"


//
// Set single-entry information into the unicast forwarding table.
//
// The mechanism to set the information is Router Manager V2.
//

#ifdef HOST_OS_WINDOWS

FibConfigEntrySetRtmV2::FibConfigEntrySetRtmV2(FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigEntrySet(fea_data_plane_manager),
      _rs4(NULL),
      _rs6(NULL)
{
    if (!WinSupport::is_rras_running()) {
	XLOG_WARNING("RRAS is not running; disabling FibConfigEntrySetRtmV2.");
        return;
    }

    _rs4 = new WinRtmPipe(fea_data_plane_manager.eventloop());

#ifdef HAVE_IPV6
    _rs6 = new WinRtmPipe(fea_data_plane_manager.eventloop());
#endif
}

FibConfigEntrySetRtmV2::~FibConfigEntrySetRtmV2()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Router Manager V2 mechanism to set "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
#ifdef HAVE_IPV6
    if (_rs6 != NULL)
        delete _rs6;
#endif
    if (_rs4 != NULL)
        delete _rs4;
}

int
FibConfigEntrySetRtmV2::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (_rs4 == NULL || (_rs4->start(AF_INET, error_msg) != XORP_OK))
        return (XORP_ERROR);
#if 0
#ifdef HAVE_IPV6
    if (_rs6 == NULL || (_rs6->start(AF_INET6, error_msg) != XORP_OK))
        return (XORP_ERROR);
#endif
#endif
    _is_running = true;

    return (XORP_OK);
}

int
FibConfigEntrySetRtmV2::stop(string& error_msg)
{
    int result;

    if (! _is_running)
	return (XORP_OK);

    if (_rs4 == NULL || (_rs4->stop(error_msg) != XORP_OK))
        result = XORP_ERROR;
#if 0
#ifdef HAVE_IPV6
    if (rs6 == NULL || (_rs6->stop(error_msg) != XORP_OK))
        result = XORP_ERROR;
#endif
#endif

    _is_running = false;

    return (XORP_OK);
}

int
FibConfigEntrySetRtmV2::add_entry4(const Fte4& fte)
{
    FteX ftex(fte);
    
    return (add_entry(ftex));
}

int
FibConfigEntrySetRtmV2::delete_entry4(const Fte4& fte)
{
    FteX ftex(fte);
    
    return (delete_entry(ftex));
}

int
FibConfigEntrySetRtmV2::add_entry6(const Fte6& fte)
{
    FteX ftex(fte);
    
    return (add_entry(ftex));
}

int
FibConfigEntrySetRtmV2::delete_entry6(const Fte6& fte)
{
    FteX ftex(fte);
    
    return (delete_entry(ftex));
}

int
FibConfigEntrySetRtmV2::add_entry(const FteX& fte)
{
    static const size_t	buffer_size = sizeof(struct rt_msghdr)
	+ (sizeof(struct sockaddr_storage) * 3);
    union {
	uint8_t		data[buffer_size];
	struct rt_msghdr rtm;
    } buffer;
    struct rt_msghdr*	rtm = &buffer.rtm;
    struct sockaddr_storage* ss;
    struct sockaddr_in*	sin_dst = NULL;
    struct sockaddr_in*	sin_netmask = NULL;
    struct sockaddr_in*	sin_nexthop = NULL;
    int			family = fte.net().af();
    IPvX		fte_nexthop = fte.nexthop();
    int			result;

    debug_msg("add_entry (network = %s nexthop = %s)\n",
	      fte.net().str().c_str(), fte_nexthop.str().c_str());

    // Check that the family is supported
    do {
	if (fte_nexthop.is_ipv4()) {
	    if (! fea_data_plane_manager().have_ipv4())
		return (XORP_ERROR);
	    break;
	}
	if (fte_nexthop.is_ipv6()) {
	    if (! fea_data_plane_manager().have_ipv6())
		return (XORP_ERROR);
	    break;
	}
	break;
    } while (false);
    
    if (fte.is_connected_route())
	return (XORP_OK);  // XXX: don't add/remove directly-connected routes

#if 0
    if (! fte.ifname().empty())
	is_interface_route = true;
#endif

#if 0
    do {
	//
	// Check for a discard or unreachable route.
	// The referenced ifname must have respectively the discard or
	// unreachable property. The next-hop is forcibly rewritten to be the
	// loopback address, in order for the RTF_BLACKHOLE or RTF_REJECT
	// flag to take effect on BSD platforms.
	//
	if (fte.ifname().empty())
	    break;
	const IfTree& iftree = fibconfig().merged_config_iftree();
	const IfTreeInterface* ifp = iftree.find_interface(fte.ifname());
	if (ifp == NULL) {
	    XLOG_ERROR("Invalid interface name: %s", fte.ifname().c_str());
	    return (XORP_ERROR);
	}
	if (ifp->discard()) {
	    is_discard_route = true;
	    fte_nexthop = IPvX::LOOPBACK(family);
	}
	if (ifp->unreachable()) {
	    is_unreachable_route = true;
	    fte_nexthop = IPvX::LOOPBACK(family);
	}
	break;
    } while (false);
#endif

    //
    // Set the request
    //
    memset(&buffer, 0, sizeof(buffer));
    rtm->rtm_msglen = sizeof(*rtm);
    
    if (family != AF_INET && family != AF_INET6)
	XLOG_UNREACHABLE();

    ss = (struct sockaddr_storage *)(rtm + 1);
    sin_dst = (struct sockaddr_in *)(ss);
    sin_nexthop = (struct sockaddr_in *)(ss + 1);
    sin_netmask = (struct sockaddr_in *)(ss + 2);
    rtm->rtm_msglen += (3 * sizeof(struct sockaddr_storage));

    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_type = RTM_ADD;
    rtm->rtm_addrs = (RTA_DST | RTA_GATEWAY | RTA_NETMASK);
    rtm->rtm_pid = ((family == AF_INET ? _rs4 : _rs6))->pid();
    rtm->rtm_seq = ((family == AF_INET ? _rs4 : _rs6))->seqno();

    // Copy the interface index.
    const IfTree& iftree = fibconfig().merged_config_iftree();
    const IfTreeVif* vifp = iftree.find_vif(fte.ifname(), fte.vifname());
    XLOG_ASSERT(vifp != NULL);
    rtm->rtm_index = vifp->pif_index();

    // Copy the destination, the nexthop, and the netmask addresses
    fte.net().masked_addr().copy_out(*sin_dst);
    if (sin_nexthop != NULL) {
	fte_nexthop.copy_out(*sin_nexthop);
    }
    fte.net().netmask().copy_out(*sin_netmask);

    result = ((family == AF_INET ? _rs4 : _rs6))->write(rtm, rtm->rtm_msglen);
    if (result != rtm->rtm_msglen) {
	XLOG_ERROR("Error writing to Rtmv2 pipe: %s", strerror(errno));
	return (XORP_ERROR);
    }
    
    //
    // TODO: here we should check the routing socket output whether the write
    // succeeded.
    //
    
    return (XORP_OK);
}

int
FibConfigEntrySetRtmV2::delete_entry(const FteX& fte)
{
    static const size_t	buffer_size = sizeof(struct rt_msghdr)
	+ (sizeof(struct sockaddr_storage) * 2);
    union {
	uint8_t		data[buffer_size];
	struct rt_msghdr rtm;
    } buffer;
    struct rt_msghdr*	rtm = &buffer.rtm;
    struct sockaddr_storage* ss;
    struct sockaddr_in*	sin_dst = NULL;
    struct sockaddr_in*	sin_netmask = NULL;
    int			family = fte.net().af();
    int			result;

    debug_msg("delete_entry (network = %s nexthop = %s)\n",
	      fte.net().str().c_str(), fte.nexthop().str().c_str());

    // Check that the family is supported
    do {
	if (fte.nexthop().is_ipv4()) {
	    if (! fea_data_plane_manager().have_ipv4())
		return (XORP_ERROR);
	    break;
	}
	if (fte.nexthop().is_ipv6()) {
	    if (! fea_data_plane_manager().have_ipv6())
		return (XORP_ERROR);
	    break;
	}
	break;
    } while (false);

    if (fte.is_connected_route())
	return (XORP_OK);  // XXX: don't add/remove directly-connected routes
    
    //
    // Set the request
    //
    memset(&buffer, 0, sizeof(buffer));
    rtm->rtm_msglen = sizeof(*rtm);

    if (family != AF_INET && family != AF_INET6)
	XLOG_UNREACHABLE();
    
    ss = (struct sockaddr_storage *)(rtm + 1);
    sin_dst = (struct sockaddr_in *)(ss);
    sin_netmask = (struct sockaddr_in *)(ss + 1);
    rtm->rtm_msglen += (2 * sizeof(struct sockaddr_storage));

    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_type = RTM_DELETE;
    rtm->rtm_addrs = RTA_DST;
    rtm->rtm_addrs |= RTA_NETMASK;
    rtm->rtm_flags = 0;
    rtm->rtm_pid = ((family == AF_INET ? _rs4 : _rs6))->pid();
    rtm->rtm_seq = ((family == AF_INET ? _rs4 : _rs6))->seqno();

    // Copy the interface index.
    const IfTree& iftree = fibconfig().merged_config_iftree();
    const IfTreeVif* vifp = iftree.find_vif(fte.ifname(), fte.vifname());
    XLOG_ASSERT(vifp != NULL);
    rtm->rtm_index = vifp->pif_index();
    
    // Copy the destination, and the netmask addresses (if needed)
    fte.net().masked_addr().copy_out(*sin_dst);
    fte.net().netmask().copy_out(*sin_netmask);
    
    result = ((family == AF_INET ? _rs4 : _rs6))->write(rtm, rtm->rtm_msglen);
    if (result != rtm->rtm_msglen) {
	XLOG_ERROR("Error writing to Rtmv2 pipe: %s", strerror(errno));
	return (XORP_ERROR);
    }
    
    //
    // TODO: here we should check the routing socket output whether the write
    // succeeded.
    //
    
    return (XORP_OK);
}

#endif // HOST_OS_WINDOWS
