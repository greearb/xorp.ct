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

#ident "$XORP: xorp/fea/ifconfig_set_ioctl.cc,v 1.11 2003/10/01 21:10:04 pavlin Exp $"


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
      _s4(-1),
      _s6(-1)
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
    
    return (XORP_OK);
}

int
IfConfigSetNetlink::stop()
{
    int ret_value4 = XORP_OK;
    int ret_value6 = XORP_OK;
    
    ret_value4 = NetlinkSocket4::stop();
    
#ifdef HAVE_IPV6
    ret_value6 = NetlinkSocket6::stop();
#endif
    
    if ((ret_value4 < 0) || (ret_value6 < 0))
	return (XORP_ERROR);
    
    return (XORP_OK);
}

#ifndef HAVE_NETLINK_SOCKETS
bool
IfConfigSetNetlink::push_config(const IfTree&)
{
    return false;
}

#else // HAVE_NETLINK_SOCKETS

/**
 * @short Base class for Netlink operations
 */
class IfNetlink {
public:
    IfNetlink(IfConfigSetNetlink& ifc_set_netlink)
	: _ifc_set_netlink(ifc_set_netlink) {}
    virtual ~IfNetlink() {}
    virtual int execute() = 0;

protected:
    IfConfigSetNetlink& ifc_set_netlink() const { return _ifc_set_netlink; }

private:
    IfConfigSetNetlink&	_ifc_set_netlink;
};

/**
 * @short Base class for IPv4 configuration netlink writes.
 */
class IfReq : public IfNetlink {
public:
    IfReq(IfConfigSetNetlink& ifc_set_netlink, const string& ifname)
	: IfNetlink(ifc_set_netlink),
	  _ifname(ifname) {
	memset(_buffer, 0, sizeof(_buffer));

	// Set the socket
	memset(&_snl, 0, sizeof(_snl));
	_snl.nl_family = AF_NETLINK;
	_snl.nl_pid    = 0;	// nl_pid = 0 if destination is the kernel
	_snl.nl_groups = 0;
    }

protected:
    const string& ifname() const { return _ifname; }

    const string _ifname;

    static const size_t _buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct ifinfomsg) + 2*sizeof(struct rtattr) + 512;

    uint8_t _buffer[_buffer_size];
    struct sockaddr_nl	_snl;
};

/**
 * @short Class to set MAC address on an interface.
 */
class IfSetMac : public IfReq {
public:
    IfSetMac(IfConfigSetNetlink& ifc_set_netlink,
	     const string& ifname,
	     uint16_t if_index,
	     const struct ether_addr& ether_addr)
	: IfReq(ifc_set_netlink, ifname),
	  _if_index(if_index),
	  _ether_addr(ether_addr) {}

    int execute() {
#ifdef RTM_SETLINK
	struct nlmsghdr		*nlh;
	struct ifinfomsg	*ifinfomsg;
	struct rtattr		*rtattr;
	int			rta_len;
	NetlinkSocket4&		ns4 = *this;
	
	//
	// Set the request
	//
	nlh = reinterpret_cast<struct nlmsghdr*>(_buffer);
	nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifinfomsg));
	nlh->nlmsg_type = RTM_SETLINK;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE;
	nlh->nlmsg_seq = ns4.seqno();
	nlh->nlmsg_pid = ns4.pid();
	ifinfomsg = reinterpret_cast<struct ifinfomsg*>(NLMSG_DATA(nlh));
	ifinfomsg->ifi_family = AF_UNSPEC;
	ifinfomsg->ifi_type = IFLA_UNSPEC;	// TODO: set to ARPHRD_ETHER ??
	ifinfomsg->ifi_index = _if_index;
	ifinfomsg->ifi_flags = 0;
	ifinfomsg->ifi_change = 0xffffffff;
	
	// Add the MAC address as an attribute
	rta_len = RTA_LENGTH(ETH_ALEN);
	if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(_buffer)) {
	    XLOG_FATAL("AF_NETLINK buffer size error: %d instead of %d",
		       sizeof(_buffer), NLMSG_ALIGN(nlh->nlmsg_len) + rta_len);
	}
	rtattr = IFLA_RTA(ifinfomsg);
	rtattr->rta_type = IFLA_ADDRESS;
	rtattr->rta_len = rta_len;
	memcpy(RTA_DATA(rtattr), &_ether_addr, ETH_ALEN);
	nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;
	
	if (ns4.sendto(_buffer, nlh->nlmsg_len, 0,
		       reinterpret_cast<struct sockaddr*>(&_snl),
		       sizeof(_snl))
	    != (ssize_t)nlh->nlmsg_len) {
	    XLOG_ERROR("error writing to netlink socket: %s",
		       strerror(errno));
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
	int ret_value;
	int s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
	    XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
	}

	memset(&ifreq, 0, sizeof(ifreq));
	strncpy(ifreq.ifr_name, ifname().c_str(), sizeof(ifreq.ifr_name) - 1);
	ifreq.ifr_hwaddr.sa_family = ARPHRD_ETHER;
	memcpy(ifreq.ifr_hwaddr.sa_data, &_ether_addr, ETH_ALEN);
#ifdef HAVE_SA_LEN
	ifreq.ifr_hwaddr.sa_len = ETH_ALEN;
#endif
	ret_value = ioctl(s, SIOCSIFHWADDR, &ifreq);
	close(s);

	return (ret_value);

#else
#error No mechanism to set the MAC address on an interface
#endif
    }

private:
    uint16_t		_if_index;	// The interface index
    struct ether_addr	_ether_addr;	// The Ethernet address
};

/**
 * @short Class to set MTU on interface.
 */
class IfSetMTU : public IfReq {
public:
    IfSetMTU(IfConfigSetNetlink& ifc_set_netlink,
	     const string& ifname,
	     uint16_t if_index,
	     uint32_t mtu)
	: IfReq(ifc_set_netlink, ifname),
	  _if_index(if_index),
	  _mtu(mtu) {}
    
    int execute() {
#ifndef HAVE_NETLINK_SOCKETS_SET_MTU_IS_BROKEN
	struct nlmsghdr		*nlh;
	struct ifinfomsg	*ifinfomsg;
	struct rtattr		*rtattr;
	int			rta_len;
	NetlinkSocket4&		ns4 = ifc_set_netlink();
	
	//
	// Set the request
	//
	nlh = reinterpret_cast<struct nlmsghdr*>(_buffer);
	nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifinfomsg));
	nlh->nlmsg_type = RTM_NEWLINK;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE;
	nlh->nlmsg_seq = ns4.seqno();
	nlh->nlmsg_pid = ns4.pid();
	ifinfomsg = reinterpret_cast<struct ifinfomsg*>(NLMSG_DATA(nlh));
	ifinfomsg->ifi_family = AF_UNSPEC;
	ifinfomsg->ifi_type = IFLA_UNSPEC;
	ifinfomsg->ifi_index = _if_index;
	ifinfomsg->ifi_flags = 0;
	ifinfomsg->ifi_change = 0xffffffff;
	
	// Add the MTU as an attribute
	unsigned int uint_mtu = _mtu;
	rta_len = RTA_LENGTH(sizeof(unsigned int));
	if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(_buffer)) {
	    XLOG_FATAL("AF_NETLINK buffer size error: %d instead of %d",
		       sizeof(_buffer), NLMSG_ALIGN(nlh->nlmsg_len) + rta_len);
	}
	rtattr = IFLA_RTA(ifinfomsg);
	rtattr->rta_type = IFLA_MTU;
	rtattr->rta_len = rta_len;
	memcpy(RTA_DATA(rtattr), &uint_mtu, sizeof(uint_mtu));
	nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;
	
	if (ns4.sendto(_buffer, nlh->nlmsg_len, 0,
		       reinterpret_cast<struct sockaddr*>(&_snl),
		       sizeof(_snl))
	    != (ssize_t)nlh->nlmsg_len) {
	    XLOG_ERROR("error writing to netlink socket: %s",
		       strerror(errno));
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
	int ret_value;
	int s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
	    XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
	}

	memset(&ifreq, 0, sizeof(ifreq));
	strncpy(ifreq.ifr_name, ifname().c_str(), sizeof(ifreq.ifr_name) - 1);
	ifreq.ifr_mtu = _mtu;
	ret_value = ioctl(s, SIOCSIFMTU, &ifreq);
	close(s);

	return (ret_value);
#endif // HAVE_NETLINK_SOCKETS_SET_MTU_IS_BROKEN
    }

private:
    uint16_t		_if_index;	// The interface index
    uint32_t		_mtu;		// The MTU
};

/**
 * @short class to set interface flags
 */
class IfSetFlags : public IfReq {
public:
    IfSetFlags(IfConfigSetNetlink& ifc_set_netlink,
	       const string& ifname,
	       uint16_t if_index,
	       uint32_t flags)
	: IfReq(ifc_set_netlink, ifname),
	  _if_index(if_index),
	  _flags(flags) {}

    int execute() {
#ifndef HAVE_NETLINK_SOCKETS_SET_FLAGS_IS_BROKEN
	struct nlmsghdr		*nlh;
	struct ifinfomsg	*ifinfomsg;
	NetlinkSocket4&		ns4 = ifc_set_netlink();
	
	//
	// Set the request
	//
	nlh = reinterpret_cast<struct nlmsghdr*>(_buffer);
	nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifinfomsg));
	nlh->nlmsg_type = RTM_NEWLINK;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE;
	nlh->nlmsg_seq = ns4.seqno();
	nlh->nlmsg_pid = ns4.pid();
	ifinfomsg = reinterpret_cast<struct ifinfomsg*>(NLMSG_DATA(nlh));
	ifinfomsg->ifi_family = AF_UNSPEC;
	ifinfomsg->ifi_type = IFLA_UNSPEC;
	ifinfomsg->ifi_index = _if_index;
	ifinfomsg->ifi_flags = _flags;
	ifinfomsg->ifi_change = 0xffffffff;
	
	if (NLMSG_ALIGN(nlh->nlmsg_len) > sizeof(_buffer)) {
	    XLOG_FATAL("AF_NETLINK buffer size error: %d instead of %d",
		       sizeof(_buffer), NLMSG_ALIGN(nlh->nlmsg_len));
	}
	nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len);
	
	if (ns4.sendto(_buffer, nlh->nlmsg_len, 0,
		       reinterpret_cast<struct sockaddr*>(&_snl),
		       sizeof(_snl))
	    != (ssize_t)nlh->nlmsg_len) {
	    XLOG_ERROR("error writing to netlink socket: %s",
		       strerror(errno));
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
	int ret_value;
	int s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
	    XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
	}

	memset(&ifreq, 0, sizeof(ifreq));
	strncpy(ifreq.ifr_name, ifname().c_str(), sizeof(ifreq.ifr_name) - 1);
	ifreq.ifr_flags = _flags;
	ret_value = ioctl(s, SIOCSIFFLAGS, &ifreq);
	close(s);

	return (ret_value);
#endif // HAVE_NETLINK_SOCKETS_SET_FLAGS_IS_BROKEN
    }

private:
    uint16_t		_if_index;	// The interface index
    uint32_t		_flags;		// The flags
};

/**
 * @short class to set IPvX interface addresses
 */
class IfSetAddr : public IfReq {
public:
    IfSetAddr(IfConfigSetNetlink& ifc_set_netlink,
	      const string& ifname,
	      uint16_t if_index,
	      bool is_bcast,
	      bool is_p2p,
	      const IPvX& addr,
	      const IPvX& dst_or_bcast,
	      uint32_t prefix_len)
	: IfReq(ifc_set_netlink, ifname),
	  _if_index(if_index),
	  _is_bcast(is_bcast),
	  _is_p2p(is_p2p),
	  _addr(addr),
	  _dst_or_bcast(dst_or_bcast),
	  _prefix_len(prefix_len) {
	debug_msg("IfSetAddr "
		  "(ifname = %s addr = %s dst/bcast = %s prefix_len = %u)\n",
		  ifname.c_str(), addr.str().c_str(),
		  dst_or_bcast.str().c_str(), prefix_len);
    }

    int execute() {
	struct nlmsghdr		*nlh;
	struct ifaddrmsg	*ifaddrmsg;
	struct rtattr		*rtattr;
	int			rta_len;
	uint8_t			*data;
	NetlinkSocket		*ns_ptr = NULL;

	// Get the pointer to the NetlinkSocket
	switch(_addr.af()) {
	case AF_INET:
	{
	    NetlinkSocket4& ns4 = ifc_set_netlink();
	    ns_ptr = &ns4;
	    break;
	}
#ifdef HAVE_IPV6
	case AF_INET6:
	{
	    NetlinkSocket6& ns6 = ifc_set_netlink();
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
	nlh = reinterpret_cast<struct nlmsghdr*>(_buffer);
	nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifaddrmsg));
	nlh->nlmsg_type = RTM_NEWADDR;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE;
	nlh->nlmsg_seq = ns_ptr->seqno();
	nlh->nlmsg_pid = ns_ptr->pid();
	ifaddrmsg = reinterpret_cast<struct ifaddrmsg*>(NLMSG_DATA(nlh));
	ifaddrmsg->ifa_family = _addr.af();
	ifaddrmsg->ifa_prefixlen = _prefix_len;
	ifaddrmsg->ifa_flags = 0;	// TODO: XXX: PAVPAVPAV: OK if 0?
	ifaddrmsg->ifa_scope = 0;	// TODO: XXX: PAVPAVPAV: OK if 0?
	ifaddrmsg->ifa_index = _if_index;

	// Add the address as an attribute
	rta_len = RTA_LENGTH(_addr.addr_size());
	if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(_buffer)) {
	    XLOG_FATAL("AF_NETLINK buffer size error: %d instead of %d",
		       sizeof(_buffer), NLMSG_ALIGN(nlh->nlmsg_len) + rta_len);
	}
	rtattr = IFA_RTA(ifaddrmsg);
	rtattr->rta_type = IFA_LOCAL;
	rtattr->rta_len = rta_len;
	data = reinterpret_cast<uint8_t*>(RTA_DATA(rtattr));
	_addr.copy_out(data);
	nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

	if (_is_bcast || _is_p2p) {
	    // Set the p2p or broadcast address	
	    rta_len = RTA_LENGTH(_dst_or_bcast.addr_size());
	    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(_buffer)) {
		XLOG_FATAL("AF_NETLINK buffer size error: %d instead of %d",
			   sizeof(_buffer), NLMSG_ALIGN(nlh->nlmsg_len) + rta_len);
	    }
	    rtattr = (struct rtattr*)(((char*)(rtattr)) + RTA_ALIGN((rtattr)->rta_len));
	    rtattr->rta_type = IFA_UNSPEC;
	    rtattr->rta_len = rta_len;
	    data = reinterpret_cast<uint8_t*>(RTA_DATA(rtattr));
	    _dst_or_bcast.copy_out(data);
	    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;
	    if (_is_p2p) {
		rtattr->rta_type = IFA_ADDRESS;
	    } else {
		rtattr->rta_type = IFA_BROADCAST;
	    }
	}

	if (ns_ptr->sendto(_buffer, nlh->nlmsg_len, 0,
			   reinterpret_cast<struct sockaddr*>(&_snl),
			   sizeof(_snl))
	    != (ssize_t)nlh->nlmsg_len) {
	    XLOG_ERROR("error writing to netlink socket: %s",
		       strerror(errno));
	    return (XORP_ERROR);
	}
	return (XORP_OK);
    }

private:
    string	_ifname;		// The interface name
    uint16_t	_if_index;		// The interface index
    bool	_is_bcast;		// True if broadcast-capable interface
    bool	_is_p2p;		// True if point-to-point interface
    IPvX	_addr;			// The local address
    IPvX	_dst_or_bcast;		// The p2p dest addr or the bcast addr
    uint32_t	_prefix_len;		// The prefix length
};

/**
 * @short class to delete IPv4 interface addresses
 */
class IfDelAddr : public IfReq {
public:
    IfDelAddr(IfConfigSetNetlink& ifc_set_netlink,
	      const string& ifname,
	      uint16_t if_index,
	      const IPvX& addr,
	      uint32_t prefix_len)
	: IfReq(ifc_set_netlink, ifname),
	  _if_index(if_index),
	  _addr(addr),
	  _prefix_len(prefix_len) {
	debug_msg("IfDelAddr(ifname = %s, addr = %s prefix_len = %u)\n",
		  ifname.c_str(), addr.str().c_str(), prefix_len);
    }

    int execute() {
	struct nlmsghdr		*nlh;
	struct ifaddrmsg	*ifaddrmsg;
	struct rtattr		*rtattr;
	int			rta_len;
	uint8_t			*data;
	NetlinkSocket		*ns_ptr = NULL;

	// Get the pointer to the NetlinkSocket
	switch(_addr.af()) {
	case AF_INET:
	{
	    NetlinkSocket4& ns4 = ifc_set_netlink();
	    ns_ptr = &ns4;
	    break;
	}
#ifdef HAVE_IPV6
	case AF_INET6:
	{
	    NetlinkSocket6& ns6 = ifc_set_netlink();
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
	nlh = reinterpret_cast<struct nlmsghdr*>(_buffer);
	nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifaddrmsg));
	nlh->nlmsg_type = RTM_DELADDR;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE;
	nlh->nlmsg_seq = ns_ptr->seqno();
	nlh->nlmsg_pid = ns_ptr->pid();
	ifaddrmsg = reinterpret_cast<struct ifaddrmsg*>(NLMSG_DATA(nlh));
	ifaddrmsg->ifa_family = _addr.af();
	ifaddrmsg->ifa_prefixlen = _prefix_len;
	ifaddrmsg->ifa_flags = 0;	// TODO: XXX: PAVPAVPAV: OK if 0?
	ifaddrmsg->ifa_scope = 0;	// TODO: XXX: PAVPAVPAV: OK if 0?
	ifaddrmsg->ifa_index = _if_index;

	// Add the address as an attribute
	rta_len = RTA_LENGTH(_addr.addr_size());
	if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(_buffer)) {
	    XLOG_FATAL("AF_NETLINK buffer size error: %d instead of %d",
		       sizeof(_buffer), NLMSG_ALIGN(nlh->nlmsg_len) + rta_len);
	}
	rtattr = IFA_RTA(ifaddrmsg);
	rtattr->rta_type = IFA_LOCAL;
	rtattr->rta_len = rta_len;
	data = reinterpret_cast<uint8_t*>(RTA_DATA(rtattr));
	_addr.copy_out(data);
	nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

	if (ns_ptr->sendto(_buffer, nlh->nlmsg_len, 0,
			   reinterpret_cast<struct sockaddr*>(&_snl),
			   sizeof(_snl))
	    != (ssize_t)nlh->nlmsg_len) {
	    XLOG_ERROR("error writing to netlink socket: %s",
		       strerror(errno));
	    return (XORP_ERROR);
	}
	return (XORP_OK);
    }

private:
    string	_ifname;		// The interface name
    uint16_t	_if_index;		// The interface index
    IPvX	_addr;			// The local address
    uint32_t	_prefix_len;		// The prefix length
};

bool
IfConfigSetNetlink::push_config(const IfTree& it)
{
    IfTree::IfMap::const_iterator ii;
    IfTreeInterface::VifMap::const_iterator vi;

    // Clear errors associated with error reporter
    ifc().er().reset();
    
    //
    // Sanity check config - bail on bad interface and bad vif names
    //
    for (ii = it.ifs().begin(); ii != it.ifs().end(); ++ii) {
	const IfTreeInterface& i = ii->second;
	if (ifc().get_ifindex(i.ifname()) == 0) {
	    ifc().er().interface_error(i.ifname(), "interface not recognized");
	    XLOG_ERROR(ifc().er().last_error().c_str());
	    return false;
	}
	for (vi = i.vifs().begin(); i.vifs().end() != vi; ++vi) {
	    const IfTreeVif& v= vi->second;
	    if (v.vifname() != i.ifname()) {
		ifc().er().vif_error(i.ifname(), v.vifname(), "Bad vif name");
		XLOG_ERROR(ifc().er().last_error().c_str());
		return false;
	    }
	}
    }
    
    //
    // Walk config
    //
    for (ii = it.ifs().begin(); ii != it.ifs().end(); ++ii) {
	const IfTreeInterface& i = ii->second;
	for (vi = i.vifs().begin(); vi != i.vifs().end(); ++vi) {
	    const IfTreeVif& v = vi->second;

	    IfTreeVif::V4Map::const_iterator a4i;
	    for (a4i = v.v4addrs().begin(); a4i != v.v4addrs().end(); ++a4i) {
		const IfTreeAddr4& a = a4i->second;
		if (a.state() != IfTreeItem::NO_CHANGE)
		    push_addr(i, v, a);
	    }

#ifdef HAVE_IPV6
	    IfTreeVif::V6Map::const_iterator a6i;
	    for (a6i = v.v6addrs().begin(); a6i != v.v6addrs().end(); ++a6i) {
		const IfTreeAddr6& a = a6i->second;
		if (a.state() != IfTreeItem::NO_CHANGE)
		    push_addr(i, v, a);
	    }
#endif // HAVE_IPV6

	    push_vif(i, v);
	}
	push_if(i);
    }

    if (ifc().er().error_count() != 0)
	return false;
    
    return true;
}

void
IfConfigSetNetlink::push_if(const IfTreeInterface& i)
{
    uint16_t if_index = ifc().get_ifindex(i.ifname());
    XLOG_ASSERT(if_index > 0);

    //
    // Set the MTU
    //
    do {
	if (i.mtu() == 0)
	    break;		// XXX: ignore the MTU setup
	
	IfTree::IfMap::const_iterator ii = ifc().pulled_config().get_if(i.ifname());
	if ((ii != ifc().pulled_config().ifs().end())
	    && (ii->second.mtu() == i.mtu()))
	    break;		// Ignore: the MTU hasn't changed
	
	if (IfSetMTU(*this, i.ifname(), if_index, i.mtu()).execute()
	    < 0) {
	    ifc().er().interface_error(i.name(),
				       c_format("Failed to set MTU of %u bytes (%s)",
						i.mtu(), strerror(errno)));
	    XLOG_ERROR(ifc().er().last_error().c_str());
	    return;
	}
	break;
    } while (false);
    
    //
    // Set the MAC address
    //
    do {
	if (i.mac().empty())
	    break;

	IfTree::IfMap::const_iterator ii = ifc().pulled_config().get_if(i.ifname());
	if ((ii != ifc().pulled_config().ifs().end())
	    && (ii->second.mac() == i.mac()))
	    break;		// Ignore: the MAC hasn't changed
	
	struct ether_addr ea;
	try {
	    EtherMac em;
	    em = EtherMac(i.mac());
	    if (em.get_ether_addr(ea) == false) {
		ifc().er().interface_error(
		    i.name(),
		    c_format("Expected Ethernet MAC address, got \"%s\"",
			     i.mac().str().c_str()));
		XLOG_ERROR(ifc().er().last_error().c_str());
		return;
	    }
	} catch (const BadMac& bm) {
	    ifc().er().interface_error(
		i.name(),
		c_format("Invalid MAC address \"%s\"", i.mac().str().c_str()));
	    XLOG_ERROR(ifc().er().last_error().c_str());
	    return;
	}
	
	if (IfSetMac(*this, i.ifname(), if_index, ea).execute() < 0) {
	    ifc().er().interface_error(
		i.name(),
		c_format("Failed to set MAC address (%s)", strerror(errno)));
	    XLOG_ERROR(ifc().er().last_error().c_str());
	    return;
	}
	
	break;
    } while(false);
}

void
IfConfigSetNetlink::push_vif(const IfTreeInterface&	i,
			     const IfTreeVif&		v)
{
    uint16_t if_index = ifc().get_ifindex(i.ifname());
    XLOG_ASSERT(if_index > 0);

    bool deleted = (i.is_marked(IfTreeItem::DELETED) |
		    v.is_marked(IfTreeItem::DELETED));
    bool enabled = i.enabled() & v.enabled();

    uint32_t curflags = i.if_flags();
    bool up = curflags & IFF_UP;
    if (up && (deleted || !enabled)) {
	curflags &= ~IFF_UP;
    } else {
	curflags |= IFF_UP;
    }
    if (IfSetFlags(*this, i.ifname(), if_index, curflags).execute() < 0) {
	ifc().er().vif_error(i.ifname(), v.vifname(),
			     c_format("Failed to set interface flags to 0x%08x (%s)",
				      curflags, strerror(errno)));
	XLOG_ERROR(ifc().er().last_error().c_str());
	return;
    }
}

void
IfConfigSetNetlink::push_addr(const IfTreeInterface&	i,
			      const IfTreeVif&		v,
			      const IfTreeAddr4&	a)
{
    uint16_t if_index = ifc().get_ifindex(i.ifname());
    XLOG_ASSERT(if_index > 0);

    bool enabled = (i.enabled() & v.enabled() & a.enabled());

    debug_msg("Pushing %s\n", a.str().c_str());

    if (a.is_marked(IfTreeItem::CREATED) && enabled == false) {
	//
	// XXX:
	// A created but disabled address will not appear in the live
	// config that was read from the kernel.
	// 
	// This is a lot of work!
	//
	IfTree::IfMap::iterator ii = ifc().live_config().get_if(i.ifname());
	XLOG_ASSERT(ii != ifc().live_config().ifs().end());
	IfTreeInterface::VifMap::iterator vi = ii->second.get_vif(v.vifname());
	XLOG_ASSERT(vi != ii->second.vifs().end());
	vi->second.add_addr(a.addr());
	IfTreeVif::V4Map::iterator ai = vi->second.get_addr(a.addr());
	ai->second = a;
	return;
    }

    bool deleted = (i.is_marked(IfTreeItem::DELETED) |
		    v.is_marked(IfTreeItem::DELETED) |
		    a.is_marked(IfTreeItem::DELETED));

    if (deleted || !enabled) {
	IfDelAddr del_addr(*this, i.ifname(), if_index, IPvX(a.addr()),
			   a.prefix_len());
	if (del_addr.execute() < 0) {
	    string reason = c_format("Failed to delete address (%s): %s",
				     (deleted) ? "deleted" : "not enabled",
				     strerror(errno));
	    ifc().er().vifaddr_error(i.ifname(), v.vifname(), a.addr(), reason);
	    XLOG_ERROR(ifc().er().last_error().c_str());
	}
	return;
    }

    //
    // Set the address
    //
    do {
	IPv4 oaddr(IPv4::ZERO());
	if (a.broadcast())
	    oaddr = a.bcast();
	else if (a.point_to_point())
	    oaddr = a.endpoint();
	
	uint32_t prefix_len = a.prefix_len();
	
	const IfTreeAddr4* ap = NULL;
	do {
	    IfTree::IfMap::const_iterator ii = ifc().pulled_config().get_if(i.ifname());
	    if (ii == ifc().pulled_config().ifs().end())
		break;
	    IfTreeInterface::VifMap::const_iterator vi = ii->second.get_vif(v.vifname());
	    if (vi == ii->second.vifs().end())
		break;
	    IfTreeVif::V4Map::const_iterator ai = vi->second.get_addr(a.addr());
	    if (ai == vi->second.v4addrs().end())
		break;
	    ap = &ai->second;
	    break;
	} while (false);
	
	if ((ap != NULL)
	    && (ap->addr() == a.addr())
	    && ((a.broadcast() && (ap->bcast() == a.bcast()))
		|| (a.point_to_point() && (ap->endpoint() == a.endpoint())))
	    && (ap->prefix_len() == prefix_len)) {
	    break;		// Ignore: the address hasn't changed
	}
	
	IfSetAddr set_addr(*this, i.ifname(), if_index,
			   a.broadcast(), a.point_to_point(),
			   IPvX(a.addr()), IPvX(oaddr), prefix_len);
	if (set_addr.execute() < 0) {
	    ifc().er().vifaddr_error(i.ifname(), v.vifname(), a.addr(),
				     c_format("Address configuration failed (%s)",
					      strerror(errno)));
	    XLOG_ERROR(ifc().er().last_error().c_str());
	}
	
	break;
    } while (false);
}

#ifdef HAVE_IPV6
void
IfConfigSetNetlink::push_addr(const IfTreeInterface&	i,
			      const IfTreeVif&		v,
			      const IfTreeAddr6&	a)
{
    uint16_t if_index = ifc().get_ifindex(i.ifname());
    XLOG_ASSERT(if_index > 0);

    bool enabled = (i.enabled() & v.enabled() & a.enabled());

    if (a.is_marked(IfTreeItem::CREATED) && enabled == false) {
	//
	// A created but disabled address will not appear in the live
	// config rippled up from the kernel via the routing socket
	// 
	// This is a lot of work!
	//
	IfTree::IfMap::iterator ii = ifc().live_config().get_if(i.ifname());
	XLOG_ASSERT(ii != ifc().live_config().ifs().end());
	IfTreeInterface::VifMap::iterator vi = ii->second.get_vif(v.vifname());
	XLOG_ASSERT(vi != ii->second.vifs().end());
	vi->second.add_addr(a.addr());
	IfTreeVif::V6Map::iterator ai = vi->second.get_addr(a.addr());
	ai->second = a;
	return;
    }

    bool deleted = (i.is_marked(IfTreeItem::DELETED) |
		    v.is_marked(IfTreeItem::DELETED) |
		    a.is_marked(IfTreeItem::DELETED));

    if (deleted || !enabled) {
	IfDelAddr6 del_addr(_s6, i.ifname(), if_index, a.addr(), a.prefix_len());
	if (del_addr.execute() < 0) {
	    string reason = c_format("Failed to delete address (%s): %s",
				     (deleted) ? "deleted" : "not enabled",
				     strerror(errno));
	    ifc().er().vifaddr_error(i.ifname(), v.vifname(), a.addr(), reason);
	    XLOG_ERROR(ifc().er().last_error().c_str());
	}
	return;
    }

    //
    // Set the address
    //
    do {
	IPv6 oaddr(IPv6::ZERO());
	if (a.point_to_point())
	    oaddr = a.endpoint();
	
	// XXX: for whatever reason a prefix length of zero does not cut it, so
	// initialize prefix to 64.  This is exactly as ifconfig does.
	uint32_t prefix_len = a.prefix_len();
	if (0 == prefix_len)
	    prefix_len = 64;

	const IfTreeAddr6* ap = NULL;
	do {
	    IfTree::IfMap::const_iterator ii = ifc().pulled_config().get_if(i.ifname());
	    if (ii == ifc().pulled_config().ifs().end())
		break;
	    IfTreeInterface::VifMap::const_iterator vi = ii->second.get_vif(v.vifname());
	    if (vi == ii->second.vifs().end())
		break;
	    IfTreeVif::V6Map::const_iterator ai = vi->second.get_addr(a.addr());
	    if (ai == vi->second.v6addrs().end())
		break;
	    ap = &ai->second;
	    break;
	} while (false);
	
	if ((ap != NULL)
	    && (ap->addr() == a.addr())
	    && ((a.point_to_point() && (ap->endpoint() == a.endpoint())))
	    && (ap->prefix_len() == prefix_len)) {
	    break;		// Ignore: the address hasn't changed
	}
	
	IfSetAddr set_addr(*this, i.ifname(), if_index,
			   false, a.point_to_point(),
			   IPvX(a.addr()), IPvX(oaddr), prefix_len);
	if (set_addr.execute() < 0) {
	    ifc().er().vifaddr_error(i.ifname(), v.vifname(), a.addr(),
				     c_format("Address configuration failed (%s)",
					      strerror(errno)));
	    XLOG_ERROR(ifc().er().last_error().c_str());
	}
	
	break;
    } while (false);
}
#endif // HAVE_IPV6

#endif // HAVE_NETLINK_SOCKETS
