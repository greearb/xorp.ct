// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/fea/click_elements/rtable1.hh,v 1.1.1.1 2002/12/11 23:56:03 hodson Exp $

#ifndef __FEA_CLICK_ELEMENTS_RTABLE1_HH__
#define __FEA_CLICK_ELEMENTS_RTABLE1_HH__

#include <click/vector.hh>
#include "ipv4address.hh"


class Rtable1 {
public:
    Rtable1();
    ~Rtable1();

    void add(IPV4address dst, IPV4address mask, IPV4address gw, int index);
    void del(IPV4address dst, IPV4address mask);
    bool lookup(IPV4address dst, IPV4address &gw, int &index) const;

    void test();

private:
    static int _invocations;

    struct Entry {
	IPV4address dst;
	IPV4address mask;
	IPV4address gw;
	int index;
	bool empty() const {return -1 == index;}
	void clear() {index = -1;}
    };
    Vector<Entry> _v;
};

#endif // __FEA_CLICK_ELEMENTS_RTABLE1_HH__
