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

#ident "$XORP: xorp/bgp/packet_coding_test.cc,v 1.1.1.1 2002/12/11 23:55:49 hodson Exp $"

#include "packet.hh"
#include "path_attribute_list.hh"

int test_simple_open_packet() {
    /* In this test we create an Open Packet, pretend to send it,
       pretend to receive it, and check that what we sent is what we
       received */
    OpenPacket openpacket(static_cast<uint16_t>(666),
			     IPv4("1.2.3.4"),
			     1234);
    
    const uint8_t *buf;
    int len;
    buf = openpacket.encode(len);

    //open packets with no parameters have a fixed length of 29 bytes
    assert(len == 29);

    //check the common header
    const uint8_t *skip = buf+16;
    uint16_t plen = htons(*((const uint16_t*)skip));
    assert(plen == 29);
    skip+=2;
    uint8_t type = *skip;
    assert(type == MESSAGETYPEOPEN);
    skip++;

    OpenPacket receivedpacket(buf, plen);
    receivedpacket.decode();
    assert(receivedpacket.get_type()==MESSAGETYPEOPEN);

    //check the information we put in came out again OK.
    assert(receivedpacket.HoldTime() == 1234);
    assert(receivedpacket.AutonomousSystemNumber() == AsNum((uint16_t)666));
    assert(receivedpacket.BGPIdentifier() == IPv4("1.2.3.4"));
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
    int len2;
    buf2 = receivedpacket.encode(len2);
    assert(len == len2);
    assert(memcmp(buf, buf2, len2) == 0);
    return 0;
}

int test_keepalive_packet() {
    /* In this test we create an Keepalive Packet, pretend to send it,
       pretend to receive it, and check that what we sent is what we
       received */
    KeepAlivePacket keepalivepacket;
    
    const uint8_t *buf;
    int len;
    buf = keepalivepacket.encode(len);

    //keepalive packets with no parameters have a fixed length of 19 bytes
    assert(len == 19);

    //check the common header
    const uint8_t *skip = buf+16;
    uint16_t plen = htons(*((const uint16_t*)skip));
    assert(plen == 19);
    skip+=2;
    uint8_t type = *skip;
    assert(type == MESSAGETYPEKEEPALIVE);
    skip++;

    KeepAlivePacket receivedpacket(buf, plen);
    receivedpacket.decode();
    assert(receivedpacket.get_type()==MESSAGETYPEKEEPALIVE);

    //try encoding the received packet, and check we get the same
    //encoded packet as when we encoded the constructed packet
    const uint8_t *buf2;
    int len2;
    buf2 = receivedpacket.encode(len2);
    assert(len == len2);
    assert(memcmp(buf, buf2, len2) == 0);
    return 0;
}

int test_notification_packets(const uint8_t *d, uint8_t ec, 
			      uint8_t esc, uint16_t l) {
    /* In this test we create a Notification Packet, pretend to send it,
       pretend to receive it, and check that what we sent is what we
       received */

    // We want to test all the constructors for NotificationPacket
    NotificationPacket *notificationpacket;
    if (esc == 0 && d == NULL) 
	notificationpacket = new NotificationPacket(ec);
    else if (d==NULL)
	notificationpacket = new NotificationPacket(ec, esc);
    else
	notificationpacket = new NotificationPacket(d, ec, esc, l);
    
    const uint8_t *buf;
    int len;
    buf = notificationpacket->encode(len);

    //notification packets have a length of 21 bytes plus the length
    //of the error data
    if (d==NULL)
	assert(len == 21);
    else
	assert(len == 21 + l);

    //check the common header
    const uint8_t *skip = buf+16;
    uint16_t plen = htons(*((const uint16_t*)skip));
    if (d==NULL)
	assert(plen == 21);
    else
	assert(plen == 21 + l);

    skip+=2;
    uint8_t type = *skip;
    assert(type == MESSAGETYPENOTIFICATION);
    skip++;

    NotificationPacket receivedpacket(buf, plen);
    receivedpacket.decode();
    assert(receivedpacket.get_type()==MESSAGETYPENOTIFICATION);
    assert(receivedpacket.error_code() == ec);
    assert(receivedpacket.error_subcode() == esc);
    if (d!=NULL)
	assert(memcmp(d, receivedpacket.error_data(), l)==0);

    //try encoding the received packet, and check we get the same
    //encoded packet as when we encoded the constructed packet
    const uint8_t *buf2;
    int len2;
    buf2 = receivedpacket.encode(len2);
    assert(len == len2);
    assert(memcmp(buf, buf2, len2) == 0);
    return 0;
}

int test_withdraw_packet(bool verbose) {
    /* In this test we create an Update Packet, pretend to send it,
       pretend to receive it, and check that what we sent is what we
       received */

    /* Here we'll create an update packet that only contains withdrawn
       routes */
    UpdatePacket updatepacket;
    IPv4Net n1("1.2.3.0/24");
    IPv4Net n2("1.2.4.0/24");
    BGPWithdrawnRoute r1(n1);
    BGPWithdrawnRoute r2(n2);
    updatepacket.add_withdrawn(r1);
    updatepacket.add_withdrawn(r2);
    
    const uint8_t *buf;
    int len;
    buf = updatepacket.encode(len);

    //update packets with have a minumum length of 23 bytes, plus all
    //the routing information
    assert(len == 31);

    //check the common header
    const uint8_t *skip = buf+16;
    uint16_t plen = htons(*((const uint16_t*)skip));
    assert(plen == 31);
    skip+=2;
    uint8_t type = *skip;
    assert(type == MESSAGETYPEUPDATE);
    skip++;

    UpdatePacket receivedpacket(buf, plen);
    receivedpacket.decode();
    assert(receivedpacket.get_type()==MESSAGETYPEUPDATE);
    list <BGPWithdrawnRoute>::const_iterator iter;
    iter = receivedpacket.withdrawn_list().begin();
    if (verbose)
	printf("Withdrawn route: %s\n", iter->net().str().c_str());
    assert(iter->net() == n1);
    iter++;
    if (verbose)
	printf("Withdrawn route: %s\n", iter->net().str().c_str());
    assert(iter->net() == n2);
    iter++;
    assert(iter == receivedpacket.withdrawn_list().end());

    //try encoding the received packet, and check we get the same
    //encoded packet as when we encoded the constructed packet
    const uint8_t *buf2;
    int len2;
    buf2 = receivedpacket.encode(len2);
    assert(len == len2);
    assert(memcmp(buf, buf2, len2) == 0);
    return 0;
}

int test_announce_packet(bool verbose) {
    /* In this test we create an Update Packet, pretend to send it,
       pretend to receive it, and check that what we sent is what we
       received */

    /* Here we'll create an update packet that only contains withdrawn
       routes */
    UpdatePacket updatepacket;
    IPv4Net n1("1.2.3.0/24");
    IPv4Net n2("1.2.4.0/24");
    NetLayerReachability r1(n1);
    NetLayerReachability r2(n2);
    updatepacket.add_nlri(r1);
    updatepacket.add_nlri(r2);

    NextHopAttribute<IPv4> nexthop_att(IPv4("10.0.0.1"));

    AsNum *as[13];
    int i;
    for (i=0;i<=9;i++) {
	as[i] = new AsNum((uint16_t)i);
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

    AggregatorAttribute agg_att(IPv4("20.20.20.2"), AsNum((uint16_t)701));
    palist.add_path_attribute(agg_att);

    CommunityAttribute com_att;
    com_att.add_community(57);
    com_att.add_community(58);
    com_att.add_community(59);
    palist.add_path_attribute(com_att);
    palist.rehash();

    const PathAttribute *pa;
    list <const PathAttribute*>::const_iterator iter;
    iter = palist.att_list().begin();
    while(iter !=  palist.att_list().end()) {
	pa = *iter;
	updatepacket.add_pathatt(*pa);
	++iter;
    }
    
    const uint8_t *buf;
    int len;
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
    if (verbose)
	printf("len == %d\n", len);
    assert(len == 110);

    //check the common header
    const uint8_t *skip = buf+16;
    uint16_t plen = htons(*((const uint16_t*)skip));
    assert(plen == 110);
    skip+=2;
    uint8_t type = *skip;
    assert(type == MESSAGETYPEUPDATE);
    skip++;

    UpdatePacket receivedpacket(buf, plen);
    receivedpacket.decode();
    assert(receivedpacket.get_type()==MESSAGETYPEUPDATE);

    //check there are no withdrawn routes
    assert(receivedpacket.withdrawn_list().begin()
	   == receivedpacket.withdrawn_list().end());

    //check the NLRI
    list <NetLayerReachability>::const_iterator ni;
    ni = receivedpacket.nlri_list().begin();
    if (verbose)
	printf("NLRI: %s\n", ni->net().str().c_str());
    assert(ni->net() == n1);
    ni++;
    if (verbose)
	printf("NLRI: %s\n", ni->net().str().c_str());
    assert(ni->net() == n2);
    ni++;
    assert(ni == receivedpacket.nlri_list().end());

    //check the path attributes
    assert(receivedpacket.pathattribute_list().size() == 8);
    list <PathAttribute*>::const_iterator pai;
    pai = receivedpacket.pathattribute_list().begin();
    while (pai != receivedpacket.pathattribute_list().end()) {
	pa = *pai;
	switch (pa->type()) {
	case ORIGIN: {
	    const OriginAttribute *oa = (const OriginAttribute *)pa;
	    assert(oa->origintype() == IGP);
	    if (verbose) printf("%s\n", pa->str().c_str());
	    break;
	}
	case AS_PATH: {
	    const ASPathAttribute *asa = (const ASPathAttribute *)pa;
	    for (int i=1; i<=9; i++)
		assert(asa->as_path().contains(*as[i]));
	    if (verbose) printf("%s\n", pa->str().c_str());
	    break;
	}
	case NEXT_HOP: {
	    const NextHopAttribute<IPv4> *nha 
		= (const NextHopAttribute<IPv4> *)pa;
	    assert(nha->nexthop() == IPv4("10.0.0.1"));
	    if (verbose) printf("%s\n", pa->str().c_str());
	    break;
	}
	case LOCAL_PREF: {
	    const LocalPrefAttribute *lpa = 
		(const LocalPrefAttribute *)pa;
	    assert(lpa->localpref() == 237);
	    if (verbose) printf("%s\n", pa->str().c_str());
	    break;
	}
	case MED: {
	    const MEDAttribute *meda = 
		(const MEDAttribute *)pa;
	    assert(meda->med() == 515);
	    if (verbose) printf("%s\n", pa->str().c_str());
	    break;
	}
	case ATOMIC_AGGREGATE: {
	    if (verbose) printf("%s\n", pa->str().c_str());
	    break;
	}
	case AGGREGATOR: {
	    const AggregatorAttribute *aa = 
		(const AggregatorAttribute *)pa;
	    assert(aa->route_aggregator() == IPv4("20.20.20.2"));
	    assert(aa->aggregator_as() == AsNum((uint16_t)701));
	    if (verbose) printf("%s\n", pa->str().c_str());
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
	    if (verbose) printf("%s\n", pa->str().c_str());
	    break;
	}
	default:
	    fprintf(stderr, "Received PathAttribute type: %d\n",
		    pa->type());
	    abort();
	}
	++pai;
    }

    //try encoding the received packet, and check we get the same
    //encoded packet as when we encoded the constructed packet
    const uint8_t *buf2;
    int len2;
    buf2 = receivedpacket.encode(len2);
    assert(len == len2);
    assert(memcmp(buf, buf2, len2) == 0);
    return 0;
}

void sep(bool verbose) {
    if (verbose) printf("-------------------------------------------------------------------------\n");
}

int main(int argc, char *argv[]) {
    int c;
    bool verbose = false;
    while ((c = getopt(argc, argv, "v")) != EOF) {
	switch (c) {
	case 'v':
	    verbose = true;
	}
    }

    sep(verbose);
    test_simple_open_packet();
    if (verbose) printf("PASS\n\n\n");

    sep(verbose);
    if (verbose) printf("Testing Open Packet with no parameters\n");
    test_keepalive_packet();
    if (verbose) printf("PASS\n\n\n");

    sep(verbose);
    if (verbose) printf("Testing Notification Packet 1\n");
    test_notification_packets(NULL, CEASE, 0, 0);
    if (verbose) printf("PASS\n\n\n");

    sep(verbose);
    if (verbose) printf("Testing Notification Packet 2\n");
    test_notification_packets(NULL, FSMERROR, 1, 0);
    if (verbose) printf("PASS\n\n\n");

    
    sep(verbose);
    if (verbose) printf("Testing Notification Packet 2\n");
    uint8_t edata[2];
    edata[0]=1;
    edata[1]=2;
    test_notification_packets(edata, MSGHEADERERR, 2, 2);
    if (verbose) printf("PASS\n\n\n");

    sep(verbose);
    if (verbose) printf("Testing Update Packet with only withdrawn routes\n");
    test_withdraw_packet(verbose);
    if (verbose) printf("PASS\n\n\n");

    sep(verbose);
    if (verbose) printf("Testing Update Packet with only announced routes\n");
    test_announce_packet(verbose);
    if (verbose) printf("PASS\n\n\n");

    sep(verbose);
    if (verbose) printf("ALL TESTS PASSED\n");

}

//  LocalWords:  const
