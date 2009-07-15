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



#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "fea/fibconfig.hh"
#include "fea/data_plane/control_socket/netlink_socket_utilities.hh"

#include "fibconfig_entry_set_netlink_socket.hh"


//
// Set single-entry information into the unicast forwarding table.
//
// The mechanism to set the information is netlink(7) sockets.
//

#ifdef HAVE_NETLINK_SOCKETS

FibConfigEntrySetNetlinkSocket::FibConfigEntrySetNetlinkSocket(FeaDataPlaneManager& fea_data_plane_manager)
    : FibConfigEntrySet(fea_data_plane_manager),
      NetlinkSocket(fea_data_plane_manager.eventloop()),
      _ns_reader(*(NetlinkSocket *)this)
{
}

FibConfigEntrySetNetlinkSocket::~FibConfigEntrySetNetlinkSocket()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the netlink(7) sockets mechanism to set "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigEntrySetNetlinkSocket::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (NetlinkSocket::start(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    _is_running = true;

    return (XORP_OK);
}

int
FibConfigEntrySetNetlinkSocket::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (NetlinkSocket::stop(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

int
FibConfigEntrySetNetlinkSocket::add_entry4(const Fte4& fte)
{
    FteX ftex(fte);
    
    return (add_entry(ftex));
}

int
FibConfigEntrySetNetlinkSocket::delete_entry4(const Fte4& fte)
{
    FteX ftex(fte);
    
    return (delete_entry(ftex));
}

int
FibConfigEntrySetNetlinkSocket::add_entry6(const Fte6& fte)
{
    FteX ftex(fte);
    
    return (add_entry(ftex));
}

int
FibConfigEntrySetNetlinkSocket::delete_entry6(const Fte6& fte)
{
    FteX ftex(fte);
    
    return (delete_entry(ftex));
}

int
FibConfigEntrySetNetlinkSocket::add_entry(const FteX& fte)
{
    static const size_t	buffer_size = sizeof(struct rtmsg)
	+ 3*sizeof(struct rtattr) + sizeof(int) + 512;
    union {
	uint8_t		data[buffer_size];
	struct nlmsghdr	nlh;
    } buffer;
    struct nlmsghdr*	nlh = &buffer.nlh;
    struct sockaddr_nl	snl;
    struct rtmsg*	rtmsg;
    struct rtattr*	rtattr;
    int			rta_len;
    uint8_t*		data;
    NetlinkSocket&	ns = *this;
    int			family = fte.net().af();
    uint32_t		if_index = 0;
    void*		rta_align_data;
    uint32_t		table_id = RT_TABLE_MAIN;	// Default value

    debug_msg("add_entry "
	      "(network = %s nexthop = %s)",
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

    memset(&buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    //
    // Set the request
    //
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*rtmsg));
    nlh->nlmsg_type = RTM_NEWROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.nl_pid();
    rtmsg = static_cast<struct rtmsg*>(NLMSG_DATA(nlh));
    rtmsg->rtm_family = family;
    rtmsg->rtm_dst_len = fte.net().prefix_len(); // The destination mask length
    rtmsg->rtm_src_len = 0;
    rtmsg->rtm_tos = 0;
    rtmsg->rtm_protocol = RTPROT_XORP;		// Mark this as a XORP route
    rtmsg->rtm_scope = RT_SCOPE_UNIVERSE;
    rtmsg->rtm_type = RTN_UNICAST;
    rtmsg->rtm_flags = RTM_F_NOTIFY;

    //
    // Set the routing/forwarding table ID.
    // If the table ID is <= 0xff, then we set it in rtmsg->rtm_table,
    // otherwise we set rtmsg->rtm_table to RT_TABLE_UNSPEC and add the
    // real value as an RTA_TABLE attribute.
    //
    if (fibconfig().unicast_forwarding_table_id_is_configured(family))
	table_id = fibconfig().unicast_forwarding_table_id(family);
    if (table_id <= 0xff)
	rtmsg->rtm_table = table_id;
    else
	rtmsg->rtm_table = RT_TABLE_UNSPEC;

    // Add the destination address as an attribute
    rta_len = RTA_LENGTH(fte.net().masked_addr().addr_bytelen());
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		   XORP_UINT_CAST(sizeof(buffer)),
		   XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
    }
    rtattr = RTM_RTA(rtmsg);
    rtattr->rta_type = RTA_DST;
    rtattr->rta_len = rta_len;
    data = static_cast<uint8_t*>(RTA_DATA(rtattr));
    fte.net().masked_addr().copy_out(data);
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    // Add the nexthop address as an attribute
    if (fte.nexthop() != IPvX::ZERO(family)) {
	rta_len = RTA_LENGTH(fte.nexthop().addr_bytelen());
	if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	    XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		       XORP_UINT_CAST(sizeof(buffer)),
		       XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
	}
	rta_align_data = reinterpret_cast<char*>(rtattr)
	    + RTA_ALIGN(rtattr->rta_len);
	rtattr = static_cast<struct rtattr*>(rta_align_data);
	rtattr->rta_type = RTA_GATEWAY;
	rtattr->rta_len = rta_len;
	data = static_cast<uint8_t*>(RTA_DATA(rtattr));
	fte.nexthop().copy_out(data);
	nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;
    }

    // Get the interface index, if it exists
    do {
	//
	// Check for discard and unreachable routes.
	// The referenced ifname must have respectively the discard or the
	// unreachable property. These use a separate route type in netlink
	// land. Unlike BSD, it need not reference a loopback interface.
	// Because this interface exists only in the FEA, it has no index.
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
	    rtmsg->rtm_type = RTN_BLACKHOLE;
	    break;
	}
	if (ifp->unreachable()) {
	    rtmsg->rtm_type = RTN_UNREACHABLE;
	    break;
	}

	const IfTreeVif* vifp = ifp->find_vif(fte.vifname());
	if (vifp == NULL) {
	    XLOG_ERROR("Invalid interface name %s vif name: %s",
		       fte.ifname().c_str(), fte.vifname().c_str());
	    return (XORP_ERROR);
	}
	if_index = vifp->pif_index();
	if (if_index != 0)
	    break;

	XLOG_FATAL("Could not find interface index for interface %s vif %s",
		   fte.ifname().c_str(), fte.vifname().c_str());
	break;
    } while (false);

    //
    // If the interface has an index in the host stack, add it
    // as an attribute.
    //
    if (if_index != 0) {
	int int_if_index = if_index;
	rta_len = RTA_LENGTH(sizeof(int_if_index));
	if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	    XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		       XORP_UINT_CAST(sizeof(buffer)),
		       XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
	}
	rta_align_data = reinterpret_cast<char*>(rtattr)
	    + RTA_ALIGN(rtattr->rta_len);
	rtattr = static_cast<struct rtattr*>(rta_align_data);
	rtattr->rta_type = RTA_OIF;
	rtattr->rta_len = rta_len;
	data = static_cast<uint8_t*>(RTA_DATA(rtattr));
	memcpy(data, &int_if_index, sizeof(int_if_index));
	nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;
    }

    // Add the route priority as an attribute
    int int_priority = fte.metric();
    rta_len = RTA_LENGTH(sizeof(int_priority));
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		   XORP_UINT_CAST(sizeof(buffer)),
		   XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
    }
    rta_align_data = reinterpret_cast<char*>(rtattr)
	+ RTA_ALIGN(rtattr->rta_len);
    rtattr = static_cast<struct rtattr*>(rta_align_data);
    rtattr->rta_type = RTA_PRIORITY;
    rtattr->rta_len = rta_len;
    data = static_cast<uint8_t*>(RTA_DATA(rtattr));
    memcpy(data, &int_priority, sizeof(int_priority));
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

#ifdef HAVE_NETLINK_SOCKET_ATTRIBUTE_RTA_TABLE
    // Add the table ID as an attribute
    if (table_id > 0xff) {
	uint32_t uint32_table_id = table_id;
	rta_len = RTA_LENGTH(sizeof(uint32_table_id));
	if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	    XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		       XORP_UINT_CAST(sizeof(buffer)),
		       XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
	}
	rta_align_data = reinterpret_cast<char*>(rtattr)
	    + RTA_ALIGN(rtattr->rta_len);
	rtattr = static_cast<struct rtattr*>(rta_align_data);
	rtattr->rta_type = RTA_TABLE;
	rtattr->rta_len = rta_len;
	data = static_cast<uint8_t*>(RTA_DATA(rtattr));
	memcpy(data, &uint32_table_id, sizeof(uint32_table_id));
	nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;
    }
#endif // HAVE_NETLINK_SOCKET_ATTRIBUTE_RTA_TABLE

    //
    // XXX: the Linux kernel doesn't keep the admin distance, hence
    // we don't add it.
    //
    
    string error_msg;
    int last_errno = 0;
    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
		  reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	XLOG_ERROR("Error writing to netlink socket: %s", strerror(errno));
	return (XORP_ERROR);
    }
    if (NlmUtils::check_netlink_request(_ns_reader, ns, nlh->nlmsg_seq,
					last_errno, error_msg)
	!= XORP_OK) {
	XLOG_ERROR("Error checking netlink request: %s", error_msg.c_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfigEntrySetNetlinkSocket::delete_entry(const FteX& fte)
{
    static const size_t	buffer_size = sizeof(struct rtmsg)
	+ 3*sizeof(struct rtattr) + sizeof(int) + 512;
    union {
	uint8_t		data[buffer_size];
	struct nlmsghdr	nlh;
    } buffer;
    struct nlmsghdr*	nlh = &buffer.nlh;
    struct sockaddr_nl	snl;
    struct rtmsg*	rtmsg;
    struct rtattr*	rtattr;
    int			rta_len;
    uint8_t*		data;
    NetlinkSocket&	ns = *this;
    int			family = fte.net().af();
    void*		rta_align_data;
    uint32_t		table_id = RT_TABLE_MAIN;	// Default value

    UNUSED(rta_align_data);

    debug_msg("delete_entry "
	      "(network = %s nexthop = %s)",
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

    memset(&buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    //
    // Set the request
    //
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*rtmsg));
    nlh->nlmsg_type = RTM_DELROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.nl_pid();
    rtmsg = static_cast<struct rtmsg*>(NLMSG_DATA(nlh));
    rtmsg->rtm_family = family;
    rtmsg->rtm_dst_len = fte.net().prefix_len(); // The destination mask length
    rtmsg->rtm_src_len = 0;
    rtmsg->rtm_tos = 0;
    rtmsg->rtm_protocol = RTPROT_XORP;		// Mark this as a XORP route
    rtmsg->rtm_scope = RT_SCOPE_UNIVERSE;
    rtmsg->rtm_type = RTN_UNICAST;
    rtmsg->rtm_flags = RTM_F_NOTIFY;

    //
    // Set the routing/forwarding table ID.
    // If the table ID is <= 0xff, then we set it in rtmsg->rtm_table,
    // otherwise we set rtmsg->rtm_table to RT_TABLE_UNSPEC and add the
    // real value as an RTA_TABLE attribute.
    //
    if (fibconfig().unicast_forwarding_table_id_is_configured(family))
	table_id = fibconfig().unicast_forwarding_table_id(family);
    if (table_id <= 0xff)
	rtmsg->rtm_table = table_id;
    else
	rtmsg->rtm_table = RT_TABLE_UNSPEC;

    // Add the destination address as an attribute
    rta_len = RTA_LENGTH(fte.net().masked_addr().addr_bytelen());
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		   XORP_UINT_CAST(sizeof(buffer)),
		   XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
    }
    rtattr = RTM_RTA(rtmsg);
    rtattr->rta_type = RTA_DST;
    rtattr->rta_len = rta_len;
    data = static_cast<uint8_t*>(RTA_DATA(rtattr));
    fte.net().masked_addr().copy_out(data);
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

#ifdef HAVE_NETLINK_SOCKET_ATTRIBUTE_RTA_TABLE
    // Add the table ID as an attribute
    if (table_id > 0xff) {
	uint32_t uint32_table_id = table_id;
	rta_len = RTA_LENGTH(sizeof(uint32_table_id));
	if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	    XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		       XORP_UINT_CAST(sizeof(buffer)),
		       XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
	}
	rta_align_data = reinterpret_cast<char*>(rtattr)
	    + RTA_ALIGN(rtattr->rta_len);
	rtattr = static_cast<struct rtattr*>(rta_align_data);
	rtattr->rta_type = RTA_TABLE;
	rtattr->rta_len = rta_len;
	data = static_cast<uint8_t*>(RTA_DATA(rtattr));
	memcpy(data, &uint32_table_id, sizeof(uint32_table_id));
	nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;
    }
#endif // HAVE_NETLINK_SOCKET_ATTRIBUTE_RTA_TABLE

    do {
	//
	// When deleting a route which points to a discard or unreachable
	// interface, pass the correct route type to the kernel.
	//
	if (fte.ifname().empty())
	    break;
	const IfTree& iftree = fibconfig().merged_config_iftree();
	const IfTreeInterface* ifp = iftree.find_interface(fte.ifname());
	//
	// XXX: unlike adding a route, we don't use XLOG_ASSERT()
	// to check whether the interface is configured in the system.
	// E.g., on startup we may attempt to clean-up leftover XORP
	// routes while we still don't have any interface tree configuration.
	//

	if (ifp != NULL) {
	    if (ifp->discard()) {
		rtmsg->rtm_type = RTN_BLACKHOLE;
		break;
	    }
	    if (ifp->unreachable()) {
		rtmsg->rtm_type = RTN_UNREACHABLE;
		break;
	    }
	}

	break;
    } while (false);

    int last_errno = 0;
    string error_msg;
    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
		  reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	XLOG_ERROR("Error writing to netlink socket: %s", strerror(errno));
	return (XORP_ERROR);
    }
    if (NlmUtils::check_netlink_request(_ns_reader, ns, nlh->nlmsg_seq,
					last_errno, error_msg)
	!= XORP_OK) {
	//
	// XXX: If the outgoing interface was taken down earlier, then
	// most likely the kernel has removed the matching forwarding
	// entries on its own. Hence, check whether all of the following
	// is true:
	//   - the error code matches
	//   - the outgoing interface is down
	//
	// If all conditions are true, then ignore the error and consider
	// the deletion was success.
	// Note that we could add to the following list the check whether
	// the forwarding entry is not in the kernel, but this is probably
	// an overkill. If such check should be performed, we should
	// use the corresponding FibConfigTableGetNetlink plugin.
	//
	do {
	    // Check whether the error code matches
	    if (last_errno != ESRCH) {
		//
		// XXX: The "No such process" error code is used by the
		// kernel to indicate there is no such forwarding entry
		// to delete.
		//
		break;
	    }

	    // Check whether the interface is down
	    if (fte.ifname().empty())
		break;		// No interface to check
	    const IfTree& iftree = fibconfig().system_config_iftree();
	    const IfTreeVif* vifp = iftree.find_vif(fte.ifname(),
						    fte.vifname());
	    if ((vifp != NULL) && vifp->enabled())
		break;		// The interface is UP

	    return (XORP_OK);
	} while (false);

	XLOG_ERROR("Error checking netlink request: %s", error_msg.c_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

#endif // HAVE_NETLINK_SOCKETS
