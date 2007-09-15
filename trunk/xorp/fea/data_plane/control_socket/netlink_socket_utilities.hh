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

// $XORP: xorp/fea/data_plane/control_socket/netlink_socket_utilities.hh,v 1.3 2007/06/04 23:17:33 pavlin Exp $

#ifndef __FEA_DATA_PLANE_CONTROL_SOCKET_NETLINK_SOCKET_UTILITIES_HH__
#define __FEA_DATA_PLANE_CONTROL_SOCKET_NETLINK_SOCKET_UTILITIES_HH__

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
				   const struct rtmsg* rtmsg, int rta_len);

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
};

#endif // __FEA_DATA_PLANE_CONTROL_SOCKET_NETLINK_SOCKET_UTILITIES_HH__
