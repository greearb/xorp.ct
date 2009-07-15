// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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



#include "libxorp/xorp.h"

#include <algorithm>

#include "elem_set.hh"
#include "policy_utils.hh"


template <class T>
ElemSetAny<T>::ElemSetAny(const Set& val) : ElemSet(_hash), _val(val) 
{
}

template <class T>
ElemSetAny<T>::ElemSetAny(const char* c_str) : ElemSet(_hash)
{
    if (!c_str)
	return;

    // create each element in the list
    set<string> s;
    policy_utils::str_to_set(c_str, s);

    for (set<string>::iterator i = s.begin(); i != s.end(); ++i) {
	const char* str = (*i).c_str();
	_val.insert(T(str));
    }
}

template <class T>
ElemSetAny<T>::ElemSetAny() : ElemSet(_hash)
{
}

template <class T>
string 
ElemSetAny<T>::str() const 
{
    string s = "";

    if (!_val.size())
	return s;

    for (typename Set::const_iterator i = _val.begin(); i != _val.end(); ++i) {
	s += (*i).str();
        s += ",";
    }

    // remove last comma
    s.erase(s.length()-1);

    return s;
}

template <class T>
void 
ElemSetAny<T>::insert(const T& s) 
{
    _val.insert(s);
}

template <class T>
void
ElemSetAny<T>::insert(const ElemSetAny<T>& s)
{
    _val.insert(s._val.begin(), s._val.end());
}

template <class T>
bool 
ElemSetAny<T>::operator==(const ElemSetAny<T>& rhs) const 
{
    return _val == rhs._val;
}

template <class T>
bool 
ElemSetAny<T>::operator!=(const ElemSetAny<T>& rhs) const 
{
    return !(*this == rhs);
}

template <class T>
bool 
ElemSetAny<T>::operator<(const ElemSetAny<T>& rhs) const 
{
    const Set& rset = rhs._val;

    // left has to be smaller
    if (_val.size() >= rset.size())
	return false;

    // for all elements on left to match, the intersection must be equal to
    // the left set.
    Set tmp;
    set_intersection(_val.begin(), _val.end(), rset.begin(), rset.end(),
        insert_iterator<Set>(tmp, tmp.begin()));

    return tmp == _val;
}

template <class T>
bool 
ElemSetAny<T>::operator>(const ElemSetAny<T>& rhs) const 
{
    return (rhs < *this);
}

template <class T>
bool 
ElemSetAny<T>::operator<=(const ElemSetAny<T>& rhs) const 
{
    return (*this < rhs || *this == rhs);
}

template <class T>
bool 
ElemSetAny<T>::operator>=(const ElemSetAny<T>& rhs) const 
{
    return (*this > rhs || *this == rhs);
}

template <class T>
bool 
ElemSetAny<T>::operator<(const T& /* rhs */) const 
{
    return _val.empty();
}

template <class T>
bool 
ElemSetAny<T>::operator>(const T& rhs) const 
{
    typename Set::const_iterator i = _val.find(rhs);

    if (i == _val.end())
	return false;

    // left has to have at least 2 elements 
    // [so it has more than 1 elements -- size of rhs].
    if (_val.size() < 2)
	return false;

    return true;
}

template <class T>
bool 
ElemSetAny<T>::operator==(const T& rhs) const 
{
    if (_val.size() != 1)
	return false;

    if (_val.find(rhs) == _val.end())
	return false;

    return true;
}

template <class T>
bool 
ElemSetAny<T>::operator!=(const T& rhs) const 
{
    if (_val.find(rhs) == _val.end())
	return true;

    return false;
}


template <class T>
bool 
ElemSetAny<T>::operator<=(const T& rhs) const 
{
    return (*this < rhs || *this == rhs);
}

template <class T>
bool 
ElemSetAny<T>::operator>=(const T& rhs) const 
{
    return (*this > rhs || *this == rhs);
}

template <class T>
bool
ElemSetAny<T>::nonempty_intersection(const ElemSetAny<T>& rhs) const
{
    Set tmp;
    set_intersection(_val.begin(), _val.end(),
		     rhs._val.begin(), rhs._val.end(),
		     insert_iterator<Set>(tmp,tmp.begin()));

    return tmp.size();
}

template <class T>
void
ElemSetAny<T>::erase(const ElemSetAny<T>& rhs)
{
    // go through all elements and delete ones present
    for (typename Set::const_iterator i = rhs._val.begin(); 
	 i != rhs._val.end(); ++i) {

	typename Set::iterator j = _val.find(*i);

	if (j != _val.end())
	    _val.erase(j);
    }
}

template <class T>
void
ElemSetAny<T>::erase(const ElemSet& rhs)
{
    erase(dynamic_cast<const ElemSetAny<T>&>(rhs));
}

template <class T>
typename ElemSetAny<T>::iterator
ElemSetAny<T>::begin()
{
    return _val.begin();
}

template <class T>
typename ElemSetAny<T>::iterator
ElemSetAny<T>::end()
{
    return _val.end();
}

template <class T>
typename ElemSetAny<T>::const_iterator
ElemSetAny<T>::begin() const
{
    return _val.begin();
}

template <class T>
typename ElemSetAny<T>::const_iterator
ElemSetAny<T>::end() const
{
    return _val.end();
}

template <class T>
const char*
ElemSetAny<T>::type() const
{
    return id;
}

// define the various sets
template <> const char* ElemSetU32::id = "set_u32";
template <> Element::Hash ElemSetU32::_hash = HASH_ELEM_SET_U32;
template class ElemSetAny<ElemU32>;

template <> const char* ElemSetCom32::id = "set_com32";
template <> Element::Hash ElemSetCom32::_hash = HASH_ELEM_SET_COM32;
template class ElemSetAny<ElemCom32>;

template <> const char* ElemSetIPv4Net::id = "set_ipv4net";
template <> Element::Hash ElemSetIPv4Net::_hash = HASH_ELEM_SET_IPV4NET;
template class ElemSetAny<ElemIPv4Net>;

template <> const char* ElemSetIPv6Net::id = "set_ipv6net";
template <> Element::Hash ElemSetIPv6Net::_hash = HASH_ELEM_SET_IPV6NET;
template class ElemSetAny<ElemIPv6Net>;

template <> const char* ElemSetStr::id = "set_str";
template <> Element::Hash ElemSetStr::_hash = HASH_ELEM_SET_STR;
template class ElemSetAny<ElemStr>;
