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

#ident "$XORP: xorp/bgp/path_attribute_list.cc,v 1.8 2003/01/31 23:39:40 mjh Exp $"

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
    // no need to clear the *_att pointers, will be done by these 3 adds.
    add_path_attribute(origin_att);
    add_path_attribute(nexthop_att);
    add_path_attribute(aspath_att);
    rehash();
}

template<class A>
PathAttributeList<A>::PathAttributeList<A>(const PathAttributeList<A>& palist)
{
    _nexthop_att = NULL;
    _aspath_att = NULL;
    _origin_att = NULL;
    const_iterator i = palist._att_list.begin();
    for ( ; i != palist._att_list.end() ; ++i)
	add_path_attribute(**i);
    rehash();
}

template<class A>
PathAttributeList<A>::~PathAttributeList<A>()
{
    debug_msg("++ ~PathAttributeList delete:\n%s\n", str().c_str());
    for (const_iterator i = _att_list.begin(); i != _att_list.end() ; ++i)
	delete (*i);
}

template<class A>
void
PathAttributeList<A>::add_path_attribute(const PathAttribute &att)
{
    size_t l;
    PathAttribute *a = PathAttribute::create(att.data(), att.wire_size(), l);
    // store a reference to the mandatory attributes, ignore others
    switch (att.type()) {
    default:
	break;

    case ORIGIN:
	_origin_att = (OriginAttribute *)a;
	break;

    case AS_PATH:
	_aspath_att = (ASPathAttribute *)a;
	break;

    case NEXT_HOP:
	_nexthop_att = (NextHopAttribute<A> *)a;
	break;
    }
    // Keep the list sorted
    debug_msg("++ add_path_attribute %s\n", att.str().c_str());
    if (!_att_list.empty()) {
	iterator i;
	for (i = _att_list.begin(); i != _att_list.end(); i++)
	    if ( *(*i) > *a) {
		_att_list.insert(i, a);
		memset(_hash, 0, 16);
		return;
	    }
    }
    // list empty, or tail insertion:
    _att_list.push_back(a);
    memset(_hash, 0, 16);
}

template<class A>
string
PathAttributeList<A>::str() const
{
    string s;
    for (const_iterator i = _att_list.begin(); i != _att_list.end() ; ++i)
	s += (*i)->str() + "\n";
    return s;
}

template<class A>
void
PathAttributeList<A>::rehash()
{
    MD5_CTX context;
    MD5Init(&context);
    for (const_iterator i = _att_list.begin(); i != _att_list.end(); i++)
	(*i)->add_hash(&context);
    MD5Final(_hash, &context);
}

template<class A>
bool
PathAttributeList<A>::
operator< (const PathAttributeList<A> &him) const
{
    assert_rehash();
    him.assert_rehash();
    debug_msg("PathAttributeList operator< %p %p\n", this, &him);

    if ((*_nexthop_att) < (*(him._nexthop_att)))
        return true;
    if ((*(him._nexthop_att)) < (*_nexthop_att))
        return false;
    if (_att_list.size() < him._att_list.size())
        return true;
    if (him._att_list.size() < _att_list.size())
        return false;

    //    return (memcmp(_hash, him.hash(), 16) < 0);
    const_iterator my_i = _att_list.begin();
    const_iterator his_i = him.att_list().begin();
    for (;;) {
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
    debug_msg("PathAttributeList operator== %p %p\n", this, &him);
    assert_rehash();
    him.assert_rehash();
    return (memcmp(_hash, him.hash(), 16) == 0);
}

template<class A>
void
PathAttributeList<A>::replace_attribute(PathAttribute* new_att)
{
    PathAttType type = new_att->type();

    switch (new_att->type()) {
    default:
	break;

    case ORIGIN:
	_origin_att = (OriginAttribute *)new_att;
	break;

    case AS_PATH:
	_aspath_att = (ASPathAttribute *)new_att;
	break;

    case NEXT_HOP:
	_nexthop_att = (NextHopAttribute<A> *)new_att;
	break;
    }

    debug_msg("Replace attribute\n");
    debug_msg("Before: \n%s\n", str().c_str());
    debug_msg("New Att: %s\n", new_att->str().c_str());
    for (iterator i = _att_list.begin(); i != _att_list.end() ; ++i)
	if ((*i)->type() == type) {
	    _att_list.insert(i, new_att);
	    delete (*i);
	    _att_list.erase(i);
	    debug_msg("After: \n%s\n", str().c_str());
	    memset(_hash, 0, 16);
	    return;
	}
    abort();
}

template<class A>
void
PathAttributeList<A>::replace_AS_path(const AsPath& new_as_path)
{
    replace_attribute(new ASPathAttribute(new_as_path));
}

template<class A>
void
PathAttributeList<A>::replace_nexthop(const A& new_nexthop)
{
    replace_attribute(new NextHopAttribute<A>(new_nexthop));
}

template<class A>
void
PathAttributeList<A>::remove_attribute_by_type(PathAttType type)
{
    // we only remove the first instance of an attribute with matching type
    iterator i;
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
const PathAttribute*
PathAttributeList<A>::find_attribute_by_type(PathAttType type) const
{
    const_iterator i;
    for (i = _att_list.begin(); i != _att_list.end(); i++)
	if ((*i)->type() == type)
	    return *i;
    return NULL;
}

template<class A>
const MEDAttribute* 
PathAttributeList<A>::med_att() const 
{
    return dynamic_cast<const MEDAttribute*>(find_attribute_by_type(MED));
}

template<class A>
const LocalPrefAttribute*
PathAttributeList<A>::local_pref_att() const 
{
    return dynamic_cast<const LocalPrefAttribute*>(
		find_attribute_by_type(LOCAL_PREF));
}

template<class A>
const AtomicAggAttribute*
PathAttributeList<A>::atomic_aggregate_att() const 
{
    return dynamic_cast<const AtomicAggAttribute*>(
	find_attribute_by_type(ATOMIC_AGGREGATE));
}

template<class A>
const AggregatorAttribute*
PathAttributeList<A>::aggregator_att() const 
{
    return dynamic_cast<const AggregatorAttribute*>(
	find_attribute_by_type(AGGREGATOR));
}

/*
 * check whether the class has been properly rehashed.
 * A _hash of all 0's means with high probability that we forgot to
 * recompute it.
 */
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
