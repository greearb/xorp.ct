// vim:set sts=4 ts=8:

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

#ident "$XORP$"

#include "config.h"
#include "elem_set.hh"
#include "policy_utils.hh"

#include <algorithm>

const char* ElemSet::id = "set";

ElemSet::ElemSet(const set<string>& val) : Element(id), _val(val) 
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
ElemSet::str() const {
    string s = "";

    if(!_val.size())
	return s;

    for(set<string>::iterator i = _val.begin(); i != _val.end(); ++i) {
	s += *i;
        s += ",";
    }

    // remove last comma
    s.erase(s.length()-1);

    return s;
}

void 
ElemSet::insert(const string& s) {
    _val.insert(s);
}

bool 
ElemSet::operator==(const ElemSet& rhs) const {
    return _val == rhs._val;
}

bool 
ElemSet::operator!=(const ElemSet& rhs) const {
    return !(*this == rhs);
}

bool 
ElemSet::operator<(const ElemSet& rhs) const {
    const set<string>& rset = rhs._val;

    // left has to be smaller
    if(_val.size() >= rset.size())
	return false;

    // for all elements on left to match, the intersection must be equal to
    // the left set.
    set<string> tmp;
    set_intersection(_val.begin(),_val.end(),rset.begin(),rset.end(),
        insert_iterator<set<string> >(tmp,tmp.begin()));

    return tmp == _val;
}

bool 
ElemSet::operator>(const ElemSet& rhs) const {
    return (rhs < *this);
}

bool 
ElemSet::operator<=(const ElemSet& rhs) const {
    return (*this < rhs || *this == rhs);
}

bool 
ElemSet::operator>=(const ElemSet& rhs) const {
    return (*this > rhs || *this == rhs);
}

bool 
ElemSet::operator<(const Element& /* rhs */) const {
    return _val.empty();
}

bool 
ElemSet::operator>(const Element& rhs) const {
    set<string>::iterator i = _val.find(rhs.str());

    if(i == _val.end())
	return false;

    // left has to have at least 2 elements 
    // [so it has more than 1 elements -- size of rhs].
    if(_val.size() < 2)
	return false;

    return true;
}

bool 
ElemSet::operator==(const Element& rhs) const {
    if(_val.size() != 1)
	return false;

    if(_val.find(rhs.str()) == _val.end())
	return false;

    return true;
}

bool 
ElemSet::operator!=(const Element& rhs) const {
    if(_val.find(rhs.str()) == _val.end())
	return true;

    return false;
}


bool 
ElemSet::operator<=(const Element& rhs) const {
    return (*this < rhs || *this == rhs);
}

bool 
ElemSet::operator>=(const Element& rhs) const {
    return (*this > rhs || *this == rhs);
}
