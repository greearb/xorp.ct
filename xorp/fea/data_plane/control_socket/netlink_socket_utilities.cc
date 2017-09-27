// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2012 XORP, Inc and Others
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

#include <xorp_config.h>
#ifdef HAVE_NETLINK_SOCKETS

// Uncomment this to have debug_msg() active in this file.
//#define DEBUG_LOGGING

#include "fea/fea_module.h"
#include "fea/fibconfig.hh"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "libproto/packet.hh"

#include "netlink_socket.hh"
#include "netlink_socket_utilities.hh"
#include "system_utilities.hh"
#include "../ifconfig/ifconfig_media.hh"


//
// NETLINK format related utilities for manipulating data
// (e.g., obtained by netlink(7) sockets mechanism).
//


string NlmUtils::nlm_print_msg(vector<uint8_t>& buffer) {
    ostringstream oss;

    size_t buffer_bytes = buffer.size();
    struct nlmsghdr* nlh;

    for (nlh = (struct nlmsghdr*)(&buffer[0]);
	 NLMSG_OK(nlh, buffer_bytes);
	 nlh = NLMSG_NEXT(nlh, buffer_bytes)) {
	void* nlmsg_data = NLMSG_DATA(nlh);

	oss << "type:  " << NlmUtils::nlm_msg_type(nlh->nlmsg_type);

	// Decode a few further.
	switch (nlh->nlmsg_type) {
	case NLMSG_ERROR: {
	    const struct nlmsgerr* err;

	    err = reinterpret_cast<const struct nlmsgerr*>(nlmsg_data);
	    if (nlh->nlmsg_len < NLMSG_LENGTH(sizeof(*err))) {
		XLOG_ERROR("AF_NETLINK nlmsgerr length error");
		break;
	    }
	    errno = -err->error;
	    oss << "  AF_NETLINK NLMSG_ERROR message: " << strerror(errno) << endl;
	    break;
	}

	case RTM_NEWROUTE:
	case RTM_DELROUTE:
	case RTM_GETROUTE: {

	    struct rtmsg* rtmsg;
	    int rta_len = RTM_PAYLOAD(nlh);

	    if (rta_len < 0) {
		XLOG_ERROR("AF_NETLINK rtmsg length error");
		break;
	    }
	    rtmsg = (struct rtmsg*)nlmsg_data;

	    oss << " rtm_type: ";
	    if (rtmsg->rtm_type == RTN_MULTICAST)
		oss << "MULTICAST";
	    else if (rtmsg->rtm_type == RTN_BROADCAST)
		oss << "BROADCAST";
	    else if (rtmsg->rtm_type == RTN_UNICAST)
		oss << "UNICAST";
	    else
		oss << (int)(rtmsg->rtm_type);

	    struct rtattr *rtattr;
	    struct rtattr *rta_array[RTA_MAX + 1];
	    int family = rtmsg->rtm_family;

	    //
	    // Get the attributes
	    //
	    memset(rta_array, 0, sizeof(rta_array));
	    rtattr = RTM_RTA(rtmsg);
	    get_rtattr(rtattr, rta_len, rta_array,
		       sizeof(rta_array) / sizeof(rta_array[0]));

	    int rtmt = rtmsg->rtm_table;
#ifdef HAVE_NETLINK_SOCKET_ATTRIBUTE_RTA_TABLE
	    if (rta_array[RTA_TABLE] != NULL) {
		const uint8_t* p = (const uint8_t*)(RTA_DATA(rta_array[RTA_TABLE]));
		rtmt = extract_host_int(p);
	    }
#endif
	    oss << " table: " << rtmt;


	    IPvX nexthop_addr(family);
	    IPvX dst_addr(family);

	    if (rta_array[RTA_DST] == NULL) {
		// The default entry
	    } else {
		// TODO: fix this!!
		dst_addr.copy_in(family, (uint8_t *)RTA_DATA(const_cast<struct rtattr *>(rta_array[RTA_DST])));
		dst_addr = system_adjust_ipvx_recv(dst_addr);
		oss << " dest: " << dst_addr.str() << "/" << (int)(rtmsg->rtm_dst_len);
	    }


	    if (rta_array[RTA_GATEWAY] != NULL) {
		nexthop_addr.copy_in(family, (uint8_t *)RTA_DATA(const_cast<struct rtattr *>(rta_array[RTA_GATEWAY])));
		oss << " nexthop: " << nexthop_addr.str();
	    }


	    if (rtmsg->rtm_protocol == RTPROT_XORP)
		oss << " proto: XORP";
	    else
		oss << " proto: " << rtmsg->rtm_protocol;

	    // Get the interface index
	    if (rta_array[RTA_OIF] != NULL) {
		const uint8_t* p = static_cast<const uint8_t *>(
		    RTA_DATA(const_cast<struct rtattr *>(rta_array[RTA_OIF])));
		oss << " iface: " << extract_host_int(p);
	    }

	    if (rta_array[RTA_PRIORITY] != NULL) {
		const uint8_t* p = static_cast<const uint8_t *>(
		    RTA_DATA(const_cast<struct rtattr *>(rta_array[RTA_PRIORITY])));
		oss << " metric: " << extract_host_int(p);
	    }
	    oss << endl;
	    break;
	}

	default:
	    // ignore
	    oss << endl;
	}
    }
    return oss.str();
}//nlm_print_msg


/**
 * @param m message type from netlink socket message
 * @return human readable form.
 */
string
NlmUtils::nlm_msg_type(uint32_t m)
{
    struct {
	uint32_t 	value;
	const char*	name;
    } nlm_msg_types[] = {
#define RTM_MSG_ENTRY(X) { X, #X }
#ifdef NLMSG_ERROR
	RTM_MSG_ENTRY(NLMSG_ERROR),
#endif
#ifdef NLMSG_DONE
	RTM_MSG_ENTRY(NLMSG_DONE),
#endif
#ifdef NLMSG_NOOP
	RTM_MSG_ENTRY(NLMSG_NOOP),
#endif
#ifdef RTM_NEWLINK
	RTM_MSG_ENTRY(RTM_NEWLINK),
#endif
#ifdef RTM_DELLINK
	RTM_MSG_ENTRY(RTM_DELLINK),
#endif
#ifdef RTM_GETLINK
	RTM_MSG_ENTRY(RTM_GETLINK),
#endif
#ifdef RTM_NEWADDR
	RTM_MSG_ENTRY(RTM_NEWADDR),
#endif
#ifdef RTM_DELADDR
	RTM_MSG_ENTRY(RTM_DELADDR),
#endif
#ifdef RTM_GETADDR
	RTM_MSG_ENTRY(RTM_GETADDR),
#endif
#ifdef RTM_NEWROUTE
	RTM_MSG_ENTRY(RTM_NEWROUTE),
#endif
#ifdef RTM_DELROUTE
	RTM_MSG_ENTRY(RTM_DELROUTE),
#endif
#ifdef RTM_GETROUTE
	RTM_MSG_ENTRY(RTM_GETROUTE),
#endif
#ifdef RTM_NEWNEIGH
	RTM_MSG_ENTRY(RTM_NEWNEIGH),
#endif
#ifdef RTM_DELNEIGH
	RTM_MSG_ENTRY(RTM_DELNEIGH),
#endif
#ifdef RTM_GETNEIGH
	RTM_MSG_ENTRY(RTM_GETNEIGH),
#endif
#ifdef RTM_NEWRULE
	RTM_MSG_ENTRY(RTM_NEWRULE),
#endif
#ifdef RTM_DELRULE
	RTM_MSG_ENTRY(RTM_DELRULE),
#endif
#ifdef RTM_GETRULE
	RTM_MSG_ENTRY(RTM_GETRULE),
#endif
#ifdef RTM_NEWQDISC
	RTM_MSG_ENTRY(RTM_NEWQDISC),
#endif
#ifdef RTM_DELQDISC
	RTM_MSG_ENTRY(RTM_DELQDISC),
#endif
#ifdef RTM_GETQDISC
	RTM_MSG_ENTRY(RTM_GETQDISC),
#endif
#ifdef RTM_NEWTCLASS
	RTM_MSG_ENTRY(RTM_NEWTCLASS),
#endif
#ifdef RTM_DELTCLASS
	RTM_MSG_ENTRY(RTM_DELTCLASS),
#endif
#ifdef RTM_GETTCLASS
	RTM_MSG_ENTRY(RTM_GETTCLASS),
#endif
#ifdef RTM_NEWTFILTER
	RTM_MSG_ENTRY(RTM_NEWTFILTER),
#endif
#ifdef RTM_DELTFILTER
	RTM_MSG_ENTRY(RTM_DELTFILTER),
#endif
#ifdef RTM_GETTFILTER
	RTM_MSG_ENTRY(RTM_GETTFILTER),
#endif
#ifdef RTM_MAX
	RTM_MSG_ENTRY(RTM_MAX),
#endif
	{ ~0U, "Unknown" }
    };
    const size_t n_nlm_msgs = sizeof(nlm_msg_types) / sizeof(nlm_msg_types[0]);
    const char* ret = 0;
    for (size_t i = 0; i < n_nlm_msgs; i++) {
	ret = nlm_msg_types[i].name;
	if (nlm_msg_types[i].value == m)
	    break;
    }
    return ret;
}

void
NlmUtils::get_rtattr(struct rtattr* rtattr, int rta_len,
		     struct rtattr* rta_array[], size_t rta_array_n)
{
    while (RTA_OK(rtattr, rta_len)) {
	if (rtattr->rta_type < rta_array_n)
	    rta_array[rtattr->rta_type] = rtattr;
	rtattr = RTA_NEXT(rtattr, rta_len);
    }

    if (rta_len) {
	XLOG_WARNING("get_rtattr() failed: AF_NETLINK deficit in rtattr: "
		     "%d rta_len remaining",
		     rta_len);
    }
}

int
NlmUtils::nlm_get_to_fte_cfg(const IfTree& iftree, FteX& fte,
			     const struct nlmsghdr* nlh,
			     struct rtmsg* rtmsg, int rta_len, const FibConfig& fibconfig,
			     string& err_msg)
{
    struct rtattr *rtattr;
    struct rtattr *rta_array[RTA_MAX + 1];
    int if_index = 0;		// XXX: initialized with an invalid value
    bool lookup_ifindex = true;
    string if_name;
    string vif_name;
    int family = fte.nexthop().af();
    bool is_deleted = false;

    // Reset the result
    fte.zero();

    // Test if this entry was deleted
    if (nlh->nlmsg_type == RTM_DELROUTE)
	is_deleted = true;


    //
    // Get the attributes
    //
    memset(rta_array, 0, sizeof(rta_array));
    rtattr = RTM_RTA(rtmsg);
    get_rtattr(rtattr, rta_len, rta_array,
	       sizeof(rta_array) / sizeof(rta_array[0]));

    // Discard the route if we are using a specific table.
    if (fibconfig.unicast_forwarding_table_id_is_configured(family)) {
	int rttable = fibconfig.unicast_forwarding_table_id(family);
	int rtmt = rtmsg->rtm_table;
#ifdef HAVE_NETLINK_SOCKET_ATTRIBUTE_RTA_TABLE
	if (rta_array[RTA_TABLE] != NULL) {
	    const uint8_t* p = (const uint8_t*)(RTA_DATA(rta_array[RTA_TABLE]));
	    rtmt = extract_host_int(p);
	}
#endif
	if ((rtmt != RT_TABLE_UNSPEC) && (rtmt != rttable)) {
	    err_msg += c_format("Ignoring route from table: %i\n", rtmt);
	    return XORP_ERROR;
	}
	else {
	    //XLOG_WARNING("Accepting route from table: %i\n", rtmt);
	}
    }


    IPvX nexthop_addr(family);
    IPvX dst_addr(family);
    int dst_mask_len = 0;

    //XLOG_INFO("rtm type: %i\n", (int)(rtmsg->rtm_type));

    //
    // Type-specific processing
    //
    switch (rtmsg->rtm_type) {
    case RTN_LOCAL:
	// TODO: XXX: PAVPAVPAV: handle it, if needed!
	err_msg += "RTM type is RTN_LOCAL, ignoring.\n";
	return (XORP_ERROR);		// TODO: is it really an error?

    case RTN_BLACKHOLE:
    case RTN_PROHIBIT:
    {
	//
	// Try to map discard routes back to the first software discard
	// interface in the tree. If we don't have one, then ignore this route.
	// We have to scan all interfaces because IfTree elements
	// are held in a map, and we don't key on this property.
	//
	const IfTreeInterface* pi = NULL;
	for (IfTree::IfMap::const_iterator ii = iftree.interfaces().begin();
	     ii != iftree.interfaces().end(); ++ii) {
	    if (ii->second->discard()) {
		pi = ii->second;
		break;
	    }
	}
	if (pi == NULL) {
	    //
	    // XXX: Cannot map a discard route back to an FEA soft discard
	    // interface.
	    //
	    err_msg += "Can't map discard route back to FEA soft discard interface.\n";
	    return (XORP_ERROR);
	}
	if_name = pi->ifname();
	vif_name = if_name;		// XXX: ifname == vifname
	// XXX: Do we need to change nexthop_addr?
	lookup_ifindex = false;
	break;
    }

    case RTN_UNREACHABLE:
    {
	//
	// Try to map unreachable routes back to the first software unreachable
	// interface in the tree. If we don't have one, then ignore this route.
	// We have to scan all interfaces because IfTree elements
	// are held in a map, and we don't key on this property.
	//
	const IfTreeInterface* pi = NULL;
	for (IfTree::IfMap::const_iterator ii = iftree.interfaces().begin();
	     ii != iftree.interfaces().end(); ++ii) {
	    if (ii->second->unreachable()) {
		pi = ii->second;
		break;
	    }
	}
	if (pi == NULL) {
	    //
	    // XXX: Cannot map an unreachable route back to an FEA soft
	    // unreachable interface.
	    //
	    err_msg += "Can't map unreachable route back to FEA soft unreachable interface.\n";
	    return (XORP_ERROR);
	}
	if_name = pi->ifname();
	vif_name = if_name;		// XXX: ifname == vifname
	// XXX: Do we need to change nexthop_addr?
	lookup_ifindex = false;
	break;
    }

    case RTN_UNICAST:
	break;

    default:
	XLOG_ERROR("nlm_get_to_fte_cfg() failed: "
		   "unrecognized AF_NETLINK route type: %d",
		   rtmsg->rtm_type);
	err_msg += c_format("Unrecognized route type: %i\n", (int)(rtmsg->rtm_type));
	return (XORP_ERROR);
    }

    //
    // Check the address family
    //
    if (rtmsg->rtm_family != family) {
	err_msg += "Invalid family.\n";
	return (XORP_ERROR);
    }

    //
    // Get the destination
    //
    if (rta_array[RTA_DST] == NULL) {
	// The default entry
    } else {
	// TODO: fix this!!
	dst_addr.copy_in(family, (uint8_t *)RTA_DATA(const_cast<struct rtattr *>(rta_array[RTA_DST])));
	dst_addr = system_adjust_ipvx_recv(dst_addr);
	if (! dst_addr.is_unicast()) {
	    // TODO: should we make this check?
	    fte.zero();
	    err_msg += c_format("Ignoring non-unicast destination: %s\n", dst_addr.str().c_str());
	    return (XORP_ERROR);
	}
    }

    //
    // Get the next-hop router address
    //
    if (rta_array[RTA_GATEWAY] != NULL) {
	nexthop_addr.copy_in(family, (uint8_t *)RTA_DATA(const_cast<struct rtattr *>(rta_array[RTA_GATEWAY])));
    }

    //
    // Get the destination mask length
    //
    dst_mask_len = rtmsg->rtm_dst_len;

    //
    // Test whether we installed this route
    //
    bool xorp_route = false;
    if (rtmsg->rtm_protocol == RTPROT_XORP)
	xorp_route = true;

    //
    // Get the interface/vif name and index
    //
    if (lookup_ifindex) {
	// Get the interface index
	if (rta_array[RTA_OIF] != NULL) {
	    const uint8_t* p = static_cast<const uint8_t *>(
		RTA_DATA(const_cast<struct rtattr *>(rta_array[RTA_OIF])));
	    if_index = extract_host_int(p);
	} else {
	    XLOG_ERROR("nlm_get_to_fte_cfg() failed: no interface found");
	    err_msg += "Could not find interface (no RTA_OIF)\n";
	    return (XORP_ERROR);
	}

	// Get the interface/vif name
	const IfTreeVif* vifp = iftree.find_vif(if_index);
	if (vifp != NULL) {
	    if_name = vifp->ifname();
	    vif_name = vifp->vifname();
	}

	// Test whether the interface/vif name was found
	if (if_name.empty() || vif_name.empty()) {
	    if (is_deleted) {
		//
		// XXX: If the route is deleted and we cannot find
		// the corresponding interface/vif, this could be because
		// an interface/vif is deleted from the kernel, and
		// the kernel might send first the upcall message
		// that deletes the interface from user space.
		// Hence the upcall message that followed to delete the
		// corresponding route for the connected subnet
		// won't find the interface/vif.
		// Propagate the route deletion with empty interface
		// and vif name.
		//
	    } else {
		//
		// XXX: A route was added, but we cannot find the corresponding
		// interface/vif.
		// This is probably because the interface in question is not in our
		// local config.
		// Another possibility:  This might happen because of a race
		// condition. E.g., an interface was added and then
		// immediately deleted, but the processing for the addition of
		// the corresponding connected route was delayed.
		// Note that the misorder in the processing might happen
		// because the interface and routing control messages are
		// received on different control sockets.
		// For the time being make it a fatal error until there is
		// enough evidence and the issue is understood.
		//
		//IPvXNet dst_subnet(dst_addr, dst_mask_len);
		err_msg += c_format("WARNING:  Decoding for route %s next hop %s failed: "
				    "could not find interface and vif for index %d",
				    dst_addr.str().c_str(),
				    nexthop_addr.str().c_str(),
				    if_index);
		return XORP_ERROR;
	    }
	}
    }

    //
    // Get the route metric
    //
    uint32_t route_metric = 0xffff;
    if (rta_array[RTA_PRIORITY] != NULL) {
	const uint8_t* p = static_cast<const uint8_t *>(
	    RTA_DATA(const_cast<struct rtattr *>(rta_array[RTA_PRIORITY])));
	int int_priority = extract_host_int(p);
	route_metric = int_priority;
    }

    //
    // TODO: define default admin distance instead of 0xffff
    //
    try {
	fte = FteX(IPvXNet(dst_addr, dst_mask_len), nexthop_addr,
		   if_name, vif_name, route_metric, 0xffff, xorp_route);
    }
    catch (XorpException& xe) {
	err_msg += "exception in nlm_get_to_fte_cfg: ";
	err_msg += xe.str();
	err_msg += "\n";
	XLOG_ERROR("exception in nlm_get_to_fte_cfg: %s", xe.str().c_str());
	return XORP_ERROR;
    }

    if (is_deleted)
	fte.mark_deleted();

    //XLOG_INFO("get_fte_cfg, route: %s", fte.str().c_str());

    return (XORP_OK);
}

/**
 * Check that a previous netlink request has succeeded.
 *
 * @param ns_reader the NetlinkSocketReader to use for reading data.
 * @param ns the NetlinkSocket to use for reading data.
 * @param seqno the sequence nomer of the netlink request to check for.
 * @param last_errno the last error number (if error).
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
NlmUtils::check_netlink_request(NetlinkSocketReader& ns_reader,
				NetlinkSocket& ns,
				uint32_t seqno,
				int& last_errno,
				string& error_msg)
{
    size_t buffer_bytes;
    struct nlmsghdr* nlh;

    last_errno = 0;		// XXX: reset the value

    //
    // Force to receive data from the kernel, and then parse it
    //
    if (ns_reader.receive_data(ns, seqno, error_msg) != XORP_OK)
	return (XORP_ERROR);

    vector<uint8_t>& buffer = ns_reader.buffer();
    buffer_bytes = buffer.size();
    for (nlh = (struct nlmsghdr*)(&buffer[0]);
	 NLMSG_OK(nlh, buffer_bytes);
	 nlh = NLMSG_NEXT(nlh, buffer_bytes)) {
	void* nlmsg_data = NLMSG_DATA(nlh);

	switch (nlh->nlmsg_type) {
	case NLMSG_ERROR:
	{
	    const struct nlmsgerr* err;

	    err = reinterpret_cast<const struct nlmsgerr*>(nlmsg_data);
	    if (nlh->nlmsg_len < NLMSG_LENGTH(sizeof(*err))) {
		error_msg += "AF_NETLINK nlmsgerr length error\n";
		return (XORP_ERROR);
	    }
	    if (err->error == 0)
		return (XORP_OK);	// No error
	    errno = -err->error;
	    last_errno = errno;
	    error_msg += c_format("AF_NETLINK NLMSG_ERROR message: %s\n",
				  strerror(errno));
	    return (XORP_ERROR);
	}
	break;

	case NLMSG_DONE:
	{
	    // End-of-message, and no ACK was received: error.
	    error_msg += "No ACK was received\n";
	    return (XORP_ERROR);
	}
	break;

	case NLMSG_NOOP:
	    break;

	default:
	    debug_msg("Unhandled type %s(%d) (%u bytes)\n",
		      nlm_msg_type(nlh->nlmsg_type).c_str(),
		      nlh->nlmsg_type, XORP_UINT_CAST(nlh->nlmsg_len));
	    break;
	}
    }

    error_msg += "No ACK was received\n";
    return (XORP_ERROR);		// No ACK was received: error.
}


int NlmUtils::nlm_decode_ipvx_address(int family, const struct rtattr* rtattr,
				      IPvX& ipvx_addr, bool& is_set, string& error_msg)
{
    is_set = false;

    if (rtattr == NULL) {
	error_msg = c_format("Missing address attribute to decode");
	return (XORP_ERROR);
    }

    //
    // Get the attribute information
    //
    size_t addr_size = RTA_PAYLOAD(rtattr);
    const uint8_t* addr_data = reinterpret_cast<const uint8_t*>(RTA_DATA(const_cast<struct rtattr*>(rtattr)));

    //
    // Test the address length
    //
    if (addr_size != IPvX::addr_bytelen(family)) {
	error_msg = c_format("Invalid address size payload: %u instead of %u",
			     XORP_UINT_CAST(addr_size),
			     XORP_UINT_CAST(IPvX::addr_bytelen(family)));
	return (XORP_ERROR);
    }

    //
    // Decode the address
    //
    ipvx_addr.copy_in(family, addr_data);
    is_set = true;

    return (XORP_OK);
}

int NlmUtils::nlm_decode_ipvx_interface_address(const struct ifinfomsg* ifinfomsg,
						const struct rtattr* rtattr,
						IPvX& ipvx_addr, bool& is_set,
						string& error_msg)
{
    int family = AF_UNSPEC;

    is_set = false;

    XLOG_ASSERT(ifinfomsg != NULL);

    if (rtattr == NULL) {
	error_msg = c_format("Missing address attribute to decode");
	return (XORP_ERROR);
    }

    // Decode the attribute type
    switch (ifinfomsg->ifi_type) {
    case ARPHRD_ETHER:
    {
	// MAC address: processed earlier when decoding the interface
	return (XORP_OK);
    }
    case ARPHRD_TUNNEL:
	// FALLTHROUGH
    case ARPHRD_SIT:
	// FALLTHROUGH
    case ARPHRD_IPGRE:
    {
	// IPv4 address
	family = IPv4::af();
	break;
    }
#ifdef HAVE_IPV6
    case ARPHRD_TUNNEL6:
    {
	// IPv6 address
	family = IPv6::af();
	break;
    }
#endif // HAVE_IPV6
    default:
	// Unknown address type: ignore
	return (XORP_OK);
    }

    XLOG_ASSERT(family != AF_UNSPEC);
    return (nlm_decode_ipvx_address(family, rtattr, ipvx_addr, is_set,
				    error_msg));
}

void NlmUtils::nlm_cond_newlink_to_fea_cfg(const IfTree& user_cfg, IfTree& iftree,
					   struct ifinfomsg* ifinfomsg,
					   int rta_len, bool& modified)
{
    struct rtattr *rtattr;
    struct rtattr *rta_array[IFLA_MAX + 1];
    uint32_t if_index = 0;
    string if_name;
    bool is_newlink = false;	// True if really a new link

    // The attributes
    memset(rta_array, 0, sizeof(rta_array));
    rtattr = IFLA_RTA(ifinfomsg);
    get_rtattr(rtattr, rta_len, rta_array,
	       sizeof(rta_array) / sizeof(rta_array[0]));

    //
    // Get the interface name
    //
    if (rta_array[IFLA_IFNAME] == NULL) {
	//
	// XXX: The kernel did not provide the interface name.
	// It could be because this is a wireless event carried to user
	// space. Such events are encapsulated in the IFLA_WIRELESS field of
	// a RTM_NEWLINK message.
	// Older kernels (2.6.18, for instance), don't send the name evidently.
	// Attempt to determine it from the ifindex.
	static bool warn_once = true;
	if (warn_once) {
	    XLOG_WARNING("Could not find interface name for interface index %d in netlink msg.\n  Attempting"
			 " work-around by using ifindex to find the name.\n  This warning will be printed only"
			 " once.\n",
			 ifinfomsg->ifi_index);
	    warn_once = false;
	}
#ifdef HAVE_IF_INDEXTONAME
	char buf[IF_NAMESIZE + 1];
	char* ifn = if_indextoname(ifinfomsg->ifi_index, buf);
	if (ifn) {
	    if_name = ifn;
	}
	else {
	    XLOG_ERROR("Cannot find ifname for index: %i, unable to process netlink NEWLINK message.\n",
		       ifinfomsg->ifi_index);
	    return;
	}
#else
	XLOG_ERROR("ERROR:  IFLA_IFNAME isn't in netlink msg, and don't have if_indextoname on this\n"
		   "  platform..we cannot properly read network device events.\n  This is likely a fatal"
		   " error.\n");
	return;
#endif
    }
    else {
	if_name = (char*)(RTA_DATA(rta_array[IFLA_IFNAME]));
    }

    //XLOG_WARNING("newlink, interface: %s  tree: %s\n", if_name.c_str(), iftree.getName().c_str());
    debug_msg("newlink, interface: %s  tree: %s\n", if_name.c_str(), iftree.getName().c_str());

    if (! user_cfg.find_interface(if_name)) {
	debug_msg("Ignoring interface: %s as it is not found in the local config.\n", if_name.c_str());
	return;
    }

    modified = true; // this is just for optimization..so if we somehow don't modify things, it's not a big deal.

    //
    // Get the interface index
    //
    if_index = ifinfomsg->ifi_index;
    if (if_index == 0) {
	XLOG_FATAL("Could not find physical interface index "
		   "for interface %s",
		   if_name.c_str());
    }
    debug_msg("interface index: %d\n", if_index);

    //
    // Add the interface (if a new one)
    //
    IfTreeInterface* ifp = iftree.find_interface(if_name);
    if (ifp == NULL) {
	iftree.add_interface(if_name);
	is_newlink = true;
	ifp = iftree.find_interface(if_name);
	XLOG_ASSERT(ifp != NULL);
    }

    //
    // Set the physical interface index for the interface
    //
    if (is_newlink || (if_index != ifp->pif_index()))
	ifp->set_pif_index(if_index);

    //
    // Get the MAC address
    //
    if (rta_array[IFLA_ADDRESS] != NULL) {
	size_t addr_size = RTA_PAYLOAD(rta_array[IFLA_ADDRESS]);
	const uint8_t* addr_data = reinterpret_cast<const uint8_t*>(RTA_DATA(const_cast<struct rtattr*>(rta_array[IFLA_ADDRESS])));
	switch (ifinfomsg->ifi_type) {
	case ARPHRD_ETHER:
	{
	    // MAC address
	    struct ether_addr ea;
	    if (addr_size != sizeof(ea))
		break;
	    memcpy(&ea, addr_data, sizeof(ea));
	    Mac mac(ea);
	    if (is_newlink || (mac != ifp->mac()))
		ifp->set_mac(mac);
	    break;
	}
	default:
	    // Either unknown type or type that will be processed later
	    break;
	}
    }
    debug_msg("MAC address: %s\n", ifp->mac().str().c_str());

    //
    // Get the MTU
    //
    if (rta_array[IFLA_MTU] != NULL) {
	unsigned int mtu;

	XLOG_ASSERT(RTA_PAYLOAD(rta_array[IFLA_MTU]) == sizeof(mtu));
	mtu = *reinterpret_cast<unsigned int*>(RTA_DATA(const_cast<struct rtattr*>(rta_array[IFLA_MTU])));
	if (is_newlink || (mtu != ifp->mtu()))
	    ifp->set_mtu(mtu);
    }
    else {
	static bool do_once = true;
	if (do_once) {
	    do_once = false;
	    XLOG_WARNING("WARNING:  MTU was not in rta_array, attempting to get it via"
			 "/sys/class/net/%s/mtu instead.  Will not print this message again.\n",
			 if_name.c_str());
	}
	// That sucks...try another way.
        unsigned int mtu = 1500; //default to something probably right.
        string ifn("/sys/class/net/");
	ifn.append(if_name);
        ifn.append("/mtu");
        ifstream ifs(ifn.c_str());
        if (ifs) {
	    ifs >> mtu;
        }
        else {
            XLOG_WARNING("WARNING: %s: MTU not in rta_array, and cannot open %s"
			 "  Defaulting to 1500\n", if_name.c_str(), ifn.c_str());
        }
	if (is_newlink || (mtu != ifp->mtu()))
	    ifp->set_mtu(mtu);
    }
    debug_msg("MTU: %u\n", ifp->mtu());

    //
    // Get the flags
    //
    unsigned int flags = ifinfomsg->ifi_flags;
    if (is_newlink || (flags != ifp->interface_flags())) {
	ifp->set_interface_flags(flags);
	ifp->set_enabled(flags & IFF_UP);
    }
    debug_msg("enabled: %s\n", bool_c_str(ifp->enabled()));

    //
    // Get the link status and baudrate
    //
    do {
	bool no_carrier = false;
	uint64_t baudrate = 0;
	string error_msg;

	if (ifconfig_media_get_link_status(if_name, no_carrier, baudrate,
					   error_msg)
	    != XORP_OK) {
	    // XXX: Use the flags
	    if ((flags & IFF_UP) && !(flags & IFF_RUNNING))
		no_carrier = true;
	    else
		no_carrier = false;
	}
	if (is_newlink || (no_carrier != ifp->no_carrier()))
	    ifp->set_no_carrier(no_carrier);
	if (is_newlink || (baudrate != ifp->baudrate()))
	    ifp->set_baudrate(baudrate);
	break;
    } while (false);
    debug_msg("no_carrier: %s\n", bool_c_str(ifp->no_carrier()));
    debug_msg("baudrate: %u\n", XORP_UINT_CAST(ifp->baudrate()));

    // XXX: vifname == ifname on this platform
    if (is_newlink)
	ifp->add_vif(if_name);
    IfTreeVif* vifp = ifp->find_vif(if_name);
    XLOG_ASSERT(vifp != NULL);

    //
    // Set the physical interface index for the vif
    //
    if (is_newlink || (if_index != vifp->pif_index()))
	vifp->set_pif_index(if_index);

    //
    // Set the vif flags
    //
    if (is_newlink || (flags != vifp->vif_flags())) {
	vifp->set_vif_flags(flags);
	vifp->set_enabled(ifp->enabled() && (flags & IFF_UP));
	vifp->set_broadcast(flags & IFF_BROADCAST);
	vifp->set_loopback(flags & IFF_LOOPBACK);
	vifp->set_point_to_point(flags & IFF_POINTOPOINT);
	vifp->set_multicast(flags & IFF_MULTICAST);
	// Propagate the flags to the existing addresses
	vifp->propagate_flags_to_addresses();
    }
    debug_msg("vif enabled: %s\n", bool_c_str(vifp->enabled()));
    debug_msg("vif broadcast: %s\n", bool_c_str(vifp->broadcast()));
    debug_msg("vif loopback: %s\n", bool_c_str(vifp->loopback()));
    debug_msg("vif point_to_point: %s\n", bool_c_str(vifp->point_to_point()));
    debug_msg("vif multicast: %s\n", bool_c_str(vifp->multicast()));

    return;
#if 0
    // This logic below adds an IP address for GRE tunnels, and perhaps
    // similar tunnels.  This causes trouble because that IP information
    // has nothing to do with actually using this interface.

    //
    // Add any interface-specific addresses
    //
    IPvX lcl_addr(AF_INET);
    IPvX peer_addr(AF_INET);
    bool has_lcl_addr = false;
    bool has_peer_addr = false;
    string error_msg;

    // Get the local address
    if (rta_array[IFLA_ADDRESS] != NULL) {
	if (nlm_decode_ipvx_interface_address(ifinfomsg,
					      rta_array[IFLA_ADDRESS],
					      lcl_addr, has_lcl_addr,
					      error_msg)
	    != XORP_OK) {
	    XLOG_WARNING("Error decoding address for interface %s vif %s: %s",
			 vifp->ifname().c_str(), vifp->vifname().c_str(),
			 error_msg.c_str());
	}
    }
    if (! has_lcl_addr)
	return;			// XXX: nothing more to do
    debug_msg("IP address before adjust: %s\n", lcl_addr.str().c_str());
    lcl_addr = system_adjust_ipvx_recv(lcl_addr);
    debug_msg("IP address: %s\n", lcl_addr.str().c_str());

    // XXX: No info about the masklen: assume it is the address bitlen
    size_t mask_len = IPvX::addr_bitlen(lcl_addr.af());

    // Get the broadcast/peer address
    if (rta_array[IFLA_BROADCAST] != NULL) {
	if (nlm_decode_ipvx_interface_address(ifinfomsg,
					      rta_array[IFLA_BROADCAST],
					      peer_addr, has_peer_addr,
					      error_msg)
	    != XORP_OK) {
	    XLOG_WARNING("Error decoding broadcast/peer address for "
			 "interface %s vif %s: %s",
			 vifp->ifname().c_str(), vifp->vifname().c_str(),
			 error_msg.c_str());
	}
	debug_msg("IP Broadcast/peer address: %s\n", peer_addr.str().c_str());
    }
    if (has_peer_addr)
	XLOG_ASSERT(lcl_addr.af() == peer_addr.af());

    // Add the address
    switch (lcl_addr.af()) {
    case AF_INET:
    {
	vifp->add_addr(lcl_addr.get_ipv4());
	IfTreeAddr4* ap = vifp->find_addr(lcl_addr.get_ipv4());
	XLOG_ASSERT(ap != NULL);
	ap->set_enabled(vifp->enabled());
	ap->set_broadcast(vifp->broadcast() && has_peer_addr);
	ap->set_loopback(vifp->loopback());
	ap->set_point_to_point(vifp->point_to_point() && has_peer_addr);
	ap->set_multicast(vifp->multicast());

	if (has_peer_addr) {
	    ap->set_prefix_len(mask_len);
	    if (ap->broadcast())
		ap->set_bcast(peer_addr.get_ipv4());
	    if (ap->point_to_point())
		ap->set_endpoint(peer_addr.get_ipv4());
	}

	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	vifp->add_addr(lcl_addr.get_ipv6());
	IfTreeAddr6* ap = vifp->find_addr(lcl_addr.get_ipv6());
	XLOG_ASSERT(ap != NULL);
	ap->set_enabled(vifp->enabled());
	ap->set_loopback(vifp->loopback());
	ap->set_point_to_point(vifp->point_to_point());
	ap->set_multicast(vifp->multicast());

	if (has_peer_addr) {
	    ap->set_prefix_len(mask_len);
	    if (ap->point_to_point())
		ap->set_endpoint(peer_addr.get_ipv6());
	}

	break;
    }
#endif // HAVE_IPV6
    default:
	break;
    }
#endif
}

void
NlmUtils::nlm_dellink_to_fea_cfg(IfTree& iftree, struct ifinfomsg* ifinfomsg,
				 int rta_len, bool& modified)
{
    struct rtattr *rtattr;
    struct rtattr *rta_array[IFLA_MAX + 1];
    uint32_t if_index = 0;
    string if_name;

    // The attributes
    memset(rta_array, 0, sizeof(rta_array));
    rtattr = IFLA_RTA(ifinfomsg);
    get_rtattr(rtattr, rta_len, rta_array,
	       sizeof(rta_array) / sizeof(rta_array[0]));

    //
    // Get the interface name
    //
    if (rta_array[IFLA_IFNAME] == NULL) {
#ifdef HAVE_IF_INDEXTONAME
	char buf[IF_NAMESIZE + 1];
	char* ifn = if_indextoname(ifinfomsg->ifi_index, buf);
	if (ifn) {
	    if_name = ifn;
	}
	else {
	    XLOG_ERROR("Cannot find ifname for index: %i, unable to process netlink DELLINK message.\n",
		       ifinfomsg->ifi_index);
	    return;
	}
#else
	XLOG_ERROR("ERROR:  IFLA_IFNAME isn't in netlink msg, and don't have if_indextoname on this\n"
		   " platform..we cannot properly read network device events.  This is likely a fatal"
		   " error.\n");
	return;
#endif
    }
    else {
	if_name = (char*)(RTA_DATA(rta_array[IFLA_IFNAME]));
    }

    XLOG_WARNING("dellink, interface: %s  tree: %s\n", if_name.c_str(), iftree.getName().c_str());

    //
    // Get the interface index
    //
    if_index = ifinfomsg->ifi_index;
    if (if_index == 0) {
	XLOG_FATAL("Could not find physical interface index "
		   "for interface %s",
		   if_name.c_str());
    }
    debug_msg("Deleting interface index: %u\n", if_index);

    //
    // Delete the interface and vif
    //
    IfTreeInterface* ifp = iftree.find_interface(if_index);
    if (ifp != NULL) {
	if (!ifp->is_marked(IfTree::DELETED)) {
	    iftree.markIfaceDeleted(ifp);
	    modified = true;
	}
    }
    IfTreeVif* vifp = iftree.find_vif(if_index);
    if (vifp != NULL) {
	if (!vifp->is_marked(IfTree::DELETED)) {
	    iftree.markVifDeleted(vifp);
	    modified = true;
	}
    }
}

void
NlmUtils::nlm_cond_newdeladdr_to_fea_cfg(const IfTree& user_config, IfTree& iftree,
					 struct ifaddrmsg* ifaddrmsg,
					 int rta_len, bool is_deleted, bool& modified)
{
    struct rtattr *rtattr;
    struct rtattr *rta_array[IFA_MAX + 1];
    uint32_t if_index = 0;
    int family = ifaddrmsg->ifa_family;

    //
    // Test the family
    //
    switch (family) {
    case AF_INET:
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	break;
#endif // HAVE_IPV6
    default:
	debug_msg("Ignoring address of family %d\n", family);
	return;
    }

    // The attributes
    memset(rta_array, 0, sizeof(rta_array));
    rtattr = IFA_RTA(ifaddrmsg);
    get_rtattr(rtattr, rta_len, rta_array,
	       sizeof(rta_array) / sizeof(rta_array[0]));

    //
    // Get the interface index
    //
    if_index = ifaddrmsg->ifa_index;
    if (if_index == 0) {
	XLOG_FATAL("Could not add or delete address for interface "
		   "with unknown index");
    }

    //
    // Locate the vif to pin data on
    //
    IfTreeVif* vifp = iftree.find_vif(if_index);
    if (vifp == NULL) {
	if (is_deleted) {
	    //
	    // XXX: In case of Linux kernel we may receive first
	    // a message to delete the interface and then a message to
	    // delete an address on that interface.
	    // However, the first message would remove all state about
	    // that interface (including its addresses).
	    // Hence, we silently ignore messages for deleting addresses
	    // if the interface is not found.
	    //
	    return;
	}
	else {
	    const IfTreeVif* vifpc = user_config.find_vif(if_index);
	    if (vifpc) {
		XLOG_FATAL("Could not find vif with index %u in IfTree", if_index);
	    }
	    //else, not in local config, so ignore it
	    return;
	}
    }
    debug_msg("Address event on interface %s vif %s with interface index %u\n",
	      vifp->ifname().c_str(), vifp->vifname().c_str(),
	      vifp->pif_index());

    modified = true; // this is just for optimization..so if we somehow don't modify things, it's not a big deal.

    //
    // Get the IP address, netmask, broadcast address, P2P destination
    //
    // The default values
    IPvX lcl_addr = IPvX::ZERO(family);
    IPvX subnet_mask = IPvX::ZERO(family);
    IPvX broadcast_addr = IPvX::ZERO(family);
    IPvX peer_addr = IPvX::ZERO(family);
    bool has_lcl_addr = false;
    bool has_broadcast_addr = false;
    bool has_peer_addr = false;
    bool is_ifa_address_reassigned = false;
    string error_msg;

    //
    // XXX: re-assign IFA_ADDRESS to IFA_LOCAL (and vice-versa).
    // This tweak is needed according to the iproute2 source code.
    //
    if (rta_array[IFA_LOCAL] == NULL) {
	rta_array[IFA_LOCAL] = rta_array[IFA_ADDRESS];
    }
    if (rta_array[IFA_ADDRESS] == NULL) {
	rta_array[IFA_ADDRESS] = rta_array[IFA_LOCAL];
	is_ifa_address_reassigned = true;
    }

    // Get the IP address
    if (rta_array[IFA_LOCAL] != NULL) {
	if (nlm_decode_ipvx_address(family, rta_array[IFA_LOCAL],
				    lcl_addr, has_lcl_addr, error_msg)
	    != XORP_OK) {
	    XLOG_FATAL("Error decoding address for interface %s vif %s: %s",
		       vifp->ifname().c_str(), vifp->vifname().c_str(),
		       error_msg.c_str());
	}
    }
    if (! has_lcl_addr) {
	XLOG_FATAL("Missing local address for interface %s vif %s",
		   vifp->ifname().c_str(), vifp->vifname().c_str());
    }
    debug_msg("IP address before adjust: %s\n", lcl_addr.str().c_str());
    lcl_addr = system_adjust_ipvx_recv(lcl_addr);
    debug_msg("IP address: %s\n", lcl_addr.str().c_str());

    // Get the netmask
    subnet_mask = IPvX::make_prefix(family, ifaddrmsg->ifa_prefixlen);
    debug_msg("IP netmask: %s\n", subnet_mask.str().c_str());

    // Get the broadcast address
    if (vifp->broadcast()) {
	switch (family) {
	case AF_INET:
	    if (rta_array[IFA_BROADCAST] != NULL) {
		if (nlm_decode_ipvx_address(family, rta_array[IFA_BROADCAST],
					    broadcast_addr, has_broadcast_addr,
					    error_msg)
		    != XORP_OK) {
		    XLOG_FATAL("Error decoding broadcast address for "
			       "interface %s vif %s: %s",
			       vifp->ifname().c_str(), vifp->vifname().c_str(),
			       error_msg.c_str());
		}
	    }
	    break;
#ifdef HAVE_IPV6
	case AF_INET6:
	    break;	// IPv6 doesn't have the idea of broadcast
#endif // HAVE_IPV6

	default:
	    XLOG_UNREACHABLE();
	    break;
	}
	debug_msg("Broadcast address: %s\n", broadcast_addr.str().c_str());
    }

    // Get the p2p address
    if (vifp->point_to_point()) {
	if ((rta_array[IFA_ADDRESS] != NULL) && !is_ifa_address_reassigned) {
	    if (rta_array[IFA_ADDRESS] != NULL) {
		if (nlm_decode_ipvx_address(family, rta_array[IFA_ADDRESS],
					    peer_addr, has_peer_addr,
					    error_msg)
		    != XORP_OK) {
		    XLOG_FATAL("Error decoding peer address for "
			       "interface %s vif %s: %s",
			       vifp->ifname().c_str(), vifp->vifname().c_str(),
			       error_msg.c_str());
		}
	    }
	}
	debug_msg("Peer address: %s\n", peer_addr.str().c_str());
    }

    debug_msg("\n");		// put an empty line between interfaces

    // Add or delete the address
    switch (family) {
    case AF_INET:
    {
	vifp->add_addr(lcl_addr.get_ipv4());
	IfTreeAddr4* ap = vifp->find_addr(lcl_addr.get_ipv4());
	XLOG_ASSERT(ap != NULL);
	ap->set_enabled(vifp->enabled());
	ap->set_broadcast(vifp->broadcast() && has_broadcast_addr);
	ap->set_loopback(vifp->loopback());
	ap->set_point_to_point(vifp->point_to_point() && has_peer_addr);
	ap->set_multicast(vifp->multicast());

	ap->set_prefix_len(subnet_mask.mask_len());
	if (ap->broadcast())
	    ap->set_bcast(broadcast_addr.get_ipv4());
	if (ap->point_to_point())
	    ap->set_endpoint(peer_addr.get_ipv4());

	// Mark as deleted if necessary
	if (is_deleted)
	    ap->mark(IfTreeItem::DELETED);

	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	vifp->add_addr(lcl_addr.get_ipv6());
	IfTreeAddr6* ap = vifp->find_addr(lcl_addr.get_ipv6());
	XLOG_ASSERT(ap != NULL);
	ap->set_enabled(vifp->enabled());
	ap->set_loopback(vifp->loopback());
	ap->set_point_to_point(vifp->point_to_point());
	ap->set_multicast(vifp->multicast());

	ap->set_prefix_len(subnet_mask.mask_len());
	if (ap->point_to_point())
	    ap->set_endpoint(peer_addr.get_ipv6());

	// Mark as deleted if necessary
	if (is_deleted)
	    ap->mark(IfTreeItem::DELETED);

	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
}



#endif // HAVE_NETLINK_SOCKETS
