// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/bgp/aspath_test.cc,v 1.19 2008/01/04 03:15:17 pavlin Exp $"

#include "bgp_module.h"

#include "libxorp/xorp.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "aspath.hh"

void
test_string(bool verbose)
{
    /*********************************************************************/
    /**** testing construction from a string                          ****/
    /*********************************************************************/

    if (verbose)
	printf("Test constructing an As Path from a string\n");

    ASPath aspath_str("65008,1,2,{3,4,5},6,{7,8},9"); // From string
    ASPath aspath_con;      // Constructed

    {
	ASSegment seq = ASSegment(AS_SEQUENCE);
	seq.add_as(AsNum(65008));
	seq.add_as(AsNum(1));
	seq.add_as(AsNum(2));
	
	aspath_con.add_segment(seq);
    }

    {
	ASSegment set = ASSegment(AS_SET);
	set.add_as(AsNum(3));
	set.add_as(AsNum(4));
	set.add_as(AsNum(5));
	
	aspath_con.add_segment(set);
    }	

    {
	ASSegment seq = ASSegment(AS_SEQUENCE);
	seq.set_type(AS_SEQUENCE);
	seq.add_as(AsNum(6));
	
	aspath_con.add_segment(seq);
    }

    {
	ASSegment set = ASSegment(AS_SET);
	set.add_as(AsNum(7));
	set.add_as(AsNum(8));
	
	aspath_con.add_segment(set);
    }	

    {
	ASSegment seq;
	seq.set_type(AS_SEQUENCE);
	seq.add_as(AsNum(9));
	
	aspath_con.add_segment(seq);
    }

    assert(aspath_str == aspath_con);

}


void
test_string_as4_compat(bool verbose)
{
    /*********************************************************************/
    /**** testing construction from a string using 4-byte syntax      ****/
    /*********************************************************************/

    if (verbose)
	printf("Test2 constructing an As Path from a string using 4-byte syntax\n");

    /* use 4-byte syntax for ASnums that are really 16-bit */
    ASPath aspath_str("0.65008,1,2,{3,0.4,5},6,{7,8},9"); // From string
    ASPath aspath_con;      // Constructed

    {
	ASSegment seq = ASSegment(AS_SEQUENCE);
	seq.add_as(AsNum(65008));
	seq.add_as(AsNum(1));
	seq.add_as(AsNum(2));
	
	aspath_con.add_segment(seq);
    }

    {
	ASSegment set = ASSegment(AS_SET);
	set.add_as(AsNum(3));
	set.add_as(AsNum(4));
	set.add_as(AsNum(5));
	
	aspath_con.add_segment(set);
    }	

    {
	ASSegment seq = ASSegment(AS_SEQUENCE);
	seq.set_type(AS_SEQUENCE);
	seq.add_as(AsNum(6));
	
	aspath_con.add_segment(seq);
    }

    {
	ASSegment set = ASSegment(AS_SET);
	set.add_as(AsNum(7));
	set.add_as(AsNum(8));
	
	aspath_con.add_segment(set);
    }	

    {
	ASSegment seq;
	seq.set_type(AS_SEQUENCE);
	seq.add_as(AsNum(9));
	
	aspath_con.add_segment(seq);
    }

    if (verbose)
	printf("Comparing\n   %s with\n   %s\n", 
	       aspath_str.str().c_str(), aspath_con.str().c_str());

    assert(aspath_str == aspath_con);
}


void
test_string_as4(bool verbose)
{
    /*********************************************************************/
    /**** testing construction from a string using 4-byte syntax      ****/
    /*********************************************************************/

    if (verbose)
	printf("Test3 constructing an As Path from a string using 4-byte syntax\n");

    /* use 4-byte syntax for ASnums that are really 16-bit */
    ASPath aspath_str("2.65008,1,2,{3,2.4,5},6,{7,8},2.9"); // From string
    ASPath aspath_con;      // Constructed

    {
	ASSegment seq = ASSegment(AS_SEQUENCE);
	seq.add_as(AsNum((uint32_t)196080));
	seq.add_as(AsNum(1));
	seq.add_as(AsNum(2));
	
	aspath_con.add_segment(seq);
    }

    {
	ASSegment set = ASSegment(AS_SET);
	set.add_as(AsNum(3));
	set.add_as(AsNum((uint32_t)131076));
	set.add_as(AsNum(5));
	
	aspath_con.add_segment(set);
    }	

    {
	ASSegment seq = ASSegment(AS_SEQUENCE);
	seq.set_type(AS_SEQUENCE);
	seq.add_as(AsNum(6));
	
	aspath_con.add_segment(seq);
    }

    {
	ASSegment set = ASSegment(AS_SET);
	set.add_as(AsNum(7));
	set.add_as(AsNum(8));
	
	aspath_con.add_segment(set);
    }	

    {
	ASSegment seq;
	seq.set_type(AS_SEQUENCE);
	seq.add_as(AsNum((uint32_t)131081));
	
	aspath_con.add_segment(seq);
    }

    if (verbose)
	printf("Comparing\n   %s with\n   %s\n", 
	       aspath_str.str().c_str(), aspath_con.str().c_str());

    assert(aspath_str == aspath_con);
}


void
test_as4_coding(bool verbose)
{
    /*********************************************************************/
    /**** testing encoding and decoding of 4-byte AS numbers          ****/
    /*********************************************************************/

    if (verbose)
	printf("Test4 coding and decoding an AS4 path\n");

    /* use 4-byte syntax for ASnums that are really 16-bit */
    ASPath aspath("2.65008,1,2,{3,2.4,5},6,{7,8},2.9"); // From string

    size_t len = ((AS4Path*)(&aspath))->wire_size();
    uint8_t *buf = new uint8_t[len];


    ((AS4Path*)&aspath)->encode(len, buf);
    
    AS4Path dec_aspath(buf, len);

    if (verbose)
	printf("Comparing\n   %s with\n   %s\n", 
	       aspath.str().c_str(), dec_aspath.str().c_str());

    delete []buf;

    assert(aspath == dec_aspath);
}


void
test_as4_coding_compat(bool verbose)
{
    /*********************************************************************/
    /**** testing encoding and decoding of compat 4-byte AS numbers   ****/
    /*********************************************************************/

    if (verbose)
	printf("Test5 coding and decoding a compat AS4 path\n");

    /* construct a 4-byte AS path */
    ASPath aspath("2.65008,1,2,{3,2.4,5},6,{7,8},2.9");
    // 23456 is AS_TRAN
    ASPath compat_aspath("23456,1,2,{3,23456,5},6,{7,8},23456");

    size_t len = aspath.wire_size();
    uint8_t *buf = new uint8_t[len];

    /* 2-byte encoding function, which should convert 4-byte-only
       ASnums to AS_TRAN */
    aspath.encode(len, buf);
    
    ASPath dec_aspath(buf, len);

    if (verbose)
	printf("Comparing\n   %s with\n   %s\n", 
	       compat_aspath.str().c_str(), dec_aspath.str().c_str());

    delete []buf;

    assert(compat_aspath == dec_aspath);
}


int
main(int argc, char* argv[])
{
    int c;
    bool verbose = false;

    while ((c = getopt(argc, argv, "v")) != EOF) {
	switch (c) {
	case 'v':
	    verbose = true;
	    break;
	}
    }

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

    for (i=0;i<=9;i++) {
	delete as[i];
    }

    ASPath *aspath= new ASPath;
    aspath->add_segment(seq1);
    aspath->add_segment(set1);
    aspath->add_segment(seq2);

    assert(aspath->num_segments() == 3);
    if (verbose) printf("Original: %s\n", aspath->str().c_str());

    ASPath *aspathcopy= new ASPath(*aspath);
    if (verbose) printf("Copy: %s\n", aspathcopy->str().c_str());
    if (verbose) printf("Deleting orginal\n");
    delete aspath;
    if (verbose) printf("Copy: %s\n", aspathcopy->str().c_str());

    AsNum *asn;
    for (i=1;i<=9;i++) {
	asn = new AsNum(i);
	assert(aspathcopy->contains(*asn) == true);
	delete asn;
    }
    asn = new AsNum(AsNum::AS_INVALID);	// XXX should never do this
    assert(aspathcopy->contains(*asn) == false);
    delete asn;

    if (verbose) 
	printf("Testing add_As_in_sequence - adding to existing sequence\n");
    asn = new AsNum(65000);
    aspathcopy->prepend_as(*asn);
    if (verbose) printf("Extended: %s\n", aspathcopy->str().c_str());
    assert(aspathcopy->contains(*asn) == true);
    delete asn;

    for (i=10;i<=12;i++) {
	as[i] = new AsNum(i);
    }
    ASSegment set2 = ASSegment(AS_SET);
    set2.add_as(*(as[10]));
    set2.add_as(*(as[11]));
    set2.add_as(*(as[12]));
    aspath= new ASPath;
    aspath->add_segment(set2);
    aspath->add_segment(seq2);
    aspath->add_segment(set1);
    aspath->add_segment(seq1);
    if (verbose) 
	printf("Testing add_As_in_sequence - adding to existing set\n");
    asn = new AsNum(65001);
    if (verbose) printf("Before: %s\n", aspath->str().c_str());
    aspath->prepend_as(*asn);
    if (verbose) printf("Extended: %s\n", aspath->str().c_str());
    assert(aspath->contains(*as[10]) == true);
    assert(aspath->contains(*as[11]) == true);
    assert(aspath->contains(*as[12]) == true);
    assert(aspath->contains(*asn) == true);
    delete asn;
    for (i=10;i<=12;i++) 
	delete as[i];


    test_string(verbose);
    test_string_as4_compat(verbose);
    test_string_as4(verbose);
    test_as4_coding(verbose);
    test_as4_coding_compat(verbose);

    if (verbose) printf("All tests passed\n");
#if 0
    printf("Check for space leak: check memory usage now....\n");
    TimerList::system_sleep(TimeVal(5, 0));
    printf("Continuing...\n");
    aspath = new ASPath(*aspathcopy);
    delete aspathcopy;
    for(i=0; i< 10000; i++) {
	aspathcopy= new ASPath(*aspath);
	delete aspath;
	aspath = new ASPath(*aspathcopy);
	delete aspathcopy;
    }
    printf("Done...\n");


    for (i=1;i<=9;i++) {
	asn = new AsNum(i);
	assert(aspath->contains(*asn) == true);
	delete asn;
    }
    asn = new AsNum;
    assert(aspath->contains(*asn) == false);
    delete asn;
    TimerList::system_sleep(TimeVal(10, 0));
#else
    delete aspath;
    delete aspathcopy;
#endif
    exit(0);
}
