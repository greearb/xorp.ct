// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/policy/backend/policytags.cc,v 1.13 2008/01/04 03:17:16 pavlin Exp $"

#include "libxorp/xorp.h"

#include "policytags.hh"
#include "policy/common/elem_set.hh"
#include "libxipc/xrl_atom.hh"
#include <sstream>

PolicyTags::PolicyTags()
{
}

PolicyTags::PolicyTags(const XrlAtomList& alist)
{
    // go through all the atoms in the list
    for(unsigned i = 0; i < alist.size(); ++i) {
	const XrlAtom& atom = alist.get(i);

	// only support u32's
	if(atom.type() != xrlatom_uint32)
	    xorp_throw(PolicyTagsError, "XrlAtomList does not contain uint32's");

	// it's good, insert it
	_tags.insert(atom.uint32());    
    }
}

PolicyTags::PolicyTags(const Element& element)
{
    // we only support set elements
    const ElemSetU32* es = dynamic_cast<const ElemSetU32*>(&element);
    if(!es)
	xorp_throw(PolicyTagsError, 
		   string("Element is not a set: ") + element.type());

    // go through all the set elements.
    for(ElemSetU32::const_iterator i = es->begin(); i != es->end(); ++i) {
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
    // just check if the set of tags is equal.
    return _tags == rhs._tags;
}

XrlAtomList
PolicyTags::xrl_atomlist() const
{
    XrlAtomList alist;

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
