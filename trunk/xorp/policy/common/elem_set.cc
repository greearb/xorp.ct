// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/policy/common/elem_set.cc,v 1.2 2005/03/25 02:54:15 pavlin Exp $"

#include "config.h"
#include "elem_set.hh"
#include "policy_utils.hh"
#include <algorithm>

const char* ElemSet::id = "set";

ElemSet::ElemSet(const Set& val) : Element(id), _val(val) 
{
}

ElemSet::ElemSet(const char* c_str) : Element(id) 
{
    if(!c_str)
	return;

    policy_utils::str_to_set(c_str,_val);
}

ElemSet::ElemSet() : Element(id) 
{
}

string 
ElemSet::str() const 
{
    string s = "";

    if(!_val.size())
	return s;

    for(Set::iterator i = _val.begin(); i != _val.end(); ++i) {
	s += *i;
        s += ",";
    }

    // remove last comma
    s.erase(s.length()-1);

    return s;
}

void 
ElemSet::insert(const string& s) 
{
    _val.insert(s);
}

void
ElemSet::insert(const ElemSet& s)
{
    const Set other = s.get_set();
    _val.insert(other.begin(), other.end());
}

bool 
ElemSet::operator==(const ElemSet& rhs) const 
{
    return _val == rhs._val;
}

bool 
ElemSet::operator!=(const ElemSet& rhs) const 
{
    return !(*this == rhs);
}

bool 
ElemSet::operator<(const ElemSet& rhs) const 
{
    const Set& rset = rhs._val;

    // left has to be smaller
    if(_val.size() >= rset.size())
	return false;

    // for all elements on left to match, the intersection must be equal to
    // the left set.
    Set tmp;
    set_intersection(_val.begin(),_val.end(),rset.begin(),rset.end(),
        insert_iterator<Set>(tmp,tmp.begin()));

    return tmp == _val;
}

bool 
ElemSet::operator>(const ElemSet& rhs) const 
{
    return (rhs < *this);
}

bool 
ElemSet::operator<=(const ElemSet& rhs) const 
{
    return (*this < rhs || *this == rhs);
}

bool 
ElemSet::operator>=(const ElemSet& rhs) const 
{
    return (*this > rhs || *this == rhs);
}

bool 
ElemSet::operator<(const Element& /* rhs */) const 
{
    return _val.empty();
}

bool 
ElemSet::operator>(const Element& rhs) const 
{
    Set::iterator i = _val.find(rhs.str());

    if(i == _val.end())
	return false;

    // left has to have at least 2 elements 
    // [so it has more than 1 elements -- size of rhs].
    if(_val.size() < 2)
	return false;

    return true;
}

bool 
ElemSet::operator==(const Element& rhs) const 
{
    if(_val.size() != 1)
	return false;

    if(_val.find(rhs.str()) == _val.end())
	return false;

    return true;
}

bool 
ElemSet::operator!=(const Element& rhs) const 
{
    if(_val.find(rhs.str()) == _val.end())
	return true;

    return false;
}


bool 
ElemSet::operator<=(const Element& rhs) const 
{
    return (*this < rhs || *this == rhs);
}

bool 
ElemSet::operator>=(const Element& rhs) const 
{
    return (*this > rhs || *this == rhs);
}

const ElemSet::Set&
ElemSet::get_set() const
{
    return _val;
}

bool
ElemSet::nonempty_intersection(const ElemSet& rhs) const
{
    Set tmp;
    set_intersection(_val.begin(), _val.end(),
		     rhs._val.begin(), rhs._val.end(),
		     insert_iterator<Set>(tmp,tmp.begin()));

    return tmp.size();
}

void
ElemSet::erase(const ElemSet& rhs)
{
    const Set& s = rhs.get_set();

    // go through all elements and delete ones present
    for (Set::const_iterator i = s.begin(); i != s.end(); ++i) {
	Set::iterator j = _val.find(*i);

	if (j != _val.end())
	    _val.erase(j);
    }
}
