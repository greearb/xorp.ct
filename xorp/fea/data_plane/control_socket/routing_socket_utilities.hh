// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2010 XORP, Inc and Others
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


#ifndef __FEA_ROUTING_SOCKET_UTILS_HH__
#define __FEA_ROUTING_SOCKET_UTILS_HH__

#include <xorp_config.h>
#if defined(HAVE_ROUTING_SOCKETS) || defined(HOST_OS_WINDOWS)

#include "fea/fte.hh"
#include "fea/iftree.hh"

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
     * @param iftree the interface tree to use.
     * @param fte the return-by-reference @ref FteX entry to return the result.
     * @param rtm the RTM routing message.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    static int rtm_get_to_fte_cfg(const IfTree& iftree, FteX& fte,
				  const struct rt_msghdr* rtm);
};

#endif
#endif // __FEA_ROUTING_SOCKET_UTILS_HH__
