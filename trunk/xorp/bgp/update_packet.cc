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

#ident "$XORP: xorp/bgp/update_packet.cc,v 1.22 2003/09/19 04:50:50 atanu Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "config.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "packet.hh"

#if 1
void
dump_bytes(const uint8_t *d, size_t l)
{
        printf("DEBUG_BYTES FN : %p %u\n", d, (uint32_t)l);
	for (u_int i=0;i<l;i++)
	    printf("%x ",*((const char *)d + i));
	printf("\n");
}
#else
void dump_bytes(const uint8_t*, size_t) {}
#endif

/* **************** UpdatePacket *********************** */

UpdatePacket::UpdatePacket() : _mpreach(0), _mpunreach(0)
{
    _Type = MESSAGETYPEUPDATE;
}


UpdatePacket::~UpdatePacket()
{
}

void
UpdatePacket::add_nlri(const BGPUpdateAttrib& nlri)
{
    _nlri_list.push_back(nlri);
}

void
UpdatePacket::multi_protocol(PathAttribute *pa)
{
    if(MP_REACH_NLRI == pa->type()) {
	if(0 != _mpreach)
	    XLOG_WARNING("Two multiprotocol reach attributes!!! %s %s", 
			 _mpreach->str().c_str(), pa->str().c_str());
	_mpreach = dynamic_cast<MPReachNLRIAttribute<IPv6>*>(pa);
    } else if(MP_UNREACH_NLRI == pa->type()) {
	if(0 != _mpunreach)
	    XLOG_WARNING("Two multiprotocol unreach attributes!!! %s %s", 
			 _mpunreach->str().c_str(), pa->str().c_str());
	_mpunreach = dynamic_cast<MPUNReachNLRIAttribute<IPv6>*>(pa);
    }
}

void
UpdatePacket::add_pathatt(const PathAttribute& pa)
{
    PathAttribute *paclone = pa.clone();
    multi_protocol(paclone);
   _pa_list.push_back(paclone);
}

void
UpdatePacket::add_pathatt(PathAttribute *pa)
{
    multi_protocol(pa);
    _pa_list.push_back(pa);
}

void
UpdatePacket::add_withdrawn(const BGPUpdateAttrib& wdr)
{
    _wr_list.push_back(wdr);
}

bool 
UpdatePacket::big_enough() const
{
    /* is the packet now large enough that we'd be best to send the
       part we already have before it exceeds the 4K size limit? */
    //XXXX needs additional tests for v6

    //quick and dirty check
    if (((_wr_list.size() + _nlri_list.size())* 4) > 2048) {
	debug_msg("withdrawn size = %u\n", (uint32_t)_wr_list.size());
	debug_msg("nlri size = %u\n", (uint32_t)_wr_list.size());
	return true;
    }
    return false;
}

const uint8_t *
UpdatePacket::encode(size_t &len, uint8_t *d) const
{
    XLOG_ASSERT( _nlri_list.empty() ||  ! pa_list().empty());

    list <PathAttribute*>::const_iterator pai;
    size_t i, pa_len = 0;
    size_t wr_len = wr_list().wire_size();
    size_t nlri_len = nlri_list().wire_size();

    // compute packet length

    for (pai = pa_list().begin() ; pai != pa_list().end(); ++pai)
	pa_len += (*pai)->wire_size();

    size_t desired_len = MINUPDATEPACKET + wr_len + pa_len + nlri_len;
    if (d != 0)		// XXX have a buffer, check length
	assert(len >= desired_len);	// XXX should throw an exception
    len = desired_len;

    if (len > MAXPACKETSIZE)	// XXX
	XLOG_FATAL("Attempt to encode a packet that is too big");

    debug_msg("Path Att: %u Withdrawn Routes: %u Net Reach: %u length: %u\n",
	      (uint32_t)pa_list().size(), (uint32_t)_wr_list.size(),
	      (uint32_t)_nlri_list.size(), (uint32_t)len);
    d = basic_encode(len, d);	// allocate buffer and fill header

    // fill withdraw list length (bytes)
    d[BGP_COMMON_HEADER_LEN] = (wr_len >> 8) & 0xff;
    d[BGP_COMMON_HEADER_LEN + 1] = wr_len & 0xff;

    // fill withdraw list
    i = BGP_COMMON_HEADER_LEN + 2;
    wr_list().encode(wr_len, d+i);
    i += wr_len;

    // fill pathattribute length (bytes)
    d[i++] = (pa_len >> 8) & 0xff;
    d[i++] = pa_len & 0xff;

    // fill path attribute list
    for (pai = pa_list().begin() ; pai != pa_list().end(); ++pai) {
        memcpy(d+i, (*pai)->data(), (*pai)->wire_size());
	i += (*pai)->wire_size();
    }	

    // fill NLRI list
    nlri_list().encode(nlri_len, d+i);
    i += nlri_len;
    return d;
}

UpdatePacket::UpdatePacket(const uint8_t *d, uint16_t l) 
    throw(CorruptMessage)
    : _mpreach(0), _mpunreach(0)
{
    debug_msg("UpdatePacket constructor called\n");
    _Type = MESSAGETYPEUPDATE;
    if (l < MINUPDATEPACKET)
	xorp_throw(CorruptMessage,
		   c_format("UpdatePacket too short length %d", l),
		   UPDATEMSGERR, ATTRLEN);
    d += BGP_COMMON_HEADER_LEN;	// move past header
    size_t wr_len = (d[0] << 8) + d[1];		// withdrawn length

    if (MINUPDATEPACKET + wr_len > l)
	xorp_throw(CorruptMessage,
		   c_format("Unreachable routes length is bogus %u > %u",
			    (uint32_t)wr_len, (uint32_t)(l - MINUPDATEPACKET)),
		   UPDATEMSGERR, ATTRLEN);
    
    size_t pa_len = (d[wr_len+2] << 8) + d[wr_len+3];	// pathatt length
    if (MINUPDATEPACKET + pa_len + wr_len > l)
	xorp_throw(CorruptMessage,
		   c_format("Pathattr length is bogus %u > %u",
			    (uint32_t)pa_len,
			    (uint32_t)(l - wr_len - MINUPDATEPACKET)),
		UPDATEMSGERR, ATTRLEN);

    size_t nlri_len = l - MINUPDATEPACKET - pa_len - wr_len;

    // Start of decoding of withdrawn routes.
    d += 2;	// point to the routes.
    _wr_list.decode(d, wr_len);
    d += wr_len;

    // Start of decoding of Path Attributes
    d += 2; // move past Total Path Attributes Length field
	
    while (pa_len > 0) {
	size_t used = 0;
        PathAttribute *pa = PathAttribute::create(d, pa_len, used);
	debug_msg("attribute size %u\n", (uint32_t)used);
	if (used == 0)
	    xorp_throw(CorruptMessage,
		   c_format("failed to read path attribute"),
		   UPDATEMSGERR, ATTRLEN);

	add_pathatt(pa);
// 	_pa_list.push_back(pa);
	d += used;
	pa_len -= used;
    }

    // Start of decoding of Network Reachability
    _nlri_list.decode(d, nlri_len);
    /* End of decoding of Network Reachability */
    debug_msg("No of withdrawn routes %u. No of path attributes %u. "
		"No of networks %u.\n",
		  (uint32_t)_wr_list.size(), (uint32_t)pa_list().size(),
		  (uint32_t)_nlri_list.size());
}

string
UpdatePacket::str() const
{
    string s = "Update Packet\n";
    debug_msg("No of withdrawn routes %u. No of path attributes %u. "
		"No of networks %u.\n",
	      (uint32_t)_wr_list.size(), (uint32_t)pa_list().size(),
	      (uint32_t)_nlri_list.size());

    s += _wr_list.str("Withdrawn");

    list <PathAttribute*>::const_iterator pai = pa_list().begin();
    while (pai != pa_list().end()) {
	s = s + " - " + (*pai)->str() + "\n";
	++pai;
    }
    
    s += _nlri_list.str("Nlri");
    return s;
}

/*
 * Helper function used when sorting Path Attribute lists.
 */
inline
bool
compare_path_attributes(PathAttribute *a, PathAttribute *b)
{
    return *a < *b;
}

bool 
UpdatePacket::operator==(const UpdatePacket& him) const 
{
    debug_msg("compare %s and %s", this->str().c_str(), him.str().c_str());

    if (_wr_list != him.wr_list())
	return false;

    //path attribute equals
    list <PathAttribute *> temp_att_list(pa_list());
    temp_att_list.sort(compare_path_attributes);
    list <PathAttribute *> temp_att_list_him(him.pa_list());
    temp_att_list_him.sort(compare_path_attributes);

    if (temp_att_list.size() != temp_att_list_him.size())
	return false;

    list <PathAttribute *>::const_iterator pai = temp_att_list.begin();
    list <PathAttribute *>::const_iterator pai_him = temp_att_list_him.begin();
    while (pai != temp_att_list.end() && pai_him != temp_att_list_him.end()) {
	
	if ( (**pai) == (**pai_him) ) {
	    ++pai;
	    ++pai_him;
	} else  {
	    debug_msg("%s did not match %s\n", (*pai)->str().c_str(),
		      (*pai_him)->str().c_str());
	    return false;
	}
    }

    //net layer reachability equals
    if (_nlri_list != him.nlri_list())
	return false;

    return true;
}
