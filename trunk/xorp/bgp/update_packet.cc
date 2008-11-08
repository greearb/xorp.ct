// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/bgp/update_packet.cc,v 1.48 2008/10/02 21:56:23 bms Exp $"

//#define DEBUG_LOGGING
//#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "packet.hh"
#include "peer.hh"

#if 1
void
dump_bytes(const uint8_t *d, size_t l)
{
        printf("DEBUG_BYTES FN : %p %u\n", d, XORP_UINT_CAST(l));
	for (u_int i=0;i<l;i++)
	    printf("%x ",*((const char *)d + i));
	printf("\n");
}
#else
void dump_bytes(const uint8_t*, size_t) {}
#endif

/* **************** UpdatePacket *********************** */

UpdatePacket::UpdatePacket()
    : _pa_list(new FastPathAttributeList<IPv4>())
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
UpdatePacket::add_pathatt(const PathAttribute& pa)
{
    _pa_list->add_path_attribute(pa);
}

void
UpdatePacket::add_pathatt(PathAttribute *pa)
{
    _pa_list->add_path_attribute(pa);
}

void
UpdatePacket::replace_pathattribute_list(FPAList4Ref& pa_list)
{
    _pa_list = pa_list;
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
	debug_msg("withdrawn size = %u\n", XORP_UINT_CAST(_wr_list.size()));
	debug_msg("nlri size = %u\n", XORP_UINT_CAST(_wr_list.size()));
	return true;
    }
    return false;
}

bool
UpdatePacket::encode(uint8_t *d, size_t &len, const BGPPeerData *peerdata) const
{
    XLOG_ASSERT( (_nlri_list.empty()) ||  !(_pa_list->is_empty()) );
    XLOG_ASSERT(d != 0);
    XLOG_ASSERT(len != 0);
    debug_msg("UpdatePacket::encode: len=%u\n", (uint32_t)len);

#if 0
    list <PathAttribute*>::const_iterator pai;
#endif
    size_t i, pa_len = 0;
    size_t wr_len = wr_list().wire_size();
    size_t nlri_len = nlri_list().wire_size();

    // compute packet length
    pa_len = BGPPacket::MAXPACKETSIZE;
    uint8_t pa_list_buf[pa_len];
    if (_pa_list->is_empty() ) {
	pa_len = 0;
    } else {
	if (_pa_list->encode(pa_list_buf, pa_len, peerdata) == false) {
	    XLOG_WARNING("failed to encode update - no space for pa list\n");
	    return false;
	}
    }

#if 0
    for (pai = pa_list().begin() ; pai != pa_list().end(); ++pai)
	pa_len += (*pai)->wire_size();
#endif

    size_t desired_len = BGPPacket::MINUPDATEPACKET + wr_len + pa_len
	+ nlri_len;
    if (len < desired_len) {
	abort(); ///XXXX
	XLOG_WARNING("failed to encode update, desired_len=%u, len=%u, wr=%u, pa=%u, nlri=%u\n",
		     (uint32_t)desired_len, (uint32_t)len, 
		     (uint32_t)wr_len, (uint32_t)pa_len, (uint32_t)nlri_len);
	return false;
    }

    len = desired_len;

    if (len > BGPPacket::MAXPACKETSIZE)		// XXX
	XLOG_FATAL("Attempt to encode a packet that is too big");

    debug_msg("Withdrawn Routes: %u Net Reach: %u length: %u\n",
	      XORP_UINT_CAST(_wr_list.size()),
	      XORP_UINT_CAST(_nlri_list.size()),
	      XORP_UINT_CAST(len));
    d = basic_encode(len, d);	// allocate buffer and fill header

    // fill withdraw list length (bytes)
    d[BGPPacket::COMMON_HEADER_LEN] = (wr_len >> 8) & 0xff;
    d[BGPPacket::COMMON_HEADER_LEN + 1] = wr_len & 0xff;

    // fill withdraw list
    i = BGPPacket::COMMON_HEADER_LEN + 2;
    wr_list().encode(wr_len, d + i);
    i += wr_len;

    // fill pathattribute length (bytes)
    d[i++] = (pa_len >> 8) & 0xff;
    d[i++] = pa_len & 0xff;

#if 0
    // fill path attribute list
    for (pai = pa_list().begin() ; pai != pa_list().end(); ++pai) {
        memcpy(d+i, (*pai)->data(), (*pai)->wire_size());
	i += (*pai)->wire_size();
    }
#endif
    memcpy(d+i, pa_list_buf, pa_len);
    i += pa_len;

    // fill NLRI list
    nlri_list().encode(nlri_len, d+i);
    i += nlri_len;
    return true;
}

UpdatePacket::UpdatePacket(const uint8_t *d, uint16_t l, 
			   const BGPPeerData* peerdata,
			   BGPMain *mainprocess,
			   bool do_checks) throw(CorruptMessage)
{
    debug_msg("UpdatePacket constructor called\n");
    _Type = MESSAGETYPEUPDATE;
    if (l < BGPPacket::MINUPDATEPACKET)
	xorp_throw(CorruptMessage,
		   c_format("Update Message too short %d", l),
		   MSGHEADERERR, BADMESSLEN, d + BGPPacket::MARKER_SIZE, 2);
    d += BGPPacket::COMMON_HEADER_LEN;		// move past header
    size_t wr_len = (d[0] << 8) + d[1];		// withdrawn length
    if (BGPPacket::MINUPDATEPACKET + wr_len > l)
	xorp_throw(CorruptMessage,
		   c_format("Unreachable routes length is bogus %u > %u",
			    XORP_UINT_CAST(wr_len),
			    XORP_UINT_CAST(l - BGPPacket::MINUPDATEPACKET)),
		   UPDATEMSGERR, MALATTRLIST);
    
    size_t pa_len = (d[wr_len+2] << 8) + d[wr_len+3];	// pathatt length
    if (BGPPacket::MINUPDATEPACKET + pa_len + wr_len > l)
	xorp_throw(CorruptMessage,
		   c_format("Pathattr length is bogus %u > %u",
			    XORP_UINT_CAST(pa_len),
			    XORP_UINT_CAST(l - wr_len - BGPPacket::MINUPDATEPACKET)),
		UPDATEMSGERR, MALATTRLIST);

    size_t nlri_len = l - BGPPacket::MINUPDATEPACKET - pa_len - wr_len;

    // Start of decoding of withdrawn routes.
    d += 2;	// point to the routes.
    _wr_list.decode(d, wr_len);
    d += wr_len;

    // Start of decoding of Path Attributes
    d += 2; // move past Total Path Attributes Length field

    size_t used = pa_len;
    _pa_list = new FastPathAttributeList<IPv4>();
    _pa_list->load_raw_data(d, used, peerdata, 
			    (nlri_len > 0), mainprocess, do_checks);
    d += used;

    // Start of decoding of Network Reachability
    _nlri_list.decode(d, nlri_len);
    /* End of decoding of Network Reachability */
    debug_msg("No of withdrawn routes %u. "
	      "No of networks %u.\n",
	      XORP_UINT_CAST(_wr_list.size()),
	      XORP_UINT_CAST(_nlri_list.size()));
}

string
UpdatePacket::str() const
{
    string s = "Update Packet\n";
    debug_msg("No of withdrawn routes %u. "
		"No of networks %u.\n",
	      XORP_UINT_CAST(_wr_list.size()),
	      XORP_UINT_CAST(_nlri_list.size()));

    if (!_wr_list.empty())
	s += _wr_list.str("Withdrawn");

    if (!_pa_list->is_empty()) {
	s += _pa_list->str();
	s += "\n";
    }
#if 0
    list <PathAttribute*>::const_iterator pai = pa_list().begin();
    while (pai != pa_list().end()) {
	s = s + " - " + (*pai)->str() + "\n";
	++pai;
    }
#endif
    
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

#if 0
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
#endif

    bool him_empty = him._pa_list->is_empty();
    bool me_empty = _pa_list->is_empty();

    if (me_empty) {
	if (!him_empty) {
	    return false;
	}
    } else {
	int count = 0;
	for (int i=0; i < MAX_ATTRIBUTE; i++)
	    if (_pa_list->find_attribute_by_type((PathAttType)i) != 0) 
		count++;
	if (him_empty) {
            return false;
	}
 
	if ( !(*_pa_list == *(him._pa_list)) ) {
	    return false;
	}
    }

    //net layer reachability equals
    if (_nlri_list != him.nlri_list())
	return false;

    return true;
}

