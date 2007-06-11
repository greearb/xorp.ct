// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_media.cc,v 1.2 2007/06/06 19:55:53 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/c_format.hh"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_MEDIA_H
#include <net/if_media.h>
#endif
#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#endif
#ifdef HAVE_LINUX_ETHTOOL_H
//
// XXX: A hack defining missing (kernel) types that are used in
// <linux/ethtool.h> with some OS distributions.
//
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#include <linux/ethtool.h>
#endif // HAVE_LINUX_ETHTOOL_H

//
// XXX: We should include  <linux/mii.h>, but that file is broken for
// older Linux versions (e.g., RedHat-7.3 with Linux kernel 2.4.20-28.7smp).
//
// #ifdef HAVE_LINUX_MII_H
// #include <linux/mii.h>
// #endif

#include "ifconfig_media.hh"


//
// Network interfaces media related utilities.
//

//
// Get the link status
//
int
ifconfig_media_get_link_status(const string& if_name, bool& no_carrier,
			       string& error_msg)
{
#ifdef SIOCGIFMEDIA
    do {
	int s;
	struct ifmediareq ifmr;

	memset(&ifmr, 0, sizeof(ifmr));
	strncpy(ifmr.ifm_name, if_name.c_str(), sizeof(ifmr.ifm_name) - 1);
	
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
	    XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
	}
	if (ioctl(s, SIOCGIFMEDIA, (caddr_t)&ifmr) < 0) {
	    //
	    // XXX: Most likely the interface doesn't support SIOCGIFMEDIA,
	    // hence this is not an error.
	    //
	    no_carrier = false;
	    close(s);
	    return (XORP_OK);
	}

	switch (IFM_TYPE(ifmr.ifm_active)) {
	case IFM_ETHER:
	case IFM_FDDI:
#ifdef IFM_TOKEN
	case IFM_TOKEN:
#endif
	case IFM_IEEE80211:
	    if ((ifmr.ifm_status & IFM_AVALID)
		&& (ifmr.ifm_status & IFM_ACTIVE)) {
		no_carrier = false;
	    } else {
		no_carrier = true;
	    }
	    break;
	default:
	    no_carrier = false;
	    break;
	}
	close(s);
	return (XORP_OK);
    } while (false);
#endif // SIOCGIFMEDIA

#if defined(SIOCETHTOOL) && defined(ETHTOOL_GLINK)
    do {
	int s;
	struct ifreq ifreq;
	struct ethtool_value edata;

	memset(&ifreq, 0, sizeof(ifreq));
	strncpy(ifreq.ifr_name, if_name.c_str(), sizeof(ifreq.ifr_name) - 1);

	memset(&edata, 0, sizeof(edata));
	edata.cmd = ETHTOOL_GLINK;
	ifreq.ifr_data = reinterpret_cast<caddr_t>(&edata);

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
	    XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
	}
	if (ioctl(s, SIOCETHTOOL, &ifreq) < 0) {
	    error_msg = c_format("ioctl(SIOCETHTOOL) for interface %s "
				 "failed: %s",
				 if_name.c_str(), strerror(errno));
	    close(s);
	    return (XORP_ERROR);
	}
	if (edata.data != 0)
	    no_carrier = false;
	else
	    no_carrier = true;
	close(s);
	return (XORP_OK);
    } while (false);
#endif // SIOCETHTOOL && ETHTOOL_GLINK

#ifdef SIOCGMIIREG
    do {
	int s;
	struct ifreq ifreq;

	memset(&ifreq, 0, sizeof(ifreq));
	strncpy(ifreq.ifr_name, if_name.c_str(), sizeof(ifreq.ifr_name) - 1);

	//
	// Define data structure and definitions used for MII ioctl's.
	//
	// We need to do this here because some of them are missing
	// from the system's <linux/mii.h> header file, and because
	// this header file is broken for older Linux versions
	// (e.g., RedHat-7.3 with Linux kernel 2.4.20-28.7smp).
	//
#ifndef MII_BMSR
#define MII_BMSR		0x01
#endif
#ifndef MII_BMSR_LINK_VALID
#define MII_BMSR_LINK_VALID	0x0004
#endif
	struct mii_data {
	    uint16_t	phy_id;
	    uint16_t	reg_num;
	    uint16_t	val_in;
	    uint16_t	val_out;
	};

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
	    XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
	}
	if (ioctl(s, SIOCGMIIPHY, &ifreq) < 0) {
	    error_msg = c_format("ioctl(SIOCGMIIPHY) for interface %s "
				 "failed: %s",
				 if_name.c_str(), strerror(errno));
	    close(s);
	    return (XORP_ERROR);
	}

	struct mii_data* mii;
	mii = reinterpret_cast<struct mii_data *>(&ifreq.ifr_addr);
	mii->reg_num = MII_BMSR;
	if (ioctl(s, SIOCGMIIREG, &ifreq) < 0) {
	    error_msg = c_format("ioctl(SIOCGMIIREG) for interface %s "
				 "failed: %s",
				 if_name.c_str(), strerror(errno));
	    close(s);
	    return (XORP_ERROR);
	}

	int bmsr = mii->val_out;
	if (bmsr & MII_BMSR_LINK_VALID)
	    no_carrier = false;
	else
	    no_carrier = true;
	close(s);
	return (XORP_OK);
    } while (false);
#endif // SIOCGMIIREG

    UNUSED(if_name);
    no_carrier = false;
    error_msg = c_format("No mechanism to test the link status");
    return (XORP_ERROR);
}
