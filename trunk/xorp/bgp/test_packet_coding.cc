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

#ident "$XORP: xorp/bgp/test_packet_coding.cc,v 1.2 2003/08/28 01:05:01 atanu Exp $"

#include "libxorp/xorp.h"
#include "packet.hh"
#include "path_attribute.hh"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/test_main.hh"

bool
test_multprotocol(TestInfo& /*info*/)
{
    BGPMultiProtocolCapability multi(AFI_IPV6, SAFI_NLRI_UNICAST);

    multi.encode();
    assert(8 == multi.length());

    BGPMultiProtocolCapability recv(multi.length(), multi.data());

    assert(multi.length() == recv.length());

    assert(memcmp(multi.data(), recv.data(), recv.length()) == 0);

    return true;
}

bool
test_simple_open_packet(TestInfo& /*info*/) 
{
    /* In this test we create an Open Packet, pretend to send it,
       pretend to receive it, and check that what we sent is what we
       received */
    OpenPacket openpacket(AsNum(666), IPv4("1.2.3.4"), 1234);

    const uint8_t *buf;
    size_t len;
    buf = openpacket.encode(len);

    //open packets with no parameters have a fixed length of 29 bytes
    assert(len == MINOPENPACKET);

    //check the common header
    const uint8_t *skip = buf+MARKER_SIZE;	// skip marker
    uint16_t plen = htons(*((const uint16_t*)skip));
    assert(plen == MINOPENPACKET);
    skip+=2;
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
    const uint8_t *buf2;
    size_t len2;
    buf2 = receivedpacket.encode(len2);
    assert(len == len2);
    assert(memcmp(buf, buf2, len2) == 0);
    delete[] buf;
    delete[] buf2;
    return true;
}

bool
test_open_packet_with_capabilities(TestInfo& /*info*/) 
{
    /* In this test we create an Open Packet, pretend to send it,
       pretend to receive it, and check that what we sent is what we
       received */
    OpenPacket openpacket(AsNum(666), IPv4("1.2.3.4"), 1234);
    // Add a multiprotocol parameter.
    openpacket.add_parameter(
	     new BGPMultiProtocolCapability(AFI_IPV6, SAFI_NLRI_UNICAST));

    const uint8_t *buf;
    size_t len;
    buf = openpacket.encode(len);

    //open packets with no parameters have a fixed length of 29 bytes
    // +8 for the multiprotocol parameter.
    assert(len == MINOPENPACKET + 8);

    //check the common header
    const uint8_t *skip = buf+MARKER_SIZE;	// skip marker
    uint16_t plen = htons(*((const uint16_t*)skip));
    // +8 for the multiprotocol parameter.
    assert(plen == MINOPENPACKET + 8);
    skip+=2;
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
    const uint8_t *buf2;
    size_t len2;
    buf2 = receivedpacket.encode(len2);
    assert(len == len2);
    assert(memcmp(buf, buf2, len2) == 0);
    delete[] buf;
    delete[] buf2;
    return true;
}

bool
test_keepalive_packet(TestInfo& /*info*/)
{
    /* In this test we create an Keepalive Packet, pretend to send it,
       pretend to receive it, and check that what we sent is what we
       received */
    KeepAlivePacket keepalivepacket;
    
    const uint8_t *buf;
    size_t len;
    buf = keepalivepacket.encode(len);

    //keepalive packets with no parameters have a fixed length of 19 bytes
    assert(len == BGP_COMMON_HEADER_LEN);

    //check the common header
    const uint8_t *skip = buf+MARKER_SIZE;	// skip marker
    uint16_t plen = htons(*((const uint16_t*)skip));
    assert(plen == BGP_COMMON_HEADER_LEN);
    skip+=2;
    uint8_t type = *skip;
    assert(type == MESSAGETYPEKEEPALIVE);
    skip++;

    KeepAlivePacket receivedpacket(buf, plen);
    assert(receivedpacket.type()==MESSAGETYPEKEEPALIVE);

    //try encoding the received packet, and check we get the same
    //encoded packet as when we encoded the constructed packet
    const uint8_t *buf2;
    size_t len2;
    buf2 = receivedpacket.encode(len2);
    assert(len == len2);
    assert(memcmp(buf, buf2, len2) == 0);
    delete[] buf;
    delete[] buf2;
    return true;
}

bool
test_notification_packets(TestInfo& info, const uint8_t *d, uint8_t ec, 
			      uint8_t esc, uint16_t l) 
{
    DOUT(info) << "test_notification_packets\n";

    /* In this test we create a Notification Packet, pretend to send it,
       pretend to receive it, and check that what we sent is what we
       received */

    // We want to test all the constructors for NotificationPacket
    NotificationPacket *notificationpacket;
    notificationpacket = new NotificationPacket(ec, esc, d, l);
    
    const uint8_t *buf;
    size_t len;
    buf = notificationpacket->encode(len);

    //notification packets have a length of 21 bytes plus the length
    //of the error data
    if (d==NULL)
	assert(len == MINNOTIFICATIONPACKET);
    else
	assert(len == MINNOTIFICATIONPACKET + l);

    //check the common header
    const uint8_t *skip = buf+MARKER_SIZE;
    uint16_t plen = htons(*((const uint16_t*)skip));
    if (d==NULL)
	assert(plen == MINNOTIFICATIONPACKET);
    else
	assert(plen == MINNOTIFICATIONPACKET + l);

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
    const uint8_t *buf2;
    size_t len2;
    buf2 = receivedpacket.encode(len2);
    assert(len == len2);
    assert(memcmp(buf, buf2, len2) == 0);

    delete notificationpacket;
    delete[] buf;
    delete[] buf2;
    return true;
}

bool
test_withdraw_packet(TestInfo& info)
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
    
    const uint8_t *buf;
    size_t len;
    buf = updatepacket.encode(len);

    //update packets with have a minumum length of 23 bytes, plus all
    //the routing information
    assert(len == 31);

    //check the common header
    const uint8_t *skip = buf+MARKER_SIZE;
    uint16_t plen = htons(*((const uint16_t*)skip));
    assert(plen == 31);
    skip+=2;
    uint8_t type = *skip;
    assert(type == MESSAGETYPEUPDATE);
    skip++;

    UpdatePacket receivedpacket(buf, plen);
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
    const uint8_t *buf2;
    size_t len2;
    buf2 = receivedpacket.encode(len2);
    assert(len == len2);
    assert(memcmp(buf, buf2, len2) == 0);
    delete[] buf;
    delete[] buf2;
    return true;
}

bool
test_announce_packet(TestInfo& info)
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
    updatepacket.add_nlri(r1);
    updatepacket.add_nlri(r2);

    NextHopAttribute<IPv4> nexthop_att(IPv4("10.0.0.1"));

    AsNum *as[13];
    int i;
    for (i=0;i<=9;i++) {
	as[i] = new AsNum(i);
    }
    AsSegment seq1 = AsSegment(AS_SEQUENCE);
    seq1.add_as(*(as[1]));
    seq1.add_as(*(as[2]));
    seq1.add_as(*(as[3]));

    AsSegment seq2 = AsSegment(AS_SEQUENCE);
    seq2.add_as(*(as[7]));
    seq2.add_as(*(as[8]));
    seq2.add_as(*(as[9]));

    AsSegment set1 = AsSegment(AS_SET);
    set1.add_as(*(as[4]));
    set1.add_as(*(as[5]));
    set1.add_as(*(as[6]));

    AsPath aspath;
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
    
    const uint8_t *buf;
    size_t len;
    buf = updatepacket.encode(len);

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
    const uint8_t *skip = buf+MARKER_SIZE;
    uint16_t plen = htons(*((const uint16_t*)skip));
    assert(plen == 110);
    skip+=2;
    uint8_t type = *skip;
    assert(type == MESSAGETYPEUPDATE);
    skip++;

    UpdatePacket receivedpacket(buf, plen);
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
    const uint8_t *buf2;
    size_t len2;
    buf2 = receivedpacket.encode(len2);
    assert(len == len2);
    assert(memcmp(buf, buf2, len2) == 0);

    // clean up
    delete[] buf;
    delete[] buf2;
    for (i=0;i<=9;i++) {
	delete as[i];
    }
    return true;
}

int
main(int argc, char** argv) 
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    TestMain t(argc, argv);

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
	    {"multiprotocol", callback(test_multprotocol)},
	    {"simple_open_packet", callback(test_simple_open_packet)},
	    {"open_packet", callback(test_open_packet_with_capabilities)},
	    {"keepalive_packet", callback(test_keepalive_packet)},
	    {"notification_packets1",
	     callback(test_notification_packets,
		      reinterpret_cast<const uint8_t *>(0),
 		      static_cast<uint8_t>(CEASE),
		      static_cast<uint8_t>(0),
		      static_cast<uint16_t>(0))},
	    {"notification_packets2",
	     callback(test_notification_packets,
		      reinterpret_cast<const uint8_t *>(0),
 		      static_cast<uint8_t>(CEASE),
		      static_cast<uint8_t>(1),
		      static_cast<uint16_t>(0))},
 	    {"notification_packets3",
 	     callback(test_notification_packets,
		      reinterpret_cast<const uint8_t *>(edata),
 		      static_cast<uint8_t>(MSGHEADERERR),
		      static_cast<uint8_t>(2),
		      static_cast<uint16_t>(2))},
	    {"withdraw_packet", callback(test_withdraw_packet)},
	    {"announce_packet", callback(test_announce_packet)},
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

    return t.exit();
}
