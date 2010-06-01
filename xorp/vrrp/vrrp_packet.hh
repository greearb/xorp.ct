// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008-2010 XORP, Inc and Others
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

// $XORP: xorp/vrrp/vrrp_packet.hh,v 1.5 2008/12/18 11:34:51 abittau Exp $

#ifndef __VRRP_VRRP_PACKET_HH__
#define __VRRP_VRRP_PACKET_HH__

#include "libxorp/ipv4.hh"
#include "libproto/packet.hh"

typedef vector<uint8_t> PAYLOAD;

/**
 * @short The VRRP header.
 */
struct VrrpHeader {
    enum Versions {
	VRRP_VERSION = 2
    };
    enum PktTypes {
	VRRP_TYPE_ADVERTISEMENT = 1
    };
    enum AuthTypes {
	VRRP_AUTH_NONE = 0
    };

    /**
     * Create a new VRRP packet.  Caller must allocate memory and assert size
     * (VRRP_MAX_PACKET_SIZE).
     *
     * @return the VRRP header.
     * @param data pointer where packet should be stored.
     */
    static VrrpHeader&	      assign(uint8_t* data);

    /**
     * Parse a VRRP packet.
     *
     * @return the VRRP header.
     * @param payload The VRRP packet starting with the VRRP header.
     */
    static const VrrpHeader&  assign(const PAYLOAD& payload);

    /**
     * Must be called when all fields have been manipulated.  This will setup
     * the final bits of information (e.g., checksum) and the packet will become
     * ready to be sent.
     *
     * @return the length of the packet.
     */
    uint32_t		      finalize();

    /**
     * Add an IP address of the virtual router to the advertisement.
     *
     * @param ip IP address to add to the advertisement.
     */
    void		      add_ip(const IPv4& ip);

    /**
     * Extract an IP address from the advertisement.
     *
     * @return the IP address at the specified index.
     * @param index the index of the IP (0..vh_ipcount).
     */
    IPv4		      ip(unsigned index) const;

#if defined(WORDS_BIGENDIAN)
    uint8_t	    vh_v:4;
    uint8_t	    vh_type:4;
#elif defined(WORDS_SMALLENDIAN)
    uint8_t	    vh_type:4;
    uint8_t	    vh_v:4;
#else
#error "Unknown endianness"
#endif
    uint8_t	    vh_vrid;
    uint8_t	    vh_priority;
    uint8_t	    vh_ipcount;
    uint8_t	    vh_auth;
    uint8_t	    vh_interval;
    uint16_t	    vh_sum;
    struct in_addr  vh_addr[0];
};

/**
 * @short VRRP authentication data.  Unused in RFC 3768.
 */
struct VrrpAuth {
    uint8_t	    va_data[8];
};

#define IP_HEADER_MIN_SIZE	20
#define VRRP_MAX_PACKET_SIZE	(IP_HEADER_MIN_SIZE			    \
			         + sizeof(VrrpHeader) + sizeof(VrrpAuth)    \
				 + sizeof(struct in_addr) * 255)

/**
 * @short A VRRP packet including the IP header.
 */
class VrrpPacket {
public:
    static const IPv4 mcast_group;

    VrrpPacket();

    /**
     * Set the source IP address in the IP header.
     *
     * @param ip source IP address in IP header.
     */
    void	    set_source(const IPv4& ip);

    /**
     * Set the virtual router ID in the VRRP header.
     *
     * @param vrid the virtual router ID in the VRRP header.
     */
    void	    set_vrid(uint8_t vrid);

    /**
     * Set the priority in the VRRP header.
     *
     * @param priority the router priority in the VRRP header.
     */
    void	    set_priority(uint8_t priority);

    /**
     * Set the advertisement interval in VRRP's header.
     *
     * @param interval the advertisement interval in VRRP's header.
     */
    void	    set_interval(uint8_t interval);

    /**
     * Remove all IPs from the VRRP advertisement.
     */
    void	    clear_ips();

    /**
     * Add an IP to the VRRP header.
     *
     * @param ip IP to add to the virtual router in the VRRP header.
     */
    void	    add_ip(const IPv4& ip);

    /**
     * Must be called when all fields are set.  This method will finalize any
     * remaining fields such as checksums.
     */
    void	    finalize();

    /**
     * Get the packet data.
     *
     * @return the packet data (IP and VRRP).
     */
    const PAYLOAD&  data() const;

    /**
     * Get the packet size.
     *
     * @return packet size.
     */
    uint32_t	    size() const;

     /**
     * Set the packet size.
     *
     * @param packet size.
     */
    void set_size(uint32_t size);

    /**
     * Set multiple IPs from a container into the VRRP header.
     *
     * @param ips collection of IP addresses to add to the VRRP header.
     */
    template<class T> void set_ips(const T& ips)
    {
	clear_ips();
	for (typename T::const_iterator i = ips.begin(); i != ips.end(); ++i)
	    add_ip(*i);
    }

private:
    PAYLOAD		_data;
    IpHeader4Writer	_ip;
    VrrpHeader&		_vrrp;
};

#endif // __VRRP_VRRP_PACKET_HH__
