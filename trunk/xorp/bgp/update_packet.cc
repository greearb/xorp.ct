// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/update_packet.cc,v 1.13 2003/01/29 20:32:33 rizzo Exp $"

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
        printf("DEBUG_BYTES FN : %p %d\n",d,l);
	for (u_int i=0;i<l;i++)
	    printf("%x ",*((const char *)d + i));
	printf("\n");
}
#else
void dump_bytes(const uint8_t*, size_t) {}
#endif

/* **************** UpdatePacket *********************** */

UpdatePacket::UpdatePacket()
{
    _Type = MESSAGETYPEUPDATE;
}


UpdatePacket::~UpdatePacket()
{
    list <PathAttribute*>::const_iterator pai = pa_list().begin();
    while(pai != pa_list().end()) {
	delete (*pai);
	++pai;
    }
}

void
UpdatePacket::add_nlri(const BGPUpdateAttrib& nlri)
{
    _nlri_list.push_back(nlri);
}

void
UpdatePacket::add_pathatt(const PathAttribute& pa)
{
    size_t l;
    PathAttribute *a = PathAttribute::create(pa.data(), pa.size(), l);

    _pa_list.push_back(a);
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
	debug_msg("withdrawn size = %d\n", _wr_list.size());
	debug_msg("nlri size = %d\n", _wr_list.size());
	return true;
    }
    return false;
}

const uint8_t *
UpdatePacket::encode(size_t &len, uint8_t *d) const
{
    XLOG_ASSERT( _nlri_list.empty() ||  ! pa_list().empty());

    list <BGPUpdateAttrib>::const_iterator uai;
    list <PathAttribute*>::const_iterator pai;
    size_t i, wr_len = 0, pa_len = 0, nlri_len = 0;

    // compute packet length

    for (uai = wr_list().begin() ; uai != wr_list().end(); ++uai)
	wr_len += uai->size();
    for (pai = pa_list().begin() ; pai != pa_list().end(); ++pai)
	pa_len += (*pai)->size();
    for (uai = nlri_list().begin() ; uai != nlri_list().end(); ++uai)
	nlri_len += uai->size();

    size_t desired_len = MINUPDATEPACKET + wr_len + pa_len + nlri_len;
    if (d != 0)		// XXX have a buffer, check length
	assert(len >= desired_len);	// XXX should throw an exception
    len = desired_len;

    if (len > MAXPACKETSIZE)	// XXX
	XLOG_FATAL("Attempt to encode a packet that is too big");

    debug_msg("Path Att: %d Withdrawn Routes: %d Net Reach: %d length: %d\n",
	      pa_list().size(),_wr_list.size(),_nlri_list.size(), len);
    d = basic_encode(len, d);	// allocate buffer and fill header

    // fill withdraw list length XXX (bytes ?)
    d[BGP_COMMON_HEADER_LEN] = (wr_len >> 8) & 0xff;
    d[BGP_COMMON_HEADER_LEN + 1] = wr_len & 0xff;

    // fill withdraw list
    i = BGP_COMMON_HEADER_LEN + 2;
    for (uai = wr_list().begin() ; uai != wr_list().end(); ++uai) {
	uai->copy_out(d+i);
	i += uai->size();
    }

    // fill pathattribute length XXX (bytes ?)
    d[i++] = (pa_len >> 8) & 0xff;
    d[i++] = pa_len & 0xff;

    // fill path attribute list
    for (pai = pa_list().begin() ; pai != pa_list().end(); ++pai) {
        memcpy(d+i, (*pai)->data(), (*pai)->size());
	i += (*pai)->size();
    }	

    // fill NLRI list
    for (uai = nlri_list().begin() ; uai != nlri_list().end(); ++uai) {
	uai->copy_out(d+i);
	i += uai->size();
    }	
    return d;
}

UpdatePacket::UpdatePacket(const uint8_t *d, uint16_t l)
	throw(CorruptMessage)
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
		   c_format("Unreachable routes length is bogus %d > %d",
			    wr_len, l - MINUPDATEPACKET),
		   UPDATEMSGERR, ATTRLEN);
    
    size_t pa_len = (d[wr_len+2] << 8) + d[wr_len+3];	// pathatt length
    if (MINUPDATEPACKET + pa_len + wr_len > l)
	xorp_throw(CorruptMessage,
		   c_format("Pathattr length is bogus %d > %d",
			    pa_len, l - wr_len - MINUPDATEPACKET),
		UPDATEMSGERR, ATTRLEN);

    size_t nlri_len = l - MINUPDATEPACKET - pa_len - wr_len;

    // Start of decoding of withdrawn routes.
    // Use a set to check for duplicates.
    d += 2;	// point to the routes.
    set <IPv4Net> x_set;

    while (wr_len >0 && wr_len >= BGPUpdateAttrib::size(d)) {
	BGPUpdateAttrib wr(d);
	wr_len -= BGPUpdateAttrib::size(d);
	d += BGPUpdateAttrib::size(d);
	if (x_set.find(wr.net()) == x_set.end()) {
	    _wr_list.push_back(wr);
	    x_set.insert(wr.net());
	} else
	    XLOG_WARNING(("Received duplicate " + wr.str() +
		       " in update message\n").c_str());
    }
    if (wr_len != 0)
	xorp_throw(CorruptMessage,
		   c_format("leftover bytes %d", wr_len),
		   UPDATEMSGERR, ATTRLEN);
   
    // Start of decoding of Path Attributes
    d += 2; // move past Total Path Attributes Length field
	
    while (pa_len > 0) {
	size_t used = 0;
        PathAttribute *pa = PathAttribute::create(d, pa_len, used);
	debug_msg("attribute size %d\n", used);
	if (used == 0)
	    xorp_throw(CorruptMessage,
		   c_format("failed to read path attribute"),
		   UPDATEMSGERR, ATTRLEN);

	_pa_list.push_back(pa);
	d += used;
	pa_len -= used;
    }

    // Start of decoding of Network Reachability
    x_set.clear();
    while (nlri_len > 0 && nlri_len >= BGPUpdateAttrib::size(d)) {
	BGPUpdateAttrib nlri(d);
	nlri_len -= BGPUpdateAttrib::size(d);
	d += BGPUpdateAttrib::size(d);
	if (x_set.find(nlri.net()) == x_set.end()) {
	    _nlri_list.push_back(nlri);
	    x_set.insert(nlri.net());
	} else
	    XLOG_WARNING(("Received duplicate nlri " + nlri.str() +
		       " in update message\n").c_str());
    }
    /* End of decoding of Network Reachability */
    debug_msg("No of withdrawn routes %d. No of path attributes %d. "
		"No of networks %d.\n",
		  _wr_list.size(), pa_list().size(),
		  _nlri_list.size());
}

string
UpdatePacket::str() const
{
    string s = "Update Packet\n";
    debug_msg("No of withdrawn routes %d. No of path attributes %d. "
		"No of networks %d.\n",
	      _wr_list.size(), pa_list().size(),
	      _nlri_list.size());

    list <BGPUpdateAttrib>::const_iterator wi = _wr_list.begin();
    while (wi != _wr_list.end()) {
	s = s + " - " + wi->str() + "\n";
	++wi;
    }

    list <PathAttribute*>::const_iterator pai = pa_list().begin();
    while (pai != pa_list().end()) {
	s = s + " - " + (*pai)->str() + "\n";
	++pai;
    }
    
    list <BGPUpdateAttrib>::const_iterator ni = _nlri_list.begin();
    while (ni != _nlri_list.end()) {
	s = s + " - " + ni->str() + "\n";
	++ni;
    }
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

    //withdrawn routes equals
    list <BGPUpdateAttrib> temp_wr_list(_wr_list);
    temp_wr_list.sort();
    list <BGPUpdateAttrib> temp_wr_list_him(him.wr_list());
    temp_wr_list_him.sort();

    if (temp_wr_list.size() != temp_wr_list_him.size())
	return false;

    list <BGPUpdateAttrib>::const_iterator wi = temp_wr_list.begin();
    list <BGPUpdateAttrib>::const_iterator wi_him = 
	temp_wr_list_him.begin();
    while (wi != temp_wr_list.end() && 
	   wi_him != temp_wr_list_him.end()) {
	
	if ( (*wi) == (*wi_him) )  {
	    ++wi;
	    ++wi_him;
	} else {
	    return false;
	}
    }

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
    list <BGPUpdateAttrib> temp_nlri_list(_nlri_list);
    temp_nlri_list.sort();
    list <BGPUpdateAttrib> temp_nlri_list_him(him.nlri_list());
    temp_nlri_list_him.sort();

    if (temp_nlri_list.size() != temp_nlri_list_him.size())
	return false;

    list <BGPUpdateAttrib>::const_iterator ni = temp_nlri_list.begin();
    list <BGPUpdateAttrib>::const_iterator ni_him = 
	temp_nlri_list_him.begin();
    while (ni != temp_nlri_list.end() && ni_him != temp_nlri_list_him.end()) {
	
	if ( (*ni) == (*ni_him) ) {
	    ++ni;
	    ++ni_him;
	} else {
	    return false;
	}
    }

    return true;
}
