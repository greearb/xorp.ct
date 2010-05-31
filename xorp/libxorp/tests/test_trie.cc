// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "libxorp_module.h"

#include "xorp.h"
#include "ipv4net.hh"
#include "ipv6net.hh"
#include "trie.hh"

class IPv4RouteEntry {};
class IPv6RouteEntry {};

Trie<IPv4, IPv4RouteEntry*> trie;
Trie<IPv6, IPv6RouteEntry*> trie6;

void test(IPv4Net test_net, IPv4RouteEntry *test_route) {
    printf("-----------------------------------------------\n");
    printf("looking up net: %s\n", test_net.str().c_str());
    Trie<IPv4, IPv4RouteEntry*>::iterator ti = trie.lookup_node(test_net);
    if (ti == trie.end()) {
	printf("Fail: no result\n");
	trie.print();
	abort();
    }
    const IPv4RouteEntry *r = ti.payload();
    if (r==test_route) {
	printf("PASS\n");
    } else {
	printf("Fail: route=%p\n", r);
	trie.print();
	abort();
    }
    printf("-----------\n");
}

void test_find(IPv4 test_addr, IPv4RouteEntry *test_route) {
    printf("-----------------------------------------------\n");
    printf("looking up net: %s\n", test_addr.str().c_str());
    Trie<IPv4, IPv4RouteEntry*>::iterator ti = trie.find(test_addr);
    if (ti == trie.end()) {
	printf("Fail: no result\n");
	trie.print();
	abort();
    }
    const IPv4RouteEntry *r = ti.payload();
    if (r==test_route) {
	printf("PASS\n");
    } else {
	printf("Fail: route=%p\n", r);
	trie.print();
	abort();
    }
    printf("-----------\n");
}

void test_less_specific(IPv4Net test_net, IPv4RouteEntry *test_route) {
    printf("-----------------------------------------------\n");
    printf("looking up less specific for net: %s\n", test_net.str().c_str());
    Trie<IPv4, IPv4RouteEntry*>::iterator ti = trie.find_less_specific(test_net);
    if (ti == trie.end()) {
	if (test_route == NULL) {
	    printf("PASS\n");
	    printf("-----------\n");
	    return;
	}
	printf("Fail: no result\n");
	trie.print();
	abort();
    }
    const IPv4RouteEntry *r = ti.payload();
    if (r==test_route) {
	printf("PASS\n");
    } else {
	printf("Fail: route=%p\n", r);
	trie.print();
	abort();
    }
    printf("-----------\n");
}

void test_upper_bound(IPv4 test_addr, IPv4 test_answer) {
    printf("-----------------------------------------------\n");
    printf("looking up upper bound: %s\n", test_addr.str().c_str());
    IPv4 lo, hi;
    trie.find_bounds(test_addr, lo, hi);
    IPv4 result= hi;
    if (test_answer == result) {
	printf("Pass: result = %s\n", result.str().c_str());
    } else {
	printf("Fail: answer should be %s, result was %s\n", 
	       test_answer.str().c_str(), result.str().c_str());
	trie.print();
	abort();
    }
    printf("-----------\n");
}

void test_lower_bound(IPv4 test_addr, IPv4 test_answer) {
    printf("-----------------------------------------------\n");
    printf("looking up lower bound: %s\n", test_addr.str().c_str());
    IPv4 lo, hi;
    trie.find_bounds(test_addr, lo, hi);
    IPv4 result= lo;
    if (test_answer == result) {
	printf("Pass: result = %s\n", result.str().c_str());
    } else {
	printf("Fail: answer should be %s, result was %s\n", 
	       test_answer.str().c_str(), result.str().c_str());
	trie.print();
	abort();
    }
    printf("-----------\n");
}

void test6(IPv6Net test_net, IPv6RouteEntry *test_route) {
    printf("-----------------------------------------------\n");
    printf("looking up net: %s\n", test_net.str().c_str());
    Trie<IPv6, IPv6RouteEntry*>::iterator ti = trie6.lookup_node(test_net);
    if (ti == trie6.end()) {
	printf("Fail: no result\n");
	trie.print();
	abort();
    }
    const IPv6RouteEntry *r = ti.payload();
    if (r==test_route) {
	printf("PASS\n");
    } else {
	printf("Fail: route=%p\n", r);
	trie.print();
	abort();
    }
    printf("-----------\n");
}

void test_find6(IPv6 test_addr, IPv6RouteEntry *test_route) {
    printf("-----------------------------------------------\n");
    printf("looking up net: %s\n", test_addr.str().c_str());
    Trie<IPv6, IPv6RouteEntry*>::iterator ti = trie6.find(test_addr);
    const IPv6RouteEntry *r = ti.payload();
    if (ti == trie6.end()) {
	printf("Fail: no result\n");
	trie.print();
	abort();
    }
    if (r==test_route) {
	printf("PASS\n");
    } else {
	printf("Fail: route=%p\n", r);
	trie.print();
	abort();
    }
    printf("-----------\n");
}

void test_upper_bound6(IPv6 test_addr, IPv6 test_answer) {
    printf("-----------------------------------------------\n");
    printf("looking up upper bound: %s\n", test_addr.str().c_str());
    IPv6 lo, hi;
    trie6.find_bounds(test_addr, lo, hi);
    IPv6 result= hi;
    if (test_answer == result) {
	printf("Pass: result = %s\n", result.str().c_str());
    } else {
	printf("Fail: answer should be %s, result was %s\n", 
	       test_answer.str().c_str(), result.str().c_str());
	trie.print();
	abort();
    }
    printf("-----------\n");
}

void
test_find_subtree()
{
    printf("Find subtree\n");

    Trie<IPv4, string> trie_subtree;
    
    IPv4Net n1(IPv4("169.229.0.136"), 29);
    trie_subtree.insert(n1, "169.229.0.136/29");

    IPv4Net n2(IPv4("192.150.187.0"), 25);
    trie_subtree.insert(n2, "192.150.187.0/25");
 
    IPv4Net n3(IPv4("192.168.254.2"), 32);
    trie_subtree.insert(n3, "192.168.254.2/32");

    IPv4Net n4(IPv4("192.168.254.3"), 32);
    trie_subtree.insert(n4, "192.168.254.3/32");

    trie_subtree.print();

    IPv4Net nsearch1(IPv4("192.150.187.248"), 29);
    Trie<IPv4, string>::iterator iter;

    iter = trie_subtree.search_subtree(nsearch1);
    for( ;iter != trie_subtree.end(); iter++) {
	printf("subtree = %s\n", iter.payload().c_str());
	printf("FAIL\n");
	abort();
    }

    IPv4Net n5(IPv4("192.150.187.248"), 29);
    trie_subtree.insert(n5, "192.150.187.248/29");

    IPv4Net nsearch2(IPv4("192.150.187.0"), 25);
    trie_subtree.erase(n2);

    trie_subtree.print();

    iter = trie_subtree.search_subtree(nsearch2);
    for( ;iter != trie_subtree.end(); iter++) {
	printf("subtree = %s\n", iter.payload().c_str());
	printf("FAIL\n");
	abort();
    }

    printf("PASS\n");
}

int main() {
    //test that find works OK with an empty trie (ie finds nothing).
    IPv4 a("1.0.0.0");
    Trie<IPv4, IPv4RouteEntry*>::iterator ti_post = trie.find(a);
    if (ti_post != trie.end()) {
	printf("FAIL\n");
	abort();
    }
    printf("PASS: find on empty trie found nothing\n");

    IPv4RouteEntry d1;
    IPv4Net n1(IPv4("1.2.1.0"), 24);
    printf("adding n1: %s route: %p\n", n1.str().c_str(), &d1);
    trie.insert(n1, &d1);
    test(n1, &d1);

    IPv4RouteEntry d2;
    IPv4Net n2(IPv4("1.2.0.0"), 16);
    printf("\n\nadding n2: %s route: %p\n", n2.str().c_str(), &d2);
    trie.insert(n2, &d2);
    test(n1, &d1);
    test(n2, &d2);

    IPv4RouteEntry d3;
    IPv4Net n3(IPv4("1.2.3.0"), 24);
    printf("\n\nadding n3: %s route: %p\n", n3.str().c_str(), &d3);
    trie.insert(n3, &d3);
    test(n1, &d1);
    test(n2, &d2);
    test(n3, &d3);

    IPv4RouteEntry d4;
    IPv4Net n4(IPv4("1.2.128.0"), 24);
    printf("\n\nadding n4: %s route: %p\n", n4.str().c_str(), &d4);
    trie.insert(n4, &d4);
    test(n1, &d1);
    test(n2, &d2);
    test(n3, &d3);
    test(n4, &d4);

    IPv4RouteEntry d5;
    IPv4Net n5(IPv4("1.2.0.0"), 20);
    printf("\n\nadding n5: %s route: %p\n", n5.str().c_str(), &d5);
    trie.insert(n5, &d5);
    test(n1, &d1);
    test(n2, &d2);
    test(n3, &d3);
    test(n4, &d4);
    test(n5, &d5);

    trie.print();

    trie.erase(n5);
    test(n1, &d1);
    test(n2, &d2);
    test(n3, &d3);
    test(n4, &d4);

    trie.print();

    trie.erase(n1);
    test(n2, &d2);
    test(n3, &d3);
    test(n4, &d4);

    trie.print();

    trie.erase(n2);
    test(n3, &d3);
    test(n4, &d4);

    trie.print();

    trie.insert(n2, &d2);
    trie.erase(n3);
    test(n2, &d2);
    test(n4, &d4);

    trie.print();

    IPv4RouteEntry d6;
    IPv4Net n6(IPv4("1.2.192.0"), 24);
    printf("\n\nadding n6: %s route: %p\n", n6.str().c_str(), &d6);
    trie.insert(n6, &d6);
    test(n2, &d2);
    test(n4, &d4);
    test(n6, &d6);

    test_find(IPv4("1.2.192.1"), &d6);
    test_find(IPv4("1.2.191.1"), &d2);

    trie.print();

    test_upper_bound(IPv4("1.2.190.1"), IPv4("1.2.191.255"));
    test_lower_bound(IPv4("1.2.190.1"), IPv4("1.2.129.0"));

    test_upper_bound(IPv4("1.2.120.1"), IPv4("1.2.127.255"));
    test_lower_bound(IPv4("1.2.120.1"), IPv4("1.2.0.0"));

    test_upper_bound(IPv4("1.2.192.1"), IPv4("1.2.192.255"));
    test_lower_bound(IPv4("1.2.192.1"), IPv4("1.2.192.0"));

    test_upper_bound(IPv4("1.2.128.1"), IPv4("1.2.128.255"));
    test_lower_bound(IPv4("1.2.128.1"), IPv4("1.2.128.0"));

    test_upper_bound(IPv4("1.2.193.1"), IPv4("1.2.255.255"));
    test_lower_bound(IPv4("1.2.193.1"), IPv4("1.2.193.0"));

    trie.erase(n4);
    test_upper_bound(IPv4("1.2.128.1"), IPv4("1.2.191.255"));
    test_lower_bound(IPv4("1.2.128.1"), IPv4("1.2.0.0"));
    test_lower_bound(IPv4("1.2.190.1"), IPv4("1.2.0.0"));

    trie.erase(n6);
    test_upper_bound(IPv4("1.2.128.1"), IPv4("1.2.255.255"));
    test_lower_bound(IPv4("1.2.128.1"), IPv4("1.2.0.0"));

    trie.erase(n2);
    test_upper_bound(IPv4("1.2.128.1"), IPv4("255.255.255.255"));
    test_lower_bound(IPv4("1.2.128.1"), IPv4("0.0.0.0"));

    IPv6RouteEntry d7;
    IPv6 ip6a("fe80::2c0:4fff:fe68:8c58");
    IPv6Net ip6net(ip6a, 96);
    trie6.insert(ip6net, &d7);
    test6(ip6net, &d7);
    test_find6(ip6a, &d7);
    test_upper_bound6(ip6a, IPv6("fe80::2c0:4fff:ffff:ffff"));

    //attempts to test remaining code paths
    trie.print();

    trie.insert(n2, &d2);
    trie.insert(n3, &d3);
    trie.insert(n1, &d1);

    IPv4RouteEntry d8;
    IPv4Net n8(IPv4("1.2.3.0"), 23);
    trie.insert(n8, &d8);

    trie.print();

    trie.erase(n8);

    IPv4RouteEntry d9;
    IPv4Net n9(IPv4("1.2.2.0"), 24);
    trie.insert(n9, &d9);
    trie.print();
    
    trie.erase(n9);

    trie.erase(n2);

    IPv4RouteEntry d10;
    IPv4Net n10(IPv4("1.2.0.0"), 15);
    trie.insert(n10, &d10); /* 1.2.0.0/15 */

    trie.print();

    trie.erase(n10);

    trie.insert(n2, &d2); /* 1.2.0.0/16 */
    trie.print();
    IPv4RouteEntry d11;
    IPv4Net n11(IPv4("1.0.0.0"), 14);
    trie.insert(n11, &d11);
    trie.print();

    trie.insert(n10, &d10); /* 1.2.0.0/15 */
    trie.print();

    trie.erase(n11);

    IPv4RouteEntry d12;
    IPv4Net n12(IPv4("1.3.0.0"), 17);
    trie.insert(n12, &d12);
    trie.print();

    trie.erase(n2);
    test_find(IPv4("1.2.2.1"), &d10);

    test_less_specific(n1, &d10); /* 1.2.1.0/24 */
    test_less_specific(n3, &d10); /* 1.2.3.0/24 */
    test_less_specific(n12, &d10); /* 1.3.0.0/17 */
    test_less_specific(n10, NULL); /* 1.2.0.0/15 */
    test_less_specific(n4, &d10); /* 1.2.128.0/24 */

    trie.print();
    test_lower_bound(IPv4("1.2.128.1"), IPv4("1.2.4.0"));
    test_upper_bound(IPv4("1.2.0.1"), IPv4("1.2.0.255"));

    trie.print();
    trie.erase(n10); /* 1.0.0.0/14 */
    trie.insert(n11, &d11);  /* 1.0.0.0/14 */
    trie.print();
    test_upper_bound(IPv4("1.0.0.1"), IPv4("1.2.0.255"));

    trie.erase(n11);
    test_lower_bound(IPv4("1.0.0.1"), IPv4("0.0.0.0"));
    test_upper_bound(IPv4("1.3.128.1"), IPv4("255.255.255.255"));
    test_upper_bound(IPv4("1.2.2.1"), IPv4("1.2.2.255"));
    
    trie.print();
    Trie<IPv4, IPv4RouteEntry*>::iterator iter;
    IPv4Net subroot(IPv4("1.2.0.0"), 21);
    iter = trie.search_subtree(subroot);
    while (iter!=trie.end()) {
	printf("------\n");
	iter++;
    }

    trie.erase(n3);
    trie.erase(n1);
    trie.insert(n11, &d11);
    trie.print();
    iter = trie.search_subtree(subroot);
    while (iter!=trie.end()) {
	printf("------\n");
	iter++;
    }

    trie.insert(n1, &d1);
    trie.erase(n1);
    iter = trie.search_subtree(subroot);
    while (iter!=trie.end()) {
	printf("------\n");
	iter++;
    }

    trie.erase(n12);
    trie.insert(n1, &d1);
    trie.insert(n3, &d3);
    trie.erase(n1);
    iter = trie.search_subtree(subroot);
    while (iter!=trie.end()) {
	printf("------\n");
	iter++;
    }

    
    trie.insert(n10, &d10);
    trie.insert(n1, &d1);
    trie.insert(n9, &d9);
    trie.print();
    iter = trie.search_subtree(n10);
    while (iter!=trie.end()) {
	printf("------\n");
	iter++;
    }

    trie.print();
    iter = trie.search_subtree(n3);
    while (iter!=trie.end()) {
	printf("------\n");
	iter++;
    }

    //-------------------------------------------------------    

    printf("Test replacement of interior node\n");

    IPv4Net n13(IPv4("1.2.2.0"), 23);
    IPv4RouteEntry d13;
    trie.insert(n13, &d13);
    trie.print();

    //-------------------------------------------------------    
    printf("==================\nTest of begin()\n");
    trie.erase(n11);
    trie.erase(n10);
    trie.print();

    //-------------------------------------------------------    
    printf("==================\nTest of lower_bound()\n");
    trie.print();
    iter = trie.lower_bound(n1); /*1.2.1.0/24*/
    assert(iter.key() == n1);
    iter = trie.lower_bound(n9); /*1.2.2.0/24*/
    assert(iter.key() == n9);
    iter = trie.lower_bound(n3); /*1.2.3.0/24*/
    assert(iter.key() == n3);
    iter = trie.lower_bound(n13); /*1.2.2.0/23*/
    assert(iter.key() == n13);

    iter = trie.lower_bound(IPNet<IPv4>("1.2.1.128/25")); 
    assert(iter.key() == n1); /*1.2.1.0/24*/

    iter = trie.lower_bound(IPNet<IPv4>("1.2.4.128/25")); 
    assert(iter == trie.end());

    iter = trie.lower_bound(IPNet<IPv4>("1.2.1.0/23")); 
    assert(iter.key() == n9); /*1.2.2.0/24*/

    iter = trie.lower_bound(IPNet<IPv4>("1.0.0.0/24")); 
    if (iter != trie.end())
	printf("iter.key = %s\n", iter.key().str().c_str());
    else
	printf("iter = end\n");
    assert(iter.key() == n1); /*1.2.1.0/24*/

    //-------------------------------------------------------    

    printf("Test /32 prefix works\n");

    IPv4Net n14(IPv4("9.9.9.9"), 32);
    IPv4RouteEntry d14;
    IPv4Net n15(IPv4("9.9.9.9"), 24);
    IPv4RouteEntry d15;
    trie.insert(n14, &d14);
    trie.insert(n15, &d15);
    test_find(IPv4("9.9.9.9"), &d14);
    test_find(IPv4("9.9.9.8"), &d15);
    trie.erase(n14);
    trie.erase(n15);

    printf("==================\nTest of preorder iterator\n");
    // this is the list of subnets in prefix order.  The test consists in
    // inserting a list of unsorted subnets and check that the iterator
    // retrieves them in the proper order.

    Trie<IPv4, IPv4RouteEntry*, TriePreOrderIterator<IPv4, IPv4RouteEntry*> > 
	preotrie;
    const char * subnets[] = {  "1.2.0.0/16",
				"1.2.0.0/20",
				"1.2.1.0/24",
				"1.2.3.0/24",
				"1.2.128.0/24" };
    IPv4RouteEntry d16;
    IPv4Net n16(subnets[3]);
    printf("adding n16: %s route: %p\n", n16.str().c_str(), &d16);
    preotrie.insert(n16, &d16);

    IPv4RouteEntry d17;
    IPv4Net n17(subnets[2]);
    printf("adding n17: %s route: %p\n", n17.str().c_str(), &d17);
    preotrie.insert(n17, &d17);

    IPv4RouteEntry d18;
    IPv4Net n18(subnets[0]);
    printf("adding n18: %s route: %p\n", n18.str().c_str(), &d18);
    preotrie.insert(n18, &d18);

    IPv4RouteEntry d19;
    IPv4Net n19(subnets[4]);
    printf("adding n19: %s route: %p\n", n19.str().c_str(), &d19);
    preotrie.insert(n19, &d19);

    IPv4RouteEntry d20;
    IPv4Net n20(subnets[1]);
    printf("adding n20: %s route: %p\n", n20.str().c_str(), &d20);
    preotrie.insert(n20, &d20);

    //-------------------------------------------------------    
    printf("-----------\n");
    printf("Test of prefix increment (++ti)\n");
    printf("-----------\n");
    TriePreOrderIterator<IPv4, IPv4RouteEntry*> ti;
    int subnetidx = 0;
    for (ti = preotrie.begin() ; ti != preotrie.end() ; ti++) {
        printf("*** node: %-26s %s\n",
               ti.cur()->k().str().c_str(),
               ti.cur()->has_payload() ? "PL" : "[]");
	if (strcmp(subnets[subnetidx++],ti.cur()->k().str().c_str())) {
	    fprintf(stderr, "invalid traversal order detected!\n");
	    exit(1);
	}
	if (!ti.cur()->has_payload()) {
	    fprintf(stderr, "iterator should not return empty nodes!\n");
	    exit(1);
	}
    }
    if (subnetidx != 5) {
	fprintf(stderr, "iterator missing nodes!\n");
	exit(1);
    }
    printf("PASS\n");

    //-------------------------------------------------------    
    printf("-----------\n");
    printf("Test of postfix increment (ti++)\n");
    printf("-----------\n");
    subnetidx = 0;     
    for (ti = preotrie.begin() ; ti != preotrie.end() ; ++ti) {
        printf("*** node: %-26s %s\n",
               ti.cur()->k().str().c_str(),
               ti.cur()->has_payload() ? "PL" : "[]");
	if (strcmp(subnets[subnetidx++],ti.cur()->k().str().c_str())) {
	    fprintf(stderr, "invalid traversal order detected!\n");
	    exit(1);
	}
	if (!ti.cur()->has_payload()) {
	    fprintf(stderr, "iterator should not return empty nodes!\n");
	    exit(1);
	}
    }
    if (subnetidx != 5) {
	fprintf(stderr, "iterator missing nodes!\n");
	exit(1);
    }
    printf("PASS\n");

    test_find_subtree();
}
