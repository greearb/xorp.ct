// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ident "$XORP: xorp/policy/backend/policytags.cc,v 1.15 2008/08/06 08:24:11 abittau Exp $"

#include <sstream>

#include "libxorp/xorp.h"
#include "policytags.hh"
#include "policy/common/elem_set.hh"
#include "libxipc/xrl_atom.hh"

PolicyTags::PolicyTags() : _tag(0)
{
}

PolicyTags::PolicyTags(const XrlAtomList& alist) : _tag(0)
{
    // first is always tag
    XLOG_ASSERT(alist.size() > 0);

    // go through all the atoms in the list
    for (unsigned i = 0; i < alist.size(); ++i) {
	const XrlAtom& atom = alist.get(i);

	// only support u32's
	if (atom.type() != xrlatom_uint32)
	    xorp_throw(PolicyTagsError, "XrlAtomList does not contain uint32's");

	uint32_t val = atom.uint32();

	if (i == 0) {
	    _tag = val;
	    continue;
	}

	// it's good, insert it
	_tags.insert(val);
    }
}

void
PolicyTags::set_ptags(const Element& element)
{
    // we only support set elements
    const ElemSetU32* es = dynamic_cast<const ElemSetU32*>(&element);
    if (!es)
	xorp_throw(PolicyTagsError, 
		   string("Element is not a set: ") + element.type());

    _tags.clear();

    // go through all the set elements.
    for (ElemSetU32::const_iterator i = es->begin(); i != es->end(); ++i) {
	const ElemU32& x = *i;

	// all ElemSet elements are represented as string, so convert and
	// insert.
	_tags.insert(x.val());
    }
}

string
PolicyTags::str() const
{
    ostringstream oss;

    for(Set::const_iterator i = _tags.begin(); i != _tags.end(); ++i)
	oss << *i << ", ";

    string res = oss.str();
    
    unsigned len = res.length();
    if(len < 2)
	return res;

    // kill last ", "
    res.erase(res.length()-2);
    return res;
}

bool
PolicyTags::operator==(const PolicyTags& rhs) const
{
    return (_tags == rhs._tags) && (_tag == rhs._tag);
}

XrlAtomList
PolicyTags::xrl_atomlist() const
{
    XrlAtomList alist;

    alist.append(XrlAtom(_tag));

    for(Set::const_iterator i = _tags.begin(); i != _tags.end(); ++i) {
	uint32_t tag = *i;

	alist.append(XrlAtom(tag));
    }
    
    return alist;
}

Element*
PolicyTags::element() const
{
    ElemSetU32* s = new ElemSetU32;
    for (Set::const_iterator i = _tags.begin(); i != _tags.end(); ++i) {
	ElemU32 e(*i);
	s->insert(e);
    }
    return s;
}

Element*
PolicyTags::element_tag() const
{
    return new ElemU32(_tag);
}

void
PolicyTags::set_tag(const Element& e)
{
    uint32_t val = dynamic_cast<const ElemU32&>(e).val();

    _tag = val;
}

void
PolicyTags::insert(const PolicyTags& ptags)
{
    // go through all the elements in ptags and insert them.
    for(Set::const_iterator i = ptags._tags.begin();
	i != ptags._tags.end(); ++i)

	_tags.insert(*i);
}

bool
PolicyTags::contains_atleast_one(const PolicyTags& tags) const
{
    Set output;

    // The two sets must not be dis-joint.
    // The intersection must contain atleast one element.
    set_intersection(tags._tags.begin(),tags._tags.end(),
		     _tags.begin(),_tags.end(),
		     insert_iterator<Set>(output,output.begin()));
	
    return !output.empty();
}

void
PolicyTags::insert(uint32_t tag)
{
    _tags.insert(tag);
}
