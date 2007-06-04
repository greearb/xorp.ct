// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_set_netlink_socket.cc,v 1.7 2007/05/23 04:08:24 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ether_compat.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif
#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "fea/ifconfig.hh"
#include "fea/ifconfig_set.hh"
#include "fea/data_plane/control_socket/netlink_socket_utilities.hh"


//
// Set information about network interfaces configuration with the
// underlying system.
//
// The mechanism to set the information is netlink(7) sockets.
//

IfConfigSetNetlinkSocket::IfConfigSetNetlinkSocket(IfConfig& ifconfig)
    : IfConfigSet(ifconfig),
      NetlinkSocket(ifconfig.eventloop()),
      _ns_reader(*(NetlinkSocket *)this)
{
#ifdef HAVE_NETLINK_SOCKETS
    ifconfig.register_ifconfig_set_primary(this);
#endif
}

IfConfigSetNetlinkSocket::~IfConfigSetNetlinkSocket()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the netlink(7) sockets mechanism to set "
		   "information about network interfaces into the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigSetNetlinkSocket::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (NetlinkSocket::start(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigSetNetlinkSocket::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (NetlinkSocket::stop(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

bool
IfConfigSetNetlinkSocket::is_discard_emulated(const IfTreeInterface& i) const
{
    UNUSED(i);

#ifdef HOST_OS_LINUX
    return (true);
#else
    return (false);
#endif
}

#ifndef HAVE_NETLINK_SOCKETS
int
IfConfigSetNetlinkSocket::config_begin(string& error_msg)
{
    debug_msg("config_begin\n");

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetNetlinkSocket::config_end(string& error_msg)
{
    debug_msg("config_end\n");

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetNetlinkSocket::add_interface(const string& ifname,
					uint32_t if_index,
					string& error_msg)
{
    debug_msg("add_interface "
	      "(ifname = %s if_index = %u)\n",
	      ifname.c_str(), if_index);

    UNUSED(ifname);
    UNUSED(if_index);

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetNetlinkSocket::add_vif(const string& ifname,
				  const string& vifname,
				  uint32_t if_index,
				  string& error_msg)
{
    debug_msg("add_vif "
	      "(ifname = %s vifname = %s if_index = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index);

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetNetlinkSocket::config_interface(const string& ifname,
					   uint32_t if_index,
					   uint32_t flags,
					   bool is_up,
					   bool is_deleted,
					   string& error_msg)
{
    debug_msg("config_interface "
	      "(ifname = %s if_index = %u flags = 0x%x is_up = %s "
	      "is_deleted = %s)\n",
	      ifname.c_str(), if_index,
	      XORP_UINT_CAST(flags),
	      bool_c_str(is_up),
	      bool_c_str(is_deleted));

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(flags);
    UNUSED(is_up);
    UNUSED(is_deleted);

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetNetlinkSocket::config_vif(const string& ifname,
				     const string& vifname,
				     uint32_t if_index,
				     uint32_t flags,
				     bool is_up,
				     bool is_deleted,
				     bool broadcast,
				     bool loopback,
				     bool point_to_point,
				     bool multicast,
				     string& error_msg)
{
    debug_msg("config_vif "
	      "(ifname = %s vifname = %s if_index = %u flags = 0x%x "
	      "is_up = %s is_deleted = %s broadcast = %s loopback = %s "
	      "point_to_point = %s multicast = %s)\n",
	      ifname.c_str(), vifname.c_str(), if_index,
	      XORP_UINT_CAST(flags),
	      bool_c_str(is_up),
	      bool_c_str(is_deleted),
	      bool_c_str(broadcast),
	      bool_c_str(loopback),
	      bool_c_str(point_to_point),
	      bool_c_str(multicast));

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(flags);
    UNUSED(is_up);
    UNUSED(is_deleted);
    UNUSED(broadcast);
    UNUSED(loopback);
    UNUSED(point_to_point);
    UNUSED(multicast);

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetNetlinkSocket::set_interface_mac_address(const string& ifname,
						    uint32_t if_index,
						    const struct ether_addr& ether_addr,
						    string& error_msg)
{
    debug_msg("set_interface_mac "
	      "(ifname = %s if_index = %u mac = %s)\n",
	      ifname.c_str(), if_index, EtherMac(ether_addr).str().c_str());

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(ether_addr);

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetNetlinkSocket::set_interface_mtu(const string& ifname,
					    uint32_t if_index,
					    uint32_t mtu,
					    string& error_msg)
{
    debug_msg("set_interface_mtu "
	      "(ifname = %s if_index = %u mtu = %u)\n",
	      ifname.c_str(), if_index, XORP_UINT_CAST(mtu));

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(mtu);

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetNetlinkSocket::add_vif_address(const string& ifname,
					  const string& vifname,
					  uint32_t if_index,
					  bool is_broadcast,
					  bool is_p2p,
					  const IPvX& addr,
					  const IPvX& dst_or_bcast,
					  uint32_t prefix_len,
					  string& error_msg)
{
    debug_msg("add_vif_address "
	      "(ifname = %s vifname = %s if_index = %u is_broadcast = %s "
	      "is_p2p = %s addr = %s dst/bcast = %s prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index,
	      bool_c_str(is_broadcast), bool_c_str(is_p2p),
	      addr.str().c_str(), dst_or_bcast.str().c_str(),
	      XORP_UINT_CAST(prefix_len));

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(is_broadcast);
    UNUSED(is_p2p);
    UNUSED(addr);
    UNUSED(dst_or_bcast);
    UNUSED(prefix_len);

    error_msg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetNetlinkSocket::delete_vif_address(const string& ifname,
					     const string& vifname,
					     uint32_t if_index,
					     const IPvX& addr,
					     uint32_t prefix_len,
					     string& error_msg)
{
    debug_msg("delete_vif_address "
	      "(ifname = %s vifname = %s if_index = %u addr = %s "
	      "prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index, addr.str().c_str(),
	      XORP_UINT_CAST(prefix_len));

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(addr);
    UNUSED(prefix_len);

    error_msg = "method not supported";

    return (XORP_ERROR);
}

#else // HAVE_NETLINK_SOCKETS

int
IfConfigSetNetlinkSocket::config_begin(string& error_msg)
{
    debug_msg("config_begin\n");

    // XXX: nothing to do

    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetNetlinkSocket::config_end(string& error_msg)
{
    debug_msg("config_end\n");

    // XXX: nothing to do

    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetNetlinkSocket::add_interface(const string& ifname,
					uint32_t if_index,
					string& error_msg)
{
    debug_msg("add_interface "
	      "(ifname = %s if_index = %u)\n",
	      ifname.c_str(), if_index);

    // XXX: nothing to do

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetNetlinkSocket::add_vif(const string& ifname,
				  const string& vifname,
				  uint32_t if_index,
				  string& error_msg)
{
    debug_msg("add_vif "
	      "(ifname = %s vifname = %s if_index = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index);

    // XXX: nothing to do

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetNetlinkSocket::config_interface(const string& ifname,
					   uint32_t if_index,
					   uint32_t flags,
					   bool is_up,
					   bool is_deleted,
					   string& error_msg)
{
    debug_msg("config_interface "
	      "(ifname = %s if_index = %u flags = 0x%x is_up = %s "
	      "is_deleted = %s)\n",
	      ifname.c_str(), if_index, flags, bool_c_str(is_up),
	      bool_c_str(is_deleted));

#ifndef HAVE_NETLINK_SOCKETS_SET_FLAGS_IS_BROKEN
    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct ifinfomsg) + 2*sizeof(struct rtattr) + 512;
    union {
	uint8_t		data[buffer_size];
	struct nlmsghdr	nlh;
    } buffer;
    struct nlmsghdr*	nlh = &buffer.nlh;
    struct sockaddr_nl	snl;
    struct ifinfomsg*	ifinfomsg;
    NetlinkSocket&	ns = *this;
    int			last_errno = 0;

    UNUSED(ifname);
    UNUSED(is_up);
    UNUSED(is_deleted);

    memset(&buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    //
    // Set the request
    //
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifinfomsg));
    nlh->nlmsg_type = RTM_NEWLINK;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.nl_pid();
    ifinfomsg = static_cast<struct ifinfomsg*>(NLMSG_DATA(nlh));
    ifinfomsg->ifi_family = AF_UNSPEC;
    ifinfomsg->ifi_type = IFLA_UNSPEC;
    ifinfomsg->ifi_index = if_index;
    ifinfomsg->ifi_flags = flags;
    ifinfomsg->ifi_change = 0xffffffff;

    if (NLMSG_ALIGN(nlh->nlmsg_len) > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		   XORP_UINT_CAST(sizeof(buffer)),
		   XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len)));
    }
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len);

    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
		  reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	error_msg = c_format("error writing to netlink socket: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }
    if (NlmUtils::check_netlink_request(_ns_reader, ns, nlh->nlmsg_seq,
					last_errno, error_msg) < 0) {
	return (XORP_ERROR);
    }
    return (XORP_OK);

#else // HAVE_NETLINK_SOCKETS_SET_FLAGS_IS_BROKEN
    //
    // XXX: a work-around in case the kernel doesn't support setting
    // the MTU on an interface by using netlink.
    // In this case, the work-around is to use ioctl(). Sigh...
    //
    struct ifreq ifreq;
    int s = socket(AF_INET, SOCK_DGRAM, 0);

    UNUSED(is_up);
    UNUSED(is_deleted);

    if (s < 0) {
	XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
    }

    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, ifname.c_str(), sizeof(ifreq.ifr_name) - 1);
    ifreq.ifr_flags = flags;
    if (ioctl(s, SIOCSIFFLAGS, &ifreq) < 0) {
	error_msg = c_format("%s", strerror(errno));
	close(s);
	return (XORP_ERROR);
    }
    close(s);

    return (XORP_OK);
#endif // HAVE_NETLINK_SOCKETS_SET_FLAGS_IS_BROKEN
}

int
IfConfigSetNetlinkSocket::config_vif(const string& ifname,
				     const string& vifname,
				     uint32_t if_index,
				     uint32_t flags,
				     bool is_up,
				     bool is_deleted,
				     bool broadcast,
				     bool loopback,
				     bool point_to_point,
				     bool multicast,
				     string& error_msg)
{
    debug_msg("config_vif "
	      "(ifname = %s vifname = %s if_index = %u flags = 0x%x "
	      "is_up = %s is_deleted = %s broadcast = %s loopback = %s "
	      "point_to_point = %s multicast = %s)\n",
	      ifname.c_str(), vifname.c_str(), if_index, flags,
	      bool_c_str(is_up),
	      bool_c_str(is_deleted),
	      bool_c_str(broadcast),
	      bool_c_str(loopback),
	      bool_c_str(point_to_point),
	      bool_c_str(multicast));

    // XXX: nothing to do

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(flags);
    UNUSED(is_up);
    UNUSED(is_deleted);
    UNUSED(broadcast);
    UNUSED(loopback);
    UNUSED(point_to_point);
    UNUSED(multicast);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetNetlinkSocket::set_interface_mac_address(const string& ifname,
						    uint32_t if_index,
						    const struct ether_addr& ether_addr,
						    string& error_msg)
{
    debug_msg("set_interface_mac "
	      "(ifname = %s if_index = %u mac = %s)\n",
	      ifname.c_str(), if_index, EtherMac(ether_addr).str().c_str());

#ifdef RTM_SETLINK
    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct ifinfomsg) + 2*sizeof(struct rtattr) + 512;
    union {
	uint8_t		data[buffer_size];
	struct nlmsghdr	nlh;
    } buffer;
    struct nlmsghdr*	nlh = &buffer.nlh;
    struct sockaddr_nl	snl;
    struct ifinfomsg*	ifinfomsg;
    struct rtattr*	rtattr;
    int			rta_len;
    NetlinkSocket&	ns = *this;
    int			last_errno = 0;

    UNUSED(ifname);

    memset(&buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    //
    // Set the request
    //
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifinfomsg));
    nlh->nlmsg_type = RTM_SETLINK;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.nl_pid();
    ifinfomsg = static_cast<struct ifinfomsg*>(NLMSG_DATA(nlh));
    ifinfomsg->ifi_family = AF_UNSPEC;
    ifinfomsg->ifi_type = IFLA_UNSPEC;	// TODO: set to ARPHRD_ETHER ??
    ifinfomsg->ifi_index = if_index;
    ifinfomsg->ifi_flags = 0;
    ifinfomsg->ifi_change = 0xffffffff;

    // Add the MAC address as an attribute
    rta_len = RTA_LENGTH(ETH_ALEN);
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		   XORP_UINT_CAST(sizeof(buffer)),
		   XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
    }
    rtattr = IFLA_RTA(ifinfomsg);
    rtattr->rta_type = IFLA_ADDRESS;
    rtattr->rta_len = rta_len;
    memcpy(RTA_DATA(rtattr), &ether_addr, ETH_ALEN);
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
		  reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	error_msg = c_format("error writing to netlink socket: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }
    if (NlmUtils::check_netlink_request(_ns_reader, ns, nlh->nlmsg_seq,
					last_errno, error_msg) < 0) {
	return (XORP_ERROR);
    }
    return (XORP_OK);

#elif defined(SIOCSIFHWADDR)
    //
    // XXX: a work-around in case the kernel doesn't support setting
    // the MAC address on an interface by using netlink.
    // In this case, the work-around is to use ioctl(). Sigh...
    //
    struct ifreq ifreq;
    int s = socket(AF_INET, SOCK_DGRAM, 0);

    UNUSED(if_index);

    if (s < 0) {
	XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
    }

    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, ifname.c_str(), sizeof(ifreq.ifr_name) - 1);
    ifreq.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    memcpy(ifreq.ifr_hwaddr.sa_data, &ether_addr, ETH_ALEN);
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    ifreq.ifr_hwaddr.sa_len = ETH_ALEN;
#endif
    if (ioctl(s, SIOCSIFHWADDR, &ifreq) < 0) {
	error_msg = c_format("%s", strerror(errno));
	close(s);
	return (XORP_ERROR);
    }
    close(s);

    return (XORP_OK);

#else
#error No mechanism to set the MAC address on an interface
#endif
}

int
IfConfigSetNetlinkSocket::set_interface_mtu(const string& ifname,
					    uint32_t if_index,
					    uint32_t mtu,
					    string& error_msg)
{
    debug_msg("set_interface_mtu "
	      "(ifname = %s if_index = %u mtu = %u)\n",
	      ifname.c_str(), if_index, mtu);

#ifndef HAVE_NETLINK_SOCKETS_SET_MTU_IS_BROKEN
    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct ifinfomsg) + 2*sizeof(struct rtattr) + 512;
    union {
	uint8_t		data[buffer_size];
	struct nlmsghdr	nlh;
    } buffer;
    struct nlmsghdr*	nlh = &buffer.nlh;
    struct sockaddr_nl	snl;
    struct ifinfomsg*	ifinfomsg;
    struct rtattr*	rtattr;
    int			rta_len;
    NetlinkSocket&	ns = *this;
    int			last_errno = 0;

    UNUSED(ifname);

    memset(&buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    //
    // Set the request
    //
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifinfomsg));
    nlh->nlmsg_type = RTM_NEWLINK;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.nl_pid();
    ifinfomsg = static_cast<struct ifinfomsg*>(NLMSG_DATA(nlh));
    ifinfomsg->ifi_family = AF_UNSPEC;
    ifinfomsg->ifi_type = IFLA_UNSPEC;
    ifinfomsg->ifi_index = if_index;
    ifinfomsg->ifi_flags = 0;
    ifinfomsg->ifi_change = 0xffffffff;

    // Add the MTU as an attribute
    unsigned int uint_mtu = mtu;
    rta_len = RTA_LENGTH(sizeof(unsigned int));
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		   XORP_UINT_CAST(sizeof(buffer)),
		   XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
    }
    rtattr = IFLA_RTA(ifinfomsg);
    rtattr->rta_type = IFLA_MTU;
    rtattr->rta_len = rta_len;
    memcpy(RTA_DATA(rtattr), &uint_mtu, sizeof(uint_mtu));
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
		  reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	error_msg = c_format("error writing to netlink socket: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }
    if (NlmUtils::check_netlink_request(_ns_reader, ns, nlh->nlmsg_seq,
					last_errno, error_msg) < 0) {
	return (XORP_ERROR);
    }
    return (XORP_OK);

#else // HAVE_NETLINK_SOCKETS_SET_MTU_IS_BROKEN
    //
    // XXX: a work-around in case the kernel doesn't support setting
    // the MTU on an interface by using netlink.
    // In this case, the work-around is to use ioctl(). Sigh...
    //
    struct ifreq ifreq;
    int s = socket(AF_INET, SOCK_DGRAM, 0);

    UNUSED(if_index);

    if (s < 0) {
	XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
    }

    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, ifname.c_str(), sizeof(ifreq.ifr_name) - 1);
    ifreq.ifr_mtu = mtu;
    if (ioctl(s, SIOCSIFMTU, &ifreq) < 0) {
	error_msg = c_format("%s", strerror(errno));
	close(s);
	return (XORP_ERROR);
    }
    close(s);

    return (XORP_OK);
#endif // HAVE_NETLINK_SOCKETS_SET_MTU_IS_BROKEN
}

int
IfConfigSetNetlinkSocket::add_vif_address(const string& ifname,
					  const string& vifname,
					  uint32_t if_index,
					  bool is_broadcast,
					  bool is_p2p,
					  const IPvX& addr,
					  const IPvX& dst_or_bcast,
					  uint32_t prefix_len,
					  string& error_msg)
{
    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct ifinfomsg) + 2*sizeof(struct rtattr) + 512;
    union {
	uint8_t		data[buffer_size];
	struct nlmsghdr	nlh;
    } buffer;
    struct nlmsghdr*	nlh = &buffer.nlh;
    struct sockaddr_nl	snl;
    struct ifaddrmsg*	ifaddrmsg;
    struct rtattr*	rtattr;
    int			rta_len;
    uint8_t*		data;
    NetlinkSocket&	ns = *this;
    void*		rta_align_data;
    int			last_errno = 0;

    debug_msg("add_vif_address "
	      "(ifname = %s vifname = %s if_index = %u is_broadcast = %s "
	      "is_p2p = %s addr = %s dst/bcast = %s prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index,
	      bool_c_str(is_broadcast),
	      bool_c_str(is_p2p), addr.str().c_str(),
	      dst_or_bcast.str().c_str(), prefix_len);

    UNUSED(ifname);
    UNUSED(vifname);

    // Check that the family is supported
    switch (addr.af()) {
    case AF_INET:
	if (! ifconfig().have_ipv4()) {
	    error_msg = "IPv4 is not supported";
	    return (XORP_ERROR);
	}
	break;

#ifdef HAVE_IPV6
    case AF_INET6:
	if (! ifconfig().have_ipv6()) {
	    error_msg = "IPv6 is not supported";
	    return (XORP_ERROR);
	}
	break;
#endif // HAVE_IPV6

    default:
	XLOG_UNREACHABLE();
	break;
    }

    memset(&buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    //
    // Set the request
    //
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifaddrmsg));
    nlh->nlmsg_type = RTM_NEWADDR;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.nl_pid();
    ifaddrmsg = static_cast<struct ifaddrmsg*>(NLMSG_DATA(nlh));
    ifaddrmsg->ifa_family = addr.af();
    ifaddrmsg->ifa_prefixlen = prefix_len;
    ifaddrmsg->ifa_flags = 0;	// TODO: XXX: PAVPAVPAV: OK if 0?
    ifaddrmsg->ifa_scope = 0;	// TODO: XXX: PAVPAVPAV: OK if 0?
    ifaddrmsg->ifa_index = if_index;

    // Add the address as an attribute
    rta_len = RTA_LENGTH(addr.addr_bytelen());
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		   XORP_UINT_CAST(sizeof(buffer)),
		   XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
    }
    rtattr = IFA_RTA(ifaddrmsg);
    rtattr->rta_type = IFA_LOCAL;
    rtattr->rta_len = rta_len;
    data = static_cast<uint8_t*>(RTA_DATA(rtattr));
    addr.copy_out(data);
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    if (is_broadcast || is_p2p) {
	// Set the p2p or broadcast address	
	rta_len = RTA_LENGTH(dst_or_bcast.addr_bytelen());
	if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	    XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		       XORP_UINT_CAST(sizeof(buffer)),
		       XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
	}
	rta_align_data = reinterpret_cast<char*>(rtattr)
	    + RTA_ALIGN(rtattr->rta_len);
	rtattr = static_cast<struct rtattr*>(rta_align_data);
	rtattr->rta_type = IFA_UNSPEC;
	rtattr->rta_len = rta_len;
	data = static_cast<uint8_t*>(RTA_DATA(rtattr));
	dst_or_bcast.copy_out(data);
	nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;
	if (is_p2p) {
	    rtattr->rta_type = IFA_ADDRESS;
	} else {
	    rtattr->rta_type = IFA_BROADCAST;
	}
    }

    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
		  reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	error_msg = c_format("error writing to netlink socket: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }
    if (NlmUtils::check_netlink_request(_ns_reader, ns, nlh->nlmsg_seq,
					last_errno, error_msg) < 0) {
	return (XORP_ERROR);
    }
    return (XORP_OK);
}

int
IfConfigSetNetlinkSocket::delete_vif_address(const string& ifname,
					     const string& vifname,
					     uint32_t if_index,
					     const IPvX& addr,
					     uint32_t prefix_len,
					     string& error_msg)
{
    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct ifinfomsg) + 2*sizeof(struct rtattr) + 512;
    union {
	uint8_t		data[buffer_size];
	struct nlmsghdr	nlh;
    } buffer;
    struct nlmsghdr*	nlh = &buffer.nlh;
    struct sockaddr_nl	snl;
    struct ifaddrmsg*	ifaddrmsg;
    struct rtattr*	rtattr;
    int			rta_len;
    uint8_t*		data;
    NetlinkSocket&	ns = *this;
    int			last_errno = 0;

    debug_msg("delete_vif_address "
	      "(ifname = %s vifname = %s if_index = %u addr = %s "
	      "prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index, addr.str().c_str(),
	      prefix_len);

    UNUSED(ifname);
    UNUSED(vifname);

    // Check that the family is supported
    switch (addr.af()) {
    case AF_INET:
	if (! ifconfig().have_ipv4()) {
	    error_msg = "IPv4 is not supported";
	    return (XORP_ERROR);
	}
	break;

#ifdef HAVE_IPV6
    case AF_INET6:
	if (! ifconfig().have_ipv6()) {
	    error_msg = "IPv6 is not supported";
	    return (XORP_ERROR);
	}
	break;
#endif // HAVE_IPV6

    default:
	XLOG_UNREACHABLE();
	break;
    }

    memset(&buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    //
    // Set the request
    //
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifaddrmsg));
    nlh->nlmsg_type = RTM_DELADDR;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.nl_pid();
    ifaddrmsg = static_cast<struct ifaddrmsg*>(NLMSG_DATA(nlh));
    ifaddrmsg->ifa_family = addr.af();
    ifaddrmsg->ifa_prefixlen = prefix_len;
    ifaddrmsg->ifa_flags = 0;	// TODO: XXX: PAVPAVPAV: OK if 0?
    ifaddrmsg->ifa_scope = 0;	// TODO: XXX: PAVPAVPAV: OK if 0?
    ifaddrmsg->ifa_index = if_index;

    // Add the address as an attribute
    rta_len = RTA_LENGTH(addr.addr_bytelen());
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		   XORP_UINT_CAST(sizeof(buffer)),
		   XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
    }
    rtattr = IFA_RTA(ifaddrmsg);
    rtattr->rta_type = IFA_LOCAL;
    rtattr->rta_len = rta_len;
    data = static_cast<uint8_t*>(RTA_DATA(rtattr));
    addr.copy_out(data);
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
		  reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	error_msg = c_format("error writing to netlink socket: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }
    if (NlmUtils::check_netlink_request(_ns_reader, ns, nlh->nlmsg_seq,
					last_errno, error_msg) < 0) {
	return (XORP_ERROR);
    }
    return (XORP_OK);
}

#endif // HAVE_NETLINK_SOCKETS
