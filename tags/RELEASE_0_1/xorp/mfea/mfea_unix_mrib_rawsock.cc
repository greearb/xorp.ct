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

#ident "$XORP: xorp/mfea/mfea_unix_mrib_rawsock.cc,v 1.22 2002/12/09 18:29:18 hodson Exp $"


//
// Implementation of Reverse Path Forwarding kernel query mechanism,
// that would be used for Multicast Routing Information Base information:
// AF_ROUTE (BSD-style routing socket).
//


#include "mfea_module.h"
#include "mfea_private.hh"
#include "mfea_unix_osdep.hh"

#if defined(KERNEL_MRIB_METHOD) && (KERNEL_MRIB_METHOD == KERNEL_MRIB_RAWSOCK)

#include <fcntl.h>
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif

#include "mrt/mrib_table.hh"
#include "mfea_node.hh"
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
 * UnixComm::open_mrib_socket_osdep:
 * 
 * Open and initialize a routing socket (AF_ROUTE) to access/lookup
 * the unicast forwarding table in the kernel.
 * (BSD-style AF_ROUTE routing socket)
 * 
 * Return value: The socket value on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::open_mrib_socket_osdep()
{
    int sock;
    
    //
    // XXX: strictly speaking, the third argument should be
    // 'protocol family', i.e PF_* instead of AF_*, but in reality
    // they are same.
    //
    if ( (sock = socket(AF_ROUTE, SOCK_RAW, family())) < 0) {
	XLOG_ERROR("Cannot open AF_ROUTE raw socket stream: %s",
		   strerror(errno));
	return (XORP_ERROR);
    }
    // Don't block
    if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0) {
	close(sock);
	XLOG_ERROR("fcntl(AF_ROUTE, O_NONBLOCK) failed: %s", strerror(errno));
	return (XORP_ERROR);
    }
    
    return (sock);
}


/**
 * UnixComm::get_mrib_osdep:
 * @dest_addr: The destination address to lookup.
 * @mrib: A reference to a #Mrib structure to return the MRIB information.
 * 
 * Get the Multicast Routing Information Base (MRIB) information
 * (the next hop router and the vif to send out packets toward a given
 * destination) for a given address.
 * (BSD-style AF_ROUTE routing socket)
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::get_mrib_osdep(const IPvX& dest_addr, Mrib& mrib)
{
    int			i, msglen;
    int			my_rtm_seq;
    struct rt_msghdr	*rtm;
    struct sockaddr_in	*sin;
    struct sockaddr	*sa, *rti_info[RTAX_MAX];
#define RTMBUFSIZE (sizeof(struct rt_msghdr) + 512)
    char		rtmbuf[RTMBUFSIZE];
    MfeaVif		*mfea_vif;

    if (dest_addr.af() != family())
	return (XORP_ERROR);
    
    // Zero the MRIB information
    mrib.set_dest_prefix(IPvXNet(dest_addr, dest_addr.addr_bitlen()));
    mrib.set_next_hop_router_addr(IPvX::ZERO(family()));
    mrib.set_next_hop_vif_index(Vif::VIF_INDEX_INVALID);
    // XXX: the UNIX kernel doesn't provide reasonable metrics
    mrib.set_metric(MRIB_DEFAULT_METRIC);
    mrib.set_metric_preference(MRIB_DEFAULT_METRIC_PREFERENCE);
    
    if (! dest_addr.is_unicast())
	return (XORP_ERROR);
    
    // Check if the lookup address is one of my own interfaces or a neighbor
    mfea_vif = mfea_node().vif_find_direct(dest_addr);
    if (mfea_vif != NULL) {
	mrib.set_next_hop_router_addr(dest_addr);
	mrib.set_next_hop_vif_index(mfea_vif->vif_index());
	return (XORP_OK);
    }
    
    memset(rtmbuf, 0, sizeof(rtmbuf));
    rtm = (struct rt_msghdr *)rtmbuf;
    
    switch (family()) {
    case AF_INET:
	rtm->rtm_msglen = sizeof(*rtm) + sizeof(struct sockaddr_in);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	rtm->rtm_msglen = sizeof(*rtm) + sizeof(struct sockaddr_in6);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	break;
    }
    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_type = RTM_GET;
    rtm->rtm_addrs = (RTA_DST | RTA_GATEWAY | RTA_IFP);
    rtm->rtm_flags = RTF_UP;
    rtm->rtm_pid = _pid;
    my_rtm_seq = _rtm_seq++;
    if (_rtm_seq == 0)
	_rtm_seq++;
    rtm->rtm_seq = my_rtm_seq;
    
    sin = (struct sockaddr_in *)(rtm + 1);
    dest_addr.copy_out(*sin);
    
    if (write(_mrib_socket, rtm, rtm->rtm_msglen) < 0) {
	XLOG_ERROR("get_mrib(AF_ROUTE) failed: "
		   "error writing to routing socket: %s",
		   strerror(errno));
	return (XORP_ERROR);
    }
    do {
	msglen = read(_mrib_socket, rtm, sizeof(rtmbuf));
	if (msglen < 0) {
	    if (errno == EINTR)
		continue;
	    XLOG_ERROR("get_mrib(AF_ROUTE) failed: "
		       "error reading from socket: %s",
		       strerror(errno));
	    return (XORP_ERROR);
	}
	if (msglen < (int)sizeof(*rtm)) {
	    XLOG_ERROR("get_mrib(AF_ROUTE) failed: "
		       "error reading from socket: "
		       "message truncated: %d instead of (at least) %d",
		       msglen, sizeof(*rtm));
	    return (XORP_ERROR);
	}
    } while ( (rtm->rtm_type != RTM_GET)
	      || (rtm->rtm_seq  != my_rtm_seq)
	      || (rtm->rtm_pid  != _pid));
    
    // version check
    if (rtm->rtm_version != RTM_VERSION) {
	XLOG_WARNING("Routing message version is %d: should be %d. "
		     "This may cause a problem.",
		     rtm->rtm_version, RTM_VERSION);
    }
    
    sa = (struct sockaddr *) (rtm + 1);
    for (i = 0; i < RTAX_MAX; i++) {
	if (rtm->rtm_addrs & (1 << i)) {
	    rti_info[i] = sa;
	    NEXT_SA(sa);
	} else {
	    rti_info[i] = NULL;
	}
    }
    
    mfea_vif = NULL;
    if ( (sa = rti_info[RTAX_GATEWAY]) != NULL) {
	IPvX sin_addr(family());
	struct sockaddr_in *sin = (struct sockaddr_in *)sa;
	sin_addr.copy_in(*sin);
	mrib.set_next_hop_router_addr(sin_addr);
	if ( ( sa = rti_info[RTAX_IFP]) != NULL) {
	    // Get the interface name and then find the vif
	    struct sockaddr_dl *ifp;
	    ifp = (struct sockaddr_dl *)sa;
	    mfea_vif = mfea_node().vif_find_by_name(ifp->sdl_data);
	}
	if (mfea_vif == NULL) {
	    // XXX: this shoudn't happen, but who knows...
	    mfea_vif = mfea_node().vif_find_direct(dest_addr);
	}
    }
    
    if (mfea_vif != NULL) {
	mrib.set_next_hop_vif_index(mfea_vif->vif_index());
	return (XORP_OK);		// XXX
    } else {
	mrib.set_next_hop_vif_index(Vif::VIF_INDEX_INVALID);
	return (XORP_ERROR);
    }
}

#endif // KERNEL_MRIB_METHOD == KERNEL_MRIB_RAWSOCK
