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

#ident "$XORP: xorp/mfea/mfea_unix_rtread_sysctl.cc,v 1.6 2003/04/15 18:55:37 pavlin Exp $"


//
// Implementation of Unicast Routing Table kernel read mechanism:
// sysctl(3).
//


#include "mfea_module.h"
#include "mfea_private.hh"
#include "mfea_unix_osdep.hh"

#if defined(KERNEL_RTREAD_METHOD) && (KERNEL_RTREAD_METHOD == KERNEL_RTREAD_SYSCTL)

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
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
 * UnixComm::open_mrib_table_socket_osdep:
 * 
 * Open and initialize a socket for reading the routing table from the kernel.
 * XXX: If we use sysctl() to read the table, we don't need such socket,
 * hence the function below is unused.
 * 
 * Return value: The socket value on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::open_mrib_table_socket_osdep()
{
    return (XORP_ERROR);
}

/**
 * UnixComm::get_mrib_table_osdep:
 * @return_mrib_table: A pointer to the routing table array composed
 * of #Mrib elements.
 * 
 * Return value: the number of entries in @return_mrib_table, or %XORP_ERROR
 * if there was an error.
 **/
int
UnixComm::get_mrib_table_osdep(Mrib *return_mrib_table[])
{
    int			i;
    caddr_t		buffer, buffer_begin, buffer_end;
    size_t		buffer_len;
    Mrib		*mrib, *mrib_table;
    int			mrib_table_n;
    struct rt_msghdr	*rtm;
    struct sockaddr	*sa, *rti_info[RTAX_MAX];
    int			mib[] = { CTL_NET, AF_ROUTE, 0, -1, NET_RT_DUMP, 0 };
    IPvX		dest_addr(family());
    int			dest_masklen;
    MfeaVif		*mfea_vif;
    
    mib[3] = family();		// XXX
    
    *return_mrib_table = NULL;
    
    // Get first the routing table size
    if (sysctl(mib, sizeof(mib)/sizeof(mib[0]), NULL, &buffer_len, NULL, 0)
	< 0) {
	XLOG_ERROR("sysctl(NET_RT_DUMP) failed: %s", strerror(errno));
	return (XORP_ERROR);
    }
    
    do {
	// Allocate buffer
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
	XLOG_ERROR("sysctl(NET_RT_DUMP) failed: %s", strerror(errno));
	return (XORP_ERROR);
    } while (true);
    
    mrib_table_n = 0;
    // Find first the routing table size, and then allocate the memory.
    for (buffer_end = buffer_begin + buffer_len; buffer < buffer_end;
	 buffer += rtm->rtm_msglen) {
	rtm = (struct rt_msghdr *)buffer;
	mrib_table_n++;
    }
    mrib_table = new Mrib[mrib_table_n](family());
    
    mrib_table_n = 0;
    buffer = buffer_begin;
    // Parse the result and place it in the table
    for (buffer_end = buffer + buffer_len; buffer < buffer_end;
	 buffer += rtm->rtm_msglen) {
	rtm = (struct rt_msghdr *)buffer;
	if (rtm->rtm_type != RTM_GET)
	    continue;
	if (rtm->rtm_version != RTM_VERSION)
	    continue;
	if ( !(rtm->rtm_flags & RTF_UP))
	    continue;		// Route is DOWN
	
	// Init the next entry
	mrib = &mrib_table[mrib_table_n];
	
	// Get the pointers to the corresponding data structures
	sa = (struct sockaddr *) (rtm + 1);
	for (i = 0; i < RTAX_MAX; i++) {
	    if (rtm->rtm_addrs & (1 << i)) {
		rti_info[i] = sa;
		NEXT_SA(sa);
	    } else {
		rti_info[i] = NULL;
	    }
	}
	
	// Get destination address and mask length
	if ( (sa = rti_info[RTAX_DST]) == NULL) {
	    XLOG_ERROR("sysctl(NET_RT_DUMP) failed: "
		       "bogus routing table entry: unknown destination");
	    continue;		// Unknown destination
	}
	if (sa->sa_family != family())
	    continue;
	KERNEL_SADDR_ADJUST(sa);
	dest_addr.copy_in(*(struct sockaddr_in *)sa);
	if (rtm->rtm_flags & RTF_HOST) {
	    dest_masklen = dest_addr.addr_bitlen();
	} else {
	    if ( (sa = rti_info[RTAX_GENMASK]) == NULL)
		sa = rti_info[RTAX_NETMASK];
	    if (sa == NULL) {
		XLOG_WARNING("sysctl(NET_RT_DUMP) failed: "
			     "bogus routing table entry: unknown mask");
		continue;		// Unknown mask
	    }
	    //
	    // XXX: we don't check for sa->family, because in some cases it
	    // is not set properly.
	    //
	    // Compute the masklen
	    sa->sa_family = family();		// XXX
	    IPvX dest_mask(family());
	    dest_mask.copy_in(*(struct sockaddr_in *)sa);
	    dest_masklen = dest_mask.prefix_length();
	}
	mrib->set_dest_prefix(IPvXNet(dest_addr, dest_masklen));
	
	// XXX: the UNIX kernel doesn't provide reasonable metrics
	mrib->set_metric_preference(mfea_node().mrib_table_default_metric_preference().get());
	mrib->set_metric(mfea_node().mrib_table_default_metric().get());
	
#ifdef RTF_MULTICAST
	if (rtm->rtm_flags & RTF_MULTICAST)
	    continue;		// XXX: ignore multicast entries
#endif // RTF_MULTICAST
	
	if (rtm->rtm_flags & RTF_GATEWAY) {
	    // The address of the gateway router and the vif to it
	    if ( (sa = rti_info[RTAX_GATEWAY]) != NULL) {
		KERNEL_SADDR_ADJUST(sa);
		if (sa->sa_family == family()) {
		    IPvX router_addr(family());
		    router_addr.copy_in(*(struct sockaddr_in *)sa);
		    mrib->set_next_hop_router_addr(router_addr);
		    mfea_vif = mfea_node().vif_find_same_subnet_or_p2p(mrib->next_hop_router_addr());
		    if ((mfea_vif != NULL)
			&& (mfea_vif->is_underlying_vif_up())) {
			mrib->set_next_hop_vif_index(mfea_vif->vif_index());
		    } else {
			mfea_vif = NULL;
			XLOG_ERROR("sysctl(NET_RT_DUMP) failed: "
				   "cannot find interface toward next-hop router %s",
				   cstring(mrib->next_hop_router_addr()));
		    }
		}
	    }
	} else if (rtm->rtm_flags & RTF_LLINFO) {
	    //  Link-local entry (could be the broadcast address as well)
	    bool not_bcast_addr = true;
#ifdef RTF_BROADCAST
	    if (rtm->rtm_flags & RTF_BROADCAST)
		not_bcast_addr = false;
#endif
	    if (not_bcast_addr)
		mrib->set_next_hop_router_addr(mrib->dest_prefix().masked_addr());
	    mfea_vif = mfea_node().vif_find_same_subnet_or_p2p(mrib->next_hop_router_addr());
	    if ((mfea_vif != NULL) && (mfea_vif->is_underlying_vif_up())) {
		mrib->set_next_hop_vif_index(mfea_vif->vif_index());
	    } else {
		mfea_vif = NULL;
		XLOG_ERROR("sysctl(NET_RT_DUMP) failed: "
			   "cannot find interface toward next-hop router %s",
			   cstring(mrib->next_hop_router_addr()));
	    }
	}
	if (mrib->next_hop_vif_index() >= mfea_node().maxvifs()) {
	    // Quite likely an entry for a directly connected LAN
	    if ( ( sa = rti_info[RTAX_GATEWAY]) != NULL) {
		if (sa->sa_family == AF_LINK) {
		    // Get the interface name and then find the vif
		    struct sockaddr_dl *ifp;
		    ifp = (struct sockaddr_dl *)sa;
		    mfea_vif = mfea_node().vif_find_by_pif_index(ifp->sdl_index);
		    if (mfea_vif != NULL)
			mrib->set_next_hop_vif_index(mfea_vif->vif_index());
		}
	    }
	}
#if 1	// TODO: remove?
	// The very last resource. Try (again) whether it is a LAN addr.
	if (mrib->next_hop_vif_index() >= mfea_node().maxvifs()) {
	    mfea_vif = mfea_node().vif_find_same_subnet_or_p2p(mrib->dest_prefix().masked_addr());
	    if ((mfea_vif != NULL) && mfea_vif_is_underlying_vif_up()) {
		mrib->set_next_hop_vif_index(mfea_vif->vif_index());
	    } else {
		mfea_vif = NULL;
	    }
	}
#endif // 1/0
	
	//
	// XXX: 'mrib->next_hop_vif_index()' may still be invalid, if the
	// particular vif was disabled. Don't ignore this routing entry.
	//
	
	mrib_table_n++;
    }
    
    // Free the temp. buffer
    delete[] buffer_begin;
    
    *return_mrib_table = mrib_table;
    return (mrib_table_n);
}

#endif // KERNEL_RTREAD_METHOD == KERNEL_RTREAD_SYSCTL
