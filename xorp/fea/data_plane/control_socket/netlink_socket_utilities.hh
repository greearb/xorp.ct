// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2012 XORP, Inc and Others
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


#ifndef __FEA_DATA_PLANE_CONTROL_SOCKET_NETLINK_SOCKET_UTILITIES_HH__
#define __FEA_DATA_PLANE_CONTROL_SOCKET_NETLINK_SOCKET_UTILITIES_HH__

#include <xorp_config.h>
#ifdef HAVE_NETLINK_SOCKETS

#include "libxorp/xorp.h"

#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "fea/fte.hh"
#include "fea/iftree.hh"


//
// Conditionally re-define some of the netlink-related macros that are not
// defined properly and might generate alignment-related compilation
// warning on some architectures (e.g, ARM/XScale) if we use
// "-Wcast-align" compilation flag.
//
#ifdef HAVE_BROKEN_MACRO_NLMSG_NEXT
#undef NLMSG_NEXT
#define NLMSG_NEXT(nlh, len)	((len) -= NLMSG_ALIGN((nlh)->nlmsg_len), \
				  (struct nlmsghdr*)(void*)(((char*)(nlh)) + NLMSG_ALIGN((nlh)->nlmsg_len)))
#endif

#ifdef HAVE_BROKEN_MACRO_RTA_NEXT
#undef RTA_NEXT
#define RTA_NEXT(rta, attrlen)	((attrlen) -= RTA_ALIGN((rta)->rta_len), \
				    (struct rtattr*)(void*)(((char*)(rta)) + RTA_ALIGN((rta)->rta_len)))
#endif

#ifdef HAVE_BROKEN_MACRO_IFA_RTA
#undef IFA_RTA
#define IFA_RTA(r)		((struct rtattr*)(void*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ifaddrmsg))))
#endif

#ifdef HAVE_BROKEN_MACRO_IFLA_RTA
#undef IFLA_RTA
#define IFLA_RTA(r)		((struct rtattr*)(void*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ifinfomsg))))
#endif

#ifdef HAVE_BROKEN_MACRO_RTM_RTA
#undef RTM_RTA
#define RTM_RTA(r)		((struct rtattr*)(void *)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct rtmsg))))
#endif


//
// TODO: XXX: a temporary definition of RTPROT_XORP (e.g., in case of Linux)
// that is used to mark the routes installed by XORP.
// XXX: RTPROT_XORP must be unique
// (see <linux/rtnetlink.h> for definition of all RTPROT_* protocols).
//
#if defined(RTPROT_UNSPEC) && !defined(RTPROT_XORP)
#define RTPROT_XORP 14
#endif

class NetlinkSocket;
class NetlinkSocketReader;
class FibConfig;

/**
 * @short Helper class for various NETLINK-format related utilities.
 */
class NlmUtils {
public:
    /**
     * Convert a message type from netlink socket message into
     * human-readable form.
     * 
     * @param m message type from netlink socket message.
     * @return human-readable message of the message type.
     */
    static string nlm_msg_type(uint32_t m);

    static string nlm_print_msg(const vector<uint8_t>& message);

    /**
     * Get pointers to set of netlink rtattr entries.
     * 
     * @param rtattr the pointer to the first rtattr entry.
     * @param rta_len the length of all rtattr entries.
     * @param rta_array the array with the pointers to store the result.
     * @param rta_array_n the maximum number of entries to store
     * in the array.
     */
    static void get_rtattr(const struct rtattr* rtattr, int rta_len,
			   const struct rtattr* rta_array[],
			   size_t rta_array_n);
    
    /**
     * Extract the routing information from netlink message.
     * 
     * @param iftree the interface tree to use.
     * @param fte the return-by-reference @ref FteX entry to return the result.
     * @param nlh the netlink message header.
     * @param rtmsg the routing message.
     * @param rta_len the routing message payload.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    static int	nlm_get_to_fte_cfg(const IfTree& iftree, FteX& fte,
				   const struct nlmsghdr* nlh,
				   const struct rtmsg* rtmsg, int rta_len,
				   const FibConfig& fibconfig, string& err_msg);

    /**
     * Check that a previous netlink request has succeeded.
     *
     * @param ns_reader the NetlinkSocketReader to use for reading data.
     * @param ns the NetlinkSocket to use for reading data.
     * @param seqno the sequence nomer of the netlink request to check for.
     * @param last_errno the last error number (if error).
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    static int check_netlink_request(NetlinkSocketReader& ns_reader,
				     NetlinkSocket& ns,
				     uint32_t seqno,
				     int& last_errno,
				     string& error_msg);

    static int nlm_decode_ipvx_address(int family, const struct rtattr* rtattr,
				       IPvX& ipvx_addr, bool& is_set, string& error_msg);


    static int nlm_decode_ipvx_interface_address(const struct ifinfomsg* ifinfomsg,
						 const struct rtattr* rtattr,
						 IPvX& ipvx_addr, bool& is_set,
						 string& error_msg);

    static void nlm_cond_newlink_to_fea_cfg(const IfTree& user_cfg, IfTree& iftree,
					    const struct ifinfomsg* ifinfomsg,
					    int rta_len, bool& modified);


    static void nlm_dellink_to_fea_cfg(IfTree& iftree, const struct ifinfomsg* ifinfomsg,
				       int rta_len, bool& modified);


    static void nlm_cond_newdeladdr_to_fea_cfg(const IfTree& user_config, IfTree& iftree,
					       const struct ifaddrmsg* ifaddrmsg,
					       int rta_len, bool is_deleted, bool& modified);

};

#endif
#endif // __FEA_DATA_PLANE_CONTROL_SOCKET_NETLINK_SOCKET_UTILITIES_HH__
