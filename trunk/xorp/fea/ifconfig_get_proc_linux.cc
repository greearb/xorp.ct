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

#ident "$XORP: xorp/fea/ifconfig_get_ioctl.cc,v 1.5 2003/05/28 21:50:54 pavlin Exp $"

#define DEBUG_LOGGING true

#include <cstdio>
#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxorp/ether_compat.h"
#include "libxorp/ipvx.hh"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include <net/if.h>
#include "ifconfig.hh"
#include "ifconfig_get.hh"
#include "kernel_utils.hh"

//
// Get information about network interfaces from the underlying system.
//
// The mechanism to obtain the information is Linux's /proc/net/dev 
//

IfConfigGetProcLinux::IfConfigGetProcLinux(IfConfig& ifc)
    : IfConfigGet(ifc)
{
#ifdef HAVE_PROC_LINUX
    register_ifc();
#endif
}

IfConfigGetProcLinux::~IfConfigGetProcLinux()
{
   stop();
}

int
IfConfigGetProcLinux::start()
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
IfConfigGetProcLinux::stop()
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

bool
IfConfigGetProcLinux::pull_config(IfTree& iftree)
{
    return read_config(iftree);
}

#ifndef HAVE_PROC_LINUX
bool
IfConfigGetProcLinux::read_config(IfTree& )
{
    return false;
}

#else // HAVE_PROC_LINUX

static char *get_name(char *name, char *p);
static bool proc_read_ifconf_linux(IfTree& iftree, int family);
static bool if_fetch_linux(IfTree& it, char * ifname, int family);

bool
IfConfigGetProcLinux::read_config(IfTree& iftree)
{
    if (proc_read_ifconf_linux(iftree, AF_INET) != true)
	return false;
    // parse file

#ifdef HAVE_IPV6
    //
    // The IPv6 information
    //
    if (proc_read_ifconf_linux(iftree, AF_INET6) != true)
	return false;
    // parse file
    //
#endif // HAVE_IPV6

    return true;
}

//
// Derived from Redhat Linux ifconfig code
//

static bool
proc_read_ifconf_linux(IfTree& iftree, int family)
{
    FILE *fh;
    char buf[512];
    int err;

    switch (family) {
    case AF_INET:
	fh = fopen("/proc/net/dev", "r");
	if (!fh) {
	    XLOG_FATAL("Warning: cannot open %s (%s).",
	    "/proc/net/dev", strerror(errno)); 
	    }	
	fgets(buf, sizeof buf, fh);	/* lose starting 2 lines of comments  */
	fgets(buf, sizeof buf, fh);

	err = 0;
	while (fgets(buf, sizeof buf, fh)) {
	    char *s, ifname[IFNAMSIZ];
	    s = get_name(ifname, buf);    
	    debug_msg("interface: %s\n",ifname);
	    if_fetch_linux(iftree, ifname, family);

	}
	if (ferror(fh)) {
	    XLOG_ERROR("/proc/net/dev read failed: %s", strerror(errno));
	}

	fclose(fh);
	break;

#ifdef HAVE_IPV6
    //
    // The IPv6 information
    //
    case AF_INET6:
	XLOG_FATAL("IPv6 support not yet implemented");
	break;
#endif // HAVE_IPV6

    default:
	XLOG_UNREACHABLE();
	break;
    }
    
    return true;
}

static char *get_name(char *name, char *p)
{
    while (isspace(*p))
	p++;
    while (*p) {
	if (isspace(*p))
	    break;
	if (*p == ':') {	/* could be an alias */
	    char *dot = p, *dotname = name;
	    *name++ = *p++;
	    while (isdigit(*p))
		*name++ = *p++;
	    if (*p != ':') {	/* it wasn't, backup */
		p = dot;
		name = dotname;
	    }
	    if (*p == '\0')
		return NULL;
	    p++;
	    break;
	}
	*name++ = *p++;
    }
    *name++ = '\0';
    return p;
}

bool if_fetch_linux(IfTree& it, char * ifname, int family)
{
    struct ifreq ifr;
    int soc;

    //
    // Need to revisit this function for IPv6 support under Linux
    //
    if (family == AF_INET6) {
	XLOG_FATAL("IPv6 is not yet supported");
	return false;
    }
    else if (family != AF_INET) {
	XLOG_FATAL("Unsupported address family");
	return false;
    }
    
    soc = socket(family, SOCK_DGRAM, 0);
    if (soc < 0) {
	XLOG_FATAL("Could not initialize ioctl() socket");
	return false;
    }

    it.add_if(ifname); // retrieve node, create if necessary 

    //
    // Fill in tree interface 
    //
    IfTreeInterface& fi = it.get_if(ifname)->second;

    // MAC address
    ether_addr ea;
    strcpy(ifr.ifr_name, ifname);
    if (ioctl(soc, SIOCGIFHWADDR, &ifr) < 0) 
        memset(&ea, 0, 32);
    else 
        memcpy(&ea, ifr.ifr_hwaddr.sa_data, 8);
    fi.set_mac(EtherMac(ea));
    const Mac& mac = fi.mac();
    XLOG_INFO("MAC address: %s",mac.str().c_str());

    // MTU 
    int mtu = 0;
    strcpy(ifr.ifr_name, ifname);
    if (ioctl(soc, SIOCGIFMTU, &ifr) < 0)
        mtu = 0;
    else
        mtu = ifr.ifr_mtu;
    fi.set_mtu(mtu);
    debug_msg("MTU: %d\n",fi.mtu());

    // fi.set_enabled(flags & IFF_UP);
    int flags = 0;
    strcpy(ifr.ifr_name, ifname);
    if (ioctl(soc, SIOCGIFFLAGS, &ifr) < 0)
        return false;
    flags = ifr.ifr_flags; 
    fi.set_enabled(flags & IFF_UP);
    debug_msg("enabled: %s\n", fi.enabled() ? "true" : "false");

    // XXX: vifname == ifname on this platform
    fi.add_vif(ifname);
    IfTreeVif& fv = fi.get_vif(ifname)->second;

    // Interface index
    int idx = 0;
    strcpy(ifr.ifr_name, ifname);
    if (ioctl(soc, SIOCGIFINDEX, &ifr) < 0)
        idx = 0;
    else
        idx = ifr.ifr_ifindex;
    fv.set_pif_index(idx);
    debug_msg("interface index: %d\n",fv.pif_index());

    // set vif flags
    fv.set_enabled(fi.enabled() && (flags & IFF_UP));
    fv.set_broadcast(flags & IFF_BROADCAST);
    fv.set_loopback(flags & IFF_LOOPBACK);
    fv.set_point_to_point(flags & IFF_POINTOPOINT);
    fv.set_multicast(flags & IFF_MULTICAST);
    debug_msg("vif enabled: %s\n",        
	fv.enabled() ? "true" : "false");
    debug_msg("vif broadcast: %s\n",      
	fv.broadcast() ? "true" : "false");
    debug_msg("vif loopback: %s\n",       
	fv.loopback() ? "true" : "false");
    debug_msg("vif point_to_point: %s\n", 
	fv.point_to_point() ? "true" : "false");
    debug_msg("vif multicast: %s\n",      
	fv.multicast() ? "true" : "false");

//
// FB: this section incomplete
//
    //
    // Get the IP address, netmask, broadcast address, P2P destination
    //
    // The default values
//    IPvX lcl_addr = IPvX::ZERO(family);
//    IPvX subnet_mask = IPvX::ZERO(family);
//    IPvX broadcast_addr = IPvX::ZERO(family);
//    IPvX peer_addr = IPvX::ZERO(family);
//    bool has_broadcast_addr = false;
//    bool has_peer_addr = false;
//    
//    // Get the IP address
//    strcpy(ifr.ifr_name, ifname);
//    if (ioctl(soc, SIOCGIFADDR, &ifr) < 0) {
//	XLOG_FATAL("No interface address found");
//	return false;
//    }
//    else {
//	lcl_addr.copy_in(ifr.ifr_addr);
//	lcl_addr = kernel_ipvx_adjust(lcl_addr);
//    }

    //
    // stopped at line 239 in ifconfig_parse_ifreq.cc
    //

    close(soc);

    return true;
}

#endif // HAVE_PROC_LINUX
