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

#ident "$XORP: xorp/fea/ifconfig_rtsock.cc,v 1.6 2003/05/02 07:50:47 pavlin Exp $"

#error "OBSOLETE FILE"

#include "fea_module.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/xorp.h"
#include "libxorp/ether_compat.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/sysctl.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_var.h>
#include <netinet/in.h>
#include <netinet/in_var.h>

#include <map>

#include "iftree.hh"
#include "ifconfig_rtsock.hh"

//
// We are assuming a bsd style interface, per W. Richard Stevens Unix
// Network Programming Volume 1 (2nd Edition).  The underlying
// interface information might be described as incongruous to what
// XORP uses and may be incongruous to other Unix systems.
//

// -------------------------------------------------------------------------
// Debug helpers

/**
 * @param m message type from routing socket message
 * @return human readable form.
 */
static string
rtm_msg_type(uint32_t m)
{
    struct {
	uint32_t 	value;
	const char*	name;
    } rtm_msg_types[] = {
#define RTM_MSG_ENTRY(X) { X, #X }
#ifdef RTM_ADD
	RTM_MSG_ENTRY(RTM_ADD),
#endif
#ifdef RTM_DELETE
	RTM_MSG_ENTRY(RTM_DELETE),
#endif
#ifdef RTM_CHANGE
	RTM_MSG_ENTRY(RTM_CHANGE),
#endif
#ifdef RTM_GET
	RTM_MSG_ENTRY(RTM_GET),
#endif
#ifdef RTM_LOSING
	RTM_MSG_ENTRY(RTM_LOSING),
#endif
#ifdef RTM_REDIRECT
	RTM_MSG_ENTRY(RTM_REDIRECT),
#endif
#ifdef RTM_MISS
	RTM_MSG_ENTRY(RTM_MISS),
#endif
#ifdef RTM_LOCK
	RTM_MSG_ENTRY(RTM_LOCK),
#endif
#ifdef RTM_OLDADD
	RTM_MSG_ENTRY(RTM_OLDADD),
#endif
#ifdef RTM_OLDDEL
	RTM_MSG_ENTRY(RTM_OLDDEL),
#endif
#ifdef RTM_RESOLVE
	RTM_MSG_ENTRY(RTM_RESOLVE),
#endif
#ifdef RTM_NEWADDR
	RTM_MSG_ENTRY(RTM_NEWADDR),
#endif
#ifdef RTM_DELADDR
	RTM_MSG_ENTRY(RTM_DELADDR),
#endif
#ifdef RTM_IFINFO
	RTM_MSG_ENTRY(RTM_IFINFO),
#endif
#ifdef RTM_NEWMADDR
	RTM_MSG_ENTRY(RTM_NEWMADDR),
#endif
#ifdef RTM_DELMADDR
	RTM_MSG_ENTRY(RTM_DELMADDR),
#endif
#ifdef RTM_IFANNOUNCE
	RTM_MSG_ENTRY(RTM_IFANNOUNCE),
#endif
	{ ~0, "Unknown" }
    };
    const size_t n_rtm_msgs = sizeof(rtm_msg_types) / sizeof(rtm_msg_types[0]);
    const char* ret = 0;
    for (size_t i = 0; i < n_rtm_msgs; i++) {
	ret = rtm_msg_types[i].name;
	if (rtm_msg_types[i].value == m)
	    break;
    }
    return ret;
}

/**
 * Generate ifconfig like flags string
 * @param flags interface flags value from routing socket message.
 * @return ifconfig like flags string
 */
static string
iff_flags(uint32_t flags)
{
    struct {
	uint32_t 	value;
	const char*	name;
    } iff_fl[] = {
#define IFF_FLAG_ENTRY(X) { IFF_##X, #X }
#ifdef IFF_UP
	IFF_FLAG_ENTRY(UP),
#endif
#ifdef IFF_BROADCAST
	IFF_FLAG_ENTRY(BROADCAST),
#endif
#ifdef IFF_DEBUG
	IFF_FLAG_ENTRY(DEBUG),
#endif
#ifdef IFF_LOOPBACK
	IFF_FLAG_ENTRY(LOOPBACK),
#endif
#ifdef IFF_POINTOPOINT
	IFF_FLAG_ENTRY(POINTOPOINT),
#endif
#ifdef IFF_SMART
	IFF_FLAG_ENTRY(SMART),
#endif
#ifdef IFF_RUNNING
	IFF_FLAG_ENTRY(RUNNING),
#endif
#ifdef IFF_NOARP
	IFF_FLAG_ENTRY(NOARP),
#endif
#ifdef IFF_PROMISC
	IFF_FLAG_ENTRY(PROMISC),
#endif
#ifdef IFF_ALLMULTI
	IFF_FLAG_ENTRY(ALLMULTI),
#endif
#ifdef IFF_OACTIVE
	IFF_FLAG_ENTRY(OACTIVE),
#endif
#ifdef IFF_SIMPLEX
	IFF_FLAG_ENTRY(SIMPLEX),
#endif
#ifdef IFF_LINK0
	IFF_FLAG_ENTRY(LINK0),
#endif
#ifdef IFF_LINK1
	IFF_FLAG_ENTRY(LINK1),
#endif
#ifdef IFF_LINK2
	IFF_FLAG_ENTRY(LINK2),
#endif
#ifdef IFF_ALTPHYS
	IFF_FLAG_ENTRY(ALTPHYS),
#endif
#ifdef IFF_MULTICAST
	IFF_FLAG_ENTRY(MULTICAST),
#endif
	{ 0, "" }  // for nitty compilers that don't like trailing ","
    };
    const size_t n_iff_fl = sizeof(iff_fl) / sizeof(iff_fl[0]);

    string ret("<");
    for (size_t i = 0; i < n_iff_fl; i++) {
	if (0 == (flags & iff_fl[i].value))
	    continue;
	flags &= ~iff_fl[i].value;
	ret += iff_fl[i].name;
	if (0 == flags)
	    break;
	ret += ",";
    }
    ret += ">";
    return ret;
}

// -------------------------------------------------------------------------
//
// We cache our own associative array of interface names to interface index
// because the genius who implemented RTM_IFANNOUNCE calls RTM_IFANNOUNCE
// after deleting the interface name from the kernel.
//

static class Index2IfnameResolver {
    typedef map<uint32_t, string> IfIndex2NameMap;

public:
    void map_ifindex(uint32_t index, const string& name)
    {
	_ifnames[index] = name;
    }

    void unmap_ifindex(uint32_t index)
    {
	IfIndex2NameMap::iterator i = _ifnames.find(index);
	if (_ifnames.end() != i)
	    _ifnames.erase(i);
    }

    const char* get_ifname(uint32_t index)
    {
	IfIndex2NameMap::const_iterator i = _ifnames.find(index);
	if (_ifnames.end() == i) {
	    char name_buf[IF_NAMESIZE];
	    const char* ifname = if_indextoname(index, name_buf);

	    if (ifname)
		map_ifindex(index, ifname);

	    return ifname;
	}
	return i->second.c_str();
    }

protected:
    IfIndex2NameMap _ifnames;

} if_resolver;

IfConfigRoutingSocket::IfConfigRoutingSocket(RoutingSocket&		  rs,
					     IfConfigUpdateReporterBase&  ur,
					     SimpleIfConfigErrorReporter& er)
    : RoutingSocketObserver(rs), IfConfig(ur, er),
      _s4(-1), _s6(-1)
{
    _s4 = socket(AF_INET, SOCK_DGRAM, 0);
    if (_s4 < 0) {
	XLOG_FATAL("Could not initialize socket for IPv4 configuration.");
    }

#ifdef HAVE_IPV6
    _s6 = socket(AF_INET6, SOCK_DGRAM, 0);
    if (_s6 < 0) {
	XLOG_FATAL("Could not initialize socket for IPv6 configuration.");
    }
#endif // HAVE_IPV6

    read_config(_live_config);
    _live_config.finalize_state();

    debug_msg("Start configuration read: %s\n", _live_config.str().c_str());
    debug_msg("\nEnd configuration read.\n");
}

IfConfigRoutingSocket::~IfConfigRoutingSocket()
{
    if (_s4 >= 0)
	close(_s4);
    if (_s6 >= 0)
	close(_s6);
}

void
IfConfigRoutingSocket::rtsock_data(const uint8_t* data,
				   size_t	  n_bytes)
{
    //    debug_msg("rtsock_data %d bytes\n", n_bytes);
    if (parse_buffer(_live_config, data, n_bytes)) {

	debug_msg("Start configuration read:\n");
	debug_msg_indent(4);
	debug_msg("%s\n", _live_config.str().c_str());
	debug_msg_indent(0);
	debug_msg("\nEnd configuration read.\n");

	report_updates(_live_config);
	_live_config.finalize_state();
    }
}

const IfTree&
IfConfigRoutingSocket::pull_config(IfTree&)
{
    // Forcing read here is probably more work than we'd like to do, but
    // we have no choice.  The application may have pushed state and attempt
    // to read information back before going into the event loop.  Only option
    // is to read_config or update config as data is poked in.  The latter
    // is computationally cheaper, but requires more coding time.  If this
    // proves to be an issue we can revisit this.
    read_config(_live_config);
    return _live_config;
}

bool
IfConfigRoutingSocket::read_config(IfTree& it)
{
    int mib[6];

    mib[0] = CTL_NET;
    mib[1] = PF_ROUTE;
    mib[2] = 0;			// protocol number - always 0
    mib[3] = PF_UNSPEC;		// Address family: any (XXX: always true?)
    mib[4] = NET_RT_IFLIST;
    mib[5] = 0;			// no flags

    // Get the table size
    size_t sz;
    if (0 != sysctl(mib, sizeof(mib)/sizeof(mib[0]), NULL, &sz, NULL , 0)) {
	XLOG_ERROR("sysctl failed: %s", strerror(errno));
	return false;
    }

    uint8_t ifdata[sz];

    // Get the data
    if (0 != sysctl(mib, sizeof(mib)/sizeof(mib[0]), ifdata, &sz, NULL , 0)) {
	XLOG_ERROR("sysctl failed: %s", strerror(errno));
	return false;
    }

    // Parse the result
    parse_buffer(it, ifdata, sz);

    return true;
}

void
IfConfigRoutingSocket::flush_config(IfTree& it)
{
    it = IfTree();
}

static IfTreeInterface*
get_if(IfTree& it, const string& ifname)
{
    IfTree::IfMap::iterator ii = it.get_if(ifname);
    if (it.ifs().end() == ii)
	return 0;
    return &ii->second;
}

static IfTreeVif*
get_vif(IfTree& it, const string& ifname, const string& vifname)
{
    IfTreeInterface* fi = get_if(it, ifname);
    if (0 == fi)
	return 0;

    IfTreeInterface::VifMap::iterator vi = fi->get_vif(vifname);
    if (fi->vifs().end() == vi)
	return 0;

    return &vi->second;
}

static void
rtm_ifinfo_to_fea_cfg(const if_msghdr*  ifm,
		      IfTree&	it)
{
    XLOG_ASSERT(RTM_IFINFO == ifm->ifm_type);

    debug_msg("%p index %d RTM_IFINFO\n", ifm, ifm->ifm_index);

    if (RTA_IFP != ifm->ifm_addrs) {
	// Interface being disabled or coming up following RTM_IFANNOUNCE
	const char* ifname = if_resolver.get_ifname(ifm->ifm_index);
	if (0 == ifname) {
	    XLOG_FATAL("Could not find interface corresponding to index %d\n",
		       ifm->ifm_index);
	}

	IfTreeInterface* fi = get_if(it, ifname);
	if (0 == fi) {
	    XLOG_FATAL("Could not find IfTreeInterface named %s\n", ifname);
	}
	fi->set_enabled(ifm->ifm_flags & IFF_UP);
	debug_msg("Set Interface %s enabled %d\n", ifname, fi->enabled());

	IfTreeVif* fv = get_vif(it, ifname, ifname);
	if (0 == fv) {
	    XLOG_FATAL("Could not find IfTreeVif on %s named %s\n",
		       ifname, ifname);
	}
	fv->set_enabled(ifm->ifm_flags & IFF_UP);
	debug_msg("Set Vif %s on Interface %s enabled %d\n",
		  ifname, ifname, fv->enabled());
	return;
    }

    const sockaddr* sock = reinterpret_cast<const sockaddr *>(ifm + 1);
    if (AF_LINK != sock->sa_family) {
	debug_msg("Ignoring RTM_INFO with sa_family = %d\n", sock->sa_family);
	return;
    }

    const sockaddr_dl* sdl = reinterpret_cast<const sockaddr_dl*>(sock);

    string ifname;
    if (sdl->sdl_nlen > 0) {
	ifname = string(sdl->sdl_data, sdl->sdl_nlen);
	if_resolver.map_ifindex(ifm->ifm_index, ifname);
	/* XXX At this point we should give up and re-read everything from
	 * kernel.
	 */
    }

    it.add_if(ifname);
    IfTree::IfMap::iterator ii = it.get_if(ifname);

    if (sdl->sdl_alen == sizeof(ether_addr)) {
	ether_addr ea;
	memcpy(&ea, sdl->sdl_data + sdl->sdl_nlen, sdl->sdl_alen);
	ii->second.set_mac(EtherMac(ea));
    } else if (sdl->sdl_alen != 0) {
	XLOG_ERROR("Address size %d uncatered for\n",
		   sdl->sdl_alen);
    }

    ii->second.set_mtu(ifm->ifm_data.ifi_mtu);
    ii->second.set_enabled(ifm->ifm_flags & IFF_UP);
    debug_msg("%s flags %s\n",
	      ifname.c_str(), iff_flags(ifm->ifm_flags).c_str());
    // XXX: vifname == ifname on this platform
    ii->second.add_vif(ifname);
    IfTreeInterface::VifMap::iterator vi = ii->second.get_vif(ifname);
    vi->second.set_enabled(true);
}

/*
 * Round up to nearest integer value of step.
 * Taken from ROUND_UP macro in Stevens.
 */
static inline size_t round_up(size_t val, size_t step)
{
    return (val & (step - 1)) ? (1 + (val | (step - 1))) : val;
}

/*
 * Step to next socket address pointer.
 * Taken from NEXT_SA macro in Stevens.
 */
static inline const sockaddr* next_sa(const sockaddr* sa)
{
    const size_t min_size = sizeof(u_long);
    size_t sa_size = sa->sa_len ? round_up(sa->sa_len, min_size) : min_size;
    const uint8_t* p = reinterpret_cast<const uint8_t*>(sa) + sa_size;
    return reinterpret_cast<const sockaddr*>(p);
}

static void
get_addrs(uint32_t amask, const sockaddr* sock, const sockaddr* rti_info[])
{
    for (uint32_t i = 0; i < RTAX_MAX; i++) {
	if (amask & (1 << i)) {
	    debug_msg("\tPresent 0x%02x af %d size %d\n",
		      1 << i, sock->sa_family, sock->sa_len);
	    rti_info[i] = sock;
	    sock = next_sa(sock);
	} else {
	    rti_info[i] = 0;
	}
    }
}

static void
rtm_addr_to_fea_cfg(const if_msghdr*	ifm,
		    IfTree&		it)
{
    assert(RTM_NEWADDR == ifm->ifm_type || RTM_DELADDR == ifm->ifm_type);
    debug_msg_indent(4);
    debug_msg("%p index %d RTM_{ NEW|DEL }ADDR\n",
	      ifm, ifm->ifm_index);

    const ifa_msghdr *ifa = reinterpret_cast<const ifa_msghdr*>(ifm);

    const char* ifname = if_resolver.get_ifname(ifm->ifm_index);
    if (0 == ifname) {
	XLOG_FATAL("Could not find interface corresponding to index %d\n",
		   ifa->ifam_index);
    }

    debug_msg("Address on %s flags %s\n",
	      ifname, iff_flags(ifm->ifm_flags).c_str());

    //
    // Locate VIF to pin data on
    //
    IfTreeVif *fv = get_vif(it, ifname, ifname);
    if (0 == fv) {
	XLOG_FATAL("Could not find vif named %s in IfTree.", ifname);
    }

    const sockaddr* sock = reinterpret_cast<const sockaddr*>(ifa + 1);
    const sockaddr* rti_info[RTAX_MAX];
    get_addrs(ifa->ifam_addrs, sock, rti_info);

    if (0 == rti_info[RTAX_IFA]) {
	debug_msg("Ignoring addr info with null RTAX_IFA entry");
	return;
    }

    if (AF_INET == rti_info[RTAX_IFA]->sa_family) {
	IPv4 a(*rti_info[RTAX_IFA]);
	fv->add_addr(a);

	IfTreeVif::V4Map::iterator ai = fv->get_addr(a);
	ai->second.set_enabled(true);	// XXX check flags for UP?

	if (rti_info[RTAX_NETMASK]) {
	    // Special case netmask 0 can have zero length
	    if (0 != rti_info[RTAX_NETMASK]->sa_len) {
		IPv4 netmask(*rti_info[RTAX_NETMASK]);
		ai->second.set_prefix(netmask.prefix_length());
	    } else {
		ai->second.set_prefix(0);
	    }
	}

	if (rti_info[RTAX_BRD]) {
	    IPv4 o(*rti_info[RTAX_BRD]);
	    if (ifa->ifam_flags & IFF_BROADCAST) {
		ai->second.set_bcast(o);
	    } else if (ifa->ifam_flags & IFF_POINTOPOINT) {
		ai->second.set_endpoint(o);
	    } else {
		// We end up here, which is confusing on FBSD 4.6.2
		debug_msg("Assuming this %s with flags 0x%08x is bcast\n",
			  o.str().c_str(), ifa->ifam_flags);
		ai->second.set_bcast(o);
	    }
	}

	// Mark as deleted if necessary
	if (RTM_DELADDR == ifa->ifam_type)
	    ai->second.mark(IfTreeItem::DELETED);

	return;
    }

#ifdef HAVE_IPV6
    if (AF_INET6 == rti_info[RTAX_IFA]->sa_family) {
	IPv6 a(*rti_info[RTAX_IFA]);
	fv->add_addr(a);

	IfTreeVif::V6Map::iterator ai = fv->get_addr(a);
	ai->second.set_enabled(true);	// XXX check flags for UP?

	if (rti_info[RTAX_NETMASK]) {
	    IPv6 netmask(*rti_info[RTAX_NETMASK]);
	    ai->second.set_prefix(netmask.prefix_length());
	}

	if (rti_info[RTAX_NETMASK]) {
	    // Special case netmask 0 can have zero length
	    if (0 != rti_info[RTAX_NETMASK]->sa_len) {
		IPv6 netmask(*rti_info[RTAX_NETMASK]);
		ai->second.set_prefix(netmask.prefix_length());
	    } else {
		ai->second.set_prefix(0);
	    }
	}

	/* Mark as deleted if necessary */
	if (RTM_DELADDR == ifa->ifam_type)
	    ai->second.mark(IfTreeItem::DELETED);

	return;
    }
#endif // HAVE_IPV6
}

#ifdef RTM_IFANNOUNCE
void
rtm_announce_to_fea_cfg(const if_msghdr* ifm, IfTree& it)
{
    const if_announcemsghdr* ifan =
	reinterpret_cast<const if_announcemsghdr*>(ifm);

    debug_msg("RTM_IFANNOUNCE %s\n",
	      (ifan->ifan_what == IFAN_DEPARTURE) ? "DEPARTURE" : "ARRIVAL");

    switch (ifan->ifan_what) {
    case IFAN_ARRIVAL:
    {
	//
	// Add interface
	//
	debug_msg("Mapping %d -> %s\n", ifan->ifan_index, ifan->ifan_name);
	if_resolver.map_ifindex(ifan->ifan_index, ifan->ifan_name);

	it.add_if(ifan->ifan_name);
	IfTreeInterface* fi = get_if(it, ifan->ifan_name);
	if (fi) {
	    fi->add_vif(ifan->ifan_name);
	} else {
	    debug_msg("Could not add interface/vif %s\n", ifan->ifan_name);
	}
	break;
    }
	
    case IFAN_DEPARTURE:
    {
	//
	// Delete interface
	//
	debug_msg("Deleting interface and vif named: %s\n", ifan->ifan_name);
	IfTreeInterface* fi = get_if(it, ifan->ifan_name);
	if (fi) {
	    fi->mark(IfTree::DELETED);
	} else {
	    debug_msg("Attempted to delete missing interface: %s\n",
		      ifan->ifan_name);
	}
	IfTreeVif* fv = get_vif(it, ifan->ifan_name, ifan->ifan_name);
	if (fv) {
	    fv->mark(IfTree::DELETED);
	} else {
	    debug_msg("Attempted to delete missing interface: %s\n",
		      ifan->ifan_name);
	}
	if_resolver.unmap_ifindex(ifan->ifan_index);
	break;
    }
	
    default:
	debug_msg("Unknown RTM_IFANNOUNCE message type %d\n", ifan->ifan_what);
	break;
    }
}
#endif // RTM_IFANNOUNCE

bool
IfConfigRoutingSocket::parse_buffer(IfTree&	   it,
				    const uint8_t* buf,
				    size_t	   buf_bytes)
{
    bool recognized = false;
    /* Reading route(4) manual page is a good start for understanding this */

    const if_msghdr* ifm = reinterpret_cast<const if_msghdr *>(buf);
    const uint8_t* last = buf + buf_bytes;

    for (const uint8_t* ptr = buf; ptr < last; ptr += ifm->ifm_msglen) {
    	ifm = reinterpret_cast<const if_msghdr*>(ptr);
	if (RTM_VERSION != ifm->ifm_version) {
	    XLOG_ERROR("Version mismatch expected %d got %d",
		       RTM_VERSION,
		       ifm->ifm_version);
	    continue;
	}

	switch (ifm->ifm_type) {
	case RTM_IFINFO:
	    rtm_ifinfo_to_fea_cfg(ifm, it);
	    recognized = true;
	    break;
	case RTM_NEWADDR:
	case RTM_DELADDR:
	    rtm_addr_to_fea_cfg(ifm, it);
	    recognized = true;
	    break;
#ifdef RTM_IFANNOUNCE
	case RTM_IFANNOUNCE:
	    rtm_announce_to_fea_cfg(ifm, it);
	    recognized = true;
	    break;
#endif // RTM_IFANNOUNCE
	case RTM_ADD:
	case RTM_MISS:
	case RTM_CHANGE:
	case RTM_GET:
	case RTM_LOSING:
	case RTM_DELETE:
	    break;
	default:
	    debug_msg("Unhandled type %s(%d) (%d bytes)\n",
		      rtm_msg_type(ifm->ifm_type).c_str(), ifm->ifm_type,
		      ifm->ifm_msglen);
	    break;
	}
    }
    return recognized;
}

/* ------------------------------------------------------------------------- */
/* Push config related material
 *
 * This is the mother of all nightmares.  How to map the changes requested
 * by the user through the abstract "model" that XORP presents to the user.
 * Why is this hard?  Because many of the concepts don't exist, eg you
 * can't disable an address on an interface, only create or destroy it.
 *
 * First, we define a set of ioctl command objects for configuring
 * interfaces. Then, we do the "mapping".  All the time, we sit with
 * crossed fingers...
 */

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
class IfReq4 : public IfIoctl {
public:
    IfReq4(int fd, const string& ifname) : IfIoctl(fd) {
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
class IfSetMac : public IfReq4 {
public:
    IfSetMac(int fd, const string& ifname, const ether_addr& ea)
	: IfReq4(fd, ifname) {
	_ifreq.ifr_addr.sa_len = ETHER_ADDR_LEN;
	_ifreq.ifr_addr.sa_family = AF_LINK;
	memcpy(_ifreq.ifr_addr.sa_data, &ea, ETHER_ADDR_LEN);
    }
    int execute() const { return ioctl(_fd, SIOCSIFLLADDR, &_ifreq); }
};

/**
 * @short Class to set MTU on interface.
 */
class IfSetMTU : public IfReq4 {
public:
    IfSetMTU(int fd, const string& ifname, size_t mtu)
	: IfReq4(fd, ifname) {
	_ifreq.ifr_mtu = mtu;
    }
    int execute() const { return ioctl(_fd, SIOCSIFMTU, &_ifreq); }
};

/**
 * @short class to set interface flags
 */
class IfSetFlags : public IfReq4 {
public:
    IfSetFlags(int fd, const string& ifname, uint32_t flags) :
	IfReq4(fd, ifname) {
	_ifreq.ifr_flags = flags;
    }
    int execute() const { return ioctl(_fd, SIOCSIFFLAGS, &_ifreq); }
};

/**
 * @short class to get interface flags
 */
class IfGetFlags : public IfReq4 {
public:
    IfGetFlags(int fd, const string& ifname, uint32_t& flags) :
	IfReq4(fd, ifname), _flags(flags) {
    }
    int execute() const {
	int r = ioctl(_fd, SIOCGIFFLAGS, &_ifreq);
	if (r >= 0) {
	    static_assert(sizeof(_ifreq.ifr_flags) == 2);
	    _flags = _ifreq.ifr_flags & 0xffff;
	    debug_msg("%s: Got flags %s (0x%08x)\n",
		      ifname(), iff_flags(_flags).c_str(), _flags);
	}
	return r;
    }

protected:
    uint32_t& _flags;
};

/**
 * @short class to set IPv4 interface addresses
 */
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
		  "(fd = %d, ifname = %s, addr = %s, dst %s, prefix %d)\n",
		  fd, ifname.c_str(),
		  addr.str().c_str(), dst_or_bcast.str().c_str(), prefix);
    }

    int execute() const { return ioctl(_fd, SIOCAIFADDR, &_ifra); }

protected:
    struct ifaliasreq _ifra;
};

/**
 * @short class to set IPv4 interface addresses
 */
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

/**
 * @short class to set IPv6 interface addresses
 */
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

	// the following should use ND6_INFINITE_LIFETIME
	_ifra.ifra_lifetime.ia6t_vltime = _ifra.ifra_lifetime.ia6t_pltime = ~0;

	debug_msg("IfSetAddr6 "
		  "(fd = %d, ifname = %s, addr = %s, dst %s, prefix %d)\n",
		  fd, ifname.c_str(),
		  addr.str().c_str(), endpoint.str().c_str(), prefix);
    }

    int execute() const {
	return ioctl(_fd, SIOCAIFADDR_IN6, &_ifra);
    }

protected:
    struct in6_aliasreq _ifra;
};

/**
 * @short class to set IPv6 interface addresses
 */
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

void
IfConfigRoutingSocket::push_addr(const IfTreeInterface&	i,
				 const IfTreeVif&	v,
				 const IfTreeAddr4&	a)
{
    bool enabled = (i.enabled() & v.enabled() & a.enabled());

    debug_msg("Pushing %s\n", a.str().c_str());

    if (a.is_marked(IfTreeItem::CREATED) && enabled == false) {
	//
	// A created but disabled address will not appear in the live
	// config rippled up from the kernel via the routing socket
	// 
	// This is a lot of work!
	//
	IfTree::IfMap::iterator ii = _live_config.get_if(i.ifname());
	assert(ii != _live_config.ifs().end());
	IfTreeInterface::VifMap::iterator vi = ii->second.get_vif(v.vifname());
	assert(vi != ii->second.vifs().end());
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
	    er().vifaddr_error(i.ifname(), v.vifname(), a.addr(), reason);
	    XLOG_ERROR(er().last_error().c_str());
	}
	return;
    }

    // Check if interface flags need setting
    uint32_t reqmask = a.flags();
    uint32_t curflags;
    if (IfGetFlags(_s4, i.ifname(), curflags).execute() < 0) {
	er().vifaddr_error(i.ifname(), v.vifname(), a.addr(),
			   c_format("Failed to get interface flags (%s)",
				    strerror(errno)));
	XLOG_ERROR(er().last_error().c_str());
	return;
    }

    if (reqmask && (curflags & reqmask) == 0) {
	XLOG_INFO("Interface flags need to be set to configure address.");
	uint32_t nflags = ((curflags & ~(IFF_BROADCAST | IFF_POINTOPOINT)) |
			   reqmask);
	if (IfSetFlags(_s4, i.ifname(), nflags).execute() < 0) {
	    er().vifaddr_error(i.ifname(), v.vifname(), a.addr(),
		c_format("Failed to set interface flags to %08x (%s)",
			 nflags, strerror(errno)));
	    XLOG_ERROR(er().last_error().c_str());
	    return;
	}
    }

    IPv4 oaddr(IPv4::ZERO());
    if (a.flags() & IFF_BROADCAST)
	oaddr = a.bcast();
    else if (a.flags() & IFF_POINTOPOINT)
	oaddr = a.endpoint();

    IfSetAddr4 set_addr(_s4, i.ifname(), a.addr(), oaddr, a.prefix());
    if (set_addr.execute() < 0) {
	er().vifaddr_error(i.ifname(), v.vifname(), a.addr(),
			   c_format("Address configuration failed (%s)",
				    strerror(errno)));
	XLOG_ERROR(er().last_error().c_str());
    }
}

void
IfConfigRoutingSocket::push_addr(const IfTreeInterface&	i,
				 const IfTreeVif&	v,
				 const IfTreeAddr6&	a)
{
    bool enabled = (i.enabled() & v.enabled() & a.enabled());

    if (a.is_marked(IfTreeItem::CREATED) && enabled == false) {
	//
	// A created but disabled address will not appear in the live
	// config rippled up from the kernel via the routing socket
	// 
	// This is a lot of work!
	//
	IfTree::IfMap::iterator ii = _live_config.get_if(i.ifname());
	assert(ii != _live_config.ifs().end());
	IfTreeInterface::VifMap::iterator vi = ii->second.get_vif(v.vifname());
	assert(vi != ii->second.vifs().end());
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
	    er().vifaddr_error(i.ifname(), v.vifname(), a.addr(), reason);
	    XLOG_ERROR(er().last_error().c_str());
	}
	return;
    }

    // Check if interface flags need setting
    uint32_t reqmask = a.flags();
    uint32_t curflags;
    if (IfGetFlags(_s6, i.ifname(), curflags).execute() < 0) {
	er().vifaddr_error(i.ifname(), v.vifname(), a.addr(),
			   c_format("Failed to get interface flags (%s)",
				    strerror(errno)));
	XLOG_ERROR(er().last_error().c_str());
	return;
    }

    if (reqmask && (curflags & reqmask) == 0) {
	XLOG_INFO("Interface flags need to be set to configure address.");
	uint32_t nflags = ((curflags & ~(IFF_BROADCAST | IFF_POINTOPOINT)) |
			   reqmask);
	if (IfSetFlags(_s6, i.ifname(), nflags).execute() < 0) {
	    er().vifaddr_error(i.ifname(), v.vifname(), a.addr(),
		c_format("Failed to set interface flags to %08x (%s)",
			 nflags, strerror(errno)));
	    XLOG_ERROR(er().last_error().c_str());
	    return;
	}
    }

    IPv6 oaddr(IPv6::ZERO());
    if (a.flags() & IFF_POINTOPOINT)
	oaddr = a.endpoint();

    // For whatever reason a prefix length of zero does not cut it, so
    // initialize prefix to 64.  This is exactly as ifconfig does.
    uint32_t prefix = a.prefix();
    if (0 == prefix)
	prefix = 64;

    IfSetAddr6 set_addr(_s6, i.ifname(), a.addr(), oaddr, prefix);
    if (set_addr.execute() < 0) {
	er().vifaddr_error(i.ifname(), v.vifname(), a.addr(),
			   c_format("Address configuration failed (%s)",
				    strerror(errno)));
	XLOG_ERROR(er().last_error().c_str());
    }
}

void
IfConfigRoutingSocket::push_vif(const IfTreeInterface&	i,
				const IfTreeVif&		v)
{
    bool deleted = (i.is_marked(IfTreeItem::DELETED) |
		    v.is_marked(IfTreeItem::DELETED));
    bool enabled = i.enabled() & v.enabled();

    uint32_t curflags;
    if (IfGetFlags(_s4, i.ifname(), curflags).execute() < 0) {
	er().vif_error(i.ifname(), v.vifname(),
		       c_format("Failed to get interface flags: %s",
				strerror(errno)));
	XLOG_ERROR(er().last_error().c_str());
	return;
    }

    bool up = curflags & IFF_UP;
    if (up && (deleted || !enabled)) {
	curflags &= ~IFF_UP;
    } else {
	curflags |= IFF_UP;
    }
    if (IfSetFlags(_s4, i.ifname(), curflags).execute() < 0) {
	er().vif_error(i.ifname(), v.vifname(),
		       c_format("Failed to set interface flags to 0x%08x (%s)",
				curflags, strerror(errno)));
	XLOG_ERROR(er().last_error().c_str());
	return;
    }
}

void
IfConfigRoutingSocket::push_if(const IfTreeInterface& i)
{
    if (i.mtu() && IfSetMTU(_s4, i.ifname(), i.mtu()).execute() < 0) {
	er().interface_error(i.name(),
			     c_format("Failed to set MTU of %d bytes (%s)",
				      i.mtu(), strerror(errno)));
	XLOG_ERROR(er().last_error().c_str());
	return;
    }

    if (i.mac().empty())
	return;

    ether_addr ea;
    try {
	EtherMac em;
	em = EtherMac(i.mac());
	if (em.get_ether_addr(ea) == false) {
	    er().interface_error(i.name(),
				 string("Expected Ethernet MAC address, "
					" got \"") + i.mac().str() +
				 string("\"")); 
	    XLOG_ERROR(er().last_error().c_str());
	    return;
	}
    } catch (const BadMac& bm) {
	er().interface_error(i.name(), string("Invalid MAC address \"") + 
			     i.mac().str() + string("\""));
	XLOG_ERROR(er().last_error().c_str());
	return;
    }

    if (IfSetMac(_s4, i.ifname(), ea).execute() < 0) {
	er().interface_error(i.name(),
			     c_format("Failed to set MAC address (%s)",
				      strerror(errno)));
	XLOG_ERROR(er().last_error().c_str());
	return;
    }
}

bool
IfConfigRoutingSocket::push_config(const IfTree& it)
{
    IfTree::IfMap::const_iterator ii;
    IfTreeInterface::VifMap::const_iterator vi;

    // Clear errors associated with error reporter
    er().reset();
    
    //
    // Sanity check config - bail on bad interface and bad vif names
    //
    for (ii = it.ifs().begin(); ii != it.ifs().end(); ++ii) {
	const IfTreeInterface& i = ii->second;
	if (if_nametoindex(i.ifname().c_str()) <= 0) {
	    er().interface_error(i.ifname(),
				 "O/S does not recognise interface");
	    XLOG_ERROR(er().last_error().c_str());
	    return false;
	}
	for (vi = i.vifs().begin(); i.vifs().end() != vi; ++vi) {
	    const IfTreeVif& v= vi->second;
	    if (v.vifname() != i.ifname()) {
		er().vif_error(i.ifname(), v.vifname(), "Bad vif name");
		XLOG_ERROR(er().last_error().c_str());
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
    return er().error_count() == 0;
}

