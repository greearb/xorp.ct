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

#ident "$XORP: xorp/fea/ifconfig_set_ioctl.cc,v 1.12 2003/10/02 16:58:19 pavlin Exp $"


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
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
#endif

#ifdef HAVE_NETINET_IN_VAR_H
#include <netinet/in_var.h>
#endif
#ifdef HAVE_NETINET6_IN6_VAR_H
#include <netinet6/in6_var.h>
#endif
#ifdef HAVE_NETINET6_ND6_H
// XXX: a hack because <netinet6/nd6.h> is not C++ friendly
#define prf_ra in6_prflags::prf_ra
#include <netinet6/nd6.h>
#endif

#include "ifconfig.hh"
#include "ifconfig_set.hh"


//
// Set information about network interfaces configuration with the
// underlying system.
//
// The mechanism to set the information is ioctl(2).
//

IfConfigSetIoctl::IfConfigSetIoctl(IfConfig& ifc)
    : IfConfigSet(ifc), _s4(-1), _s6(-1)
{
#ifdef HAVE_IOCTL_SIOCGIFCONF
    register_ifc();
#endif
}

IfConfigSetIoctl::~IfConfigSetIoctl()
{
    stop();
}

int
IfConfigSetIoctl::start()
{
    if (_s4 < 0) {
	_s4 = socket(AF_INET, SOCK_DGRAM, 0);
	if (_s4 < 0) {
	    XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
	}
    }
    
#ifdef HAVE_IPV6
    if (_s6 < 0) {
	_s6 = socket(AF_INET6, SOCK_DGRAM, 0);
	if (_s6 < 0) {
	    XLOG_FATAL("Could not initialize IPv6 ioctl() socket");
	}
    }
#endif // HAVE_IPV6
    
    return (XORP_OK);
}

int
IfConfigSetIoctl::stop()
{
    if (_s4 >= 0) {
	close(_s4);
	_s4 = -1;
    }
    if (_s6 >= 0) {
	close(_s6);
	_s6 = -1;
    }
    
    return (XORP_OK);
}

#ifdef HAVE_IOCTL_SIOCGIFCONF

/**
 * @short Base class for Ioctl operations
 */
class IfIoctl {
public:
    IfIoctl(int fd) : _fd(fd) {}
    virtual ~IfIoctl() {}
    virtual int execute() const = 0;

protected:
    int fd() const { return _fd; }

private:
    int _fd;
};

/**
 * @short Base class for IPv4 configuration ioctls.
 */
class IfReq : public IfIoctl {
public:
    IfReq(int fd, const string& ifname)
	: IfIoctl(fd),
	  _ifname(ifname) {
	memset(&_ifreq, 0, sizeof(_ifreq));
	strncpy(_ifreq.ifr_name, ifname.c_str(), sizeof(_ifreq.ifr_name) - 1);
    }

protected:
    const string& ifname() const { return _ifname; }

    const string _ifname;
    struct ifreq _ifreq;
};

/**
 * @short Class to set MAC address on an interface.
 */
class IfSetMac : public IfReq {
public:
    IfSetMac(int fd, const string& ifname, const struct ether_addr& ether_addr)
	: IfReq(fd, ifname),
	  _ether_addr(ether_addr) {}

    int execute() const {
	struct ifreq ifreq_copy;	// A local copy

	memcpy(&ifreq_copy, &_ifreq, sizeof(ifreq_copy));

#if defined(SIOCSIFLLADDR)
	// XXX: FreeBSD
	ifreq_copy.ifr_addr.sa_family = AF_LINK;
	memcpy(ifreq_copy.ifr_addr.sa_data, &_ether_addr, ETHER_ADDR_LEN);
#ifdef HAVE_SA_LEN
	ifreq_copy.ifr_addr.sa_len = ETHER_ADDR_LEN;
#endif
	return ioctl(fd(), SIOCSIFLLADDR, &ifreq_copy);

#elif defined(SIOCSIFADDR) && defined(AF_LINK)
	// XXX: NetBSD
	ifreq_copy.ifr_addr.sa_family = AF_LINK;
	memcpy(ifreq_copy.ifr_addr.sa_data, &_ether_addr, ETHER_ADDR_LEN);
#ifdef HAVE_SA_LEN
	ifreq_copy.ifr_addr.sa_len = ETHER_ADDR_LEN;
#endif
	return ioctl(fd(), SIOCSIFADDR, &ifreq_copy);

#elif defined(SIOCSIFHWADDR)
	// XXX: Linux
	ifreq_copy.ifr_hwaddr.sa_family = ARPHRD_ETHER;
	memcpy(ifreq_copy.ifr_hwaddr.sa_data, &_ether_addr, ETH_ALEN);
#ifdef HAVE_SA_LEN
	ifreq_copy.ifr_hwaddr.sa_len = ETH_ALEN;
#endif
	return ioctl(fd(), SIOCSIFHWADDR, &ifreq_copy);

#else
#error No mechanism to set the MAC address on an interface
#endif
    }

private:
    struct ether_addr _ether_addr;	// The Ethernet address
};

/**
 * @short Class to set MTU on interface.
 */
class IfSetMTU : public IfReq {
public:
    IfSetMTU(int fd, const string& ifname, uint32_t mtu)
	: IfReq(fd, ifname) {
	_ifreq.ifr_mtu = mtu;
    }
    int execute() const { return ioctl(fd(), SIOCSIFMTU, &_ifreq); }
};

/**
 * @short class to set interface flags
 */
class IfSetFlags : public IfReq {
public:
    IfSetFlags(int fd, const string& ifname, uint32_t flags) :
	IfReq(fd, ifname) {
	_ifreq.ifr_flags = flags;
    }
    int execute() const { return ioctl(fd(), SIOCSIFFLAGS, &_ifreq); }
};

/**
 * @short class to set IPv4 interface addresses
 *
 * Note: if the system has ioctl(SIOCAIFADDR) (e.g., FreeBSD), then
 * the interface address is added as an alias, otherwise it overwrites the
 * previous address (if such exists).
 */
class IfSetAddr4 : public IfIoctl {
public:
    IfSetAddr4(int fd, const string& ifname, uint16_t if_index, bool is_p2p,
	       const IPv4& addr, const IPv4& dst_or_bcast, uint32_t prefix_len)
	: IfIoctl(fd),
	  _ifname(ifname),
	  _if_index(if_index),
	  _is_p2p(is_p2p),
	  _addr(addr),
	  _dst_or_bcast(dst_or_bcast),
	  _prefix_len(prefix_len) {
	debug_msg("IfSetAddr4 "
		  "(fd = %d, ifname = %s addr = %s dst/bcast = %s prefix_len = %u)\n",
		  fd, ifname.c_str(),
		  addr.str().c_str(), dst_or_bcast.str().c_str(),
		  prefix_len);
    }

    int execute() const {
#ifdef SIOCAIFADDR
	//
	// Add an alias address
	//
	struct in_aliasreq ifra;

	memset(&ifra, 0, sizeof(ifra));
	strncpy(ifra.ifra_name, _ifname.c_str(), sizeof(ifra.ifra_name) - 1);
	_addr.copy_out(ifra.ifra_addr);
	if (_is_p2p)
	    _dst_or_bcast.copy_out(ifra.ifra_dstaddr);
	else
	    _dst_or_bcast.copy_out(ifra.ifra_broadaddr);
	IPv4::make_prefix(_prefix_len).copy_out(ifra.ifra_mask);

	return ioctl(fd(), SIOCAIFADDR, &ifra);
#endif // SIOCAIFADDR

	//
	// Set a new address
	//
	int ret_value;
	struct ifreq ifreq;

	memset(&ifreq, 0, sizeof(ifreq));
	strncpy(ifreq.ifr_name, _ifname.c_str(), sizeof(ifreq.ifr_name) - 1);

	// Set the address
	_addr.copy_out(ifreq.ifr_addr);
	ret_value = ioctl(fd(), SIOCSIFADDR, &ifreq);
	if (ret_value < 0)
	    return ret_value;

	// Set the netmask
	IPv4::make_prefix(_prefix_len).copy_out(ifreq.ifr_addr);
	ret_value = ioctl(fd(), SIOCSIFNETMASK, &ifreq);
	if (ret_value < 0)
	    return ret_value;

	// Set the p2p or broadcast address
	if (_is_p2p) {
	    _dst_or_bcast.copy_out(ifreq.ifr_dstaddr);
	    ret_value = ioctl(fd(), SIOCSIFDSTADDR, &ifreq);
	} else {
	    _dst_or_bcast.copy_out(ifreq.ifr_broadaddr);
	    ret_value = ioctl(fd(), SIOCSIFBRDADDR, &ifreq);
	}
	return ret_value;
    }

private:
    string	_ifname;		// The interface name
    uint16_t	_if_index;		// The interface index
    bool	_is_p2p;		// True if point-to-point interface
    IPv4	_addr;			// The local address
    IPv4	_dst_or_bcast;		// The p2p dest addr or the bcast addr
    uint32_t	_prefix_len;		// The prefix length
};

/**
 * @short class to set IPv6 interface addresses
 */
#ifdef HAVE_IPV6
class IfSetAddr6 : public IfIoctl {
public:
    IfSetAddr6(int fd, const string& ifname, uint16_t if_index, bool is_p2p,
	       const IPv6& addr, const IPv6& endpoint, uint32_t prefix_len)
	: IfIoctl(fd),
	  _ifname(ifname),
	  _if_index(if_index),
	  _is_p2p(is_p2p),
	  _addr(addr),
	  _endpoint(endpoint),
	  _prefix_len(prefix_len) {
	debug_msg("IfSetAddr6 "
		  "(fd = %d, ifname = %s, addr = %s, dst %s, prefix_len %u)\n",
		  fd, ifname.c_str(), addr.str().c_str(),
		  endpoint.str().c_str(), prefix_len);
    }

    int execute() const {
#ifdef SIOCAIFADDR_IN6
	//
	// Add an alias address
	//
	struct in6_aliasreq ifra;

	memset(&ifra, 0, sizeof(ifra));
	strncpy(ifra.ifra_name, _ifname.c_str(), sizeof(ifra.ifra_name) - 1);
	_addr.copy_out(ifra.ifra_addr);
	if (_is_p2p)
	    _endpoint.copy_out(ifra.ifra_dstaddr);
	IPv6::make_prefix(_prefix_len).copy_out(ifra.ifra_prefixmask);
	ifra.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME;
	ifra.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;

	return ioctl(fd(), SIOCAIFADDR_IN6, &ifra);
#endif // SIOCAIFADDR_IN6

	//
	// Set a new address
	//
#ifndef SIOCAIFADDR_IN6
	//
	// XXX: Linux uses a weird struct in6_ifreq to do this, and this
	// name clashes with the KAME in6_ifreq.
	// For now, we don't make this code specific only to Linux.
	//
	int ret_value;
	struct in6_ifreq in6_ifreq;

	memset(&in6_ifreq, 0, sizeof(in6_ifreq));
	in6_ifreq.ifr6_ifindex = _if_index;

	// Set the address and the prefix length
	_addr.copy_out(in6_ifreq.ifr6_addr);
	in6_ifreq.ifr6_prefixlen = _prefix_len;
	ret_value = ioctl(fd(), SIOCSIFADDR, &in6_ifreq);
	if (ret_value < 0)
	    return ret_value;

	// Set the p2p address
	if (_is_p2p) {
	    _endpoint.copy_out(in6_ifreq.ifr6_addr);
	    ret_value = ioctl(fd(), SIOCSIFDSTADDR, &ifreq);
	}
	return ret_value;
#endif // ! SIOCAIFADDR_IN6
    }

private:
    string	_ifname;		// The interface name
    uint16_t	_if_index;		// The interface index
    bool	_is_p2p;		// True if point-to-point interface
    IPv6	_addr;			// The local address
    IPv6	_endpoint;		// The p2p dest addr
    uint32_t	_prefix_len;		// The prefix length
};
#endif // HAVE_IPV6

/**
 * @short class to delete IPv4 interface addresses
 */
class IfDelAddr4 : public IfReq {
public:
    IfDelAddr4(int fd, const string& ifname, uint16_t if_index,
	       const IPv4& addr, uint32_t prefix_len)
	: IfReq(fd, ifname) {
	addr.copy_out(_ifreq.ifr_addr);
	debug_msg("IfDelAddr4(fd = %d, ifname = %s, addr = %s prefix_len = %u)\n",
		  fd, ifname.c_str(), addr.str().c_str(), prefix_len);
	UNUSED(if_index);
	UNUSED(prefix_len);
    }

    int execute() const { return ioctl(fd(), SIOCDIFADDR, &_ifreq); }

private:
};

/**
 * @short class to delete IPv6 interface addresses
 */
#ifdef HAVE_IPV6
class IfDelAddr6 : public IfIoctl {
public:
    IfDelAddr6(int fd, const string& ifname, uint16_t if_index,
	       const IPv6& addr, uint32_t prefix_len)
	: IfIoctl(fd),
	  _ifname(ifname),
	  _if_index(if_index),
	  _addr(addr),
	  _prefix_len(prefix_len) {
	debug_msg("IfDelAddr6(fd = %d, ifname = %s, addr = %s prefix_len = %u)\n",
		  fd, ifname.c_str(), addr.str().c_str(), prefix_len);
    }

    int execute() const {
#ifdef SIOCDIFADDR_IN6
	struct in6_ifreq in6_ifreq;

	memset(&in6_ifreq, 0, sizeof(in6_ifreq));
	strncpy(in6_ifreq.ifr_name, _ifname.c_str(),
		sizeof(in6_ifreq.ifr_name) - 1);
	_addr.copy_out(in6_ifreq.ifr_addr);

	return ioctl(fd(), SIOCDIFADDR_IN6, &in6_ifreq);
#endif // SIOCDIFADDR_IN6

#ifndef SIOCDIFADDR_IN6
	//
	// XXX: Linux uses a weird struct in6_ifreq to do this, and this
	// name clashes with the KAME in6_ifreq.
	// For now, we don't make this code specific only to Linux.
	//
	struct in6_ifreq in6_ifreq;

	memset(&in6_ifreq, 0, sizeof(in6_ifreq));
	in6_ifreq.ifr6_ifindex = _if_index;
	_addr.copy_out(in6_ifreq.ifr6_addr);
	in6_ifreq.ifr6_prefixlen = _prefix_len;

	return ioctl(fd(), SIOCDIFADDR, &in6_ifreq);
#endif // !SIOCDIFADDR_IN6
    }

private:
    string	_ifname;		// The interface name
    uint16_t	_if_index;		// The interface index
    IPv6	_addr;			// The local address
    uint32_t	_prefix_len;		// The prefix length
};
#endif // HAVE_IPV6

bool
IfConfigSetIoctl::push_config(const IfTree& it)
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
	if (if_nametoindex(i.ifname().c_str()) <= 0) {
	    ifc().er().interface_error(i.ifname(),
				       "O/S does not recognise interface");
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
IfConfigSetIoctl::push_if(const IfTreeInterface& i)
{
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
	
	if (IfSetMTU(_s4, i.ifname(), i.mtu()).execute() < 0) {
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
	
	if (IfSetMac(_s4, i.ifname(), ea).execute() < 0) {
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
IfConfigSetIoctl::push_vif(const IfTreeInterface&	i,
			   const IfTreeVif&		v)
{
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
    if (IfSetFlags(_s4, i.ifname(), curflags).execute() < 0) {
	ifc().er().vif_error(i.ifname(), v.vifname(),
			     c_format("Failed to set interface flags to 0x%08x (%s)",
				      curflags, strerror(errno)));
	XLOG_ERROR(ifc().er().last_error().c_str());
	return;
    }
}

void
IfConfigSetIoctl::push_addr(const IfTreeInterface&	i,
			    const IfTreeVif&		v,
			    const IfTreeAddr4&		a)
{
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
	IfDelAddr4 del_addr(_s4, i.ifname(), v.pif_index(), a.addr(), a.prefix_len());
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
	
	IfSetAddr4 set_addr(_s4, i.ifname(), v.pif_index(), a.point_to_point(),
			    a.addr(), oaddr, prefix_len);
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
IfConfigSetIoctl::push_addr(const IfTreeInterface&	i,
			    const IfTreeVif&		v,
			    const IfTreeAddr6&		a)
{
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
	IfDelAddr6 del_addr(_s6, i.ifname(), v.pif_index(), a.addr(), a.prefix_len());
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
	
	IfSetAddr6 set_addr(_s6, i.ifname(), v.pif_index(), a.point_to_point(),
			    a.addr(), oaddr, prefix_len);
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

#endif // HAVE_IOCTL_SIOCGIFCONF
