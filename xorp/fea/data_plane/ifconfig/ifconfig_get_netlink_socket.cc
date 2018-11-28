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


#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

#include "fea/ifconfig.hh"
#include "fea/fibconfig.hh"
#include "ifconfig_get_netlink_socket.hh"
#include "../control_socket/netlink_socket_utilities.hh"

//
// Parse information about network interface configuration change from
// the underlying system.
//
// The information to parse is in NETLINK format
// (e.g., obtained by netlink(7) sockets mechanism).
//
// Reading netlink(3) manual page is a good start for understanding this
//


//
// Get information about network interfaces from the underlying system.
//
// The mechanism to obtain the information is netlink(7) sockets.
//

IfConfigGetNetlinkSocket::IfConfigGetNetlinkSocket(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigGet(fea_data_plane_manager),
      NetlinkSocket(fea_data_plane_manager.eventloop(),
		    fea_data_plane_manager.fibconfig().get_netlink_filter_table_id()),
      _ns_reader(*(NetlinkSocket *)this)
{
    can_get_single = -1;
}

IfConfigGetNetlinkSocket::~IfConfigGetNetlinkSocket()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the netlink(7) sockets mechanism to get "
		   "information about network interfaces from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigGetNetlinkSocket::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (NetlinkSocket::start(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigGetNetlinkSocket::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (NetlinkSocket::stop(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

int
IfConfigGetNetlinkSocket::pull_config(const IfTree* local_cfg, IfTree& iftree)
{
    return read_config(local_cfg, iftree);
}


int IfConfigGetNetlinkSocket::try_read_config_one(IfTree& iftree, const char* ifname,
						  int ifindex) {
    int nl_errno = 0;
    int rv = read_config_one(iftree, ifname, ifindex, nl_errno);
    if (rv != XORP_OK) {
	if ((nl_errno == EINVAL) && (can_get_single == -1)) {
	    // Attempt work-around, maybe kernel is too old to do single requests.
	    can_get_single = 0;
	    nl_errno = 0;
	    rv = read_config_one(iftree, ifname, ifindex, nl_errno);
	    if (rv == XORP_OK) {
		// Might have worked...look for device.
		if (iftree.find_interface(ifname)) {
		    XLOG_WARNING("WARNING:  It seems that we cannot get a single Network device"
				 " via NETLINK, probably due to an older kernel.  Will enable"
				 " work-around to grab entire device listing instead.  This may"
				 " cause a slight performance hit on systems with lots of interfaces"
				 " but for most users it should not be noticeable.");
		}
		else {
		    // Still no luck..assume netlink get single can still work.
		    can_get_single = -1;
		}
	    }
	}
    }
    else {
	if (can_get_single == -1) {
	    XLOG_WARNING("NOTE:  Netlink get single network device works on this system.");
	    can_get_single = 1;
	}
    }
    return rv;
}

int
IfConfigGetNetlinkSocket::pull_config_one(IfTree& iftree, const char* ifname, int ifindex)
{
    return try_read_config_one(iftree, ifname, ifindex);
}


int findDeviceIndex(const char* device_name) {
   struct ifreq ifr;
   memset(&ifr, 0, sizeof(ifr));
   strncpy(ifr.ifr_name, device_name, IFNAMSIZ-1); //max length of a device name.

   int ioctl_sock = socket(AF_INET, SOCK_STREAM, 0);
   if (ioctl_sock >= 0) {
      if (ioctl(ioctl_sock, SIOCGIFINDEX, &ifr) >= 0) {
         close(ioctl_sock);
         return ifr.ifr_ifindex;
      }
      else {
         close(ioctl_sock);
      }
   }
   return -1;
}//findDeviceIndex


int
IfConfigGetNetlinkSocket::read_config_one(IfTree& iftree,
					  const char* ifname, int if_index,
					  int& nl_errno)
{
    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct ifinfomsg) + sizeof(struct ifaddrmsg) + 512;
    union {
	uint8_t		data[buffer_size];
	struct nlmsghdr	nlh;
    } buffer;
    struct nlmsghdr*	nlh = &buffer.nlh;
    struct sockaddr_nl	snl;
    struct ifinfomsg*	ifinfomsg;
    NetlinkSocket&	ns = *this;

    //XLOG_WARNING("read_config_one:  tree: %s  ifname: %s  if_index: %i\n",
    //		 iftree.getName().c_str(), ifname, if_index);


    if ((if_index < 1) && ifname) {
	// Try to resolve it based on ifname
	if_index = findDeviceIndex(ifname);
	//XLOG_WARNING("tried to resolve ifindex: %i\n", if_index);
    }
    if (if_index < 1) {
	return XORP_ERROR;
    }

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    //
    // Set the request for network interfaces
    //
    memset(&buffer, 0, sizeof(buffer));
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifinfomsg));
    nlh->nlmsg_type = RTM_GETLINK;
    nlh->nlmsg_flags = NLM_F_REQUEST; // Get info for single device

    /** Older kernels (2.6.18, for instance), cannot get a single network device
     * unless configured for CONFIG_NET_WIRELESS_RTNETLINK.
     * They just return EINVAL.  So, for these, we need to include the NLM_F_ROOT
     * flag.
     */
    if (can_get_single == 0) {
	nlh->nlmsg_flags |= NLM_F_ROOT; // get the whole table
    }

    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.nl_pid();
    ifinfomsg = reinterpret_cast<struct ifinfomsg*>(NLMSG_DATA(nlh));
    ifinfomsg->ifi_family = AF_UNSPEC;
    ifinfomsg->ifi_type = IFLA_UNSPEC;
    ifinfomsg->ifi_index = if_index;
    ifinfomsg->ifi_flags = 0;
    ifinfomsg->ifi_change = 0xffffffff;

    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
		  reinterpret_cast<struct sockaddr*>(&snl),
		  sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	XLOG_ERROR("Error writing to netlink socket: %s", strerror(errno));
	return (XORP_ERROR);
    }

    //
    // Force to receive data from the kernel, and then parse it
    //
    //
    string error_msg;
    if (_ns_reader.receive_data(ns, nlh->nlmsg_seq, error_msg) != XORP_OK) {
	XLOG_ERROR("Error reading from netlink socket: %s", error_msg.c_str());
	return (XORP_ERROR);
    }

    bool modified = false;
    if (parse_buffer_netlink_socket(ifconfig(), iftree, _ns_reader.buffer(),
				    modified, nl_errno)
	!= XORP_OK) {
	return (XORP_ERROR);
    }
    return XORP_OK;
}

int
IfConfigGetNetlinkSocket::parse_buffer_netlink_socket(IfConfig& ifconfig,
						      IfTree& iftree,
						      vector<uint8_t>& buffer,
						      bool& modified, int& nl_errno)
{
    size_t buffer_bytes = buffer.size();
    struct nlmsghdr* nlh;
    bool recognized = false;

    for (nlh = (struct nlmsghdr*)(&buffer[0]);
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
	    XLOG_ERROR("AF_NETLINK NLMSG_ERROR: %s  msg->len: %u msg->type: %hu(%s) "
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
	    struct ifinfomsg* ifinfomsg;
	    int rta_len = IFLA_PAYLOAD(nlh);

	    if (rta_len < 0) {
		XLOG_ERROR("AF_NETLINK ifinfomsg length error");
		break;
	    }
	    ifinfomsg = (struct ifinfomsg*)(nlmsg_data);
	    if (nlh->nlmsg_type == RTM_NEWLINK)
		NlmUtils::nlm_cond_newlink_to_fea_cfg(ifconfig.user_config(), iftree,
						      ifinfomsg, rta_len, modified);
	    else
		NlmUtils::nlm_dellink_to_fea_cfg(iftree, ifinfomsg, rta_len, modified);
	    recognized = true;
	}
	break;

	case RTM_NEWADDR:
	case RTM_DELADDR:
	{
	    struct ifaddrmsg* ifaddrmsg;
	    int rta_len = IFA_PAYLOAD(nlh);

	    if (rta_len < 0) {
		XLOG_ERROR("AF_NETLINK ifaddrmsg length error");
		break;
	    }
	    ifaddrmsg = (struct ifaddrmsg*)(nlmsg_data);
	    if (nlh->nlmsg_type == RTM_NEWADDR) {
		NlmUtils::nlm_cond_newdeladdr_to_fea_cfg(ifconfig.user_config(), iftree,
							 ifaddrmsg, rta_len, false, modified);
	    } else {
		NlmUtils::nlm_cond_newdeladdr_to_fea_cfg(ifconfig.user_config(), iftree,
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


int
IfConfigGetNetlinkSocket::read_config_all(IfTree& iftree)
{
    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct ifinfomsg) + sizeof(struct ifaddrmsg) + 512;
    union {
	uint8_t		data[buffer_size];
	struct nlmsghdr	nlh;
    } buffer;
    struct nlmsghdr*	nlh = &buffer.nlh;
    struct sockaddr_nl	snl;
    struct ifinfomsg*	ifinfomsg;
    struct ifaddrmsg*	ifaddrmsg;
    NetlinkSocket&	ns = *this;

    //
    // Set the request. First the socket, then the request itself.
    //
    // Note that first we send a request for the network interfaces,
    // then a request for the IPv4 addresses, and then a request
    // for the IPv6 addresses.
    //

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    //
    // Set the request for network interfaces
    //
    memset(&buffer, 0, sizeof(buffer));
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifinfomsg));
    nlh->nlmsg_type = RTM_GETLINK;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;	// Get the whole table
    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.nl_pid();
    ifinfomsg = reinterpret_cast<struct ifinfomsg*>(NLMSG_DATA(nlh));
    ifinfomsg->ifi_family = AF_UNSPEC;
    ifinfomsg->ifi_type = IFLA_UNSPEC;
    ifinfomsg->ifi_index = 0;
    ifinfomsg->ifi_flags = 0;
    ifinfomsg->ifi_change = 0xffffffff;

    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
		  reinterpret_cast<struct sockaddr*>(&snl),
		  sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	XLOG_ERROR("Error writing to netlink socket: %s", strerror(errno));
	return (XORP_ERROR);
    }

    //
    // Force to receive data from the kernel, and then parse it
    //
    //
    // XXX: setting the flag below is a work-around hack because of a
    // Linux kernel bug: when we read the whole table the kernel
    // may not set the NLM_F_MULTI flag for the multipart messages.
    //
    string error_msg;
    ns.set_multipart_message_read(true);
    if (_ns_reader.receive_data(ns, nlh->nlmsg_seq, error_msg) != XORP_OK) {
	ns.set_multipart_message_read(false);
	XLOG_ERROR("Error reading from netlink socket: %s", error_msg.c_str());
	return (XORP_ERROR);
    }
    // XXX: reset the multipart message read hackish flag
    ns.set_multipart_message_read(false);
    if (parse_buffer_netlink_socket(ifconfig(), iftree, _ns_reader.buffer())
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    //
    // Create a list with the interface indexes
    //
    list<uint32_t> if_index_list;
    uint32_t if_index;

    IfTree::IfMap::const_iterator if_iter;
    for (if_iter = iftree.interfaces().begin();
	 if_iter != iftree.interfaces().end();
	 ++if_iter) {
	const IfTreeInterface& iface = *(if_iter->second);
	IfTreeInterface::VifMap::const_iterator vif_iter;
	for (vif_iter = iface.vifs().begin();
	     vif_iter != iface.vifs().end();
	     ++vif_iter) {
	    const IfTreeVif& vif = *(vif_iter->second);
	    if_index = vif.pif_index();
	    if_index_list.push_back(if_index);
	}
    }

    //
    // Send requests for the addresses of each interface we just found
    //
    list<uint32_t>::const_iterator if_index_iter;
    for (if_index_iter = if_index_list.begin();
	 if_index_iter != if_index_list.end();
	 ++if_index_iter) {
	if_index = *if_index_iter;

	//
	// Set the request for IPv4 addresses
	//
	if (fea_data_plane_manager().have_ipv4()) {
	    memset(&buffer, 0, sizeof(buffer));
	    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifaddrmsg));
	    nlh->nlmsg_type = RTM_GETADDR;
	    // Get the whole table
	    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	    nlh->nlmsg_seq = ns.seqno();
	    nlh->nlmsg_pid = ns.nl_pid();
	    ifaddrmsg = reinterpret_cast<struct ifaddrmsg*>(NLMSG_DATA(nlh));
	    ifaddrmsg->ifa_family = AF_INET;
	    ifaddrmsg->ifa_prefixlen = 0;
	    ifaddrmsg->ifa_flags = 0;
	    ifaddrmsg->ifa_scope = 0;
	    ifaddrmsg->ifa_index = if_index;

	    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
			  reinterpret_cast<struct sockaddr*>(&snl),
			  sizeof(snl))
		!= (ssize_t)nlh->nlmsg_len) {
		XLOG_ERROR("Error writing to netlink socket: %s",
			   strerror(errno));
		return (XORP_ERROR);
	    }

	    //
	    // Force to receive data from the kernel, and then parse it
	    //
	    //
	    // XXX: setting the flag below is a work-around hack because of a
	    // Linux kernel bug: when we read the whole table the kernel
	    // may not set the NLM_F_MULTI flag for the multipart messages.
	    //
	    string error_msg;
	    ns.set_multipart_message_read(true);
	    if (_ns_reader.receive_data(ns, nlh->nlmsg_seq, error_msg)
		!= XORP_OK) {
		ns.set_multipart_message_read(false);
		XLOG_ERROR("Error reading from netlink socket: %s",
			   error_msg.c_str());
		return (XORP_ERROR);
	    }
	    // XXX: reset the multipart message read hackish flag
	    ns.set_multipart_message_read(false);
	    if (parse_buffer_netlink_socket(ifconfig(), iftree,
					    _ns_reader.buffer())
		!= XORP_OK) {
		return (XORP_ERROR);
	    }
	}

#ifdef HAVE_IPV6
	//
	// Set the request for IPv6 addresses
	//
	if (fea_data_plane_manager().have_ipv6()) {
	    memset(&buffer, 0, sizeof(buffer));
	    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifaddrmsg));
	    nlh->nlmsg_type = RTM_GETADDR;
	    // Get the whole table
	    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	    nlh->nlmsg_seq = ns.seqno();
	    nlh->nlmsg_pid = ns.nl_pid();
	    ifaddrmsg = reinterpret_cast<struct ifaddrmsg*>(NLMSG_DATA(nlh));
	    ifaddrmsg->ifa_family = AF_INET6;
	    ifaddrmsg->ifa_prefixlen = 0;
	    ifaddrmsg->ifa_flags = 0;
	    ifaddrmsg->ifa_scope = 0;
	    ifaddrmsg->ifa_index = if_index;

	    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
			  reinterpret_cast<struct sockaddr*>(&snl),
			  sizeof(snl))
		!= (ssize_t)nlh->nlmsg_len) {
		XLOG_ERROR("Error writing to netlink socket: %s",
			   strerror(errno));
		return (XORP_ERROR);
	    }

	    //
	    // Force to receive data from the kernel, and then parse it
	    //
	    //
	    // XXX: setting the flag below is a work-around hack because of a
	    // Linux kernel bug: when we read the whole table the kernel
	    // may not set the NLM_F_MULTI flag for the multipart messages.
	    //
	    string error_msg;
	    ns.set_multipart_message_read(true);
	    if (_ns_reader.receive_data(ns, nlh->nlmsg_seq, error_msg)
		!= XORP_OK) {
		ns.set_multipart_message_read(false);
		XLOG_ERROR("Error reading from netlink socket: %s",
			   error_msg.c_str());
		return (XORP_ERROR);
	    }
	    // XXX: reset the multipart message read hackish flag
	    ns.set_multipart_message_read(false);
	    if (parse_buffer_netlink_socket(ifconfig(), iftree,
					    _ns_reader.buffer())
		!= XORP_OK) {
		return (XORP_ERROR);
	    }
	}
#endif // HAVE_IPV6
    }

    //
    // Get the VLAN vif info
    //
    IfConfigVlanGet* ifconfig_vlan_get;
    ifconfig_vlan_get = fea_data_plane_manager().ifconfig_vlan_get();
    if (ifconfig_vlan_get != NULL) {
	bool modified = false;
	if (ifconfig_vlan_get->pull_config(iftree, modified) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}//read_config_all


int
IfConfigGetNetlinkSocket::read_config(const IfTree* local_cfg, IfTree& iftree)
{
    if ((!local_cfg) || (can_get_single == 0)) {
	return read_config_all(iftree);
    }

    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct ifinfomsg) + sizeof(struct ifaddrmsg) + 512;
    union {
	uint8_t		data[buffer_size];
	struct nlmsghdr	nlh;
    } buffer;
    struct nlmsghdr*	nlh = &buffer.nlh;
    struct sockaddr_nl	snl;
    struct ifaddrmsg*	ifaddrmsg;
    NetlinkSocket&	ns = *this;

    //
    // Set the request. First the socket, then the request itself.
    //
    // Note that first we send a request for the network interfaces,
    // then a request for the IPv4 addresses, and then a request
    // for the IPv6 addresses.
    //

    IfTree::IfMap::const_iterator if_iter;
    for (if_iter = local_cfg->interfaces().begin();
	 if_iter != local_cfg->interfaces().end();
	 ++if_iter) {
	const IfTreeInterface& iface = *(if_iter->second);
	IfTreeInterface::VifMap::const_iterator vif_iter;
	for (vif_iter = iface.vifs().begin();
	     vif_iter != iface.vifs().end();
	     ++vif_iter) {
	    const IfTreeVif& vif = *(vif_iter->second);
	    try_read_config_one(iftree, vif.vifname().c_str(), vif.pif_index());
	}
    }

    //
    // Create a list with the interface indexes
    //
    list<uint32_t> if_index_list;
    uint32_t if_index;

    for (if_iter = iftree.interfaces().begin();
	 if_iter != iftree.interfaces().end();
	 ++if_iter) {
	const IfTreeInterface& iface = *(if_iter->second);
	IfTreeInterface::VifMap::const_iterator vif_iter;
	for (vif_iter = iface.vifs().begin();
	     vif_iter != iface.vifs().end();
	     ++vif_iter) {
	    const IfTreeVif& vif = *(vif_iter->second);
	    if_index = vif.pif_index();
	    if_index_list.push_back(if_index);
	}
    }

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    //
    // Send requests for the addresses of each interface we just found
    //
    list<uint32_t>::const_iterator if_index_iter;
    for (if_index_iter = if_index_list.begin();
	 if_index_iter != if_index_list.end();
	 ++if_index_iter) {
	if_index = *if_index_iter;

	//
	// Set the request for IPv4 addresses
	//
	if (fea_data_plane_manager().have_ipv4()) {
	    memset(&buffer, 0, sizeof(buffer));
	    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifaddrmsg));
	    nlh->nlmsg_type = RTM_GETADDR;
	    // Get the whole table
	    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	    nlh->nlmsg_seq = ns.seqno();
	    nlh->nlmsg_pid = ns.nl_pid();
	    ifaddrmsg = reinterpret_cast<struct ifaddrmsg*>(NLMSG_DATA(nlh));
	    ifaddrmsg->ifa_family = AF_INET;
	    ifaddrmsg->ifa_prefixlen = 0;
	    ifaddrmsg->ifa_flags = 0;
	    ifaddrmsg->ifa_scope = 0;
	    ifaddrmsg->ifa_index = if_index;

	    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
			  reinterpret_cast<struct sockaddr*>(&snl),
			  sizeof(snl))
		!= (ssize_t)nlh->nlmsg_len) {
		XLOG_ERROR("Error writing to netlink socket: %s",
			   strerror(errno));
		return (XORP_ERROR);
	    }

	    //
	    // Force to receive data from the kernel, and then parse it
	    //
	    //
	    // XXX: setting the flag below is a work-around hack because of a
	    // Linux kernel bug: when we read the whole table the kernel
	    // may not set the NLM_F_MULTI flag for the multipart messages.
	    //
	    string error_msg;
	    ns.set_multipart_message_read(true);
	    if (_ns_reader.receive_data(ns, nlh->nlmsg_seq, error_msg)
		!= XORP_OK) {
		ns.set_multipart_message_read(false);
		XLOG_ERROR("Error reading from netlink socket: %s",
			   error_msg.c_str());
		return (XORP_ERROR);
	    }
	    // XXX: reset the multipart message read hackish flag
	    ns.set_multipart_message_read(false);
	    if (parse_buffer_netlink_socket(ifconfig(), iftree,
					    _ns_reader.buffer())
		!= XORP_OK) {
		return (XORP_ERROR);
	    }
	}

#ifdef HAVE_IPV6
	//
	// Set the request for IPv6 addresses
	//
	if (fea_data_plane_manager().have_ipv6()) {
	    memset(&buffer, 0, sizeof(buffer));
	    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifaddrmsg));
	    nlh->nlmsg_type = RTM_GETADDR;
	    // Get the whole table
	    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	    nlh->nlmsg_seq = ns.seqno();
	    nlh->nlmsg_pid = ns.nl_pid();
	    ifaddrmsg = reinterpret_cast<struct ifaddrmsg*>(NLMSG_DATA(nlh));
	    ifaddrmsg->ifa_family = AF_INET6;
	    ifaddrmsg->ifa_prefixlen = 0;
	    ifaddrmsg->ifa_flags = 0;
	    ifaddrmsg->ifa_scope = 0;
	    ifaddrmsg->ifa_index = if_index;

	    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
			  reinterpret_cast<struct sockaddr*>(&snl),
			  sizeof(snl))
		!= (ssize_t)nlh->nlmsg_len) {
		XLOG_ERROR("Error writing to netlink socket: %s",
			   strerror(errno));
		return (XORP_ERROR);
	    }

	    //
	    // Force to receive data from the kernel, and then parse it
	    //
	    //
	    // XXX: setting the flag below is a work-around hack because of a
	    // Linux kernel bug: when we read the whole table the kernel
	    // may not set the NLM_F_MULTI flag for the multipart messages.
	    //
	    string error_msg;
	    ns.set_multipart_message_read(true);
	    if (_ns_reader.receive_data(ns, nlh->nlmsg_seq, error_msg)
		!= XORP_OK) {
		ns.set_multipart_message_read(false);
		XLOG_ERROR("Error reading from netlink socket: %s",
			   error_msg.c_str());
		return (XORP_ERROR);
	    }
	    // XXX: reset the multipart message read hackish flag
	    ns.set_multipart_message_read(false);
	    if (parse_buffer_netlink_socket(ifconfig(), iftree,
					    _ns_reader.buffer())
		!= XORP_OK) {
		return (XORP_ERROR);
	    }
	}
#endif // HAVE_IPV6
    }

    //
    // Get the VLAN vif info
    //
    IfConfigVlanGet* ifconfig_vlan_get;
    ifconfig_vlan_get = fea_data_plane_manager().ifconfig_vlan_get();
    if (ifconfig_vlan_get != NULL) {
	bool modified = false;
	if (ifconfig_vlan_get->pull_config(iftree, modified) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

#endif // HAVE_NETLINK_SOCKETS
