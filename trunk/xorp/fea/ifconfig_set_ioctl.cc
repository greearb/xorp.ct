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

#ident "$XORP: xorp/fea/ifconfig_set_ioctl.cc,v 1.6 2003/08/22 04:23:03 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ether_compat.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <net/if.h>
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
#endif
#ifdef HAVE_NETINET6_IN6_VAR_H
#include <netinet6/in6_var.h>
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

    int _fd;
};

/**
 * @short Base class for IPv4 configuration ioctls.
 */
class IfReq : public IfIoctl {
public:
    IfReq(int fd, const string& ifname) : IfIoctl(fd) {
	memset(&_ifreq, 0, sizeof(_ifreq));
	strncpy(_ifreq.ifr_name, ifname.c_str(), sizeof(_ifreq.ifr_name));
    }

protected:
    const char* ifname() const { return _ifreq.ifr_name; }

    struct ifreq _ifreq;
};

/**
 * @short Class to set MAC address on an interface.
 */
// TODO: XXX: PAVPAVPAV: this won't work on Linux!!
#ifdef SIOCSIFLLADDR
class IfSetMac : public IfReq {
public:
    IfSetMac(int fd, const string& ifname, const struct ether_addr& ea)
	: IfReq(fd, ifname) {
#ifdef HAVE_SA_LEN
	_ifreq.ifr_addr.sa_len = ETHER_ADDR_LEN;
#endif
	_ifreq.ifr_addr.sa_family = AF_LINK;
	memcpy(_ifreq.ifr_addr.sa_data, &ea, ETHER_ADDR_LEN);
    }
    int execute() const { return ioctl(_fd, SIOCSIFLLADDR, &_ifreq); }
};
#else
class IfSetMac : public IfReq {
public:
    IfSetMac(int fd, const string& ifname, const struct ether_addr& ea)
	: IfReq(fd, ifname) {
	UNUSED(ea);
    }
    int execute() const { return -1; }
};
#endif // ! SIOCSIFLLADDR

/**
 * @short Class to set MTU on interface.
 */
class IfSetMTU : public IfReq {
public:
    IfSetMTU(int fd, const string& ifname, size_t mtu)
	: IfReq(fd, ifname) {
	_ifreq.ifr_mtu = mtu;
    }
    int execute() const { return ioctl(_fd, SIOCSIFMTU, &_ifreq); }
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
    int execute() const { return ioctl(_fd, SIOCSIFFLAGS, &_ifreq); }
};

/**
 * @short class to get interface flags
 */
class IfGetFlags : public IfReq {
public:
    IfGetFlags(int fd, const string& ifname, uint32_t& flags) :
	IfReq(fd, ifname), _flags(flags) {
    }
    int execute() const {
	int r = ioctl(_fd, SIOCGIFFLAGS, &_ifreq);
	if (r >= 0) {
	    static_assert(sizeof(_ifreq.ifr_flags) == 2);
	    _flags = _ifreq.ifr_flags & 0xffff;
	    debug_msg("%s: Got flags %s (0x%08x)\n",
		      ifname(), IfConfigGet::iff_flags(_flags).c_str(), _flags);
	}
	return r;
    }

protected:
    uint32_t& _flags;
};

/**
 * @short class to set IPv4 interface addresses
 */
// TODO: XXX: PAVPAVPAV: This won't work on Lunux!!!
#ifdef SIOCAIFADDR
class IfSetAddr4 : public IfIoctl {
public:
    IfSetAddr4(int fd, const string& ifname,
	       const IPv4& addr, const IPv4& dst_or_bcast, size_t prefix)
	: IfIoctl(fd) {
	strncpy(_ifra.ifra_name, ifname.c_str(), sizeof(_ifra.ifra_name));
	addr.copy_out(_ifra.ifra_addr);
	dst_or_bcast.copy_out(_ifra.ifra_broadaddr);
	IPv4::make_prefix(prefix).copy_out(_ifra.ifra_mask);
	debug_msg("IfSetAddr4 "
		  "(fd = %d, ifname = %s, addr = %s, dst %s, prefix %u)\n",
		  fd, ifname.c_str(),
		  addr.str().c_str(), dst_or_bcast.str().c_str(),
		  (uint32_t)prefix);
    }

    int execute() const { return ioctl(_fd, SIOCAIFADDR, &_ifra); }

protected:
    struct ifaliasreq _ifra;
};
#else
class IfSetAddr4 : public IfIoctl {
public:
    IfSetAddr4(int fd, const string& ifname,
	       const IPv4& addr, const IPv4& dst_or_bcast, size_t prefix)
	: IfIoctl(fd) {
	UNUSED(ifname);
	UNUSED(addr);
	UNUSED(dst_or_bcast);
	UNUSED(prefix);
    }

    int execute() const { return -1; }
    
protected:
    
};

#endif // ! SIOCAIFADDR

/**
 * @short class to set IPv6 interface addresses
 */
// TODO: XXX: PAVPAVPAV: this won't work on MacOS X!!
#ifdef HAVE_IPV6
#ifdef SIOCAIFADDR_IN6
class IfSetAddr6 : public IfIoctl {
public:
    IfSetAddr6(int fd, const string& ifname,
	       const IPv6& addr, const IPv6& endpoint, size_t prefix)
	: IfIoctl(fd) {
	memset(&_ifra, 0, sizeof(_ifra));
	strncpy(_ifra.ifra_name, ifname.c_str(), sizeof(_ifra.ifra_name));
	addr.copy_out(_ifra.ifra_addr);

	if (IPv6::ZERO() != endpoint)
	    endpoint.copy_out(_ifra.ifra_dstaddr);

	IPv6::make_prefix(prefix).copy_out(_ifra.ifra_prefixmask);

	// The following should use ND6_INFINITE_LIFETIME
	_ifra.ifra_lifetime.ia6t_vltime = _ifra.ifra_lifetime.ia6t_pltime = ~0;

	debug_msg("IfSetAddr6 "
		  "(fd = %d, ifname = %s, addr = %s, dst %s, prefix %u)\n",
		  fd, ifname.c_str(), addr.str().c_str(),
		  endpoint.str().c_str(), (uint32_t)prefix);
    }

    int execute() const {
	return ioctl(_fd, SIOCAIFADDR_IN6, &_ifra);
    }

protected:
    struct in6_aliasreq _ifra;
};

#else // ! SIOCAIFADDR_IN6
class IfSetAddr6 : public IfIoctl {
public:
    IfSetAddr6(int fd, const string& ifname,
	       const IPv6& addr, const IPv6& endpoint, size_t prefix)
	: IfIoctl(fd) {
	
	UNUSED(ifname);
	UNUSED(addr);
	UNUSED(endpoint);
	UNUSED(prefix);
    }

    int execute() const { return -1; }

protected:

};
#endif // ! SIOCAIFADDR_IN6
#endif // HAVE_IPV6

/**
 * @short class to set IPv4 interface addresses
 */
// TODO: XXX: PAVPAVPAV: this won't work on Linux!!
#ifndef HOST_OS_LINUX
class IfDelAddr4 : public IfIoctl {
public:
    IfDelAddr4(int fd, const string& ifname, const IPv4& addr)
	: IfIoctl(fd) {
	strncpy(_ifra.ifra_name, ifname.c_str(), sizeof(_ifra.ifra_name));
	addr.copy_out(_ifra.ifra_addr);
	debug_msg("IfDelAddr4(fd = %d, ifname = %s, addr = %s)\n",
		  fd, ifname.c_str(), addr.str().c_str());
    }

    int execute() const { return ioctl(_fd, SIOCDIFADDR, &_ifra); }

protected:
    struct ifaliasreq _ifra;
};
#else
class IfDelAddr4 : public IfIoctl {
public:
    IfDelAddr4(int fd, const string& ifname, const IPv4& addr)
	: IfIoctl(fd) {
	UNUSED(ifname);
	UNUSED(addr);
	debug_msg("IfDelAddr4(fd = %d, ifname = %s, addr = %s)\n",
		  fd, ifname.c_str(), addr.str().c_str());
    }

    int execute() const { return -1; }

protected:
};
#endif // HOST_OS_LINUX

/**
 * @short class to set IPv6 interface addresses
 */
#ifdef HAVE_IPV6
// TODO: XXX: PAVPAVPAV: this won't work on Linux!!
#ifndef HOST_OS_LINUX
class IfDelAddr6 : public IfIoctl {
public:
    IfDelAddr6(int fd, const string& ifname, const IPv6& addr) : IfIoctl(fd) {
	strncpy(_ifra.ifra_name, ifname.c_str(), sizeof(_ifra.ifra_name));
	addr.copy_out(_ifra.ifra_addr);
	debug_msg("IfDelAddr6(fd = %d, ifname = %s, addr = %s)\n",
		  fd, ifname.c_str(), addr.str().c_str());
    }

    int execute() const { return ioctl(_fd, SIOCDIFADDR, &_ifra); }

protected:
    struct ifaliasreq _ifra;
};
#else
class IfDelAddr6 : public IfIoctl {
public:
    IfDelAddr6(int fd, const string& ifname, const IPv6& addr) : IfIoctl(fd) {
	UNUSED(ifname);
	UNUSED(addr);
	debug_msg("IfDelAddr6(fd = %d, ifname = %s, addr = %s)\n",
		  fd, ifname.c_str(), addr.str().c_str());
    }

    int execute() const { return -1; }

protected:
};
#endif // HOST_OS_LINUX
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
				       c_format("Failed to set MTU of %d bytes (%s)",
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

    uint32_t curflags;
    if (IfGetFlags(_s4, i.ifname(), curflags).execute() < 0) {
	ifc().er().vif_error(i.ifname(), v.vifname(),
			     c_format("Failed to get interface flags: %s",
				      strerror(errno)));
	XLOG_ERROR(ifc().er().last_error().c_str());
	return;
    }

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
	if (IfDelAddr4(_s4, i.ifname(), a.addr()).execute()) {
	    string reason = 
		c_format("Failed to delete address (%s): %s",
			 (deleted) ? "deleted" : "not enabled",
			 (errno) ? strerror(errno) : "not sys err");
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
	
	uint32_t prefix = a.prefix();
	
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
	    && (ap->prefix() == prefix)) {
	    break;		// Ignore: the address hasn't changed
	}
	
	IfSetAddr4 set_addr(_s4, i.ifname(), a.addr(), oaddr, prefix);
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
	if (IfDelAddr6(_s6, i.ifname(), a.addr()).execute()) {
	    string reason = 
		c_format("Failed to delete address (%s): %s",
			 (deleted) ? "deleted" : "not enabled",
			 (errno) ? strerror(errno) : "not sys err");
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
	uint32_t prefix = a.prefix();
	if (0 == prefix)
	    prefix = 64;

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
	    && (ap->prefix() == prefix)) {
	    break;		// Ignore: the address hasn't changed
	}
	
	IfSetAddr6 set_addr(_s6, i.ifname(), a.addr(), oaddr, prefix);
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
