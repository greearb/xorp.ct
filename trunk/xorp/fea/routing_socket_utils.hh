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

// $XORP: xorp/fea/routing_socket_utils.hh,v 1.5 2004/06/10 22:40:57 hodson Exp $

#ifndef __FEA_ROUTING_SOCKET_UTILS_HH__
#define __FEA_ROUTING_SOCKET_UTILS_HH__

#include "fte.hh"
#include "iftree.hh"

/**
 * @short Helper class for various RTM-format related utilities.
 */
class RtmUtils {
public:
    /**
     * Convert a message type from routing socket message into
     * human-readable form.
     * 
     * @param m message type from routing socket message.
     * @return human-readable message of the message type.
     */
    static string rtm_msg_type(uint32_t m);

    /**
     * Get pointers to set of socket addresses as defined by a mask.
     * 
     * @param amask the mask that defines the set of socket addresses.
     * @param sock the pointer to the first socket address.
     * @param rti_info the array with the pointers to store the result.
     */
    static void	get_rta_sockaddr(uint32_t amask, const struct sockaddr* sock,
				 const struct sockaddr* rti_info[]);

    /**
     * Get the mask length encoded in sockaddr.
     * 
     * @param family the address family.
     * @param sock the socket address with the encoded mask length.
     * @return the mask length if successfully decoded, otherwise -1.
     */
    static int get_sock_mask_len(int family, const struct sockaddr* sock);
    
    /**
     * Extract the routing information from RTM message.
     * 
     * @param fte the return-by-reference @ref FteX entry to return the result.
     * @param iftree the interface tree.
     * @param rtm the RTM routing message.
     * @return true on success, otherwise false.
     */
    static bool rtm_get_to_fte_cfg(FteX& fte, const IfTree& iftree,
	const struct rt_msghdr* rtm);
};

#endif // __FEA_ROUTING_SOCKET_UTILS_HH__
