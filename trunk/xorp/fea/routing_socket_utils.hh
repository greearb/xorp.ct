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

// $XORP: xorp/fea/ifconfig.hh,v 1.2 2003/03/10 23:20:15 hodson Exp $

#ifndef __FEA_ROUTING_SOCKET_UTILS_HH__
#define __FEA_ROUTING_SOCKET_UTILS_HH__

#include "fte.hh"

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
    static void	get_rta_sockaddr(uint32_t amask, const sockaddr* sock,
				 const sockaddr* rti_info[]);

    /**
     * Get the masklen encoded in sockaddr.
     * 
     * @param family the address family.
     * @param sock the socket address with the encoded masklen.
     * @return the masklen if successfully decoded, otherwise XORP_ERROR.
     */
    static int	get_sock_masklen(int family, const sockaddr* sock);
    
    /**
     * Extract the routing information from RTM message.
     * 
     * @param fte the return-by-reference @ref FteX entry to return the result.
     * @param rtm the RTM routing message.
     */
    static int	rtm_get_to_fte_cfg(FteX& fte, const struct rt_msghdr* rtm);
};

#endif // __FEA_ROUTING_SOCKET_UTILS_HH__
