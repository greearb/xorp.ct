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

#ident "$XORP: xorp/fea/ifconfig_get_proc_linux.cc,v 1.2 2003/08/28 14:42:19 pavlin Exp $"

#define PROC_LINUX_FILE_V4 "/proc/net/dev"
#define PROC_LINUX_FILE_V6 "/proc/net/if_inet6"

#include <cstdio>
#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

//#define DEBUG_LOGGING
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
    return (XORP_OK);
}

int
IfConfigGetProcLinux::stop()
{
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
static bool if_fetch_linux_v4(IfTree& it);

#ifdef HAVE_IPV6
static bool if_fetch_linux_v6(IfTree& it);
#endif // HAVE_IPV6

bool
IfConfigGetProcLinux::read_config(IfTree& iftree)
{
    if (proc_read_ifconf_linux(iftree, AF_INET) != true)
	return false;

#ifdef HAVE_IPV6
    //
    // The IPv6 information
    //
    if (proc_read_ifconf_linux(iftree, AF_INET6) != true)
	return false;
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
    switch (family) {
    case AF_INET:
	if_fetch_linux_v4(iftree);
	break;

#ifdef HAVE_IPV6
    //
    // IPv6 information
    //
    case AF_INET6:
	if_fetch_linux_v6(iftree);
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

// 
// IPv4 Interface info fetch derived from ifconfig_parse_ifreq.cc
//
bool if_fetch_linux_v4(IfTree& it)
{
    struct ifreq ifr;
    int soc;
    FILE *fh;
    char buf[512];
    int err;
    char *s, ifname[IFNAMSIZ];

    soc = socket(AF_INET, SOCK_DGRAM, 0);
    if (soc < 0) {
	XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
	return false;
    }

    fh = fopen(PROC_LINUX_FILE_V4, "r");
    if (!fh) {
	XLOG_FATAL("Warning: cannot open %s (%s).",
	PROC_LINUX_FILE_V4, strerror(errno)); 
	}	
    fgets(buf, sizeof buf, fh);	/* lose starting 2 lines of comments  */
    s = get_name(ifname, buf);    
    if (strcmp(ifname, "Inter-|") != 0) {
	XLOG_ERROR("%s: improper file contents", PROC_LINUX_FILE_V4);
	return false;
    }
    fgets(buf, sizeof buf, fh);
    s = get_name(ifname, buf);    
    if (strcmp(ifname, "face") != 0) {
	XLOG_ERROR("%s: improper file contents", PROC_LINUX_FILE_V4);
	return false;
    }

    err = 0;
    while (fgets(buf, sizeof buf, fh)) {
	s = get_name(ifname, buf);    

	it.add_if(ifname); // retrieve node, create if necessary 
	debug_msg("interface: %s\n",ifname);

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
	debug_msg("MAC address: %s\n",mac.str().c_str());

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
	// Get the IP address, netmask, broadcast address, P2P destination
	//
	// The default values
	IPvX lcl_addr = IPvX::ZERO(AF_INET);
	IPvX subnet_mask = IPvX::ZERO(AF_INET);
	IPvX broadcast_addr = IPvX::ZERO(AF_INET);
	IPvX peer_addr = IPvX::ZERO(AF_INET);
	bool has_broadcast_addr = false;
	bool has_peer_addr = false;

	// Get the IP address
	strcpy(ifr.ifr_name, ifname);
	ifr.ifr_addr.sa_family = AF_INET;
	if (ioctl(soc, SIOCGIFADDR, &ifr) == 0) {
	    lcl_addr.copy_in(ifr.ifr_addr);
	    lcl_addr = kernel_ipvx_adjust(lcl_addr);
	}
	debug_msg("IP address: %s\n", lcl_addr.str().c_str());

	// Get the netmask
	strcpy(ifr.ifr_name, ifname);
	if (ioctl(soc, SIOCGIFNETMASK, &ifr) >= 0)
	    subnet_mask.copy_in(ifr.ifr_addr);
	debug_msg("IP netmask: %s\n", subnet_mask.str().c_str());

	// Get the broadcast address	
	if (fv.broadcast()) {
	    strcpy(ifr.ifr_name, ifname);
	    if (ioctl(soc, SIOCGIFBRDADDR, &ifr) >= 0) {
		broadcast_addr.copy_in(ifr.ifr_addr);
		has_broadcast_addr = true;
	    }
	    debug_msg("Broadcast address: %s\n", broadcast_addr.str().c_str());
	}

	// Get the p2p address
	if (fv.point_to_point()) {
	    strcpy(ifr.ifr_name, ifname);
	    if (ioctl(soc, SIOCGIFDSTADDR, &ifr) >= 0) {
		peer_addr.copy_in(ifr.ifr_dstaddr);
		has_peer_addr = true;
	    }
	}
	debug_msg("Peer address: %s\n", peer_addr.str().c_str());

	debug_msg("\n"); // put empty line between interfaces

	// Add the address
	fv.add_addr(lcl_addr.get_ipv4());
	IfTreeAddr4& fa = fv.get_addr(lcl_addr.get_ipv4())->second;
	fa.set_enabled(fv.enabled() && (flags & IFF_UP));
	fa.set_broadcast(fv.broadcast()
			 && (flags & IFF_BROADCAST)
			 && has_broadcast_addr);
	fa.set_loopback(fv.loopback() && (flags & IFF_LOOPBACK));
	fa.set_point_to_point(fv.point_to_point()
			      && (flags & IFF_POINTOPOINT)
			      && has_peer_addr);
	fa.set_multicast(fv.multicast() && (flags & IFF_MULTICAST));
	
	fa.set_prefix(subnet_mask.prefix_length());
	if (fa.broadcast())
	    fa.set_bcast(broadcast_addr.get_ipv4());
	if (fa.point_to_point())
	    fa.set_endpoint(peer_addr.get_ipv4());
    }
    if (ferror(fh)) {
	XLOG_ERROR("%s read failed: %s", 
	    PROC_LINUX_FILE_V4, strerror(errno));
    }

    // Clean up, shut down socket
    fclose(fh);
    close(soc);

    return true;
}

// 
// IPv6 Interface info fetch derived from ifconfig_parse_ifreq.cc
//
#ifdef HAVE_IPV6
bool if_fetch_linux_v6(IfTree& it)
{
    FILE *fh;
    int soc;
    char * ifname;
    struct ifreq ifr;
    char addr6p[8][5], addr6[40];
    int plen, scope, dad_status, if_idx;

    fh = fopen(PROC_LINUX_FILE_V6, "r");
    if (!fh) {
	XLOG_FATAL("Warning: cannot open %s (%s).",
	    PROC_LINUX_FILE_V6, strerror(errno)); 
	return false;
	}	

    soc = socket(AF_INET6, SOCK_DGRAM, 0);
    if (soc < 0) {
	XLOG_FATAL("Could not initialize ioctl() socket");
	return false;
    }

    //while (fscanf(fh, "%s\n", ifname) != EOF) {
    while (fscanf(fh, "%4s%4s%4s%4s%4s%4s%4s%4s %02x %02x %02x %02x %20s\n",
		  addr6p[0], addr6p[1], addr6p[2], addr6p[3],
		  addr6p[4], addr6p[5], addr6p[6], addr6p[7],
		  &if_idx, &plen, &scope, &dad_status, ifname) != EOF) {
	it.add_if(ifname); // retrieve node, create if necessary 
	debug_msg("interface: %s\n",ifname);

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
	debug_msg("MAC address: %s\n",mac.str().c_str());

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
	fv.set_pif_index(if_idx);
	debug_msg("interface index: %d\n",fv.pif_index());

	// set vif flags
	fv.set_enabled(fi.enabled() && (flags & IFF_UP));
	fv.set_broadcast(false);
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
	// Get the IP address, netmask, broadcast address, P2P destination
	//
	// The default values
	IPvX lcl_addr = IPvX::ZERO(AF_INET6);
	IPvX subnet_mask = IPvX::ZERO(AF_INET6);
	IPvX peer_addr = IPvX::ZERO(AF_INET6);
	bool has_peer_addr = false;

	// Get the IP address
	sprintf(addr6, "%s:%s:%s:%s:%s:%s:%s:%s",
		addr6p[0], addr6p[1], addr6p[2], addr6p[3],
		addr6p[4], addr6p[5], addr6p[6], addr6p[7]);
	lcl_addr = IPvX(addr6);
	debug_msg("IP address: %s\n", lcl_addr.str().c_str());

	// Get the netmask
	subnet_mask = IPvX::make_prefix(AF_INET6, plen);
	debug_msg("IP netmask: %s\n", subnet_mask.str().c_str());

	// Get the p2p address
	if (fv.point_to_point()) {
	    XLOG_FATAL("IPv6 point-to-point address unimplemented");
	    has_peer_addr = true;
	    // debug_msg("Peer address: %s\n", peer_addr.str().c_str());
	}

	debug_msg("\n"); // put empty line between interfaces

	// Add the address
	fv.add_addr(lcl_addr.get_ipv6());
	IfTreeAddr6& fa = fv.get_addr(lcl_addr.get_ipv6())->second;
	fa.set_enabled(fv.enabled() && (flags & IFF_UP));
	fa.set_loopback(fv.loopback() && (flags & IFF_LOOPBACK));
	fa.set_point_to_point(fv.point_to_point() && (flags & IFF_POINTOPOINT));
	fa.set_multicast(fv.multicast() && (flags & IFF_MULTICAST));
	
	fa.set_prefix(subnet_mask.prefix_length());
	if (fa.point_to_point())
	    fa.set_endpoint(peer_addr.get_ipv6());
    }
    if (ferror(fh)) {
	XLOG_ERROR("%s read failed: %s", 
	    PROC_LINUX_FILE_V6, strerror(errno));
    }

    fclose(fh);
    close(soc);

    return true;
}
#endif // HAVE_IPV6

#endif // HAVE_PROC_LINUX
