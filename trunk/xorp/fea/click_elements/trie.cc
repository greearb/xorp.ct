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

#ident "$XORP: xorp/fea/click_elements/trie.cc,v 1.1.1.1 2002/12/11 23:56:03 hodson Exp $"

#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const u_long topbit = 0x80000000;
const int INVALID = -1;

#ifdef CLICK
#define speak click_chatter
#define	newline	""
#else
#define speak printf
#define	newline	"\n"
#endif

class Trie {
public:
	struct Tree {
		Tree *ptrs[2];
		int val;

		Tree() {
			ptrs[0] = ptrs[1] = 0;
			val = INVALID;
		}
	};

	bool _debug;
	bool _pretty;

	Tree _head;

	Trie() {
		_debug = true;
		_pretty = false;
	}

	bool
	empty() {
		return (0 ==_head.ptrs[0]) && (0 ==_head.ptrs[1]);
	}

	void
	print() {
		prt(&_head, (char *)"");
	}

	void
	prt(Tree *ptr, char *st) {
		char result[1024];

		if(0 == ptr) {
			return;
		}

		if(INVALID != ptr->val)
			speak("%#x\t%s" newline, ptr->val, st);

		sprintf(result, "%s0", st);
 		prt(ptr->ptrs[0], result);

		sprintf(result, "%s1", st);
 		prt(ptr->ptrs[1], result);
	}

	bool insert(u_long address, int mask_length, int val) {
#ifdef	PARANOIA
		if(INVALID == val) {
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
				ptr->ptrs[index] = new Tree;
				if(0 == ptr->ptrs[index]) {
					if(_debug)
						speak("insert: new failed"
						      newline);
					return false;
				}
			}

			ptr = ptr->ptrs[index];
		}

		if(INVALID != ptr->val) {
			speak("insert: value already assigned" newline);
			return false;
		}

		ptr->val = val;

		return true;
	}

	bool del(u_long address, int mask_length) {
		return del(&_head, address, mask_length);
	}

	bool del(Tree *ptr, u_long address, int mask_length) {
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
			if(INVALID == ptr->val) {
				if(_debug)
					speak("del:2 not in table" newline);
				return false;
			}
			ptr->val = INVALID;
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

		if((INVALID == next->val) && (0 == next->ptrs[0]) &&
		   (0 == next->ptrs[1])) {
 			delete ptr->ptrs[index];
 			ptr->ptrs[index] = 0;
		}

		return status;
	}

	int find(u_long address) {
		Tree *ptr = &_head;
		Tree *next;
		int val = INVALID;

		// The loop should not require bounding. Defensive
		for(int i = 0; i <= 32; i++) {
			int index = (address & topbit) == topbit;
			address <<= 1;

			if(_pretty)
				speak("%d", index);
			val = INVALID != ptr->val ? ptr->val : val;
			if(0 == (next = ptr->ptrs[index])) {
				if(_pretty)
					speak("" newline);
				return val;
			}
			ptr = next;
		}
		speak("find: should never happen" newline);
		return INVALID;
	}
};

u_long
addr(char *a)
{
	return ntohl(inet_addr(a));
}


class Trie f;

void
test1()
{

	u_long s, startval = addr("128.16.64.16");
	int iterate = 1000000;
	const int start = 20;
	const int end = 32;
	const int step = 0x1000;

	speak("inserting %d routes" newline, iterate * (end - start));

	s = startval ;
	for(int i = 0; i < iterate; i++, s += step)
		for(int j = start; j < end; j++)
			if(!f.insert(s, j, s)) {
				speak("%d %#lx %d" newline, i, s, j);
				return;
			}

	speak("searching for %d routes" newline, iterate * (end - start));

	s = startval ;
	for(int i = 0; i < iterate; i++, s += step)
		for(int j = start; j < end; j++)
			if(s != (u_long)f.find(s)) {
				speak("%d %#lx %d" newline, i, s, j);
				return;
			}

	speak("deleting %d routes" newline, iterate * (end - start));

	s = startval;
	for(int i = 0; i < iterate; i++, s+= step)
		for(int j = start; j < end; j++)
			if(!f.del(s, j)) {
				speak("%#lx %d" newline, s, j);
				return;
			}

}

void
test2()
{
	bool failed = false;

	// Add three routes with different prefix lengths

	f.insert(addr("128.16.64.16"), 16, 0x16);
	f.insert(addr("128.16.64.16"), 24, 0x24);
	f.insert(addr("128.16.64.16"), 32, 0x32);
	
	// Find these three routes

	if(0x32 != f.find(addr("128.16.64.16")))
		failed = true;

	if(0x24 != f.find(addr("128.16.64.17")))
		failed = true;

	if(0x16 != f.find(addr("128.16.65.17")))
		failed = true;

	// Remove the same route twice

	if(true != f.del(addr("128.16.64.16"), 16))
		failed = true;

	if(false != f.del(addr("128.16.64.16"), 16))
		failed = true;

	// Lookup the routes again. The last route should be missing.

	if(0x32 != f.find(addr("128.16.64.16")))
		failed = true;

	if(0x24 != f.find(addr("128.16.64.17")))
		failed = true;

	if(INVALID != f.find(addr("128.16.65.17")))
		failed = true;

	// Remove the rest of the routes
	if(true != f.del(addr("128.16.64.16"), 24))
		failed = true;

	if(true != f.del(addr("128.16.64.16"), 32))
		failed = true;

	if(failed)
		speak("Tests failed" newline);
}

void
test3()
{
	bool failed = false;

	f.insert(addr("128.16.64.16"), 16, 0x16);
	f.insert(addr("128.16.64.16"), 24, 0x24);
	f.insert(addr("128.16.64.16"), 32, 0x32);

	f.print();

	if(true != f.del(addr("128.16.64.16"), 32))
		failed = true;

	if(true != f.del(addr("128.16.64.16"), 24))
		failed = true;

	if(true != f.del(addr("128.16.64.16"), 16))
		failed = true;

	if(failed)
		speak("Tests failed" newline);
}

void
test4()
{
	int length = 8;
	bool failed = false;

	f.insert(addr("170.16.64.16"), length, length);

	f.print();

	if(true != f.del(addr("170.16.64.16"), length))
		failed = true;

	if(failed)
		speak("Tests failed" newline);
}

int
main()
{
	/*
	** Every test should leave the table empty
	*/

     	test1();
    	test2();
  	test3();
	test4();

	if(false == f.empty()) {
		speak("Failure the tree is not empty"newline);
		f.print();
	}

	return 0;
}
