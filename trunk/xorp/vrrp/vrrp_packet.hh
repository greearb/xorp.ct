// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 XORP, Inc.
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

// $XORP: xorp/vrrp/vrrp_packet.hh,v 1.2 2008/10/09 18:03:49 abittau Exp $

#ifndef __VRRP_VRRP_PACKET_HH__
#define __VRRP_VRRP_PACKET_HH__

#include "libxorp/ipv4.hh"
#include "libproto/packet.hh"

typedef vector<uint8_t> PAYLOAD;

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

    static VrrpHeader&	      assign(uint8_t* data);
    static const VrrpHeader&  assign(const PAYLOAD& payload);
    uint32_t		      finalize();
    void		      add_ip(const IPv4& ip);
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

struct VrrpAuth {
    uint8_t	    va_data[8];
};

#define IP_HEADER_MIN_SIZE	20
#define VRRP_MAX_PACKET_SIZE	(IP_HEADER_MIN_SIZE			    \
			         + sizeof(VrrpHeader) + sizeof(VrrpAuth)    \
				 + sizeof(struct in_addr) * 255)

class VrrpPacket {
public:
    enum {
	IPPROTO_VRRP = 112
    };

    static const IPv4 mcast_group;

    VrrpPacket();

    void	    set_source(const IPv4& ip);
    void	    set_vrid(uint8_t vrid);
    void	    set_priority(uint8_t priority);
    void	    set_interval(uint8_t interval);
    void	    clear_ips();
    void	    add_ip(const IPv4& ip);
    void	    finalize();
    const PAYLOAD&  data() const;
    uint32_t	    size() const;

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
