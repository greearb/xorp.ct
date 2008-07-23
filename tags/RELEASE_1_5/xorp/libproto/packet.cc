// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2006-2008 XORP, Inc.
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

#ident "$XORP: xorp/libproto/packet.cc,v 1.5 2008/01/04 03:16:19 pavlin Exp $"


//
// Packet related manipulation tools
//

#include "libproto_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "checksum.h"
#include "packet.hh"


int
IpHeader4::fragment(size_t mtu, list<vector<uint8_t> >& fragments,
		    bool do_checksum, string& error_msg) const
{
    const IpHeader4& orig_ip4 = *this;
    size_t datalen = orig_ip4.ip_len();

    //
    // If the data packet is small enough, then don't fragment it
    //
    if (datalen <= mtu) {
	return (XORP_OK);
    }

    //
    // Fragment the inner packet, then encapsulate and send each fragment
    //
    vector<uint8_t> frag_buf(datalen);	// The buffer for the fragments
    IpHeader4Writer frag_ip4(&frag_buf[0]);
    size_t frag_optlen = 0;		// The optlen to copy into fragments
    size_t frag_ip_hl;

    if (orig_ip4.ip_off() & IpHeader4::FRAGMENT_FLAGS_IP_DF) {
	// Fragmentation is forbidded
	error_msg = c_format("Cannot fragment encapsulated IP packet "
			     "from %s to %s: "
			     "fragmentation not allowed",
			     cstring(orig_ip4.ip_src()),
			     cstring(orig_ip4.ip_dst()));
	XLOG_WARNING("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    if (((mtu - (orig_ip4.ip_header_len())) & ~7) < 8) {
	//
	// Fragmentation is possible only if we can put at least
	// 8 octets per fragment (except the last one).
	//
	error_msg = c_format("Cannot fragment encapsulated IP packet "
			     "from %s to %s: "
			     "cannot send fragment with size less than 8 octets",
			     cstring(orig_ip4.ip_src()),
			     cstring(orig_ip4.ip_dst()));
	XLOG_WARNING("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    //
    // Copy the IP header and the options that should be copied during
    // fragmentation.
    // XXX: the code below is taken from FreeBSD's ip_optcopy()
    // in netinet/ip_output.c
    //
    memcpy(&frag_buf[0], _data, IpHeader4::SIZE);
    {
	register const u_char *cp;
	register u_char *dp;
	int opt, optlen, cnt;

	cp = (const u_char *)(orig_ip4.data() + orig_ip4.size());
	dp = (u_char *)(frag_ip4.data() + frag_ip4.size());
	cnt = orig_ip4.ip_header_len() - IpHeader4::SIZE;
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
	    opt = cp[0];
	    if (opt == IpHeader4::OPTIONS_IPOPT_EOL)
		break;
	    if (opt == IpHeader4::OPTIONS_IPOPT_NOP) {
		// Preserve for IP mcast tunnel's LSRR alignment.
		*dp++ = IpHeader4::OPTIONS_IPOPT_NOP;
		optlen = 1;
		continue;
	    }

	    //
	    // Check for bogus lengths
	    //
	    if ((size_t)cnt < IpHeader4::OPTIONS_IPOPT_OLEN + sizeof(*cp)) {
		error_msg = c_format("Cannot fragment encapsulated IP "
				     "packet from %s to %s: "
				     "malformed IPv4 option",
				     cstring(orig_ip4.ip_src()),
				     cstring(orig_ip4.ip_dst()));
		XLOG_WARNING("%s", error_msg.c_str());
		return (XORP_ERROR);
	    }
	    optlen = cp[IpHeader4::OPTIONS_IPOPT_OLEN];
	    if (((size_t)optlen < IpHeader4::OPTIONS_IPOPT_OLEN + sizeof(*cp))
		|| (optlen > cnt)) {
		error_msg = c_format("Cannot fragment encapsulated IP "
				     "packet from %s to %s: "
				     "malformed IPv4 option",
				     cstring(orig_ip4.ip_src()),
				     cstring(orig_ip4.ip_dst()));
		XLOG_WARNING("%s", error_msg.c_str());
		return (XORP_ERROR);
	    }

	    if (optlen > cnt)
		optlen = cnt;
	    if (IpHeader4::OPTIONS_COPIED_FLAG & opt) {
		memcpy(dp, cp, optlen);
		dp += optlen;
	    }
	}
	for (optlen = dp - (u_char *)(frag_ip4.data() + frag_ip4.size());
	     optlen & 0x3; optlen++) {
	    *dp++ = IpHeader4::OPTIONS_IPOPT_EOL;
	}
	frag_optlen = optlen;	// return (optlen);
    }
    // The header length with the remaining IP options
    frag_ip_hl = IpHeader4::SIZE + frag_optlen;
    frag_ip4.set_ip_header_len(frag_ip_hl);

    //
    // Do the fragmentation
    //
    size_t data_start = 0;
    size_t data_end = datalen;
    // Send the first segment with all the options
    {
	vector<uint8_t> first_frag(mtu);
	IpHeader4Writer first_ip(&first_frag[0]);
	size_t first_ip_hl = orig_ip4.ip_header_len();
	size_t nfb = (mtu - first_ip_hl) / 8;
	size_t first_frag_len = first_ip_hl + nfb*8;

	first_frag.resize(first_frag_len);
	memcpy(&first_frag[0], _data, first_frag_len);

	// Correct the IP header
	first_ip.set_ip_off(first_ip.ip_off() | IpHeader4::FRAGMENT_FLAGS_IP_MF);
	first_ip.set_ip_len(first_frag_len);
	first_ip.set_ip_sum(0);
	if (do_checksum) {
	    first_ip.set_ip_sum(ntohs(inet_checksum(first_ip.data(),
						    first_ip_hl)));
	}

	// Add the first fragment
	fragments.push_back(first_frag);

	data_start += first_frag_len;
    }
    
    //
    // Create the remaining of the fragments
    //
    while (data_start < data_end) {
	size_t nfb = (mtu - frag_ip_hl) / 8;
	size_t frag_len = frag_ip_hl + nfb*8;
	size_t frag_data_len = nfb*8;
	bool is_last_fragment = false;

	// Compute the fragment length
	if (data_end - data_start <= frag_data_len) {
	    frag_data_len = data_end - data_start;
	    frag_len = frag_ip_hl + frag_data_len;
	    is_last_fragment = true;
	}

	// Copy the data
	frag_buf.resize(frag_len);
	memcpy(&frag_buf[0] + frag_ip_hl, _data + data_start, frag_data_len);

	// The IP packet total length
	frag_ip4.set_ip_len(frag_len);

	// The IP fragment flags and offset
	{
	    unsigned short ip_off_field = orig_ip4.ip_off();
	    unsigned short frag_ip_off_flags = ip_off_field & ~IpHeader4::FRAGMENT_OFFSET_MASK;
	    unsigned short frag_ip_off = (ip_off_field & IpHeader4::FRAGMENT_OFFSET_MASK);

	    if (! is_last_fragment)
		frag_ip_off_flags |= IpHeader4::FRAGMENT_FLAGS_IP_MF;
	    
	    frag_ip_off += (data_start - orig_ip4.ip_header_len()) / 8;	// XXX
	    frag_ip4.set_ip_off(frag_ip_off_flags | frag_ip_off);
	}

	frag_ip4.set_ip_sum(0);
	if (do_checksum) {
	    frag_ip4.set_ip_sum(ntohs(inet_checksum(frag_ip4.data(),
						    frag_ip_hl)));
	}

	// Add the fragment
	fragments.push_back(frag_buf);

	data_start += frag_data_len;
    }

    return (XORP_OK);
}
