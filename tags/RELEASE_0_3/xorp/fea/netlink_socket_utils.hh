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

// $XORP: xorp/fea/netlink_socket_utils.hh,v 1.2 2003/05/05 19:34:00 pavlin Exp $

#ifndef __FEA_NETLINK_SOCKET_UTILS_HH__
#define __FEA_NETLINK_SOCKET_UTILS_HH__

#include "fte.hh"

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
     */
    static void get_rta_attr(const struct rtattr* rtattr, int rta_len,
			     const struct rtattr* rta_array[]);

    /**
     * Extract the routing information from netlink message.
     * 
     * @param fte the return-by-reference @ref FteX entry to return the result.
     * @param nlm the netlink message.
     * @return true on success, otherwise false.
     */
    static bool	nlm_get_to_fte_cfg(FteX& fte, const struct rtmsg* rtmsg,
				   int rta_len);
};

#endif // __FEA_NETLINK_SOCKET_UTILS_HH__
