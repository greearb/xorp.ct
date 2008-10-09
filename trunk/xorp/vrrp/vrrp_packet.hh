// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 XORP, Inc.
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

// $XORP: xorp/devnotes/template.hh,v 1.10 2008/07/23 05:09:59 pavlin Exp $

#ifndef __VRRP_VRRP_PACKET_HH__
#define __VRRP_VRRP_PACKET_HH__

#include "libxorp/ipv4.hh"
#include "libproto/packet.hh"

typedef vector<uint8_t> PAYLOAD;

struct VRRPHeader {
    enum Versions {
	VRRP_VERSION = 2
    };
    enum PktTypes {
	VRRP_TYPE_ADVERTISEMENT = 1
    };
    enum AuthTypes {
	VRRP_AUTH_NONE = 0
    };

    static VRRPHeader&	      assign(uint8_t* data);
    static const VRRPHeader&  assign(const PAYLOAD& payload);
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

struct VRRPAuth {
    uint8_t	    va_data[8];
};

#define IP_HEADER_MIN_SIZE	20
#define VRRP_MAX_PACKET_SIZE	(IP_HEADER_MIN_SIZE			    \
			         + sizeof(VRRPHeader) + sizeof(VRRPAuth)    \
				 + sizeof(struct in_addr) * 255)

class VRRPPacket {
public:
    enum {
	IPPROTO_VRRP = 112
    };

    static const IPv4 mcast_group;

    VRRPPacket();

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
    VRRPHeader&		_vrrp;
};

#endif // __VRRP_VRRP_PACKET_HH__
