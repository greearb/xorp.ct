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

#ident "$XORP: xorp/mfea/mfea_unix_if_ioctl.cc,v 1.2 2003/03/05 23:13:08 pavlin Exp $"


//
// Implementation of ioctl(2)-based method to obtain from the kernel
// information about the network interfaces.
//


#include "mfea_module.h"
#include "mfea_private.hh"
#include "mfea_unix_osdep.hh"

#if defined(KERNEL_IF_METHOD) && (KERNEL_IF_METHOD == KERNEL_IF_IOCTL)

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
 * The interfaces information is obtained by using ioctl(SIOCGIFCONF).
 * 
 * Return value: The number of created vifs, otherwise %XORP_ERROR.
 **/
int
UnixComm::get_mcast_vifs_osdep(vector<MfeaVif *>& mfea_vifs_vector)
{
    int			ifnum, len, lastlen;
    char		*ptr;
    struct ifconf	ifconf;
    struct ifreq	*ifreq;
    struct ifreq	ifrcopy;
#ifdef HAVE_IPV6
    struct in6_ifreq	*ifreq6;
    struct in6_ifreq	ifrcopy6;
#endif // HAVE_IPV6
    struct sockaddr_in	*sinptr;
    MfeaVif		*mfea_vif = NULL;
    int			found_vifs = 0;
    
    ifnum = 32;			// XXX: initial guess
    ifconf.ifc_buf = NULL;
    lastlen = 0;
    
    // Loop until SIOCGIFCONF success.
    for ( ; ; ) {
	ifconf.ifc_len = ifnum*sizeof(struct ifreq);
	if (ifconf.ifc_buf != NULL)
	    delete[] ifconf.ifc_buf;
	ifconf.ifc_buf = new char[ifconf.ifc_len];
	if (ioctl(_ioctl_socket, SIOCGIFCONF, &ifconf) < 0) {
	    // Check UNPv1, 2e, pp 435 for an explanation why we need this
	    if ((errno != EINVAL) || (lastlen != 0)) {
		XLOG_ERROR("ioctl(SIOCGIFCONF) failed: %s", strerror(errno));
		delete[] ifconf.ifc_buf;
		return (XORP_ERROR);
	    }
	} else {
	    if (ifconf.ifc_len == lastlen)
		break;		// success, len has not changed
	    lastlen = ifconf.ifc_len;
	}
	ifnum += 10;
    }
    
    found_vifs = 0;
    mfea_vif = NULL;
    
    // Read the info
    for (ptr = ifconf.ifc_buf; ptr < ifconf.ifc_buf + ifconf.ifc_len; ) {
	ifreq = (struct ifreq *)ptr;
#ifdef HAVE_IPV6
	ifreq6 = (struct in6_ifreq *)ifreq;
#endif // HAVE_IPV6
#ifdef HAVE_SA_LEN
	len = MAX(sizeof(struct sockaddr), ifreq->ifr_addr.sa_len);
#else
	switch (ifreq->ifr_addr.sa_family) {
#ifdef HAVE_IPV6
	case AF_INET6:
	    len = sizeof(struct sockaddr_in6);
	    break;
#endif // HAVE_IPV6
	case AF_INET:
	default:
	    len = sizeof(struct sockaddr);
	    break;
	}
#endif // HAVE_SA_LEN
	len += sizeof(ifreq->ifr_name);
	len = MAX(len, (int)sizeof(struct ifreq));
	ptr += len;				// Point to the next entry
	
	// Get the vif name
	char *cptr, tmp_vif_name[IFNAMSIZ+1];
	strncpy(tmp_vif_name, ifreq->ifr_name, sizeof(tmp_vif_name));
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
	    int flags = 0;
	    // Get the flags
	    switch (family()) {
	    case AF_INET:
		memcpy(&ifrcopy, ifreq, sizeof(ifrcopy));
		if (ioctl(_ioctl_socket, SIOCGIFFLAGS, &ifrcopy) < 0) {
		    XLOG_ERROR("ioctl(SIOCGIFFLAGS, vif %s) failed: %s",
			       vif_name.c_str(), strerror(errno));
		    continue;
		}
		flags = ifrcopy.ifr_flags;
		break;
#ifdef HAVE_IPV6
	    case AF_INET6:
		memcpy(&ifrcopy6, ifreq6, sizeof(ifrcopy6));
		if (ioctl(_ioctl_socket, SIOCGIFFLAGS, &ifrcopy6) < 0) {
		    XLOG_ERROR("ioctl(SIOCGIFFLAGS, vif %s) failed: %s",
			       vif_name.c_str(), strerror(errno));
		    continue;
		}
		// XXX: in FreeBSD-4.0 'ifcopy6.ifr_flags' doesn't work anymore
		flags = ifrcopy6.ifr_ifru.ifru_flags;
		break;
#endif // HAVE_IPV6
	    default:
		break;
	    }
	    // Create the new vif
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

	// Check if an interface with same address family
	if (ifreq->ifr_addr.sa_family != family())
	    continue;
	
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
	    sinptr = (struct sockaddr_in *)&ifreq->ifr_addr;
	    KERNEL_SADDR_ADJUST(sinptr);		// XXX
	    lcl_addr.copy_in(*sinptr);
	}
	
	// Get the netmask
	switch (family()) {
	case AF_INET:
#ifdef SIOCGIFNETMASK
	    memcpy(&ifrcopy, ifreq, sizeof(ifrcopy));
	    if (ioctl(_ioctl_socket, SIOCGIFNETMASK, &ifrcopy) < 0) {
		if (! mfea_vif->is_p2p()) {
		    XLOG_ERROR("ioctl(SIOCGIFNETMASK) failed: %s",
			       strerror(errno));
		}
	    } else {
		// The interface is configured
		sinptr = (struct sockaddr_in *)&ifrcopy.ifr_addr;
		// XXX: SIOCGIFNETMASK doesn't return proper sin_family
		sinptr->sin_family = family();
		subnet_mask.copy_in(*sinptr);
	    }
#endif // SIOCGIFNETMASK
	    break;
#ifdef HAVE_IPV6
	case AF_INET6:
#ifdef SIOCGIFNETMASK
	    memcpy(&ifrcopy6, ifreq6, sizeof(ifrcopy6));
	    if ((ioctl(_ioctl_socket, SIOCGIFNETMASK_IN6, &ifrcopy6) < 0)
		&& (ioctl(_ioctl_socket, SIOCGIFNETMASK, &ifrcopy6) < 0)) {
		if (! mfea_vif->is_p2p()) {
		    XLOG_ERROR("ioctl(SIOCGIFNETMASK_IN6) failed: %s",
			       strerror(errno));
		}
	    } else {
		// The interface is configured
		sinptr = (struct sockaddr_in *)&ifrcopy6.ifr_addr;
		// XXX: SIOCGIFNETMASK_IN6 doesn't return proper sin_family
		sinptr->sin_family = family();
		subnet_mask.copy_in(*sinptr);
	    }
#endif // SIOCGIFNETMASK
	    break;
#endif // HAVE_IPV6
	default:
	    XLOG_ASSERT(false);
	    break;
	}
	
	// Get the broadcast address	
	
	if (mfea_vif->is_broadcast_capable()) {
	    switch (family()) {
	    case AF_INET:
#ifdef SIOCGIFBRDADDR
		memcpy(&ifrcopy, ifreq, sizeof(ifrcopy));
		if (ioctl(_ioctl_socket, SIOCGIFBRDADDR, &ifrcopy) < 0) {
		    XLOG_ERROR("ioctl(SIOCGIFBRADDR) failed: %s",
			       strerror(errno));
		} else {
		    sinptr = (struct sockaddr_in *)&ifrcopy.ifr_broadaddr;
		    // XXX: just in case it doesn't return proper sin_family
		    sinptr->sin_family = family();
		    broadcast_addr.copy_in(*sinptr);
		}
#endif // SIOCGIFBRDADDR
		break;
#ifdef HAVE_IPV6
	    case AF_INET6:
		break;		// IPv6 doesn't have the idea of broadcast
#endif // HAVE_IPV6
	    default:
		XLOG_ASSERT(false);
		break;
	    }
	}
	
	// Get the p2p address
	if (mfea_vif->is_p2p()) {
	    switch (family()) {
	    case AF_INET:
#ifdef SIOCGIFDSTADDR
		memcpy(&ifrcopy, ifreq, sizeof(ifrcopy));
		if (ioctl(_ioctl_socket, SIOCGIFDSTADDR, &ifrcopy) < 0) {
		    // Probably the p2p address is not configured
		} else {
		    // The p2p address is configured
		    sinptr = (struct sockaddr_in *)&ifrcopy.ifr_dstaddr;
		    // Just in case it doesn't return proper sin_family
		    sinptr->sin_family = family();
		    peer_addr.copy_in(*sinptr);
		}
#endif // SIOCGIFDSTADDR
		break;
#ifdef HAVE_IPV6
	    case AF_INET6:
#ifdef SIOCGIFDSTADDR
		memcpy(&ifrcopy6, ifreq6, sizeof(ifrcopy6));
		if (ioctl(_ioctl_socket, SIOCGIFDSTADDR_IN6, &ifrcopy6) < 0) {
		    // Probably the p2p address is not configured
		} else {
		    // The p2p address is configured
		    sinptr = (struct sockaddr_in *)&ifrcopy6.ifr_dstaddr;
		    // Just in case it doesn't return proper sin_family
		    sinptr->sin_family = family();
		    peer_addr.copy_in(*sinptr);
		}
#endif // SIOCGIFDSTADDR
		break;
#endif // HAVE_IPV6
	    default:
		XLOG_ASSERT(false);
		break;
	    }
	}
	
	// Add the address
	IPvXNet subnet_addr(lcl_addr, subnet_mask.prefix_length());
	mfea_vif->add_address(lcl_addr, subnet_addr, broadcast_addr, peer_addr);
    }
    delete[] ifconf.ifc_buf;
    
    return (found_vifs);
}

#endif // KERNEL_IF_METHOD == KERNEL_IF_IOCTL
