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

// $XORP: xorp/fea/click_elements/rtable2.hh,v 1.3 2002/12/09 18:29:01 hodson Exp $

#ifndef __FEA_CLICK_ELEMENTS_RTABLE2_HH__
#define __FEA_CLICK_ELEMENTS_RTABLE2_HH__

#include <click/vector.hh>
#include "ipv4address.hh"

// #define TEST

class Rtable2 {
public:
    Rtable2();
    ~Rtable2();

    void add(IPV4address dst, IPV4address mask, IPV4address gw, int index);
    void del(IPV4address dst, IPV4address mask);
    bool lookup(IPV4address dst, IPV4address &gw, int &index) const;
    bool lookup(IPV4address dst, IPV4address &match, IPV4address &mask,
		IPV4address &gw, int &index) const;

    void zero();
    bool copy(Rtable2 *rt);

    bool start_table_read();
    enum status {OK, TABLE_CHANGED, ILLEGAL_OPERATION};
    bool next_table_read(IPV4address &dst, IPV4address &mask, IPV4address &gw, 
			 int &index, status &status);

    void test();

private:
#ifdef	TEST
    static int _invocations;
#endif

    bool _dirty;

    bool _table_read;
    int _index_table_read;

    struct Entry {
	IPV4address dst;
	IPV4address mask;
	IPV4address gw;
	int index;
	bool empty() const {return -1 == index;}
	void clear() {index = -1;}
    };

    bool lookup_entry(IPV4address dst, Entry& entry) const;

    Vector<Entry> _v;
};

#endif // __FEA_CLICK_ELEMENTS_RTABLE2_HH__
