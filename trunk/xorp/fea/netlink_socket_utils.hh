// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/fea/netlink_socket_utils.hh,v 1.9 2004/06/10 22:40:56 hodson Exp $

#ifndef __FEA_NETLINK_SOCKET_UTILS_HH__
#define __FEA_NETLINK_SOCKET_UTILS_HH__

#include "fte.hh"

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
     * @param fte the return-by-reference @ref FteX entry to return the result.
     * @param nlh the netlink message header.
     * @param rtmsg the routing message.
     * @param rta_len the routing message payload.
     * @return true on success, otherwise false.
     */
    static bool	nlm_get_to_fte_cfg(FteX& fte, const struct nlmsghdr* nlh,
				   const struct rtmsg* rtmsg, int rta_len);

    /**
     * Check that a previous netlink request has succeeded.
     *
     * @param ns_reader the NetlinkSocketReader to use for reading data.
     * @param ns the NetlinkSocket to use for reading data.
     * @param seqno the sequence nomer of the netlink request to check for.
     * @param errmsg the error message (if an error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    static int check_netlink_request(NetlinkSocketReader& ns_reader,
				     NetlinkSocket& ns,
				     uint32_t seqno,
				     string& errmsg);
};

#endif // __FEA_NETLINK_SOCKET_UTILS_HH__
