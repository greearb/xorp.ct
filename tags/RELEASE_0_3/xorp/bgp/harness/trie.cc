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

#ident "$XORP: xorp/bgp/harness/trie.cc,v 1.3 2003/03/10 23:20:09 hodson Exp $"

// #define DEBUG_LOGGING 
// #define DEBUG_PRINT_FUNCTION_NAME 
#define PARANOIA

#include <string>
#include <stdlib.h>
#include <stdio.h>

#include "bgp/bgp_module.h"
#include "config.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "trie.hh"

Trie::~Trie()
{
    del(&_head);
}

const
UpdatePacket *
Trie::lookup(const string& net) const
{
    IPv4Net n(net.c_str());
    Payload payload = find(n);
    const UpdatePacket *update = payload.get();

    if(0 == update)
	return 0;

    /*
    ** Look for this net in the matching update packet.
    */
    list <BGPUpdateAttrib>::const_iterator ni;
    ni = update->nlri_list().begin();
    for(; ni != update->nlri_list().end(); ni++)
	if(ni->net() == n)
	    return update;

    return 0;
}

void
Trie::process_update_packet(const TimeVal& tv, const uint8_t *buf, size_t len)
{
    Payload payload(tv, buf, len, this);
    const UpdatePacket *update = payload.get();

    /*
    ** First process the withdraws, if any are present.
    */
    list <BGPUpdateAttrib>::const_iterator wi;
    wi = update->wr_list().begin();
    for(; wi != update->wr_list().end(); wi++)
	del(wi->net());

    /*
    ** If there are no nlri's present then there is nothing to save so
    ** out of here.
    */
    if(update->nlri_list().empty())
	return;

    /*
    ** Now save the NLRI information.
    */
    list <BGPUpdateAttrib>::const_iterator ni;
    ni = update->nlri_list().begin();
    for(; ni != update->nlri_list().end(); ni++) {
	/* If a previous entry exists remove it */
	const UpdatePacket *up = lookup(ni->net().str());
	if(up)
	    if(!del(ni->net()))
		XLOG_FATAL("Could not remove nlri: %s",
			   ni->net().str().c_str());
	if(!insert(ni->net(), payload))
	    XLOG_FATAL("Could not add nlri: %s",
		       ni->net().str().c_str());
    }
}

void
Trie::tree_walk_table(const TreeWalker& tw) const
{
    tree_walk_table(tw, &_head, "");
}

void
Trie::save_routing_table(FILE *fp) const
{
    print(fp);
}

/* Private below here */
const uint32_t topbit = 0x80000000;

#define speak debug_msg
#define	newline	"\n"

bool
Trie::empty() const {
    return (0 ==_head.ptrs[0]) && (0 ==_head.ptrs[1]);
}

void
Trie::tree_walk_table(const TreeWalker& tw, const Tree *ptr,
		      const char *st) const
{
    char result[1024];

    if(0 == ptr) {
	return;
    }

    TimeVal tv;
    const UpdatePacket *update = ptr->p.get(tv);
    if(0 != update) {
	IPv4 nh;
	list<PathAttribute*> l = update->pa_list();
	list<PathAttribute*>::const_iterator i;
	for(i = l.begin(); i != l.end(); i++) {
	    if(NEXT_HOP == (*i)->type()) {
 		IPv4NextHopAttribute *nha = 
 		    dynamic_cast<IPv4NextHopAttribute *>(*i);
		nh = nha->nexthop();
	    }
	    
	}
	const IPNet<IPv4> net = IPNet<IPv4>(bit_string_to_subnet(st).c_str());
	tw->dispatch(update, net, tv);
#if	0
	fprintf(fp, "%ssummary %s %s %s\n",  update->str().c_str(), st,
		bit_string_to_subnet(st).c_str(), nh.str().c_str());
#endif
    }

    sprintf(result, "%s0", st);
    tree_walk_table(tw, ptr->ptrs[0], result);

    sprintf(result, "%s1", st);
    tree_walk_table(tw, ptr->ptrs[1], result);
}

void
Trie::print(FILE *fp) const
{
    prt(fp, &_head, "");
}

void
Trie::prt(FILE *fp, const Tree *ptr, const char *st) const
{
    char result[1024];

    if(0 == ptr) {
	return;
    }

    const UpdatePacket *update = ptr->p.get();
    if(0 != update) {
	IPv4 nh;
	list<PathAttribute*> l = update->pa_list();
	list<PathAttribute*>::const_iterator i;
	for(i = l.begin(); i != l.end(); i++) {
	    if(NEXT_HOP == (*i)->type()) {
 		IPv4NextHopAttribute *nha = 
 		    dynamic_cast<IPv4NextHopAttribute *>(*i);
		nh = nha->nexthop();
	    }
	    
	}
	fprintf(fp, "%ssummary %s %s %s\n",  update->str().c_str(), st,
		bit_string_to_subnet(st).c_str(), nh.str().c_str());
    }

    sprintf(result, "%s0", st);
    prt(fp, ptr->ptrs[0], result);

    sprintf(result, "%s1", st);
    prt(fp, ptr->ptrs[1], result);
}

string
Trie::bit_string_to_subnet(const char *st) const
{
    int len = strlen(st);
    int val = 0;
    for(int i = 0; i < len; i++) {
	val <<= 1;
	if('1' == st[i])
	    val |= 1;
    }
    val <<= 32 - len;
    struct in_addr in;
    in.s_addr = htonl(val);

    return c_format("%s/%d", inet_ntoa(in), len);
}

bool
Trie::insert(const IPv4Net& net, Payload& p)
{
    return insert(ntohl(net.masked_addr().addr()), net.prefix_len(), p);
}

bool
Trie::insert(uint32_t address, int mask_length, Payload& p)
{
#ifdef	PARANOIA
    if(0 == p.get()) {
	speak("insert: Attempt to store an invalid entry"
	      newline);
	return false;
    }
#endif
    Tree *ptr = &_head;
    for(int i = 0; i < mask_length; i++) {
	int index = (address & topbit) == topbit;
	address <<= 1;

	if(0 == ptr->ptrs[index]) {
	    ptr->ptrs[index] = new Tree();
	    if(0 == ptr->ptrs[index]) {
		if(_debug)
		    speak("insert: new failed"
			  newline);
		return false;
	    }
	}

	ptr = ptr->ptrs[index];
    }

    if(0 != ptr->p.get()) {
	speak("insert: value already assigned" newline);
	return false;
    }

    ptr->p = p;

    return true;
}

void
Trie::del(Tree *ptr)
{
    if(0 == ptr)
	return;
    del(ptr->ptrs[0]);
    delete ptr->ptrs[0];
    ptr->ptrs[0] = 0;
    del(ptr->ptrs[1]);
    delete ptr->ptrs[1];
    ptr->ptrs[1] = 0;
}

bool
Trie::del(const IPv4Net& net)
{
    return del(ntohl(net.masked_addr().addr()), net.prefix_len());
}

bool
Trie::del(uint32_t address, int mask_length)
{
    return del(&_head, address, mask_length);
}

bool
Trie::del(Tree *ptr, uint32_t address, int mask_length)
{
    if((0 == ptr) && (0 != mask_length)) {
	if(_debug)
	    speak("del:1 not in table" newline);
	return false;
    }
    int index = (address & topbit) == topbit;
    address <<= 1;

    if(0 == mask_length) {
	if(0 == ptr) {
	    if(_debug)
		speak("del:1 zero pointer" newline);
	    return false;
	}
	if(0 == ptr->p.get()) {
	    if(_debug)
		speak("del:2 not in table" newline);
	    return false;
	}
	ptr->p = Payload();	// Invalidate this entry.
	return true;
    }

    if(0 == ptr) {
	if(_debug)
	    speak("del:2 zero pointer" newline);
	return false;
    }

    Tree *next = ptr->ptrs[index];

    bool status = del(next, address, --mask_length);

    if(0 == next) {
	if(_debug)
	    speak("del: no next pointer" newline);
	return false;
    }

    if((0 == next->p.get()) && (0 == next->ptrs[0]) &&
       (0 == next->ptrs[1])) {
	delete ptr->ptrs[index];
	ptr->ptrs[index] = 0;
    }

    return status;
}

Trie::Payload 
Trie::find(const IPv4Net& net) const
{
    return find(ntohl(net.masked_addr().addr()), net.prefix_len());
}

Trie::Payload
Trie::find(uint32_t address, const int mask_length) const
{
    const Tree *ptr = &_head;
    Tree *next;

    struct in_addr in;
    in.s_addr = htonl(address);
    debug_msg("find: %#x %s/%d\n", address, inet_ntoa(in), mask_length);
    speak("find: %#x %s/%d\n", address, inet_ntoa(in), mask_length);

    // The loop should not require bounding. Defensive
    for(int i = 0; i <= 32; i++) {
	int index = (address & topbit) == topbit;
	address <<= 1;

	if(_pretty)
	    speak("%d", index);
	Payload p = ptr->p;
	if(mask_length == i)
	    return p;
	if(0 == (next = ptr->ptrs[index])) {
	    if(_pretty)
		speak("" newline);
	    return Payload();
	}
	ptr = next;
    }
    speak("find: should never happen" newline);
    return Payload();
}

Trie::Payload
Trie::find(uint32_t address) const
{
    const Tree *ptr = &_head;
    Tree *next;

    struct in_addr in;
    in.s_addr = htonl(address);
    debug_msg("find: %#x %s\n", address, inet_ntoa(in));

    // The loop should not require bounding. Defensive
    for(int i = 0; i <= 32; i++) {
	int index = (address & topbit) == topbit;
	address <<= 1;

	if(_pretty)
	    speak("%d", index);
	Payload p = ptr->p;
	if(0 == (next = ptr->ptrs[index])) {
	    if(_pretty)
		speak("" newline);
	    return p;
	}
	ptr = next;
    }
    speak("find: should never happen" newline);
    return Payload();
}
