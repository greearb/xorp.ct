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

#ident "$XORP: xorp/mfea/mfea_unix_if_sysctl.cc,v 1.29 2002/12/09 18:29:18 hodson Exp $"


//
// Implementation of sysctl(3)-based method to obtain from the kernel
// information about the network interfaces.
//


#include "mfea_module.h"
#include "mfea_private.hh"
#include "mfea_unix_osdep.hh"

#if defined(KERNEL_IF_METHOD) && (KERNEL_IF_METHOD == KERNEL_IF_SYSCTL)

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
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
 * The interfaces information is obtained by using sysctl().
 * 
 * Return value: The number of created vifs, otherwise %XORP_ERROR.
 **/
int
UnixComm::get_mcast_vifs_osdep(vector<MfeaVif *>& mfea_vifs_vector)
{
    int			found_vifs = 0;
    MfeaVif		*mfea_vif = NULL;
    caddr_t		buffer, buffer_begin, buffer_end;
    size_t		buffer_len;
    struct if_msghdr	*ifm;
    struct ifa_msghdr	*ifam;
    struct sockaddr	*sa, *rti_info[RTAX_MAX];
    int			mib[] = {CTL_NET, AF_ROUTE, 0, -1, NET_RT_IFLIST, 0};
    
    mib[3] = family();		// XXX
    
    // Get first the interface table size
    if (sysctl(mib, sizeof(mib)/sizeof(mib[0]), NULL, &buffer_len, NULL, 0)
	< 0) {
	XLOG_ERROR("sysctl(NET_RT_IFLIST) failed: %s", strerror(errno));
	return (XORP_ERROR);
    }
    
    // Allocate buffer space
    do {
	buffer = new char[buffer_len];
	buffer_begin = buffer;
	if (sysctl(mib, sizeof(mib)/sizeof(mib[0]), buffer, &buffer_len,
		   NULL, 0) >= 0) {
	    break;
	}
	delete[] buffer_begin;
	if (errno == ENOMEM) {
	    // Buffer is too small. Try again
	    continue;
	}
	XLOG_ERROR("sysctl(NET_RT_IFLIST) failed: %s", strerror(errno));
	return (XORP_ERROR);
    } while (true);
    
    // Parse the result and place it in the vif vector
    for (buffer_end = buffer_begin + buffer_len; buffer < buffer_end;
	 buffer += ifm->ifm_msglen) {
	ifm = (struct if_msghdr *)buffer;
	
	// Ignore entries that don't match
	if (ifm->ifm_version != RTM_VERSION)
	    continue;
	if ((ifm->ifm_type != RTM_IFINFO) && (ifm->ifm_type != RTM_NEWADDR))
	    continue;
	
	if (ifm->ifm_type == RTM_IFINFO) {
	    mfea_vif = NULL;
	    // A new interface

	    // Get the pointers to the corresponding data structures
	    sa = (struct sockaddr *) (ifm + 1);
	    for (size_t i = 0; i < RTAX_MAX; i++) {
		if (ifm->ifm_addrs & (1 << i)) {
		    rti_info[i] = sa;
		    NEXT_SA(sa);
		} else {
		    rti_info[i] = NULL;
		}
	    }
	    if ( (sa = rti_info[RTAX_IFP]) == NULL) {
		XLOG_WARNING("Ignoring invalid interface");
		continue;
	    }
	    
	    // Get the vif name
	    char *cptr, tmp_vif_name[IFNAMSIZ+1];
	    memset(tmp_vif_name, 0, sizeof(tmp_vif_name));
	    struct sockaddr_dl *sdl = (struct sockaddr_dl *)sa;
	    if (sdl->sdl_nlen <= 0) {
		XLOG_ERROR("Ignoring interface with unknown name");
		continue;
	    }
	    snprintf(tmp_vif_name,
		     sizeof(tmp_vif_name) / sizeof(tmp_vif_name[0]),
		     "%*s",
		     sdl->sdl_nlen,
		     &sdl->sdl_data[0]);
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
		if (mfea_vif_tmp->name() == vif_name) {
		    // Found
		    mfea_vif = mfea_vif_tmp;
		    break;
		}
	    }
	    
	    // Add a new vif	
	    if (mfea_vif == NULL) {
		int flags = ifm->ifm_flags;
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

	    // Get the physical interface index
	    if (mfea_vif->pif_index() == 0) {
		unsigned int pif_index = 0;
		pif_index = ifm->ifm_index;
		if (pif_index > 0)
		    mfea_vif->set_pif_index(pif_index);
	    }
	    // Try to get again the physical interface index
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
		    strncpy(ifridx.ifr_name, mfea_vif->name().c_str(),
			    IFNAMSIZ);
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
	} // RTM_INFO
	
	if (ifm->ifm_type == RTM_NEWADDR) {
	    // Misc. checks
	    if (mfea_vif == NULL) {
		XLOG_ASSERT(false);		// Cannot happen
		continue;
	    }
	    ifam = (struct ifa_msghdr *)ifm;
	    if ((mfea_vif->pif_index() > 0)
		&& (mfea_vif->pif_index() != ifam->ifam_index)) {
		XLOG_ASSERT(false);		// Cannot happen
		continue;
	    }
	    
	    // Get the pointers to the corresponding data structures
	    sa = (struct sockaddr *)(ifam + 1);
	    for (size_t i = 0; i < RTAX_MAX; i++) {
		if (ifam->ifam_addrs & (1 << i)) {
		    rti_info[i] = sa;
		    NEXT_SA(sa);
		} else {
		    rti_info[i] = NULL;
		}
	    }
	    
	    // Check if an interface with an IP address and same address family
	    if (( (sa = rti_info[RTAX_IFA]) == NULL)
		|| (sa->sa_family != family())) {
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
		KERNEL_SADDR_ADJUST(sa);			// XXX
		lcl_addr.copy_in(*(struct sockaddr_in *)sa);
	    }
	    
	    // Get the netmask
	    if ((sa = rti_info[RTAX_NETMASK]) != NULL) {
		sa->sa_family = family(); // XXX: for IPv4 it may be invalid
		subnet_mask.copy_in(*(struct sockaddr_in *)sa);
	    }
	    
	    // Get the broadcast address
	    if (((sa = rti_info[RTAX_BRD]) != NULL) && mfea_vif->is_broadcast_capable()) {
		broadcast_addr.copy_in(*(struct sockaddr_in *)sa);
	    }
	    
	    // Set the p2p address
	    if (((sa = rti_info[RTAX_BRD]) != NULL) && mfea_vif->is_p2p()) {
		peer_addr.copy_in(*(struct sockaddr_in *)sa);
	    }
	    
#ifdef HAVE_IPV6
	    if (family() == AF_INET6) {
		//
		// Get IPv6 specific flags
		//
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
		// Ignore the anycast addresses.
		// XXX: what about TENTATIVE, DUPLICATED, DETACHED, DEPRECATED?
		//
		if (ifrcopy6.ifr_ifru.ifru_flags6 & IN6_IFF_ANYCAST)
		    continue;
	    }
#endif // HAVE_IPV6
	    
	    // Add the address
	    IPvXNet subnet_addr(lcl_addr, subnet_mask.prefix_length());
	    mfea_vif->add_address(lcl_addr, subnet_addr, broadcast_addr,
				 peer_addr);
	}
    }
    // Free the temp. buffer
    delete[] buffer_begin;
    
    return (found_vifs);
}

#endif // KERNEL_IF_METHOD == KERNEL_IF_SYSCTL
