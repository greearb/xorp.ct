// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/mfea/mfea_unix_if_getifaddrs.cc,v 1.2 2003/01/26 04:06:22 pavlin Exp $"


//
// Implementation of getifaddrs(3)-based method to obtain from the kernel
// information about the network interfaces.
//


#include "mfea_module.h"
#include "mfea_private.hh"
#include "mfea_unix_osdep.hh"

#if defined(KERNEL_IF_METHOD) && (KERNEL_IF_METHOD == KERNEL_IF_GETIFADDRS)

#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif

#include "mfea_vif.hh"
#include "mfea_unix_comm.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//

//
// Local variables
//

//
// Local functions prototypes
//


/**
 * UnixComm::get_mcast_vifs_osdep:
 * @mfea_vifs_vector: Reference to the vector to store the created vifs.
 * 
 * Query the kernel to find network interfaces that are multicast-capable.
 * The interfaces information is obtained by using getifaddrs().
 * 
 * Return value: The number of created vifs, otherwise %XORP_ERROR.
 **/
int
UnixComm::get_mcast_vifs_osdep(vector<MfeaVif *>& mfea_vifs_vector)
{
    int			found_vifs = 0;
    MfeaVif		*mfea_vif = NULL;
    struct ifaddrs	*ifa, *ifap;
    
    if (getifaddrs(&ifap) != 0) {
	XLOG_ERROR("getifaddrs() failed: %s", strerror(errno));
	return (XORP_ERROR);
    }
    
    // Parse the result and place it in the vif vector
    for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
	if (ifa->ifa_name == NULL) {
	    XLOG_WARNING("Ignoring interface with unknown name");
	    continue;
	}
	
	// Get the vif name
	char *cptr, tmp_vif_name[IFNAMSIZ+1];
	strncpy(tmp_vif_name, ifa->ifa_name, sizeof(tmp_vif_name));
	if ( (cptr = strchr(tmp_vif_name, ':')) != NULL) {
	    // Replace colon with null. Needed because in Solaris the
	    // interface name changes for aliases
	    *cptr = '\0';
	}
	string vif_name(tmp_vif_name);

	// Try to find if there is already an interface with same name
	mfea_vif = NULL;
	vector<MfeaVif *>::iterator iter;
	for (iter = mfea_vifs_vector.begin();
	     iter != mfea_vifs_vector.end();
	     ++iter) {
	    MfeaVif *mfea_vif_tmp = *iter;
	    if (mfea_vif_tmp->name() ==  vif_name) {
		// Found
		mfea_vif = mfea_vif_tmp;
		break;
	    }
	}
	
	// Add a new vif	
	if (mfea_vif == NULL) {
	    int flags = ifa->ifa_flags;
	    mfea_vif = new MfeaVif(mfea_node(), Vif(vif_name.c_str()));
	    mfea_vif->set_vif_index(found_vifs);
	    // Set the flags
	    if (flags & IFF_UP)
		mfea_vif->set_underlying_vif_up(true);
	    else
		mfea_vif->set_underlying_vif_up(false);
	    if (flags & IFF_POINTOPOINT)
		mfea_vif->set_p2p(true);
	    if (flags & IFF_LOOPBACK)
		mfea_vif->set_loopback(true);
	    if (flags & IFF_MULTICAST)
		mfea_vif->set_multicast_capable(true);
	    if (flags & IFF_BROADCAST)
		mfea_vif->set_broadcast_capable(true);
	    mfea_vifs_vector.push_back(mfea_vif);
	    found_vifs++;
	}
	
	//
	// Try to get the physical interface index
	//
#ifdef AF_LINK
	if ((ifa->ifa_addr != NULL) && (ifa->ifa_addr->sa_family == AF_LINK)) {
	    // Link-level address
	    struct sockaddr_dl *sdl = (struct sockaddr_dl *)ifa->ifa_addr;
	    mfea_vif->set_pif_index(sdl->sdl_index);
	    if (ifa->ifa_data != NULL) {
		// XXX: if we need more info (e.g., stats), check 'ifa_data'
		// ('struct if_data' as defined in include file <net/if.h>)
	    }
	}
#endif // AF_LINK
	if (mfea_vif->pif_index() == 0) {
	    unsigned int pif_index = 0;
#ifdef HAVE_IF_NAMETOINDEX
	    if (pif_index == 0) {
		// TODO: check whether for Solaris we have to use the
		// original interface name instead (i.e., the one that has
		// unique vif name per alias (see the ':' substitution
		// above).
		pif_index = if_nametoindex(mfea_vif->name().c_str());
	    }
#endif // HAVE_IF_NAMETOINDEX
#ifdef SIOCGIFINDEX
	    if (pif_index == 0) {
		struct ifreq ifridx;
		memset(&ifridx, 0, sizeof(ifridx));
		strncpy(ifridx.ifr_name, mfea_vif->name().c_str(), IFNAMSIZ);
		if (ioctl(_ioctl_socket, SIOCGIFINDEX, &ifridx) < 0) {
		    XLOG_ERROR("ioctl(SIOCGIFINDEX, vif %s) failed: %s",
			       ifridx.ifr_name, strerror(errno));
		} else {
		    pif_index = ifridx.ifr_ifindex;
		}
	    }
#endif // SIOCGIFINDEX
	    if (pif_index > 0)
		mfea_vif->set_pif_index(pif_index);
	}

	// Check if an interface with an IP address and same address family
	if ((ifa->ifa_addr == NULL)
	    || (ifa->ifa_addr->sa_family != family())) {
	    continue;
	}
	
	//
	// Get the IP address, netmask, broadcast address, P2P destination
	//
	// The default values
	IPvX lcl_addr = IPvX::ZERO(family());
	IPvX subnet_mask = IPvX::ZERO(family());
	IPvX broadcast_addr = IPvX::ZERO(family());
	IPvX peer_addr = IPvX::ZERO(family());
	
	// Get the IP address
	{
	    KERNEL_SADDR_ADJUST(ifa->ifa_addr);		// XXX
	    lcl_addr.copy_in(*(struct sockaddr_in *)ifa->ifa_addr);
	}
	
	// Get the netmask
	if (ifa->ifa_netmask != NULL) {
	    // XXX: for IPv4 the 'sa_family' it may be invalid
	    ifa->ifa_netmask->sa_family = family();
	    subnet_mask.copy_in(*(struct sockaddr_in *)ifa->ifa_netmask);
	}
	
	// Get the broadcast address
	if ((ifa->ifa_broadaddr != NULL) && mfea_vif->is_broadcast_capable()) {
	    broadcast_addr.copy_in(*(struct sockaddr_in *)ifa->ifa_broadaddr);
	}
	
	// Get the p2p address
	if ((ifa->ifa_dstaddr != NULL) && mfea_vif->is_p2p()) {
	    peer_addr.copy_in(*(struct sockaddr_in *)ifa->ifa_dstaddr);
	}
	
#ifdef HAVE_IPV6
	if (family() == AF_INET6) {
	    //
	    // Get IPv6 specific flags
	    //

#ifdef HOST_OS_MACOSX
	    //
	    // XXX: Some OS such as MacOS X 10.2.3 don't have struct in6_ifreq
	    // TODO: for now, allow the code to compile, but abort at run time
	    //
	    XLOG_FATAL("MacOS X doesn't have struct in6_ifreq. Aborting...");
	    return (XORP_ERROR);
	    
#else // ! HOST_OS_MACOSX
	    
	    struct in6_ifreq ifrcopy6;
	    memset(&ifrcopy6, 0, sizeof(ifrcopy6));
	    // TODO: see the Solaris-related ':' comment above
	    strncpy(ifrcopy6.ifr_name, mfea_vif->name().c_str(),
		    sizeof(ifrcopy6.ifr_name));
	    lcl_addr.copy_out(ifrcopy6.ifr_addr);
	    if (ioctl(_ioctl_socket, SIOCGIFAFLAG_IN6, &ifrcopy6) < 0) {
		XLOG_ERROR("ioctl(SIOCGIFAFLAG_IN6) failed: %s",
			   strerror(errno));
		continue;
	    }
	    //
	    // Ignore the anycast addresses
	    // XXX: what about TENTATIVE, DUPLICATED, DETACHED, DEPRECATED?
	    //
	    if (ifrcopy6.ifr_ifru.ifru_flags6 & IN6_IFF_ANYCAST)
		continue;
	    
#endif // ! HOST_OS_MACOSX
	    
	}
#endif // HAVE_IPV6
	
	// Add the address
	IPvXNet subnet_addr(lcl_addr, subnet_mask.prefix_length());
	mfea_vif->add_address(lcl_addr, subnet_addr, broadcast_addr, peer_addr);
    }
    freeifaddrs(ifap);
    
    return (found_vifs);
}

#endif // KERNEL_IF_METHOD == KERNEL_IF_GETIFADDRS
