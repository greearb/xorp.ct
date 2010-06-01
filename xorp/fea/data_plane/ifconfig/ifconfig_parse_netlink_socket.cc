// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#include <xorp_config.h>
#ifdef HAVE_NETLINK_SOCKETS


#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ether_compat.h"

#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif

#include "fea/ifconfig.hh"
#include "fea/data_plane/control_socket/system_utilities.hh"
#include "fea/data_plane/control_socket/netlink_socket_utilities.hh"

#include "ifconfig_get_netlink_socket.hh"
#include "ifconfig_media.hh"


//
// Parse information about network interface configuration change from
// the underlying system.
//
// The information to parse is in NETLINK format
// (e.g., obtained by netlink(7) sockets mechanism).
//
// Reading netlink(3) manual page is a good start for understanding this
//

static void nlm_cond_newlink_to_fea_cfg(const IfTree& user_config, IfTree& iftree,
					const struct ifinfomsg* ifinfomsg,
					int rta_len, bool& modified);
static void nlm_dellink_to_fea_cfg(IfTree& iftree,
				   const struct ifinfomsg* ifinfomsg,
				   int rta_len, bool& modified);
static void nlm_cond_newdeladdr_to_fea_cfg(const IfTree& user_config, IfTree& iftree,
					   const struct ifaddrmsg* ifaddrmsg,
					   int rta_len, bool is_deleted,
					   bool& modified);

int
IfConfigGetNetlinkSocket::parse_buffer_netlink_socket(IfConfig& ifconfig,
						      IfTree& iftree,
						      const vector<uint8_t>& buffer,
						      bool& modified, int& nl_errno)
{
    size_t buffer_bytes = buffer.size();
    AlignData<struct nlmsghdr> align_data(buffer);
    const struct nlmsghdr* nlh;
    bool recognized = false;

    for (nlh = align_data.payload();
	 NLMSG_OK(nlh, buffer_bytes);
	 nlh = NLMSG_NEXT(nlh, buffer_bytes)) {
	void* nlmsg_data = NLMSG_DATA(nlh);
	
	//XLOG_WARNING("nlh msg received, type %s(%d) (%d bytes)\n",
	//	     NlmUtils::nlm_msg_type(nlh->nlmsg_type).c_str(),
	//	     nlh->nlmsg_type, nlh->nlmsg_len);

	switch (nlh->nlmsg_type) {
	case NLMSG_ERROR:
	{
	    const struct nlmsgerr* err;
	    
	    err = reinterpret_cast<const struct nlmsgerr*>(nlmsg_data);
	    if (nlh->nlmsg_len < NLMSG_LENGTH(sizeof(*err))) {
		XLOG_ERROR("AF_NETLINK nlmsgerr length error");
		break;
	    }
	    errno = -err->error;
	    nl_errno = -err->error;
	    XLOG_ERROR("AF_NETLINK NLMSG_ERROR: %s  msg->len: %hu msg->type: %hu(%s) "
		       " msg->flags: %hu msg->seq: %u  msg->pid: %u",
		       strerror(errno), err->msg.nlmsg_len, err->msg.nlmsg_type,
		       NlmUtils::nlm_msg_type(err->msg.nlmsg_type).c_str(),
		       err->msg.nlmsg_flags, err->msg.nlmsg_seq, err->msg.nlmsg_pid);
	}
	break;
	
	case NLMSG_DONE:
	{
	    if (! recognized)
		return (XORP_ERROR);
	    return (XORP_OK);
	}
	break;
	
	case NLMSG_NOOP:
	    break;
	    
	case RTM_NEWLINK:
	case RTM_DELLINK:
	{
	    const struct ifinfomsg* ifinfomsg;
	    int rta_len = IFLA_PAYLOAD(nlh);
	    
	    if (rta_len < 0) {
		XLOG_ERROR("AF_NETLINK ifinfomsg length error");
		break;
	    }
	    ifinfomsg = reinterpret_cast<const struct ifinfomsg*>(nlmsg_data);
	    if (nlh->nlmsg_type == RTM_NEWLINK)
		nlm_cond_newlink_to_fea_cfg(ifconfig.user_config(), iftree,
					    ifinfomsg, rta_len, modified);
	    else
		nlm_dellink_to_fea_cfg(iftree, ifinfomsg, rta_len, modified);
	    recognized = true;
	}
	break;
	
	case RTM_NEWADDR:
	case RTM_DELADDR:
	{
	    const struct ifaddrmsg* ifaddrmsg;
	    int rta_len = IFA_PAYLOAD(nlh);
	    
	    if (rta_len < 0) {
		XLOG_ERROR("AF_NETLINK ifaddrmsg length error");
		break;
	    }
	    ifaddrmsg = reinterpret_cast<const struct ifaddrmsg*>(nlmsg_data);
	    if (nlh->nlmsg_type == RTM_NEWADDR) {
		nlm_cond_newdeladdr_to_fea_cfg(ifconfig.user_config(), iftree,
					       ifaddrmsg, rta_len, false, modified);
	    } else {
		nlm_cond_newdeladdr_to_fea_cfg(ifconfig.user_config(), iftree,
					       ifaddrmsg, rta_len, true, modified);
	    }
	    recognized = true;
	}
	break;
	
	default:
	    debug_msg("Unhandled type %s(%d) (%d bytes)\n",
		      NlmUtils::nlm_msg_type(nlh->nlmsg_type).c_str(),
		      nlh->nlmsg_type, nlh->nlmsg_len);
	    //XLOG_WARNING("Unhandled type %s(%d) (%d bytes)\n",
	    //	 NlmUtils::nlm_msg_type(nlh->nlmsg_type).c_str(),
	    //	 nlh->nlmsg_type, nlh->nlmsg_len);
	    break;
	}
    }
    
    if (! recognized)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

static int
nlm_decode_ipvx_address(int family, const struct rtattr* rtattr,
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

static int
nlm_decode_ipvx_interface_address(const struct ifinfomsg* ifinfomsg,
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

static void
nlm_cond_newlink_to_fea_cfg(const IfTree& user_cfg, IfTree& iftree, const struct ifinfomsg* ifinfomsg,
			    int rta_len, bool& modified)
{
    const struct rtattr *rtattr;
    const struct rtattr *rta_array[IFLA_MAX + 1];
    uint32_t if_index = 0;
    string if_name;
    bool is_newlink = false;	// True if really a new link
    
    // The attributes
    memset(rta_array, 0, sizeof(rta_array));
    rtattr = IFLA_RTA(ifinfomsg);
    NlmUtils::get_rtattr(rtattr, rta_len, rta_array,
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

    if (! user_cfg.find_interface(if_name)) {
	//XLOG_WARNING("Ignoring interface: %s as it is not found in the local config.\n", if_name.c_str());
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
}

static void
nlm_dellink_to_fea_cfg(IfTree& iftree, const struct ifinfomsg* ifinfomsg,
		       int rta_len, bool& modified)
{
    const struct rtattr *rtattr;
    const struct rtattr *rta_array[IFLA_MAX + 1];
    uint32_t if_index = 0;
    string if_name;
    
    // The attributes
    memset(rta_array, 0, sizeof(rta_array));
    rtattr = IFLA_RTA(ifinfomsg);
    NlmUtils::get_rtattr(rtattr, rta_len, rta_array,
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

static void
nlm_cond_newdeladdr_to_fea_cfg(const IfTree& user_config, IfTree& iftree, const struct ifaddrmsg* ifaddrmsg,
			       int rta_len, bool is_deleted, bool& modified)
{
    const struct rtattr *rtattr;
    const struct rtattr *rta_array[IFA_MAX + 1];
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
    NlmUtils::get_rtattr(rtattr, rta_len, rta_array,
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
