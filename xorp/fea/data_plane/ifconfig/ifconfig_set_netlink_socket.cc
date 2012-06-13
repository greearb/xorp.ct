// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
//
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#include <xorp_config.h>

#ifdef HAVE_NETLINK_SOCKETS


#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ether_compat.h"
#include "libxorp/ipvx.hh"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif
#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "fea/ifconfig.hh"
#include "fea/fibconfig.hh"
#include "fea/data_plane/control_socket/netlink_socket_utilities.hh"

#include "ifconfig_set_netlink_socket.hh"


//
// Set information about network interfaces configuration with the
// underlying system.
//
// The mechanism to set the information is netlink(7) sockets.
//

IfConfigSetNetlinkSocket::IfConfigSetNetlinkSocket(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigSet(fea_data_plane_manager),
      NetlinkSocket(fea_data_plane_manager.eventloop(),
		    fea_data_plane_manager.fibconfig().get_netlink_filter_table_id()),
      _ns_reader(*(NetlinkSocket *)this)
{
}

IfConfigSetNetlinkSocket::~IfConfigSetNetlinkSocket()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the netlink(7) sockets mechanism to set "
		   "information about network interfaces into the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigSetNetlinkSocket::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (NetlinkSocket::start(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigSetNetlinkSocket::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (NetlinkSocket::stop(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

bool
IfConfigSetNetlinkSocket::is_discard_emulated(const IfTreeInterface& i) const
{
    UNUSED(i);

#ifdef HOST_OS_LINUX
    return (true);
#else
    return (false);
#endif
}

bool
IfConfigSetNetlinkSocket::is_unreachable_emulated(const IfTreeInterface& i)
    const
{
    UNUSED(i);

#ifdef HOST_OS_LINUX
    return (true);
#else
    return (false);
#endif
}

int
IfConfigSetNetlinkSocket::config_begin(string& error_msg)
{
    // XXX: nothing to do

    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetNetlinkSocket::config_end(string& error_msg)
{
    // XXX: nothing to do

    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetNetlinkSocket::config_interface_begin(
    const IfTreeInterface* pulled_ifp,
    IfTreeInterface& config_iface,
    string& error_msg)
{
    int ret_value = XORP_OK;
    bool was_disabled = false;
    bool should_disable = false;

    if (pulled_ifp == NULL) {
	// Nothing to do: the interface has been deleted from the system
	return (XORP_OK);
    }

#ifdef HOST_OS_LINUX
    //
    // XXX: Set the interface DOWN otherwise we may not be able to
    // set the MAC address or the MTU (limitation imposed by the Linux kernel).
    //
    if (pulled_ifp->enabled())
	should_disable = true;
#endif

    //
    // Set the MTU
    //
    if (config_iface.mtu() != pulled_ifp->mtu()) {
	if (should_disable && (! was_disabled)) {
	    if (set_interface_status(config_iface.ifname(),
				     config_iface.pif_index(),
				     config_iface.interface_flags(),
				     false,
				     error_msg)
		!= XORP_OK) {
		ret_value = XORP_ERROR;
		goto done;
	    }
	    was_disabled = true;
	}
	if (set_interface_mtu(config_iface.ifname(),
			      config_iface.pif_index(),
			      config_iface.mtu(),
			      error_msg)
	    != XORP_OK) {
	    ret_value = XORP_ERROR;
	    goto done;
	}
    }

    //
    // Set the MAC address
    //
    if (config_iface.mac() != pulled_ifp->mac()) {
	if (should_disable && (! was_disabled)) {
	    if (set_interface_status(config_iface.ifname(),
				     config_iface.pif_index(),
				     config_iface.interface_flags(),
				     false,
				     error_msg)
		!= XORP_OK) {
		ret_value = XORP_ERROR;
		goto done;
	    }
	    was_disabled = true;
	}
	if (set_interface_mac_address(config_iface.ifname(),
				      config_iface.pif_index(),
				      config_iface.mac(),
				      error_msg)
	    != XORP_OK) {
	    ret_value = XORP_ERROR;
	    goto done;
	}
    }

 done:
    if (was_disabled) {
	// We need to "hide" the Linux quirk of toggling the interface down / up
	// and make sure that our configuration is up to date.  Otherwise we'll
	// later be notified (by the observer) that the interface went down
	// unexpectedly and things may break, when instead it is only a
	// transient state.
        wait_interface_status(pulled_ifp, false);
	if (set_interface_status(config_iface.ifname(),
				 config_iface.pif_index(),
				 config_iface.interface_flags(),
				 true,
				 error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
        wait_interface_status(pulled_ifp, true);
    }

    return (ret_value);
}

int
IfConfigSetNetlinkSocket::config_interface_end(
    const IfTreeInterface* pulled_ifp,
    const IfTreeInterface& config_iface,
    string& error_msg)
{
    if (pulled_ifp == NULL) {
	// Nothing to do: the interface has been deleted from the system
	return (XORP_OK);
    }

    //
    // Set the interface status
    //
    if (config_iface.enabled() != pulled_ifp->enabled()) {
	if (set_interface_status(config_iface.ifname(),
				 config_iface.pif_index(),
				 config_iface.interface_flags(),
				 config_iface.enabled(),
				 error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
}

int
IfConfigSetNetlinkSocket::config_vif_begin(const IfTreeInterface* pulled_ifp,
					   const IfTreeVif* pulled_vifp,
					   const IfTreeInterface& config_iface,
					   const IfTreeVif& config_vif,
					   string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(config_iface);
    UNUSED(config_vif);
    UNUSED(error_msg);

    if (pulled_vifp == NULL) {
	// Nothing to do: the vif has been deleted from the system
	return (XORP_OK);
    }

    // XXX: nothing to do

    return (XORP_OK);
}

int
IfConfigSetNetlinkSocket::config_vif_end(const IfTreeInterface* pulled_ifp,
					 const IfTreeVif* pulled_vifp,
					 const IfTreeInterface& config_iface,
					 const IfTreeVif& config_vif,
					 string& error_msg)
{
    UNUSED(pulled_ifp);

    if (pulled_vifp == NULL) {
	// Nothing to do: the vif has been deleted from the system
	return (XORP_OK);
    }

    //
    // XXX: If the interface name and vif name are different, then
    // they might have different status: the interface can be UP, while
    // the vif can be DOWN.
    //
    if (config_iface.ifname() != config_vif.vifname()) {
	//
	// Set the vif status
	//
	if (config_vif.enabled() != pulled_vifp->enabled()) {
	    //
	    // XXX: The interface and vif status setting mechanism is
	    // equivalent for this platform.
	    //
	    if (set_interface_status(config_vif.vifname(),
				     config_vif.pif_index(),
				     config_vif.vif_flags(),
				     config_vif.enabled(),
				     error_msg)
		!= XORP_OK) {
		return (XORP_ERROR);
	    }
	}
    }

    return (XORP_OK);
}

int
IfConfigSetNetlinkSocket::config_add_address(const IfTreeInterface* pulled_ifp,
					     const IfTreeVif* pulled_vifp,
					     const IfTreeAddr4* pulled_addrp,
					     const IfTreeInterface& config_iface,
					     const IfTreeVif& config_vif,
					     const IfTreeAddr4& config_addr,
					     string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);

    //
    // Test whether a new address
    //
    do {
	if (pulled_addrp == NULL)
	    break;
	if (pulled_addrp->addr() != config_addr.addr())
	    break;
	if (pulled_addrp->broadcast() != config_addr.broadcast())
	    break;
	if (pulled_addrp->broadcast()
	    && (pulled_addrp->bcast() != config_addr.bcast())) {
	    break;
	}
	if (pulled_addrp->point_to_point() != config_addr.point_to_point())
	    break;
	if (pulled_addrp->point_to_point()
	    && (pulled_addrp->endpoint() != config_addr.endpoint())) {
	    break;
	}
	if (pulled_addrp->prefix_len() != config_addr.prefix_len())
	    break;

	// XXX: Same address, therefore ignore it
	return (XORP_OK);
    } while (false);

    //
    // Delete the old address if necessary
    //
    if (pulled_addrp != NULL) {
	if (delete_addr(config_iface.ifname(), config_vif.vifname(),
			config_vif.pif_index(), IPvX(config_addr.addr()),
			config_addr.prefix_len(), error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    //
    // Add the address
    //
    if (add_addr(config_iface.ifname(), config_vif.vifname(),
		 config_vif.pif_index(), IPvX(config_addr.addr()),
		 config_addr.prefix_len(),
		 config_addr.broadcast(), IPvX(config_addr.bcast()),
		 config_addr.point_to_point(), IPvX(config_addr.endpoint()),
		 error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetNetlinkSocket::config_delete_address(
    const IfTreeInterface* pulled_ifp,
    const IfTreeVif* pulled_vifp,
    const IfTreeAddr4* pulled_addrp,
    const IfTreeInterface& config_iface,
    const IfTreeVif& config_vif,
    const IfTreeAddr4& config_addr,
    string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);
    UNUSED(pulled_addrp);

    //
    // Delete the address
    //
    if (delete_addr(config_iface.ifname(), config_vif.vifname(),
		    config_vif.pif_index(), IPvX(config_addr.addr()),
		    config_addr.prefix_len(), error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetNetlinkSocket::config_add_address(const IfTreeInterface* pulled_ifp,
					     const IfTreeVif* pulled_vifp,
					     const IfTreeAddr6* pulled_addrp,
					     const IfTreeInterface& config_iface,
					     const IfTreeVif& config_vif,
					     const IfTreeAddr6& config_addr,
					     string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);

    //
    // Test whether a new address
    //
    do {
	if (pulled_addrp == NULL)
	    break;
	if (pulled_addrp->addr() != config_addr.addr())
	    break;
	if (pulled_addrp->point_to_point() != config_addr.point_to_point())
	    break;
	if (pulled_addrp->point_to_point()
	    && (pulled_addrp->endpoint() != config_addr.endpoint())) {
	    break;
	}
	if (pulled_addrp->prefix_len() != config_addr.prefix_len())
	    break;

	// XXX: Same address, therefore ignore it
	return (XORP_OK);
    } while (false);

    //
    // Delete the old address if necessary
    //
    if (pulled_addrp != NULL) {
	if (delete_addr(config_iface.ifname(), config_vif.vifname(),
			config_vif.pif_index(), IPvX(config_addr.addr()),
			config_addr.prefix_len(), error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    //
    // Add the address
    //
    if (add_addr(config_iface.ifname(), config_vif.vifname(),
		 config_vif.pif_index(), IPvX(config_addr.addr()),
		 config_addr.prefix_len(),
		 false, IPvX::ZERO(config_addr.addr().af()),
		 config_addr.point_to_point(), IPvX(config_addr.endpoint()),
		 error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetNetlinkSocket::config_delete_address(
    const IfTreeInterface* pulled_ifp,
    const IfTreeVif* pulled_vifp,
    const IfTreeAddr6* pulled_addrp,
    const IfTreeInterface& config_iface,
    const IfTreeVif& config_vif,
    const IfTreeAddr6& config_addr,
    string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);
    UNUSED(pulled_addrp);

    //
    // Delete the address
    //
    if (delete_addr(config_iface.ifname(), config_vif.vifname(),
		    config_vif.pif_index(), IPvX(config_addr.addr()),
		    config_addr.prefix_len(), error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetNetlinkSocket::set_interface_status(const string& ifname,
					       uint32_t if_index,
					       uint32_t interface_flags,
					       bool is_enabled,
					       string& error_msg)
{
    //
    // Update the interface flags
    //
    if (is_enabled)
	interface_flags |= IFF_UP;
    else
	interface_flags &= ~IFF_UP;

    struct ifreq ifreq;
    int s = -1;
    bool test_ioctl = false;

    // Evidently, some older systems don't return errors, but just do not
    // actually work.
    static int netlink_set_status_works = -1;

    if (netlink_set_status_works != 0) {
	static const size_t	buffer_size = sizeof(struct nlmsghdr)
	    + sizeof(struct ifinfomsg) + 2*sizeof(struct rtattr) + 512;
	union {
	    uint8_t		data[buffer_size];
	    struct nlmsghdr	nlh;
	} buffer;
	struct nlmsghdr*	nlh = &buffer.nlh;
	struct sockaddr_nl	snl;
	struct ifinfomsg*	ifinfomsg;
	NetlinkSocket&	ns = *this;
	int			last_errno = 0;

	memset(&buffer, 0, sizeof(buffer));

	// Set the socket
	memset(&snl, 0, sizeof(snl));
	snl.nl_family = AF_NETLINK;
	snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
	snl.nl_groups = 0;

	//
	// Set the request
	//
	nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifinfomsg));
	nlh->nlmsg_type = RTM_NEWLINK;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
	nlh->nlmsg_seq = ns.seqno();
	nlh->nlmsg_pid = ns.nl_pid();
	ifinfomsg = static_cast<struct ifinfomsg*>(NLMSG_DATA(nlh));
	ifinfomsg->ifi_family = AF_UNSPEC;
	ifinfomsg->ifi_type = IFLA_UNSPEC;
	ifinfomsg->ifi_index = if_index;
	ifinfomsg->ifi_flags = interface_flags;
	ifinfomsg->ifi_change = 0xffffffff;

	if (NLMSG_ALIGN(nlh->nlmsg_len) > sizeof(buffer)) {
	    XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		       XORP_UINT_CAST(sizeof(buffer)),
		       XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len)));
	}
	nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len);

	if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
		      reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	    != (ssize_t)nlh->nlmsg_len) {
	    error_msg += c_format("Cannot set the interface flags to 0x%x on "
				  "interface %s using netlink, error: %s\n",
				  interface_flags, ifname.c_str(), strerror(errno));
	    test_ioctl = true;
	    goto use_ioctl;
	}
	if (NlmUtils::check_netlink_request(_ns_reader, ns, nlh->nlmsg_seq,
					    last_errno, error_msg)
	    != XORP_OK) {
	    error_msg += c_format("Request to set the interface flags to 0x%x on "
				  "interface %s using netlink failed: %s\n",
				  interface_flags, ifname.c_str(),
				  error_msg.c_str());
	    test_ioctl = true;
	    goto use_ioctl; // try ioctl method.
	}

	if (netlink_set_status_works == -1) {
	    // test that it actually worked using ioctl.
	    s = socket(AF_INET, SOCK_DGRAM, 0);
	    memset(&ifreq, 0, sizeof(ifreq));
	    strncpy(ifreq.ifr_name, ifname.c_str(), sizeof(ifreq.ifr_name) - 1);
	    ifreq.ifr_flags = interface_flags;
	    if (ioctl(s, SIOCGIFFLAGS, &ifreq) < 0) {
		error_msg += c_format("Cannot get the interface flags on "
				      "interface %s: %s\n",
				      ifname.c_str(),
				      strerror(errno));
	    }
	    else {
		// Make sure we could at least set the enable/disable
		if ((!!is_enabled) != (!!(ifreq.ifr_flags & IFF_UP))) {
		    error_msg += c_format("WARNING:  Settting interface status using netlink failed"
					  " on: %s.  Will try to use ioctl method instead.\n",
					  ifname.c_str());
		    // Seems it really is broken, don't try netlink again..just use ioctl.
		    test_ioctl = true;
		    goto use_ioctl;
		}
		else {
		    netlink_set_status_works = 1; // looks like it worked
		}
	    }
	}
    }
    else {
	//
	// XXX: a work-around in case the kernel doesn't support setting
	// the flags on an interface by using netlink.
	// In this case, the work-around is to use ioctl(). Sigh...
	//
      use_ioctl:
	if (s < 0) {
	    s = socket(AF_INET, SOCK_DGRAM, 0);
	}

	if (s < 0) {
	    XLOG_ERROR("Could not initialize IPv4 ioctl() socket while trying to set status: %s",
		       strerror(errno));
	    return XORP_ERROR;
	}

	memset(&ifreq, 0, sizeof(ifreq));

	strncpy(ifreq.ifr_name, ifname.c_str(), sizeof(ifreq.ifr_name) - 1);
	ifreq.ifr_flags = interface_flags;
	if (ioctl(s, SIOCSIFFLAGS, &ifreq) < 0) {
	    error_msg += c_format("Cannot set the interface flags to 0x%x on "
				  "interface %s using SIOCSIFFLAGS: %s\n",
				  interface_flags,
				  ifname.c_str(),
				  strerror(errno));
	    close(s);
	    return (XORP_ERROR);
	}
	else {
	    // setting *seemed* to work, but let's check to make sure.
	    if (test_ioctl) {
		memset(&ifreq, 0, sizeof(ifreq));
		strncpy(ifreq.ifr_name, ifname.c_str(), sizeof(ifreq.ifr_name) - 1);
		ifreq.ifr_flags = interface_flags;
		if (ioctl(s, SIOCGIFFLAGS, &ifreq) >= 0) {
		    if ((!!is_enabled) == (!!(ifreq.ifr_flags & IFF_UP))) {
			if (netlink_set_status_works != 0) {
			    // Ok, it all worked.  We assume that netlink just doesn't work, so
			    // don't try it again.
			    error_msg += c_format("WARNING:  Settting interface status using netlink failed"
						  " on: %s but ioctl method worked.  Will use ioctl method from"
						  " now on.\n",
						  ifname.c_str());
			    netlink_set_status_works = 0;
			}
		    }
		}
	    }
	}
	close(s);
    }
    if (error_msg.size()) {
	XLOG_WARNING("%s", error_msg.c_str());
	error_msg = ""; //we worked around the error..don't pass back warning messages!
    }
    return (XORP_OK);
}

void
IfConfigSetNetlinkSocket::wait_interface_status(const IfTreeInterface* ifp,
						bool is_enabled)
{
    NetlinkSocket* ns = dynamic_cast<NetlinkSocket*>(fea_data_plane_manager()
						     .ifconfig_observer());
    string error_msg;

    // This is supposed to run only on Linux.  I could enforce this more, but
    // I'll leave the code flexible in case other platforms need such a
    // mechanism.
    if (!ns)
	return;

    while (ifp->enabled() != is_enabled) {
	if (ns->force_recvmsg(true, error_msg) != XORP_OK)
	    XLOG_ERROR("Netlink force_recvmsg(): %s", error_msg.c_str());
    }
}

int
IfConfigSetNetlinkSocket::set_interface_mac_address(const string& ifname,
						    uint32_t if_index,
						    const Mac& mac,
						    string& error_msg)
{
    struct ether_addr ether_addr;

    mac.copy_out(ether_addr);

#ifdef RTM_SETLINK
    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct ifinfomsg) + 2*sizeof(struct rtattr) + 512;
    union {
	uint8_t		data[buffer_size];
	struct nlmsghdr	nlh;
    } buffer;
    struct nlmsghdr*	nlh = &buffer.nlh;
    struct sockaddr_nl	snl;
    struct ifinfomsg*	ifinfomsg;
    struct rtattr*	rtattr;
    int			rta_len;
    NetlinkSocket&	ns = *this;
    int			last_errno = 0;

    memset(&buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    //
    // Set the request
    //
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifinfomsg));
    nlh->nlmsg_type = RTM_SETLINK;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.nl_pid();
    ifinfomsg = static_cast<struct ifinfomsg*>(NLMSG_DATA(nlh));
    ifinfomsg->ifi_family = AF_UNSPEC;
    ifinfomsg->ifi_type = IFLA_UNSPEC;	// TODO: set to ARPHRD_ETHER ??
    ifinfomsg->ifi_index = if_index;
    ifinfomsg->ifi_flags = 0;
    ifinfomsg->ifi_change = 0xffffffff;

    // Add the MAC address as an attribute
    rta_len = RTA_LENGTH(ETH_ALEN);
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		   XORP_UINT_CAST(sizeof(buffer)),
		   XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
    }
    rtattr = IFLA_RTA(ifinfomsg);
    rtattr->rta_type = IFLA_ADDRESS;
    rtattr->rta_len = rta_len;
    memcpy(RTA_DATA(rtattr), &ether_addr, ETH_ALEN);
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
		  reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	error_msg += c_format("Cannot set the MAC address to %s "
			     "on interface %s: %s\n",
			     mac.str().c_str(),
			     ifname.c_str(),
			     strerror(errno));
	return (XORP_ERROR);
    }

    string em;
    if (NlmUtils::check_netlink_request(_ns_reader, ns, nlh->nlmsg_seq,
					last_errno, em)
	!= XORP_OK) {
	error_msg += c_format("Cannot set the MAC address to %s "
			      "on interface %s using netlink: %s",
			      mac.str().c_str(),
			      ifname.c_str(), em.c_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);

#elif defined(SIOCSIFHWADDR)
    //
    // XXX: a work-around in case the kernel doesn't support setting
    // the MAC address on an interface by using netlink.
    // In this case, the work-around is to use ioctl(). Sigh...
    //
    struct ifreq ifreq;

    UNUSED(if_index);

    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
	XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
    }

    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, ifname.c_str(), sizeof(ifreq.ifr_name) - 1);
    ifreq.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    memcpy(ifreq.ifr_hwaddr.sa_data, &ether_addr, ETH_ALEN);
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    ifreq.ifr_hwaddr.sa_len = ETH_ALEN;
#endif
    if (ioctl(s, SIOCSIFHWADDR, &ifreq) < 0) {
	error_msg += c_format("Cannot set the MAC address to %s "
			      "on interface %s using SIOCSIFHWADDR: %s",
			      mac.str().c_str(),
			      ifname.c_str(),
			      strerror(errno));
	close(s);
	return (XORP_ERROR);
    }
    close(s);

    return (XORP_OK);

#else
#error No mechanism to set the MAC address on an interface
#endif
}

int
IfConfigSetNetlinkSocket::set_interface_mtu(const string& ifname,
					    uint32_t if_index,
					    uint32_t mtu,
					    string& error_msg)
{
#ifndef HAVE_NETLINK_SOCKETS_SET_MTU_IS_BROKEN
    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct ifinfomsg) + 2*sizeof(struct rtattr) + 512;
    union {
	uint8_t		data[buffer_size];
	struct nlmsghdr	nlh;
    } buffer;
    struct nlmsghdr*	nlh = &buffer.nlh;
    struct sockaddr_nl	snl;
    struct ifinfomsg*	ifinfomsg;
    struct rtattr*	rtattr;
    int			rta_len;
    NetlinkSocket&	ns = *this;
    int			last_errno = 0;

    memset(&buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    //
    // Set the request
    //
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifinfomsg));
    nlh->nlmsg_type = RTM_NEWLINK;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.nl_pid();
    ifinfomsg = static_cast<struct ifinfomsg*>(NLMSG_DATA(nlh));
    ifinfomsg->ifi_family = AF_UNSPEC;
    ifinfomsg->ifi_type = IFLA_UNSPEC;
    ifinfomsg->ifi_index = if_index;
    ifinfomsg->ifi_flags = 0;
    ifinfomsg->ifi_change = 0xffffffff;

    // Add the MTU as an attribute
    unsigned int uint_mtu = mtu;
    rta_len = RTA_LENGTH(sizeof(unsigned int));
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		   XORP_UINT_CAST(sizeof(buffer)),
		   XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
    }
    rtattr = IFLA_RTA(ifinfomsg);
    rtattr->rta_type = IFLA_MTU;
    rtattr->rta_len = rta_len;
    memcpy(RTA_DATA(rtattr), &uint_mtu, sizeof(uint_mtu));
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
		  reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	error_msg = c_format("Cannot set the MTU to %u on "
			     "interface %s: %s",
			     mtu, ifname.c_str(), strerror(errno));
	return (XORP_ERROR);
    }
    if (NlmUtils::check_netlink_request(_ns_reader, ns, nlh->nlmsg_seq,
					last_errno, error_msg)
	!= XORP_OK) {
	error_msg = c_format("Cannot set the MTU to %u on "
			     "interface %s: %s",
			     mtu, ifname.c_str(), error_msg.c_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);

#else // HAVE_NETLINK_SOCKETS_SET_MTU_IS_BROKEN
    //
    // XXX: a work-around in case the kernel doesn't support setting
    // the MTU on an interface by using netlink.
    // In this case, the work-around is to use ioctl(). Sigh...
    //
    struct ifreq ifreq;

    UNUSED(if_index);

    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
	XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
    }

    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, ifname.c_str(), sizeof(ifreq.ifr_name) - 1);
    ifreq.ifr_mtu = mtu;
    if (ioctl(s, SIOCSIFMTU, &ifreq) < 0) {
	error_msg = c_format("Cannot set the MTU to %u on "
			     "interface %s: %s",
			     mtu,
			     ifname.c_str(),
			     strerror(errno));
	close(s);
	return (XORP_ERROR);
    }
    close(s);

    return (XORP_OK);
#endif // HAVE_NETLINK_SOCKETS_SET_MTU_IS_BROKEN
}

int
IfConfigSetNetlinkSocket::add_addr(const string& ifname,
				   const string& vifname,
				   uint32_t if_index,
				   const IPvX& addr,
				   uint32_t prefix_len,
				   bool is_broadcast,
				   const IPvX& broadcast_addr,
				   bool is_point_to_point,
				   const IPvX& endpoint_addr,
				   string& error_msg)
{
    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct ifinfomsg) + 2*sizeof(struct rtattr) + 512;
    union {
	uint8_t		data[buffer_size];
	struct nlmsghdr	nlh;
    } buffer;
    struct nlmsghdr*	nlh = &buffer.nlh;
    struct sockaddr_nl	snl;
    struct ifaddrmsg*	ifaddrmsg;
    struct rtattr*	rtattr;
    int			rta_len;
    uint8_t*		data;
    NetlinkSocket&	ns = *this;
    void*		rta_align_data;
    int			last_errno = 0;

    memset(&buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    // There are all sorts of bugs with how Xorp handles VLANs (as vifs under
    // a physical interface).
    // 1:  It passes the pif_index..when it should use the vif's ifindex.
    // 2:  It's passing the pif_index as 0, which is wrong in all cases.
    // Add some hacks to work-around this until we clean up vifs more properly.
    if ((if_index == 0) ||
	(strcmp(ifname.c_str(), vifname.c_str()) != 0)) {
	if_index = if_nametoindex(vifname.c_str());
    }

    //
    // Set the request
    //
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifaddrmsg));
    nlh->nlmsg_type = RTM_NEWADDR;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.nl_pid();
    ifaddrmsg = static_cast<struct ifaddrmsg*>(NLMSG_DATA(nlh));
    ifaddrmsg->ifa_family = addr.af();
    ifaddrmsg->ifa_prefixlen = prefix_len;
    ifaddrmsg->ifa_flags = 0;	// TODO: XXX: PAVPAVPAV: OK if 0?
    ifaddrmsg->ifa_scope = 0;	// TODO: XXX: PAVPAVPAV: OK if 0?
    ifaddrmsg->ifa_index = if_index;

    // Add the address as an attribute
    rta_len = RTA_LENGTH(addr.addr_bytelen());
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		   XORP_UINT_CAST(sizeof(buffer)),
		   XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
    }
    rtattr = IFA_RTA(ifaddrmsg);
    rtattr->rta_type = IFA_LOCAL;
    rtattr->rta_len = rta_len;
    data = static_cast<uint8_t*>(RTA_DATA(rtattr));
    addr.copy_out(data);
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    if (is_broadcast || is_point_to_point) {
	// Set the broadcast or point-to-point address
	rta_len = RTA_LENGTH(addr.addr_bytelen());
	if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	    XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		       XORP_UINT_CAST(sizeof(buffer)),
		       XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
	}
	rta_align_data = reinterpret_cast<char*>(rtattr)
	    + RTA_ALIGN(rtattr->rta_len);
	rtattr = static_cast<struct rtattr*>(rta_align_data);
	rtattr->rta_type = IFA_UNSPEC;
	rtattr->rta_len = rta_len;
	data = static_cast<uint8_t*>(RTA_DATA(rtattr));
	if (is_broadcast) {
	    rtattr->rta_type = IFA_BROADCAST;
	    broadcast_addr.copy_out(data);
	}
	if (is_point_to_point) {
	    rtattr->rta_type = IFA_ADDRESS;
	    endpoint_addr.copy_out(data);
	}
	nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;
    }

    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
		  reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	error_msg = c_format("IfConfigSetNetlinkSocket::add_addr: sendto: Cannot add address '%s' "
			     "on interface '%s' vif '%s', if_index: %i: %s",
			     addr.str().c_str(),
			     ifname.c_str(), vifname.c_str(), if_index, strerror(errno));
	return (XORP_ERROR);
    }
    if (NlmUtils::check_netlink_request(_ns_reader, ns, nlh->nlmsg_seq,
					last_errno, error_msg)
	!= XORP_OK) {
	error_msg = c_format("IfConfigSetNetlinkSocket::add_addr: check_nl_req: Cannot add address '%s' "
			     "on interface '%s' vif '%s', if_index: %i : %s",
			     addr.str().c_str(),
			     ifname.c_str(), vifname.c_str(), if_index,
			     error_msg.c_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetNetlinkSocket::delete_addr(const string& ifname,
				      const string& vifname,
				      uint32_t if_index,
				      const IPvX& addr,
				      uint32_t prefix_len,
				      string& error_msg)
{
    static const size_t	buffer_size = sizeof(struct nlmsghdr)
	+ sizeof(struct ifinfomsg) + 2*sizeof(struct rtattr) + 512;
    union {
	uint8_t		data[buffer_size];
	struct nlmsghdr	nlh;
    } buffer;
    struct nlmsghdr*	nlh = &buffer.nlh;
    struct sockaddr_nl	snl;
    struct ifaddrmsg*	ifaddrmsg;
    struct rtattr*	rtattr;
    int			rta_len;
    uint8_t*		data;
    NetlinkSocket&	ns = *this;
    int			last_errno = 0;

    memset(&buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    // There are all sorts of bugs with how Xorp handles VLANs (as vifs under
    // a physical interface).
    // 1:  It passes the pif_index..when it should use the vif's ifindex.
    // 2:  It's passing the pif_index as 0, which is wrong in all cases.
    // Add some hacks to work-around this until we clean up vifs more properly.
    if ((if_index == 0) ||
	(strcmp(ifname.c_str(), vifname.c_str()) != 0)) {
	if_index = if_nametoindex(vifname.c_str());
    }

    //
    // Set the request
    //
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*ifaddrmsg));
    nlh->nlmsg_type = RTM_DELADDR;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.nl_pid();
    ifaddrmsg = static_cast<struct ifaddrmsg*>(NLMSG_DATA(nlh));
    ifaddrmsg->ifa_family = addr.af();
    ifaddrmsg->ifa_prefixlen = prefix_len;
    ifaddrmsg->ifa_flags = 0;	// TODO: XXX: PAVPAVPAV: OK if 0?
    ifaddrmsg->ifa_scope = 0;	// TODO: XXX: PAVPAVPAV: OK if 0?
    ifaddrmsg->ifa_index = if_index;

    // Add the address as an attribute
    rta_len = RTA_LENGTH(addr.addr_bytelen());
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		   XORP_UINT_CAST(sizeof(buffer)),
		   XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
    }
    rtattr = IFA_RTA(ifaddrmsg);
    rtattr->rta_type = IFA_LOCAL;
    rtattr->rta_len = rta_len;
    data = static_cast<uint8_t*>(RTA_DATA(rtattr));
    addr.copy_out(data);
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
		  reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	error_msg = c_format("Cannot delete address '%s' "
			     "on interface '%s' vif '%s': %s",
			     addr.str().c_str(),
			     ifname.c_str(), vifname.c_str(),
			     strerror(errno));
	return (XORP_ERROR);
    }
    if (NlmUtils::check_netlink_request(_ns_reader, ns, nlh->nlmsg_seq,
					last_errno, error_msg)
	!= XORP_OK) {
	error_msg = c_format("Cannot delete address '%s' "
			     "on interface '%s' vif '%s': %s",
			     addr.str().c_str(),
			     ifname.c_str(), vifname.c_str(),
			     error_msg.c_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

#endif // HAVE_NETLINK_SOCKETS
