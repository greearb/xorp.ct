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

#ident "$XORP: xorp/fea/click_elements/rtable1.cc,v 1.1.1.1 2002/12/11 23:56:03 hodson Exp $"

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
// ALWAYS INCLUDE click/config.h
#include <click/config.h>
// ALWAYS INCLUDE click/package.hh
#include <click/package.hh>
#include <click/error.hh>
#include <click/glue.hh>
#include "rtable1.hh"
#include "ipv4address.hh"

int Rtable1::_invocations = 0;

Rtable1::Rtable1() {
//     click_chatter("Rtable1::Constructor called %d", ++_invocations);
}

Rtable1::~Rtable1() {
//     click_chatter("Rtable1::Destructor called %d", _invocations--);
}

void
Rtable1::add(IPV4address dst, IPV4address mask, IPV4address gw, int index) {

    struct Entry e;
    e.dst = (dst & mask);
    e.mask = mask;
    e.gw = gw;
    e.index = index;

//    click_chatter("Rtable1::add(%s,", dst.string().cc());

    for(int i = 0; i < _v.size(); i++) {
	if(_v[i].empty()) {
	    _v[i] = e;
	    return;
	}
    }

    _v.push_back(e);
}

void
Rtable1::del(IPV4address dst, IPV4address mask) {

    for(int i = 0; i < _v.size(); i++) {
	if(_v[i].empty()) {
	    if((dst == _v[i].dst) && (mask == _v[i].mask)) {
		_v[i].clear();
		return;
	    }
	}
    }
}

bool
Rtable1::lookup(IPV4address dst, IPV4address& gw, int& index) const
{
    int best = -1;

    for(int i = 0; i < _v.size(); i++) {
	if(!_v[i].empty()) {
	    if(dst.matches_prefix(_v[i].dst, _v[i].mask)) {
		if(-1 == best) {
		    best = i;
		    continue;
		}
		if(_v[i].mask.longest_mask(_v[best].mask)) {
		    best = i;
		}
	    }
	}
    }
    
    if(-1 == best)
	return false;

    gw = _v[best].gw;
    index = _v[best].index;
    return true;
}

void
Rtable1::test()
{
#if	0
    int bogus_routes = 100000;

    click_chatter("Rtable1::test()");
    click_chatter("create %d bogus_routes", bogus_routes);
    
    for(int i = 0; i < bogus_routes; i++)
	add(0,0,0,0);
#endif
}
// generate Vector template instance
#include <click/vector.cc>

ELEMENT_PROVIDES(HoopyFrood)
