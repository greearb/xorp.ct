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

#ident "$XORP: xorp/bgp/path_attribute_list.cc,v 1.1.1.1 2002/12/11 23:55:49 hodson Exp $"

//#define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "path_attribute_list.hh"

#define PARANOID

template<class A>
PathAttributeList<A>::PathAttributeList<A>()
{
    _nexthop_att = NULL;
    _aspath_att = NULL;
    _origin_att = NULL;
    memset(_hash, 0, 16);
}

template<class A>
PathAttributeList<A>::
  PathAttributeList<A>(const NextHopAttribute<A> &nexthop_att,
			  const ASPathAttribute &aspath_att,
			  const OriginAttribute &origin_att)
{
    add_path_attribute(origin_att);
    add_path_attribute(nexthop_att);
    add_path_attribute(aspath_att);
    rehash();
}

template<class A>
PathAttributeList<A>::
PathAttributeList<A>(const PathAttributeList<A>& palist)
{
    _nexthop_att = NULL;
    _aspath_att = NULL;
    _origin_att = NULL;
    list<const PathAttribute*>::const_iterator i;
    i = palist._att_list.begin();
    while (i != palist._att_list.end()) {
	add_path_attribute(**i);
	++i;
    }
    rehash();
}

template<class A>
PathAttributeList<A>::~PathAttributeList<A>()
{
    typedef list<const PathAttribute*>::const_iterator Iter;
    Iter i = _att_list.begin();
    while (i != _att_list.end()) {
	delete (*i);
	++i;
    }
    _nexthop_att = (NextHopAttribute<A>*)0xd0d0;
    _aspath_att = (ASPathAttribute*)0xd0d0;
    _origin_att = (OriginAttribute*)0xd0d0;
}

template<class A>
void
PathAttributeList<A>::add_path_attribute(const PathAttribute &att)
{
    PathAttribute *new_att = NULL;
    switch (att.type()) {
    case ORIGIN:
	_origin_att = new OriginAttribute((const OriginAttribute&)att);
	new_att = _origin_att;
	break;
    case AS_PATH:
	_aspath_att = new ASPathAttribute((const ASPathAttribute&)att);
	new_att = _aspath_att;
	break;
    case NEXT_HOP:
	_nexthop_att =
	    new NextHopAttribute<A>((const NextHopAttribute<A>&)att);
	new_att = _nexthop_att;
	break;
    case MED:
	new_att = new MEDAttribute((const MEDAttribute&)att);
	break;
    case LOCAL_PREF:
	new_att = new LocalPrefAttribute((const LocalPrefAttribute&)att);
	break;
    case ATOMIC_AGGREGATE:
	new_att = new AtomicAggAttribute((const AtomicAggAttribute&)att);
	break;
    case AGGREGATOR:
	new_att =
	    new AggregatorAttribute((const AggregatorAttribute&)att);
	break;
    case COMMUNITY:
	new_att =
	    new CommunityAttribute((const CommunityAttribute&)att);
	break;
    case UNKNOWN:
	new_att =
	    new UnknownAttribute((const UnknownAttribute&)att);
	break;
    }
    if (new_att != NULL) {
	// Keep the list sorted
	if (_att_list.empty()) {
	    _att_list.push_back(new_att);
	} else {
	    list <const PathAttribute*>::iterator i;
	    for (i = _att_list.begin(); i != _att_list.end(); i++) {
		if ((*i)->sorttype() > att.sorttype()) {
		    _att_list.insert(i, new_att);
		    memset(_hash, 0, 16);
		    return;
		}
	    }
	    _att_list.push_back(new_att);
	}
    }
    memset(_hash, 0, 16);
}

template<class A>
string
PathAttributeList<A>::str() const
{
    string s;
#if 0
    s = _nexthop_att->str() + "\n";
    s += _aspath_att->str() + "\n";
    s += _origin_att->str() + "\n";
#endif
    typedef list<const PathAttribute*>::const_iterator Iter;
    Iter i = _att_list.begin();
    while (i != _att_list.end()) {
	s += (*i)->str() + "\n";
	++i;
    }
    return s;
}

template<class A>
void
PathAttributeList<A>::rehash()
{
    MD5_CTX context;
    MD5Init(&context);
    list<const PathAttribute*>::const_iterator i;
    for (i = _att_list.begin(); i != _att_list.end(); i++) {
	(*i)->add_hash(&context);
    }
    MD5Final(_hash, &context);
}

template<class A>
bool
PathAttributeList<A>::
operator< (const PathAttributeList<A> &him) const
{
    assert_rehash();
    list<const PathAttribute*>::const_iterator my_i;
    my_i = _att_list.begin();
    list<const PathAttribute*>::const_iterator his_i;
    his_i = him.att_list().begin();
    while (1) {
	// are they equal.
	if ((my_i == _att_list.end()) && (his_i == him.att_list().end())) {
	    return false;
	}
	if (my_i == _att_list.end()) {
	    return true;
	}
	if (his_i == him.att_list().end()) {
	    return false;
	}
	if ((**my_i) < (**his_i)) {
	    return true;
	}
	if ((**his_i) < (**my_i)) {
	    return false;
	}
	my_i++;
	his_i++;
    }
    abort();
}

template<class A>
bool
PathAttributeList<A>::
operator == (const PathAttributeList<A> &him) const
{
    assert_rehash();
    if (memcmp(_hash, him.hash(), 16) == 0)
	return true;
    return false;
}

template<class A>
void
PathAttributeList<A>::replace_attribute(const PathAttribute* new_att,
					PathAttType type)
{
    debug_msg("Replace attribute\n");
    debug_msg("Before: \n%s\n", str().c_str());
    debug_msg("New Att: %s\n", new_att->str().c_str());
    typedef list<const PathAttribute*>::iterator Iter;
    Iter i = _att_list.begin();
    while (i != _att_list.end()) {
	if ((*i)->type() == type) {
	    _att_list.insert(i, new_att);
	    delete (*i);
	    _att_list.erase(i);
	    debug_msg("After: \n%s\n", str().c_str());
	    memset(_hash, 0, 16);
	    return;
	}
	++i;
    }
    abort();
}

template<class A>
void
PathAttributeList<A>::replace_AS_path(const AsPath& new_as_path)
{
    ASPathAttribute *aspath_att = new ASPathAttribute(new_as_path);
    replace_attribute(aspath_att, AS_PATH);
}

template<class A>
void
PathAttributeList<A>::replace_nexthop(const A& new_nexthop)
{
    NextHopAttribute<A> *nh_att = new NextHopAttribute<A>(new_nexthop);
    replace_attribute(nh_att, NEXT_HOP);
}

template<class A>
void
PathAttributeList<A>::remove_attribute_by_type(PathAttType type)
{
    // we only remove the first instance of an attribute with matching type
    list<const PathAttribute*>::iterator i;
    for (i = _att_list.begin(); i != _att_list.end(); i++) {
	if ((*i)->type() == type) {
	    delete *i;
	    _att_list.erase(i);
	    memset(_hash, 0, 16);
	    return;
	}
    }
}

template<class A>
void
PathAttributeList<A>::assert_rehash() const
{
#ifdef PARANOID
    for (int i = 0; i<16; i++)
	if (_hash[i]!=0)
	    return;
    XLOG_FATAL("Missing rehash - attempted to use modified PathAttributeList without first calling rehash()\n");
#endif
}

template class PathAttributeList<IPv4>;
template class PathAttributeList<IPv6>;
