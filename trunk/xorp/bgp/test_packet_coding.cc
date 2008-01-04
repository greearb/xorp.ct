// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/test_packet_coding.cc,v 1.20 2007/12/10 23:26:29 mjh Exp $"

#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/test_main.hh"

#include "libproto/packet.hh"

#include "bgp.hh"
#include "local_data.hh"
#include "packet.hh"
#include "path_attribute.hh"


bool
test_multprotocol(TestInfo& /*info*/, BGPPeerData* /*peerdata*/)
{
    BGPMultiProtocolCapability multi(AFI_IPV6, SAFI_UNICAST);

    multi.encode();
    assert(8 == multi.length());

    BGPMultiProtocolCapability recv(multi.length(), multi.data());

    assert(multi.length() == recv.length());

    assert(memcmp(multi.data(), recv.data(), recv.length()) == 0);

    return true;
}

bool
test_multiprotocol_reach_ipv4(TestInfo& /*info*/, BGPPeerData *peerdata)
{
    uint8_t buf[BGPPacket::MAXPACKETSIZE], buf2[BGPPacket::MAXPACKETSIZE];
    size_t enc_len = BGPPacket::MAXPACKETSIZE,  recv_len = BGPPacket::MAXPACKETSIZE;

    MPReachNLRIAttribute<IPv4> mpreach(SAFI_MULTICAST);

    assert(mpreach.optional());
    assert(!mpreach.transitive());

    assert(mpreach.encode(buf, enc_len, peerdata));
    assert(12 == enc_len);

    mpreach.set_nexthop(IPv4("128.16.0.0"));

    mpreach.add_nlri(IPNet<IPv4>("128.16.0.0/1"));
    mpreach.add_nlri(IPNet<IPv4>("128.16.0.0/2"));
    mpreach.add_nlri(IPNet<IPv4>("128.16.0.0/3"));
    mpreach.add_nlri(IPNet<IPv4>("128.16.0.0/4"));
    mpreach.add_nlri(IPNet<IPv4>("128.16.0.0/5"));
    mpreach.add_nlri(IPNet<IPv4>("128.16.0.0/6"));
    mpreach.add_nlri(IPNet<IPv4>("128.16.0.0/7"));
    mpreach.add_nlri(IPNet<IPv4>("128.16.0.0/8"));
    mpreach.add_nlri(IPNet<IPv4>("128.16.0.0/9"));
    mpreach.add_nlri(IPNet<IPv4>("128.16.0.0/10"));
    mpreach.add_nlri(IPNet<IPv4>("128.16.0.0/11"));
    mpreach.add_nlri(IPNet<IPv4>("128.16.0.0/12"));
    mpreach.add_nlri(IPNet<IPv4>("128.16.0.0/13"));
    mpreach.add_nlri(IPNet<IPv4>("128.16.0.0/14"));
    mpreach.add_nlri(IPNet<IPv4>("128.16.0.0/15"));
    mpreach.add_nlri(IPNet<IPv4>("128.16.0.0/16"));

    enc_len = BGPPacket::MAXPACKETSIZE;
    assert(mpreach.encode(buf, enc_len, peerdata));
    assert(52 == enc_len);

    MPReachNLRIAttribute<IPv4> recv(buf);
    // we need to re-encode to get the received data as a buffer
    recv.encode(buf2, recv_len, peerdata);

    assert(enc_len == recv_len);
    assert(memcmp(buf, buf2, recv_len) == 0);

    return true;
}

bool
test_multiprotocol_unreach(TestInfo& /*info*/, BGPPeerData* peerdata)
{
    MPUNReachNLRIAttribute<IPv6> mpunreach(SAFI_UNICAST);
    uint8_t buf[BGPPacket::MAXPACKETSIZE], buf2[BGPPacket::MAXPACKETSIZE];
    size_t enc_len = BGPPacket::MAXPACKETSIZE, recv_len = BGPPacket::MAXPACKETSIZE;

    assert(mpunreach.optional());
    assert(!mpunreach.transitive());

    assert(mpunreach.encode(buf, enc_len, peerdata));
    assert(6 == enc_len);

    mpunreach.add_withdrawn(IPNet<IPv6>("2000::/3"));
    enc_len = BGPPacket::MAXPACKETSIZE;
    mpunreach.encode(buf, enc_len, peerdata);

    assert(8 == enc_len);

    MPUNReachNLRIAttribute<IPv6> recv(buf);
    // we need to re-encode to get the received data as a buffer
    assert(recv.encode(buf2, recv_len, peerdata));

    assert(enc_len == recv_len);
    assert(memcmp(buf, buf2, recv_len) == 0);

    return true;
}

bool
test_refresh(TestInfo& /*info*/, BGPPeerData* /*peerdata*/)
{
    BGPRefreshCapability refresh;

    refresh.encode();
    assert(4 == refresh.length());

    BGPRefreshCapability recv(refresh.length(), refresh.data());

    assert(refresh.length() == recv.length());

    assert(memcmp(refresh.data(), recv.data(), recv.length()) == 0);

    return true;
}

bool
test_simple_open_packet(TestInfo& /*info*/, BGPPeerData* peerdata) 
{
    /* In this test we create an Open Packet, pretend to send it,
       pretend to receive it, and check that what we sent is what we
       received */
    OpenPacket openpacket(AsNum(666), IPv4("1.2.3.4"), 1234);
    uint8_t buf[BGPPacket::MAXPACKETSIZE];
    size_t len = BGPPacket::MAXPACKETSIZE;

    assert(openpacket.encode(buf, len, peerdata));

    //open packets with no parameters have a fixed length of 29 bytes
    assert(len == BGPPacket::MINOPENPACKET);

    //check the common header
    const uint8_t *skip = buf + BGPPacket::MARKER_SIZE;		// skip marker
    uint16_t plen = extract_16(skip);
    assert(plen == BGPPacket::MINOPENPACKET);
    skip += 2;
    uint8_t type = *skip;
    assert(type == MESSAGETYPEOPEN);
    skip++;

    OpenPacket receivedpacket(buf, plen);
    assert(receivedpacket.type()==MESSAGETYPEOPEN);

    //check the information we put in came out again OK.
    assert(receivedpacket.HoldTime() == 1234);
    assert(receivedpacket.as() == AsNum(666));
    assert(receivedpacket.id() == IPv4("1.2.3.4"));
    assert(receivedpacket.Version() == 4);

#ifdef	OPEN_PACKET_OPTIONS
    //check there are no parameters
    const list<const BGPParameter*>& pl = pd->parameter_list();
    assert(pl.begin() == pl.end());
    assert(pd->unsupported_parameters() == false);
#endif

    //try encoding the received packet, and check we get the same
    //encoded packet as when we encoded the constructed packet
    uint8_t buf2[BGPPacket::MAXPACKETSIZE];
    size_t len2 = BGPPacket::MAXPACKETSIZE;
    assert(receivedpacket.encode(buf2, len2, peerdata));
    assert(len == len2);
    assert(memcmp(buf, buf2, len2) == 0);
    return true;
}

bool
test_open_packet_with_capabilities(TestInfo& /*info*/, BGPPeerData* peerdata) 
{
    /* In this test we create an Open Packet, pretend to send it,
       pretend to receive it, and check that what we sent is what we
       received */
    OpenPacket openpacket(AsNum(666), IPv4("1.2.3.4"), 1234);
    // Add a multiprotocol parameter.
    openpacket.add_parameter(
	     new BGPMultiProtocolCapability(AFI_IPV6, SAFI_UNICAST));
    // Add a refresh parameter
    openpacket.add_parameter(
	     new BGPRefreshCapability());
    
    uint8_t buf[BGPPacket::MAXPACKETSIZE];
    size_t len = BGPPacket::MAXPACKETSIZE;

    assert(openpacket.encode(buf, len, peerdata));

    //open packets with no parameters have a fixed length of 29 bytes
    // +8 for the multiprotocol parameter.
    // +4 for the refresh parameter.
    assert(len == BGPPacket::MINOPENPACKET + 8 + 4);

    //check the common header
    const uint8_t *skip = buf + BGPPacket::MARKER_SIZE;		// skip marker
    uint16_t plen = extract_16(skip);
    // +8 for the multiprotocol parameter.
    // +4 for the refresh parameter.
    assert(plen == BGPPacket::MINOPENPACKET + 8 + 4);
    skip += 2;
    uint8_t type = *skip;
    assert(type == MESSAGETYPEOPEN);
    skip++;

    OpenPacket receivedpacket(buf, plen);
    assert(receivedpacket.type()==MESSAGETYPEOPEN);

    //check the information we put in came out again OK.
    assert(receivedpacket.HoldTime() == 1234);
    assert(receivedpacket.as() == AsNum(666));
    assert(receivedpacket.id() == IPv4("1.2.3.4"));
    assert(receivedpacket.Version() == 4);

    //try encoding the received packet, and check we get the same
    //encoded packet as when we encoded the constructed packet
    uint8_t buf2[BGPPacket::MAXPACKETSIZE];
    size_t len2 = BGPPacket::MAXPACKETSIZE;

    assert(receivedpacket.encode(buf2, len2, peerdata));

    assert(len == len2);
    assert(memcmp(buf, buf2, len2) == 0);
    return true;
}

bool
test_keepalive_packet(TestInfo& /*info*/, BGPPeerData* peerdata)
{
    /* In this test we create an Keepalive Packet, pretend to send it,
       pretend to receive it, and check that what we sent is what we
       received */
    KeepAlivePacket keepalivepacket;
    
    uint8_t buf[BGPPacket::MAXPACKETSIZE];
    size_t len = BGPPacket::MAXPACKETSIZE;

    assert(keepalivepacket.encode(buf, len, peerdata));

    //keepalive packets with no parameters have a fixed length of 19 bytes
    assert(len == BGPPacket::COMMON_HEADER_LEN);

    //check the common header
    const uint8_t *skip = buf + BGPPacket::MARKER_SIZE;		// skip marker
    uint16_t plen = extract_16(skip);
    assert(plen == BGPPacket::COMMON_HEADER_LEN);
    skip+=2;
    uint8_t type = *skip;
    assert(type == MESSAGETYPEKEEPALIVE);
    skip++;

    KeepAlivePacket receivedpacket(buf, plen);
    assert(receivedpacket.type()==MESSAGETYPEKEEPALIVE);

    //try encoding the received packet, and check we get the same
    //encoded packet as when we encoded the constructed packet
    uint8_t buf2[BGPPacket::MAXPACKETSIZE];
    size_t len2 = BGPPacket::MAXPACKETSIZE;
    assert(receivedpacket.encode(buf2, len2, peerdata));
    assert(len == len2);
    assert(memcmp(buf, buf2, len2) == 0);
    return true;
}

bool
test_notification_packets(TestInfo& info, const uint8_t *d, uint8_t ec, 
			      uint8_t esc, uint16_t l, BGPPeerData *peerdata) 
{
    DOUT(info) << "test_notification_packets\n";

    /* In this test we create a Notification Packet, pretend to send it,
       pretend to receive it, and check that what we sent is what we
       received */

    // We want to test all the constructors for NotificationPacket
    NotificationPacket *notificationpacket;
    notificationpacket = new NotificationPacket(ec, esc, d, l);
    
    uint8_t buf[BGPPacket::MAXPACKETSIZE];
    size_t len = BGPPacket::MAXPACKETSIZE;
    assert(notificationpacket->encode(buf, len, peerdata));

    //notification packets have a length of 21 bytes plus the length
    //of the error data
    if (d==NULL)
	assert(len == BGPPacket::MINNOTIFICATIONPACKET);
    else
	assert(len == BGPPacket::MINNOTIFICATIONPACKET + l);

    //check the common header
    const uint8_t *skip = buf + BGPPacket::MARKER_SIZE;
    uint16_t plen = extract_16(skip);
    if (d == NULL)
	assert(plen == BGPPacket::MINNOTIFICATIONPACKET);
    else
	assert(plen == BGPPacket::MINNOTIFICATIONPACKET + l);

    skip+=2;
    uint8_t type = *skip;
    assert(type == MESSAGETYPENOTIFICATION);
    skip++;

    NotificationPacket receivedpacket(buf, plen);
    assert(receivedpacket.type()==MESSAGETYPENOTIFICATION);
    assert(receivedpacket.error_code() == ec);
    assert(receivedpacket.error_subcode() == esc);
    if (d!=NULL)
	assert(memcmp(d, receivedpacket.error_data(), l)==0);

    //try encoding the received packet, and check we get the same
    //encoded packet as when we encoded the constructed packet
    uint8_t buf2[BGPPacket::MAXPACKETSIZE];
    size_t len2 = BGPPacket::MAXPACKETSIZE;
    assert(receivedpacket.encode(buf2, len2, peerdata));
    assert(len == len2);
    assert(memcmp(buf, buf2, len2) == 0);

    delete notificationpacket;
    return true;
}

bool
test_withdraw_packet(TestInfo& info, BGPPeerData* peerdata)
{
    /* In this test we create an Update Packet, pretend to send it,
       pretend to receive it, and check that what we sent is what we
       received */

    /* Here we'll create an update packet that only contains withdrawn
       routes */
    UpdatePacket updatepacket;
    IPv4Net n1("1.2.3.0/24");
    IPv4Net n2("1.2.4.0/24");
    BGPUpdateAttrib r1(n1);
    BGPUpdateAttrib r2(n2);
    updatepacket.add_withdrawn(r1);
    updatepacket.add_withdrawn(r2);
    
    uint8_t buf[BGPPacket::MAXPACKETSIZE];
    size_t len = BGPPacket::MAXPACKETSIZE;
    assert(updatepacket.encode(buf, len, peerdata));

    //update packets with have a minumum length of 23 bytes, plus all
    //the routing information
    assert(len == 31);

    //check the common header
    const uint8_t *skip = buf + BGPPacket::MARKER_SIZE;
    uint16_t plen = extract_16(skip);
    assert(plen == 31);
    skip+=2;
    uint8_t type = *skip;
    assert(type == MESSAGETYPEUPDATE);
    skip++;

    UpdatePacket receivedpacket(buf, plen, peerdata);
    assert(receivedpacket.type()==MESSAGETYPEUPDATE);
    BGPUpdateAttribList::const_iterator iter;
    iter = receivedpacket.wr_list().begin();
    DOUT(info) << "Withdrawn route: "
	       << iter->net().str().c_str() 
	       << " n1"  
	       <<  n1.str().c_str()
	       << endl;
    assert(iter->net() == n1);
    iter++;
    DOUT(info) << "Withdrawn route: " << iter->net().str().c_str() << endl;
    assert(iter->net() == n2);
    iter++;
    assert(iter == receivedpacket.wr_list().end());

    //try encoding the received packet, and check we get the same
    //encoded packet as when we encoded the constructed packet
    uint8_t buf2[BGPPacket::MAXPACKETSIZE];
    size_t len2 = BGPPacket::MAXPACKETSIZE;
    assert(receivedpacket.encode(buf2, len2, peerdata));

    assert(len == len2);
    assert(memcmp(buf, buf2, len2) == 0);

    return true;
}

bool
test_announce_packet1(TestInfo& info, BGPPeerData* peerdata)
{
    /* In this test we create an Update Packet, pretend to send it,
       pretend to receive it, and check that what we sent is what we
       received */

    /* Here we'll create an update packet that only contains announce
       routes */
    UpdatePacket updatepacket;
    IPv4Net n1("1.2.3.0/24");
    IPv4Net n2("1.2.4.0/24");
    BGPUpdateAttrib r1(n1);
    BGPUpdateAttrib r2(n2);
    updatepacket.add_nlri(r1);
    updatepacket.add_nlri(r2);

    NextHopAttribute<IPv4> nexthop_att(IPv4("10.0.0.1"));

    AsNum *as[13];
    int i;
    for (i=0;i<=9;i++) {
	as[i] = new AsNum(i);
    }
    ASSegment seq1 = ASSegment(AS_SEQUENCE);
    seq1.add_as(*(as[1]));
    seq1.add_as(*(as[2]));
    seq1.add_as(*(as[3]));

    ASSegment seq2 = ASSegment(AS_SEQUENCE);
    seq2.add_as(*(as[7]));
    seq2.add_as(*(as[8]));
    seq2.add_as(*(as[9]));

    ASSegment set1 = ASSegment(AS_SET);
    set1.add_as(*(as[4]));
    set1.add_as(*(as[5]));
    set1.add_as(*(as[6]));

    ASPath aspath;
    aspath.add_segment(seq1);
    aspath.add_segment(set1);
    aspath.add_segment(seq2);
    ASPathAttribute aspath_att(aspath);

    OriginAttribute origin_att(IGP);

    PathAttributeList<IPv4> palist(nexthop_att, aspath_att, origin_att);

    LocalPrefAttribute local_pref_att(237);
    palist.add_path_attribute(local_pref_att);
    
    MEDAttribute med_att(515);
    palist.add_path_attribute(med_att);

    AtomicAggAttribute atomic_agg_att;
    palist.add_path_attribute(atomic_agg_att);

    AggregatorAttribute agg_att(IPv4("20.20.20.2"), AsNum(701));
    palist.add_path_attribute(agg_att);

    CommunityAttribute com_att;
    com_att.add_community(57);
    com_att.add_community(58);
    com_att.add_community(59);
    palist.add_path_attribute(com_att);
    palist.rehash();

    const PathAttribute *pa;
    PathAttributeList<IPv4>::const_iterator iter;
    iter = palist.begin();
    while(iter !=  palist.end()) {
	pa = *iter;
	updatepacket.add_pathatt(*pa);
	++iter;
    }
    
    uint8_t buf[BGPPacket::MAXPACKETSIZE];
    size_t len = BGPPacket::MAXPACKETSIZE;
    assert(updatepacket.encode(buf, len, peerdata));

    //update packets with have a minumum length of 23 bytes, plus all
    //the routing information

    //23 bytes, plus 8 bytes of NLRI, 
    //+ 4 bytes origin,
    //+ 7 bytes nexthop 
    //+ (3+3*(2+(2*3))) = 27 bytes Aspath
    //+ 7 bytes local pref
    //+ 7 bytes MED
    //+ 3 bytes Atomic Aggregate
    //+ 9 bytes Aggregator
    //+ 15 bytes Community
    DOUT(info) << "len == " << len << endl;
    assert(len == 110);

    //check the common header
    const uint8_t *skip = buf + BGPPacket::MARKER_SIZE;
    uint16_t plen = extract_16(skip);
    assert(plen == 110);
    skip+=2;
    uint8_t type = *skip;
    assert(type == MESSAGETYPEUPDATE);
    skip++;

    UpdatePacket receivedpacket(buf, plen, peerdata);
    assert(receivedpacket.type()==MESSAGETYPEUPDATE);

    //check there are no withdrawn routes
    assert(receivedpacket.wr_list().empty());

    //check the NLRI
    BGPUpdateAttribList::const_iterator ni;
    ni = receivedpacket.nlri_list().begin();
    DOUT(info) << "NLRI: " << ni->net().str().c_str() << endl;
    assert(ni->net() == n1);
    ni++;
    DOUT(info) << "NLRI: " << ni->net().str().c_str() << endl;
    assert(ni->net() == n2);
    ni++;
    assert(ni == receivedpacket.nlri_list().end());

    //check the path attributes
    assert(receivedpacket.pa_list().size() == 8);
    list <PathAttribute*>::const_iterator pai;
    pai = receivedpacket.pa_list().begin();
    while (pai != receivedpacket.pa_list().end()) {
	pa = *pai;
	switch (pa->type()) {
	case ORIGIN: {
	    const OriginAttribute *oa = (const OriginAttribute *)pa;
	    assert(oa->origin() == IGP);
	    DOUT(info) << pa->str().c_str() << endl;
	    break;
	}
	case AS_PATH: {
	    const ASPathAttribute *asa = (const ASPathAttribute *)pa;
	    for (int i=1; i<=9; i++)
		assert(asa->as_path().contains(*as[i]));
	    DOUT(info) << pa->str().c_str() << endl;
	    break;
	}
	case NEXT_HOP: {
	    const NextHopAttribute<IPv4> *nha 
		= (const NextHopAttribute<IPv4> *)pa;
	    assert(nha->nexthop() == IPv4("10.0.0.1"));
	    DOUT(info) << pa->str().c_str() << endl;
	    break;
	}
	case LOCAL_PREF: {
	    const LocalPrefAttribute *lpa = 
		(const LocalPrefAttribute *)pa;
	    assert(lpa->localpref() == 237);
	    DOUT(info) << pa->str().c_str() << endl;
	    break;
	}
	case MED: {
	    const MEDAttribute *meda = 
		(const MEDAttribute *)pa;
	    assert(meda->med() == 515);
	    DOUT(info) << pa->str().c_str() << endl;
	    break;
	}
	case ATOMIC_AGGREGATE: {
	    DOUT(info) << pa->str().c_str() << endl;
	    break;
	}
	case AGGREGATOR: {
	    const AggregatorAttribute *aa = 
		(const AggregatorAttribute *)pa;
	    assert(aa->route_aggregator() == IPv4("20.20.20.2"));
	    assert(aa->aggregator_as() == AsNum(701));
	    DOUT(info) << pa->str().c_str() << endl;
	    break;
	}
	case COMMUNITY: {
	    const CommunityAttribute *ca =
		(const CommunityAttribute*)pa;
	    set <uint32_t>::const_iterator iter;
	    iter = ca->community_set().begin();
	    assert(*iter == 57);
	    ++iter;
	    assert(*iter == 58);
	    ++iter;
	    assert(*iter == 59);
	    ++iter;
	    assert(iter == ca->community_set().end());
	    DOUT(info) << pa->str().c_str() << endl;
	    break;
	}
	default:
	    XLOG_FATAL("Received PathAttribute type: %d\n", pa->type());
	}
	++pai;
    }

    //try encoding the received packet, and check we get the same
    //encoded packet as when we encoded the constructed packet
    uint8_t buf2[BGPPacket::MAXPACKETSIZE];
    size_t len2 = BGPPacket::MAXPACKETSIZE;
    assert(receivedpacket.encode(buf2, len2, peerdata));

    assert(len == len2);
    assert(memcmp(buf, buf2, len2) == 0);

    // clean up
    for (i=0;i<=9;i++) {
	delete as[i];
    }
    return true;
}

bool
test_announce_packet2(TestInfo& info, BGPPeerData* peerdata)
{
    /* In this test we create an Update Packet, pretend to send it,
       pretend to receive it, and check that what we sent is what we
       received */

    /* Here we'll create an update packet that only contains announce
       routes */
    UpdatePacket updatepacket;
    IPv4Net n1("1.2.3.0/24");
    IPv4Net n2("0.0.0.0/0");	/* This used to trigger a bug in the decoder */
    BGPUpdateAttrib r1(n1);
    BGPUpdateAttrib r2(n2);
    updatepacket.add_nlri(r1);
    updatepacket.add_nlri(r2);

    NextHopAttribute<IPv4> nexthop_att(IPv4("10.0.0.1"));

    AsNum *as[13];
    int i;
    for (i=0;i<=9;i++) {
	as[i] = new AsNum(i);
    }
    ASSegment seq1 = ASSegment(AS_SEQUENCE);
    seq1.add_as(*(as[1]));
    seq1.add_as(*(as[2]));
    seq1.add_as(*(as[3]));

    ASSegment seq2 = ASSegment(AS_SEQUENCE);
    seq2.add_as(*(as[7]));
    seq2.add_as(*(as[8]));
    seq2.add_as(*(as[9]));

    ASSegment set1 = ASSegment(AS_SET);
    set1.add_as(*(as[4]));
    set1.add_as(*(as[5]));
    set1.add_as(*(as[6]));

    ASPath aspath;
    aspath.add_segment(seq1);
    aspath.add_segment(set1);
    aspath.add_segment(seq2);
    ASPathAttribute aspath_att(aspath);

    OriginAttribute origin_att(IGP);

    PathAttributeList<IPv4> palist(nexthop_att, aspath_att, origin_att);

    LocalPrefAttribute local_pref_att(237);
    palist.add_path_attribute(local_pref_att);
    
    MEDAttribute med_att(515);
    palist.add_path_attribute(med_att);

    AtomicAggAttribute atomic_agg_att;
    palist.add_path_attribute(atomic_agg_att);

    AggregatorAttribute agg_att(IPv4("20.20.20.2"), AsNum(701));
    palist.add_path_attribute(agg_att);

    CommunityAttribute com_att;
    com_att.add_community(57);
    com_att.add_community(58);
    com_att.add_community(59);
    palist.add_path_attribute(com_att);
    palist.rehash();

    const PathAttribute *pa;
    PathAttributeList<IPv4>::const_iterator iter;
    iter = palist.begin();
    while(iter !=  palist.end()) {
	pa = *iter;
	updatepacket.add_pathatt(*pa);
	++iter;
    }
    
    uint8_t buf[BGPPacket::MAXPACKETSIZE];
    size_t len = BGPPacket::MAXPACKETSIZE;
    assert(updatepacket.encode(buf, len, peerdata));

    //update packets with have a minumum length of 23 bytes, plus all
    //the routing information

    //23 bytes, plus 5 bytes of NLRI, 
    //+ 4 bytes origin,
    //+ 7 bytes nexthop 
    //+ (3+3*(2+(2*3))) = 27 bytes Aspath
    //+ 7 bytes local pref
    //+ 7 bytes MED
    //+ 3 bytes Atomic Aggregate
    //+ 9 bytes Aggregator
    //+ 15 bytes Community
    DOUT(info) << "len == " << len << endl;
    assert(len == 107);

    //check the common header
    const uint8_t *skip = buf + BGPPacket::MARKER_SIZE;
    uint16_t plen = extract_16(skip);
    assert(plen == 107);
    skip+=2;
    uint8_t type = *skip;
    assert(type == MESSAGETYPEUPDATE);
    skip++;

    UpdatePacket receivedpacket(buf, plen, peerdata);
    assert(receivedpacket.type()==MESSAGETYPEUPDATE);

    //check there are no withdrawn routes
    assert(receivedpacket.wr_list().empty());

    //check the NLRI
    BGPUpdateAttribList::const_iterator ni;
    ni = receivedpacket.nlri_list().begin();
    DOUT(info) << "NLRI: " << ni->net().str().c_str() << endl;
    assert(ni->net() == n1);
    ni++;
    DOUT(info) << "NLRI: " << ni->net().str().c_str() << endl;
    assert(ni->net() == n2);
    ni++;
    assert(ni == receivedpacket.nlri_list().end());

    //check the path attributes
    assert(receivedpacket.pa_list().size() == 8);
    list <PathAttribute*>::const_iterator pai;
    pai = receivedpacket.pa_list().begin();
    while (pai != receivedpacket.pa_list().end()) {
	pa = *pai;
	switch (pa->type()) {
	case ORIGIN: {
	    const OriginAttribute *oa = (const OriginAttribute *)pa;
	    assert(oa->origin() == IGP);
	    DOUT(info) << pa->str().c_str() << endl;
	    break;
	}
	case AS_PATH: {
	    const ASPathAttribute *asa = (const ASPathAttribute *)pa;
	    for (int i=1; i<=9; i++)
		assert(asa->as_path().contains(*as[i]));
	    DOUT(info) << pa->str().c_str() << endl;
	    break;
	}
	case NEXT_HOP: {
	    const NextHopAttribute<IPv4> *nha 
		= (const NextHopAttribute<IPv4> *)pa;
	    assert(nha->nexthop() == IPv4("10.0.0.1"));
	    DOUT(info) << pa->str().c_str() << endl;
	    break;
	}
	case LOCAL_PREF: {
	    const LocalPrefAttribute *lpa = 
		(const LocalPrefAttribute *)pa;
	    assert(lpa->localpref() == 237);
	    DOUT(info) << pa->str().c_str() << endl;
	    break;
	}
	case MED: {
	    const MEDAttribute *meda = 
		(const MEDAttribute *)pa;
	    assert(meda->med() == 515);
	    DOUT(info) << pa->str().c_str() << endl;
	    break;
	}
	case ATOMIC_AGGREGATE: {
	    DOUT(info) << pa->str().c_str() << endl;
	    break;
	}
	case AGGREGATOR: {
	    const AggregatorAttribute *aa = 
		(const AggregatorAttribute *)pa;
	    assert(aa->route_aggregator() == IPv4("20.20.20.2"));
	    assert(aa->aggregator_as() == AsNum(701));
	    DOUT(info) << pa->str().c_str() << endl;
	    break;
	}
	case COMMUNITY: {
	    const CommunityAttribute *ca =
		(const CommunityAttribute*)pa;
	    set <uint32_t>::const_iterator iter;
	    iter = ca->community_set().begin();
	    assert(*iter == 57);
	    ++iter;
	    assert(*iter == 58);
	    ++iter;
	    assert(*iter == 59);
	    ++iter;
	    assert(iter == ca->community_set().end());
	    DOUT(info) << pa->str().c_str() << endl;
	    break;
	}
	default:
	    XLOG_FATAL("Received PathAttribute type: %d\n", pa->type());
	}
	++pai;
    }

    //try encoding the received packet, and check we get the same
    //encoded packet as when we encoded the constructed packet
    uint8_t buf2[BGPPacket::MAXPACKETSIZE];
    size_t len2 = BGPPacket::MAXPACKETSIZE;
    assert(receivedpacket.encode(buf2, len2, peerdata));
    assert(len == len2);
    assert(memcmp(buf, buf2, len2) == 0);

    // clean up
    for (i=0;i<=9;i++) {
	delete as[i];
    }
    return true;
}

int
main(int argc, char** argv) 
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    TestMain t(argc, argv);

    EventLoop eventloop;
    LocalData localdata(eventloop);
    Iptuple iptuple;
    BGPPeerData pd(localdata, iptuple, AsNum(0), IPv4(),0);

    string test_name =
	t.get_optional_args("-t", "--test", "run only the specified test");
    t.complete_args_parsing();

    try {
	uint8_t edata[2];
	edata[0]=1;
	edata[1]=2;

	struct test {
	    string test_name;
	    XorpCallback1<bool, TestInfo&>::RefPtr cb;
	} tests[] = {
	    {"multiprotocol", callback(test_multprotocol, &pd)},
	    {"multiprotocol_reach_ipv4",
	     callback(test_multiprotocol_reach_ipv4, &pd)},
	    {"multiprotocol_unreach", callback(test_multiprotocol_unreach, &pd)},
	    {"refresh", callback(test_refresh, &pd)},
	    {"simple_open_packet", callback(test_simple_open_packet, &pd)},
	    {"open_packet", callback(test_open_packet_with_capabilities, &pd)},
	    {"keepalive_packet", callback(test_keepalive_packet, &pd)},
	    {"notification_packets1",
	     callback(test_notification_packets,
		      reinterpret_cast<const uint8_t *>(0),
 		      static_cast<uint8_t>(CEASE),
		      static_cast<uint8_t>(0),
		      static_cast<uint16_t>(0),
		      &pd)},
	    {"notification_packets2",
	     callback(test_notification_packets,
		      reinterpret_cast<const uint8_t *>(0),
 		      static_cast<uint8_t>(CEASE),
		      static_cast<uint8_t>(1),
		      static_cast<uint16_t>(0),
		      &pd)},
 	    {"notification_packets3",
 	     callback(test_notification_packets,
		      reinterpret_cast<const uint8_t *>(edata),
 		      static_cast<uint8_t>(MSGHEADERERR),
		      static_cast<uint8_t>(2),
		      static_cast<uint16_t>(2),
		      &pd)},
	    {"withdraw_packet", callback(test_withdraw_packet, &pd)},
	    {"announce_packet1", callback(test_announce_packet1, &pd)},
	    {"announce_packet2", callback(test_announce_packet2, &pd)},
	};

	if("" == test_name) {
	    for(unsigned int i = 0; i < sizeof(tests) / sizeof(struct test); 
		i++)
		t.run(tests[i].test_name, tests[i].cb);
	} else {
	    for(unsigned int i = 0; i < sizeof(tests) / sizeof(struct test); 
		i++)
		if(test_name == tests[i].test_name) {
		    t.run(tests[i].test_name, tests[i].cb);
		    return t.exit();
		}
	    t.failed("No test with name " + test_name + " found\n");
	}
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    xlog_stop();
    xlog_exit();

    return t.exit();
}
