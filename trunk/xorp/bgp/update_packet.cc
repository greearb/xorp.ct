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

#ident "$XORP: xorp/bgp/update_packet.cc,v 1.5 2003/01/17 05:51:07 mjh Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "config.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "packet.hh"

#ifdef DEBUG_LOGGING
inline static void dump_bytes(const uint8_t *d, size_t l)
{
    //    debug_msg("DEBUG_BYTES FN : %p %d\n",d,l);
	for(u_int i=0;i<l;i++)
	    debug_msg("%x ",*((const char *)d + i));
	debug_msg("\n");
}
#else
inline static void dump_bytes(const uint8_t*, size_t) {}
#endif

/* **************** UpdatePacket *********************** */

UpdatePacket::UpdatePacket()
{
    _Type = MESSAGETYPEUPDATE;
    //    _pathattributes = NULL;
    _Data = NULL;
}

UpdatePacket::UpdatePacket(const uint8_t *d, uint16_t l)
{
    debug_msg("UpdatePacket constructor called\n");
    //    _withdrawnroutes = NULL;
    //_pathattributes = NULL;
    //    _networkreachability = NULL;
    
    _Length = l; 
    _Type = MESSAGETYPEUPDATE;
    uint8_t *data = new uint8_t[l];
    memcpy(data,d,l);
    _Data = data;
}

UpdatePacket::~UpdatePacket() {
    if (_Data != NULL)
	delete[] _Data;
    list <PathAttribute*>::iterator pai = _att_list.begin();
    while(pai != _att_list.end()) {
	delete (*pai);
	++pai;
    }
}

void
UpdatePacket::add_nlri(const NetLayerReachability& nlri)
{
    _nlri_list.push_back(nlri);
}

void
UpdatePacket::add_pathatt(const PathAttribute& pa)
{
    PathAttribute *attribute;
    switch (pa.type()) {
    case ORIGIN:
	attribute = new OriginAttribute((const OriginAttribute&)pa);
	break;
    case AS_PATH:
	attribute = new ASPathAttribute((const ASPathAttribute&)pa);
	break;
    case NEXT_HOP: {
	/*
	 * We know pa is a NextHopAttribute, but we don't know if
	 * it's the IPv4 version or the IPv6 version.  We need to use
	 * dynamic_cast to find out.  
         */
	const NextHopAttribute<IPv4> *v4nh;
	v4nh = dynamic_cast<const NextHopAttribute<IPv4>*>(&pa);
	if (v4nh != NULL) {
	    attribute = 
		new NextHopAttribute<IPv4>((const NextHopAttribute<IPv4>&)pa);
	    break;
	} 
	const NextHopAttribute<IPv6> *v6nh;
	v6nh = dynamic_cast<const NextHopAttribute<IPv6>*>(&pa);
	if (v6nh != NULL) {
	    attribute = 
		new NextHopAttribute<IPv6>((const NextHopAttribute<IPv6>&)pa);
	    break;
	} 
	abort();
    }
    case MED:
	attribute = new MEDAttribute((const MEDAttribute&)pa);
	break;
    case LOCAL_PREF:
	attribute = new LocalPrefAttribute((const LocalPrefAttribute&)pa);
	break;
    case ATOMIC_AGGREGATE:
	attribute = 
	    new AtomicAggAttribute((const AtomicAggAttribute&)pa);
	break;
    case AGGREGATOR:
	attribute = 
	    new AggregatorAttribute((const AggregatorAttribute&)pa);
	break;
    case COMMUNITY:
	attribute = 
	    new CommunityAttribute((const CommunityAttribute&)pa);
	break;
    case UNKNOWN:
	attribute = 
	    new UnknownAttribute((const UnknownAttribute&)pa);
	break;
    }	
	
    _att_list.push_back(attribute);
}

void
UpdatePacket::add_withdrawn(const BGPWithdrawnRoute& wdr)
{
    _withdrawn_list.push_back(wdr);
}


bool 
UpdatePacket::big_enough() const {
    /* is the packet now large enough that we'd be best to send the
       part we already have before it exceeds the 4K size limit? */
    //XXXX needs additional tests for v6

    //quick and dirty check
    if (((_withdrawn_list.size() + _nlri_list.size())* 4) > 2048) {
	debug_msg("withdrawn size = %d\n", _withdrawn_list.size());
	debug_msg("nlri size = %d\n", _withdrawn_list.size());
	return true;
    }
    return false;
}

const uint8_t *UpdatePacket::encode(int &len) const
{
    int size = 5 + _att_list.size() + 
	(2*_withdrawn_list.size()) +(2*_nlri_list.size()); 
    int position = 2; // counter only starts after marker and length have been set.
    debug_msg("Path Att: %d Withdrawn Routes: %d Net Reach: %d size: %d\n",
	      _att_list.size(),_withdrawn_list.size(),_nlri_list.size(), size);
	
    XLOG_ASSERT(0 == _nlri_list.size() || 0 != _att_list.size());

    size_t route_position = 0;
    size_t route_size = 0;
    size_t path_position = 0;
    size_t path_size = 0;
    size_t total_size = BGP_COMMON_HEADER_LEN; // Base header size	

    struct iovec io[size];
	
    io[0].iov_base = const_cast<char*>((const char *)&_Marker[0]);
    io[0].iov_len = MARKER_SIZE;
	
    // 1 is set below, since total size is currently unknown.
	
    io[2].iov_base = const_cast<char*>((const char *)&_Type);
    io[2].iov_len = 1;
    position++;
	
    route_position = position;
    position++;
       
    list <BGPWithdrawnRoute>::const_iterator wi = _withdrawn_list.begin();
    while (wi != _withdrawn_list.end()) {
	assert(position < size);
	io[position].iov_base = 
	    (char *)(const_cast<uint8_t*>(wi->get_size()));
	io[position].iov_len = 1;
	position++;
		
	assert(position < size);
	io[position].iov_base = const_cast<char*>(wi->get_data());
	io[position].iov_len = wi->get_byte_size();
	route_size += wi->get_byte_size()+1;
	position++;
		
	wi++;
    }
	
    total_size = total_size + route_size;

    path_position = position;
    position++;

    list <PathAttribute*>::const_iterator pai = _att_list.begin();
    while(pai != _att_list.end())
	{
	    PathAttribute* pa = *pai;
	    debug_msg("Looping through path attributes\n");
	    io[position].iov_base = 
		(char *)const_cast<uint8_t *>(pa->encode_and_get_data());
	    io[position].iov_len = pa->get_size();
	    path_size = path_size + pa->get_size();
	    position++;
	    ++pai;
	}	
	
    total_size = total_size + path_size;

    list <NetLayerReachability>::const_iterator ni = _nlri_list.begin();
    while (ni != _nlri_list.end()) {
	assert(position < size);
	io[position].iov_base = 
	    (char *)const_cast<uint8_t *>(ni->get_size());		
	io[position].iov_len = 1;
	position++;
		
	assert(position < size);
	io[position].iov_base = const_cast<char *>(ni->get_data());
	io[position].iov_len = ni->get_byte_size();
	position++;
		
	total_size += ni->get_byte_size()+1;

	ni++;

    }	

    if (route_size > MAXPACKETSIZE)
	XLOG_FATAL("Bad route size in encode()");
    uint16_t net_route_size = route_size;
    net_route_size = htons(net_route_size);
    io[route_position].iov_base = (char *)&net_route_size;
    io[route_position].iov_len = 2;
	
    total_size = total_size + 2;
	
    if (path_size > MAXPACKETSIZE)
	XLOG_FATAL("Bad path size in encode()");
    uint16_t net_path_size = path_size;
    net_path_size = htons(net_path_size);
    io[path_position].iov_base = (char *)&net_path_size;
    io[path_position].iov_len = 2;
	
    total_size = total_size + 2;
	
    if (total_size < MINPACKETSIZE || total_size > MAXPACKETSIZE) {
	debug_msg("Throw exception packet length invalid (%d)\n",
		  total_size);
	XLOG_FATAL("Attempt to encode a packet that is too big");
    }
    uint16_t net_total_size = total_size;
    net_total_size = htons(net_total_size);
    io[1].iov_base = (char *)&net_total_size;
    io[1].iov_len = 2;
    debug_msg("size %d total_size %d\n",size,total_size);

    return flatten(&io[0], size, len);
}

void
UpdatePacket::decode()
    throw(CorruptMessage)
{
    int urlength = 0;
    int plength = 0; // Path attributes length
    int nlength = 0;

    const uint8_t *data = _Data;
	
    /* shift the data by the header length */
    data = data + BGP_COMMON_HEADER_LEN;

    urlength = ntohs((uint16_t &)(*data));

    //check unreachable routes length is feasible
    nlength = _Length - MINUPDATEPACKET;
    if (nlength < urlength)
	xorp_throw(CorruptMessage,
		   c_format("Unreachable routes length is bogus %d < %d",
			    nlength, urlength),
		   UPDATEMSGERR, ATTRLEN);
    
    nlength = _Length - MINUPDATEPACKET - urlength;

    data = data + sizeof(uint16_t); // move passed unreachable routes length field
    size_t urrlength = 0; // Length of a single unreachable route
    size_t bytes;

    /* Start of decoding of Unreachable routes */
    set<BGPWithdrawnRoute> withdrawn_set;
    assert(urlength >=0);
    while (urlength > 0)
	{
	    //urrlength is the number of bits of address prefix. 
	    urrlength = (uint8_t &)(*data);
	    //bytes is the number of bytes, excluding header
	    bytes = (urrlength+7)/8;  
	    //check bytes is sane.  note this is only for v4, v6 is
	    //done elsewhere
	    if ((bytes > (uint)urlength) || (bytes > 4))
		xorp_throw(CorruptMessage,
			   c_format("inconsistent length %d %d",
			    bytes, urlength),
		   UPDATEMSGERR, ATTRLEN);
	    data++;

	    BGPWithdrawnRoute withdrawn((uint32_t &)(*data), urrlength);
	    if (withdrawn_set.find(withdrawn) == withdrawn_set.end()) {
		_withdrawn_list.push_back(withdrawn);
	    } else {
		XLOG_WARNING(("Received duplicate " + withdrawn.str() +
			   " in update message\n").c_str());
	    }
	    data += bytes;
	    urlength -= (bytes + 1);
	}
    if (urlength < 0)
	xorp_throw(CorruptMessage,
		   c_format("negative length %d", urlength),
		   UPDATEMSGERR, ATTRLEN);
    /* End of decoding of Unreachable routes */
   
    /* Start of decoding of Path Attributes */
    plength = ntohs((uint16_t &)(*data)); 

    nlength = nlength - plength; /* Remove path length from network
				    reachability length */
    //check path attribute length was feasible
    if (nlength < 0)
	xorp_throw(CorruptMessage,
		   c_format("negative length %d", nlength),
		   UPDATEMSGERR, ATTRLEN);
    
    data = data + sizeof(uint16_t); // move passed Total Path Attributes Length field
	
    uint8_t paflags = 0; // Parameter flags
    size_t palength = 0; // Parameter length
    uint8_t pa_type; // Parameter type
    size_t shift = 0; // if extended Parameter need to shift by 3 instead of 4.

    PathAttribute* pa_temp = NULL;

    assert(_att_list.size() == 0);

    assert(plength >= 0);
    while (plength > 0)
	{
	    
	    paflags = (uint8_t &)(*data);
	    
	    
	    if ((paflags & 0x10) == 0x10)
		{
		    palength = (uint16_t &)*(data + 2*sizeof(uint8_t));
		    shift = 4;
		}
	    else
		{
		    palength = (uint8_t &)*(data + 2*sizeof(uint8_t));
		    shift = 3;
		}
	    if (palength + shift > (uint)plength)
		xorp_throw(CorruptMessage,
			   c_format("inconsistent length %d %d",
				    palength + shift,
				    plength),
			   UPDATEMSGERR, ATTRLEN);

	    debug_msg("palength=%d\n", palength);

	    // get the path parameter type.
	    pa_type = (uint8_t &)*(data + sizeof(uint8_t));
	    switch (pa_type)
		{
		case ORIGIN:
		    {
			pa_temp = new OriginAttribute(data,palength+shift);
			break;
		    }
		case AS_PATH:
		    {
			pa_temp = new ASPathAttribute(data,palength+shift);
			break;
		    }
		case NEXT_HOP:
		    {
			pa_temp = new IPv4NextHopAttribute(data,palength+shift);
			break;
		    }
		case MED:
		    {
			pa_temp = new MEDAttribute(data,palength+shift);
			break;
		    }
		case LOCAL_PREF:
		    {
			pa_temp = new LocalPrefAttribute(data,palength+shift);
			break;
		    }
		case ATOMIC_AGGREGATE:
		    {
			pa_temp = new AtomicAggAttribute(data,palength+shift);
			break;
		    }
		case AGGREGATOR:
		    {
			pa_temp = new AggregatorAttribute(data,palength+shift);
			break;
		    }
		case COMMUNITY:
		    {
			pa_temp = new CommunityAttribute(data,palength+shift);
			break;
		    }
		default:
		    {
			//this will throw an error if the attribute isn't
			//optional transitive.
			pa_temp = new UnknownAttribute(data,palength+shift);
			break;
		    }
		}

		_att_list.push_back(pa_temp);
		debug_msg("plength %d, palength %d, shift %d, pathatt size %d\n",plength,palength,shift,_att_list.size());
		data = data + palength + shift;
		plength = plength - (palength + shift);
		debug_msg("plength %d, palength %d, shift %d, pathatt size %d\n",plength,palength,shift,_att_list.size());
		dump_bytes(data,10);
	}
    if (plength < 0)
	xorp_throw(CorruptMessage,
		   c_format("negative length %d", nlength),
		   UPDATEMSGERR, ATTRLEN);
    /* End of decoding of Path Attributes */
   
    /* Start of decoding of Network Reachability */
    size_t nnlength = 0;

    set<NetLayerReachability> nlri_set;
    assert(nlength >= 0);
    while (nlength > 0)
	{
	    
	    nnlength = (uint8_t &)(*data);
	    bytes = (nnlength+7)/8;
	    debug_msg("bits: %d bytes: %d\n", nnlength, bytes);
	    if ((bytes > (uint)nlength) || (bytes > 4))
		xorp_throw(CorruptMessage,
			   c_format("inconsistent length %d %d",
			    bytes, nlength),
		   UPDATEMSGERR, ATTRLEN);
	    data++;
	    nlength--;

	    NetLayerReachability nlri((uint32_t &)(*data), nnlength);
	    //check this isn't a duplicate NLRI
	    if (nlri_set.find(nlri) == nlri_set.end()) {
		nlri_set.insert(nlri);
		_nlri_list.push_back(nlri);
	    } else {
		XLOG_WARNING(("Received duplicate " + nlri.str() +
			   " in update message\n").c_str());
	    }
	    data += bytes;
	    nlength = nlength - bytes;
	}
    if (nlength < 0)
	xorp_throw(CorruptMessage,
		   c_format("negative length %d", nlength),
		   UPDATEMSGERR, ATTRLEN);
	
    /* End of decoding of Network Reachability */
	debug_msg("No of withdrawn routes %d. No of path attributes %d. No of networks %d.\n",
		  _withdrawn_list.size(), _att_list.size(),
		  _nlri_list.size());
}


string UpdatePacket::str() const
{
    string s = "Update Packet\n";
    debug_msg("No of withdrawn routes %d. No of path attributes %d. No of networks %d.\n",
	      _withdrawn_list.size(), _att_list.size(),
	      _nlri_list.size());

    list <BGPWithdrawnRoute>::const_iterator wi = _withdrawn_list.begin();
    while (wi != _withdrawn_list.end()) {
	s = s + " - " + wi->str() + "\n";
	++wi;
    }

    list <PathAttribute*>::const_iterator pai = _att_list.begin();
    while (pai != _att_list.end()) {
	s = s + " - " + (*pai)->str() + "\n";
	++pai;
    }
    
    list <NetLayerReachability>::const_iterator ni = _nlri_list.begin();
    while (ni != _nlri_list.end()) {
	s = s + " - " + ni->str() + "\n";
	++ni;
    }
    return s;
}

/*
** Helper function used when sorting Path Attribute lists.
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
    list <BGPWithdrawnRoute> temp_withdrawn_list(_withdrawn_list);
    temp_withdrawn_list.sort();
    list <BGPWithdrawnRoute> temp_withdrawn_list_him(him.withdrawn_list());
    temp_withdrawn_list_him.sort();

    if(temp_withdrawn_list.size() != temp_withdrawn_list_him.size())
	return false;

    list <BGPWithdrawnRoute>::const_iterator wi = temp_withdrawn_list.begin();
    list <BGPWithdrawnRoute>::const_iterator wi_him = 
	temp_withdrawn_list_him.begin();
    while (wi != temp_withdrawn_list.end() && 
	   wi_him != temp_withdrawn_list_him.end()) {
	
	if ( (*wi) == (*wi_him) )  {
	    ++wi;
	    ++wi_him;
	} else {
	    return false;
	}
    }

    //path attribute equals
    list <PathAttribute *> temp_att_list(_att_list);
    temp_att_list.sort(compare_path_attributes);
    list <PathAttribute *> temp_att_list_him(him.pathattribute_list());
    temp_att_list_him.sort(compare_path_attributes);

    if(temp_att_list.size() != temp_att_list_him.size())
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
    list <NetLayerReachability> temp_nlri_list(_nlri_list);
    temp_nlri_list.sort();
    list <NetLayerReachability> temp_nlri_list_him(him.nlri_list());
    temp_nlri_list_him.sort();

    if(temp_nlri_list.size() != temp_nlri_list_him.size())
	return false;

    list <NetLayerReachability>::const_iterator ni = temp_nlri_list.begin();
    list <NetLayerReachability>::const_iterator ni_him = 
	temp_nlri_list_him.begin();
    while (ni != temp_nlri_list.end() && ni_him != temp_nlri_list_him.end()) {
	
	if ( (*ni) == (*ni_him) )	    {
	    ++ni;
	    ++ni_him;
	} else {
	    return false;
	}
    }

    return true;
}
