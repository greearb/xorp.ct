// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/fea/ifconfig_set_netlink.cc,v 1.6 2003/10/13 23:32:41 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ether_compat.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include <net/if.h>
#include <net/if_arp.h>

// TODO: XXX: PAVPAVPAV: move this include somewhere else!!
#ifdef HOST_OS_LINUX
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "ifconfig.hh"
#include "ifconfig_set.hh"
#include "netlink_socket_utils.hh"


//
// Set information about network interfaces configuration with the
// underlying system.
//
// The mechanism to set the information is netlink(7) sockets.
//

IfConfigSetNetlink::IfConfigSetNetlink(IfConfig& ifc)
    : IfConfigSet(ifc),
      NetlinkSocket4(ifc.eventloop()),
      NetlinkSocket6(ifc.eventloop()),
      _ns_reader(*(NetlinkSocket4 *)this, *(NetlinkSocket6 *)this)
{
#ifdef HAVE_NETLINK_SOCKETS
    register_ifc();
#endif
}

IfConfigSetNetlink::~IfConfigSetNetlink()
{
    stop();
}

int
IfConfigSetNetlink::start()
{
    if (NetlinkSocket4::start() < 0)
	return (XORP_ERROR);
    
#ifdef HAVE_IPV6
    if (NetlinkSocket6::start() < 0) {
	NetlinkSocket4::stop();
	return (XORP_ERROR);
    }
#endif

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigSetNetlink::stop()
{
    int ret_value4 = XORP_OK;
    int ret_value6 = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    ret_value4 = NetlinkSocket4::stop();
    
#ifdef HAVE_IPV6
    ret_value6 = NetlinkSocket6::stop();
#endif
    
    if ((ret_value4 < 0) || (ret_value6 < 0))
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

#ifndef HAVE_NETLINK_SOCKETS
int
IfConfigSetNetlink::set_interface_mac_address(const string& ifname,
					      uint16_t if_index,
					      const struct ether_addr& ether_addr,
					      string& reason)
{
    debug_msg("set_interface_mac "
	      "(ifname = %s if_index = %u mac = %s)\n",
	      ifname.c_str(), if_index, EtherMac(ether_addr).str().c_str());

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(ether_addr);

    reason = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetNetlink::set_interface_mtu(const string& ifname,
				      uint16_t if_index,
				      uint32_t mtu,
				      string& reason)
{
    debug_msg("set_interface_mtu "
	      "(ifname = %s if_index = %u mtu = %u)\n",
	      ifname.c_str(), if_index, mtu);

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(mtu);

    reason = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetNetlink::set_interface_flags(const string& ifname,
					uint16_t if_index,
					uint32_t flags,
					string& reason)
{
    debug_msg("set_interface_flags "
	      "(ifname = %s if_index = %u flags = 0x%x)\n",
	      ifname.c_str(), if_index, flags);

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(flags);

    reason = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetNetlink::set_vif_address(const string& ifname,
				    uint16_t if_index,
				    bool is_broadcast,
				    bool is_p2p,
				    const IPvX& addr,
				    const IPvX& dst_or_bcast,
				    uint32_t prefix_len,
				    string& reason)
{
    debug_msg("set_vif_address "
	      "(ifname = %s if_index = %u is_broadcast = %s is_p2p = %s "
	      "addr = %s dst/bcast = %s prefix_len = %u)\n",
	      ifname.c_str(), if_index, (is_broadcast)? "true" : "false",
	      (is_p2p)? "true" : "false", addr.str().c_str(),
	      dst_or_bcast.str().c_str(), prefix_len);

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(is_broadcast);
    UNUSED(is_p2p);
    UNUSED(addr);
    UNUSED(dst_or_bcast);
    UNUSED(prefix_len);

    reason = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetNetlink::delete_vif_address(const string& ifname,
				       uint16_t if_index,
				       const IPvX& addr,
				       uint32_t prefix_len,
				       string& reason)
{
    debug_msg("delete_vif_address "
	      "(ifname = %s if_index = %u addr = %s prefix_len = %u)\n",
	      ifname.c_str(), if_index, addr.str().c_str(), prefix_len);

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(addr);
    UNUSED(prefix_len);

    reason = "method not supported";

    return (XORP_ERROR);
}

#else // HAVE_NETLINK_SOCKETS

int
IfConfigSetNetlink::set_interface_mac_address(const string& ifname,
					      uint16_t if_index,
					      const struct ether_addr& ether_addr,
					      string& reason)
{
    debug_msg("set_interface_mac "
	      "(ifname = %s if_index = %u mac = %s)\n",
	      ifname.c_str(), if_index, EtherMac(ether_addr).str().c_str());

#ifdef RTM_SETLINK
    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct ifinfomsg) + 2*sizeof(struct rtattr) + 512;
    uint8_t		buffer[buffer_size];
    struct sockaddr_nl	snl;
    struct nlmsghdr*	nlh;
    struct ifinfomsg*	ifinfomsg;
    struct rtattr*	rtattr;
    int			rta_len;
    NetlinkSocket4&	ns4 = *this;

    UNUSED(ifname);

    memset(buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    //
    // Set the request
    //
    nlh = reinterpret_cast<struct nlmsghdr*>(buffer);
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifinfomsg));
    nlh->nlmsg_type = RTM_SETLINK;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns4.seqno();
    nlh->nlmsg_pid = ns4.pid();
    ifinfomsg = reinterpret_cast<struct ifinfomsg*>(NLMSG_DATA(nlh));
    ifinfomsg->ifi_family = AF_UNSPEC;
    ifinfomsg->ifi_type = IFLA_UNSPEC;	// TODO: set to ARPHRD_ETHER ??
    ifinfomsg->ifi_index = if_index;
    ifinfomsg->ifi_flags = 0;
    ifinfomsg->ifi_change = 0xffffffff;

    // Add the MAC address as an attribute
    rta_len = RTA_LENGTH(ETH_ALEN);
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %d instead of %d",
		   sizeof(buffer), NLMSG_ALIGN(nlh->nlmsg_len) + rta_len);
    }
    rtattr = IFLA_RTA(ifinfomsg);
    rtattr->rta_type = IFLA_ADDRESS;
    rtattr->rta_len = rta_len;
    memcpy(RTA_DATA(rtattr), &ether_addr, ETH_ALEN);
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    if (ns4.sendto(buffer, nlh->nlmsg_len, 0,
		   reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	reason = c_format("error writing to netlink socket: %s",
			  strerror(errno));
	return (XORP_ERROR);
    }
    if (NlmUtils::check_netlink_request(_ns_reader, ns4, nlh->nlmsg_seq,
					reason) < 0) {
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
#ifdef HAVE_SA_LEN
    ifreq.ifr_hwaddr.sa_len = ETH_ALEN;
#endif
    if (ioctl(s, SIOCSIFHWADDR, &ifreq) < 0) {
	reason = c_format("%s", strerror(errno));
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
IfConfigSetNetlink::set_interface_mtu(const string& ifname,
				      uint16_t if_index,
				      uint32_t mtu,
				      string& reason)
{
    debug_msg("set_interface_mtu "
	      "(ifname = %s if_index = %u mtu = %u)\n",
	      ifname.c_str(), if_index, mtu);

#ifndef HAVE_NETLINK_SOCKETS_SET_MTU_IS_BROKEN
    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct ifinfomsg) + 2*sizeof(struct rtattr) + 512;
    uint8_t		buffer[buffer_size];
    struct sockaddr_nl	snl;
    struct nlmsghdr*	nlh;
    struct ifinfomsg*	ifinfomsg;
    struct rtattr*	rtattr;
    int			rta_len;
    NetlinkSocket4&	ns4 = *this;

    UNUSED(ifname);

    memset(buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    //
    // Set the request
    //
    nlh = reinterpret_cast<struct nlmsghdr*>(buffer);
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifinfomsg));
    nlh->nlmsg_type = RTM_NEWLINK;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns4.seqno();
    nlh->nlmsg_pid = ns4.pid();
    ifinfomsg = reinterpret_cast<struct ifinfomsg*>(NLMSG_DATA(nlh));
    ifinfomsg->ifi_family = AF_UNSPEC;
    ifinfomsg->ifi_type = IFLA_UNSPEC;
    ifinfomsg->ifi_index = if_index;
    ifinfomsg->ifi_flags = 0;
    ifinfomsg->ifi_change = 0xffffffff;

    // Add the MTU as an attribute
    unsigned int uint_mtu = mtu;
    rta_len = RTA_LENGTH(sizeof(unsigned int));
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %d instead of %d",
		   sizeof(buffer), NLMSG_ALIGN(nlh->nlmsg_len) + rta_len);
    }
    rtattr = IFLA_RTA(ifinfomsg);
    rtattr->rta_type = IFLA_MTU;
    rtattr->rta_len = rta_len;
    memcpy(RTA_DATA(rtattr), &uint_mtu, sizeof(uint_mtu));
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    if (ns4.sendto(buffer, nlh->nlmsg_len, 0,
		   reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	reason = c_format("error writing to netlink socket: %s",
			  strerror(errno));
	return (XORP_ERROR);
    }
    if (NlmUtils::check_netlink_request(_ns_reader, ns4, nlh->nlmsg_seq,
					reason) < 0) {
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
	reason = c_format("%s", strerror(errno));
	close(s);
	return (XORP_ERROR);
    }
    close(s);

    return (XORP_OK);
#endif // HAVE_NETLINK_SOCKETS_SET_MTU_IS_BROKEN
}

int
IfConfigSetNetlink::set_interface_flags(const string& ifname,
					uint16_t if_index,
					uint32_t flags,
					string& reason)
{
    debug_msg("set_interface_flags "
	      "(ifname = %s if_index = %u flags = 0x%x)\n",
	      ifname.c_str(), if_index, flags);

#ifndef HAVE_NETLINK_SOCKETS_SET_FLAGS_IS_BROKEN
    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct ifinfomsg) + 2*sizeof(struct rtattr) + 512;
    uint8_t		buffer[buffer_size];
    struct sockaddr_nl	snl;
    struct nlmsghdr*	nlh;
    struct ifinfomsg*	ifinfomsg;
    NetlinkSocket4&	ns4 = *this;

    UNUSED(ifname);

    memset(buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    //
    // Set the request
    //
    nlh = reinterpret_cast<struct nlmsghdr*>(buffer);
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifinfomsg));
    nlh->nlmsg_type = RTM_NEWLINK;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns4.seqno();
    nlh->nlmsg_pid = ns4.pid();
    ifinfomsg = reinterpret_cast<struct ifinfomsg*>(NLMSG_DATA(nlh));
    ifinfomsg->ifi_family = AF_UNSPEC;
    ifinfomsg->ifi_type = IFLA_UNSPEC;
    ifinfomsg->ifi_index = if_index;
    ifinfomsg->ifi_flags = flags;
    ifinfomsg->ifi_change = 0xffffffff;

    if (NLMSG_ALIGN(nlh->nlmsg_len) > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %d instead of %d",
		   sizeof(buffer), NLMSG_ALIGN(nlh->nlmsg_len));
    }
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len);

    if (ns4.sendto(buffer, nlh->nlmsg_len, 0,
		   reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	reason = c_format("error writing to netlink socket: %s",
			  strerror(errno));
	return (XORP_ERROR);
    }
    if (NlmUtils::check_netlink_request(_ns_reader, ns4, nlh->nlmsg_seq,
					reason) < 0) {
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

    if (s < 0) {
	XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
    }

    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, ifname.c_str(), sizeof(ifreq.ifr_name) - 1);
    ifreq.ifr_flags = flags;
    if (ioctl(s, SIOCSIFFLAGS, &ifreq) < 0) {
	reason = c_format("%s", strerror(errno));
	close(s);
	return (XORP_ERROR);
    }
    close(s);

    return (XORP_OK);
#endif // HAVE_NETLINK_SOCKETS_SET_FLAGS_IS_BROKEN
}

int
IfConfigSetNetlink::set_vif_address(const string& ifname,
				    uint16_t if_index,
				    bool is_broadcast,
				    bool is_p2p,
				    const IPvX& addr,
				    const IPvX& dst_or_bcast,
				    uint32_t prefix_len,
				    string& reason)
{
    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct ifinfomsg) + 2*sizeof(struct rtattr) + 512;
    uint8_t		buffer[buffer_size];
    struct sockaddr_nl	snl;
    struct nlmsghdr*	nlh;
    struct ifaddrmsg*	ifaddrmsg;
    struct rtattr*	rtattr;
    int			rta_len;
    uint8_t*		data;
    NetlinkSocket*	ns_ptr = NULL;

    debug_msg("set_vif_address "
	      "(ifname = %s if_index = %u is_broadcast = %s is_p2p = %s "
	      "addr = %s dst/bcast = %s prefix_len = %u)\n",
	      ifname.c_str(), if_index, (is_broadcast)? "true" : "false",
	      (is_p2p)? "true" : "false", addr.str().c_str(),
	      dst_or_bcast.str().c_str(), prefix_len);

    UNUSED(ifname);

    memset(buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    // Get the pointer to the NetlinkSocket
    switch(addr.af()) {
    case AF_INET:
    {
	NetlinkSocket4& ns4 = *this;
	ns_ptr = &ns4;
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	NetlinkSocket6& ns6 = *this;
	ns_ptr = &ns6;
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }

    //
    // Set the request
    //
    nlh = reinterpret_cast<struct nlmsghdr*>(buffer);
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifaddrmsg));
    nlh->nlmsg_type = RTM_NEWADDR;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns_ptr->seqno();
    nlh->nlmsg_pid = ns_ptr->pid();
    ifaddrmsg = reinterpret_cast<struct ifaddrmsg*>(NLMSG_DATA(nlh));
    ifaddrmsg->ifa_family = addr.af();
    ifaddrmsg->ifa_prefixlen = prefix_len;
    ifaddrmsg->ifa_flags = 0;	// TODO: XXX: PAVPAVPAV: OK if 0?
    ifaddrmsg->ifa_scope = 0;	// TODO: XXX: PAVPAVPAV: OK if 0?
    ifaddrmsg->ifa_index = if_index;

    // Add the address as an attribute
    rta_len = RTA_LENGTH(addr.addr_size());
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %d instead of %d",
		   sizeof(buffer), NLMSG_ALIGN(nlh->nlmsg_len) + rta_len);
    }
    rtattr = IFA_RTA(ifaddrmsg);
    rtattr->rta_type = IFA_LOCAL;
    rtattr->rta_len = rta_len;
    data = reinterpret_cast<uint8_t*>(RTA_DATA(rtattr));
    addr.copy_out(data);
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    if (is_broadcast || is_p2p) {
	// Set the p2p or broadcast address	
	rta_len = RTA_LENGTH(dst_or_bcast.addr_size());
	if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	    XLOG_FATAL("AF_NETLINK buffer size error: %d instead of %d",
		       sizeof(buffer), NLMSG_ALIGN(nlh->nlmsg_len) + rta_len);
	}
	rtattr = (struct rtattr*)(((char*)(rtattr)) + RTA_ALIGN((rtattr)->rta_len));
	rtattr->rta_type = IFA_UNSPEC;
	rtattr->rta_len = rta_len;
	data = reinterpret_cast<uint8_t*>(RTA_DATA(rtattr));
	dst_or_bcast.copy_out(data);
	nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;
	if (is_p2p) {
	    rtattr->rta_type = IFA_ADDRESS;
	} else {
	    rtattr->rta_type = IFA_BROADCAST;
	}
    }

    if (ns_ptr->sendto(buffer, nlh->nlmsg_len, 0,
		       reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	reason = c_format("error writing to netlink socket: %s",
			  strerror(errno));
	return (XORP_ERROR);
    }
    if (NlmUtils::check_netlink_request(_ns_reader, *ns_ptr, nlh->nlmsg_seq,
					reason) < 0) {
	return (XORP_ERROR);
    }
    return (XORP_OK);
}

int
IfConfigSetNetlink::delete_vif_address(const string& ifname,
				       uint16_t if_index,
				       const IPvX& addr,
				       uint32_t prefix_len,
				       string& reason)
{
    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct ifinfomsg) + 2*sizeof(struct rtattr) + 512;
    uint8_t		buffer[buffer_size];
    struct sockaddr_nl	snl;
    struct nlmsghdr*	nlh;
    struct ifaddrmsg*	ifaddrmsg;
    struct rtattr*	rtattr;
    int			rta_len;
    uint8_t*		data;
    NetlinkSocket*	ns_ptr = NULL;

    debug_msg("delete_vif_address "
	      "(ifname = %s if_index = %u addr = %s prefix_len = %u)\n",
	      ifname.c_str(), if_index, addr.str().c_str(), prefix_len);

    UNUSED(ifname);

    memset(buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    // Get the pointer to the NetlinkSocket
    switch(addr.af()) {
    case AF_INET:
    {
	NetlinkSocket4& ns4 = *this;
	ns_ptr = &ns4;
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	NetlinkSocket6& ns6 = *this;
	ns_ptr = &ns6;
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }

    //
    // Set the request
    //
    nlh = reinterpret_cast<struct nlmsghdr*>(buffer);
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifaddrmsg));
    nlh->nlmsg_type = RTM_DELADDR;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns_ptr->seqno();
    nlh->nlmsg_pid = ns_ptr->pid();
    ifaddrmsg = reinterpret_cast<struct ifaddrmsg*>(NLMSG_DATA(nlh));
    ifaddrmsg->ifa_family = addr.af();
    ifaddrmsg->ifa_prefixlen = prefix_len;
    ifaddrmsg->ifa_flags = 0;	// TODO: XXX: PAVPAVPAV: OK if 0?
    ifaddrmsg->ifa_scope = 0;	// TODO: XXX: PAVPAVPAV: OK if 0?
    ifaddrmsg->ifa_index = if_index;

    // Add the address as an attribute
    rta_len = RTA_LENGTH(addr.addr_size());
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %d instead of %d",
		   sizeof(buffer), NLMSG_ALIGN(nlh->nlmsg_len) + rta_len);
    }
    rtattr = IFA_RTA(ifaddrmsg);
    rtattr->rta_type = IFA_LOCAL;
    rtattr->rta_len = rta_len;
    data = reinterpret_cast<uint8_t*>(RTA_DATA(rtattr));
    addr.copy_out(data);
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    if (ns_ptr->sendto(buffer, nlh->nlmsg_len, 0,
		       reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	reason = c_format("error writing to netlink socket: %s",
			  strerror(errno));
	return (XORP_ERROR);
    }
    if (NlmUtils::check_netlink_request(_ns_reader, *ns_ptr, nlh->nlmsg_seq,
					reason) < 0) {
	return (XORP_ERROR);
    }
    return (XORP_OK);
}

#endif // HAVE_NETLINK_SOCKETS
