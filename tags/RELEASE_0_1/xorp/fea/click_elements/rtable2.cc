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

#ident "$XORP: xorp/fea/click_elements/rtable2.cc,v 1.4 2002/12/09 18:29:01 hodson Exp $"

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
// ALWAYS INCLUDE click/config.h
#include <click/config.h>
// ALWAYS INCLUDE click/package.hh
#include <click/package.hh>
#include <click/error.hh>
#include <click/glue.hh>
#include "rtable2.hh"
#include "ipv4address.hh"

#ifdef	TEST
int Rtable2::_invocations = 0;
#endif

Rtable2::Rtable2() {
#ifdef TEST
    click_chatter("Rtable2::Constructor called %d", ++_invocations);
#endif
    _dirty = false;
    _table_read = false;
}

Rtable2::~Rtable2() {
#ifdef	TEST
     click_chatter("Rtable2::Destructor called %d", _invocations--);
#endif
}

void
Rtable2::add(IPV4address dst, IPV4address mask, IPV4address gw, int index) {

    _dirty = true;

    struct Entry e;
    e.dst = (dst & mask);
    e.mask = mask;
    e.gw = gw;
    e.index = index;

//    click_chatter("Rtable2::add(%s,", dst.string().cc());

    for(int i = 0; i < _v.size(); i++) {
	if(_v[i].empty()) {
	    _v[i] = e;
	    return;
	}
    }

    _v.push_back(e);

}

void
Rtable2::del(IPV4address dst, IPV4address mask) 
{
    _dirty = true;

    for(int i = 0; i < _v.size(); i++) {
	if(!_v[i].empty()) {
	    if((dst == _v[i].dst) && (mask == _v[i].mask)) {
		_v[i].clear();
		return;
	    }
	}
    }

    click_chatter("Rtable2::del No match for %s/%s", dst.string().cc(),
		  mask.string().cc());
		  
}

bool
Rtable2::lookup_entry(IPV4address dst, Entry& entry) const
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

    entry = _v[best];
    return true;
}

bool
Rtable2::lookup(IPV4address dst, IPV4address& gw, int& index) const
{
    Entry entry;

    bool result;

    if(true == (result = lookup_entry(dst, entry))) {
	gw = entry.gw;
	index = entry.index;
    }

    return result;
}

bool
Rtable2::lookup(IPV4address dst, IPV4address& match, IPV4address& mask, 
		IPV4address& gw, int& index) const
{
    Entry entry;

    bool result;

    if(true == (result = lookup_entry(dst, entry))) {
	match = entry.dst;
	mask = entry.mask;
	gw = entry.gw;
	index = entry.index;
    }
    
    return result;
}

void
Rtable2::zero()
{
    for(int i = 0; i < _v.size(); i++)
	_v[i].clear();
}

bool
Rtable2::copy(Rtable2 *rt)
{
    zero();

    for(int i = 0; i < rt->_v.size(); i++)
	add(rt->_v[i].dst, rt->_v[i].mask, rt->_v[i].gw, rt->_v[i].index);

    return true;
}

bool
Rtable2::start_table_read()
{
    if(true == _table_read)
	return false;

    _dirty = false;

    _table_read = true;
    _index_table_read = 0;

    return true;
}

bool
Rtable2::next_table_read(IPV4address& dst, IPV4address &mask, IPV4address &gw, 
			 int &index, status &status)
{
    if(true != _table_read) {
	status = ILLEGAL_OPERATION;
	return false;
    }

    if(true == _dirty) {
	_table_read = false;

	status = TABLE_CHANGED;
	return false;
    }

//     click_chatter("Rtable2::next_table_read %d", _index_table_read);

    if((_index_table_read >= _v.size()) || _v[_index_table_read].empty()) {
	_table_read = false;

	status = OK;
	return false;
    }

    dst = _v[_index_table_read].dst;
    mask = _v[_index_table_read].mask;
    gw = _v[_index_table_read].gw;
    index = _v[_index_table_read].index;

    while(++_index_table_read < _v.size())
	if(!_v[_index_table_read].empty())
	    break;
	
    status = OK;
    return true;
}


void
Rtable2::test()
{
#ifdef	TEST
    int bogus_routes = 100000;

    click_chatter("Rtable2::test()");
    click_chatter("create %d bogus_routes", bogus_routes);
    
    for(int i = 0; i < bogus_routes; i++)
	add(0,0,0,0);
#endif
}
// generate Vector template instance
#include <click/vector.cc>

ELEMENT_PROVIDES(HoopyFrood)
