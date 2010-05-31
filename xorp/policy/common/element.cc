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



#include "element.hh"
#include "elem_null.hh"
#include "elem_filter.hh"
#include "elem_bgp.hh"
#include "policy_exception.hh"
#include "operator.hh"

// Initialization of static members.
// Remember to be unique in id's.
const char*            ElemInt32::id          = "i32";
const char*            ElemU32::id	      = "u32";
const char*            ElemCom32::id          = "com32";
const char*            ElemStr::id	      = "txt";
const char*            ElemBool::id	      = "bool";
const char*	       ElemNull::id	      = "null";
template<> const char* ElemIPv4::id	      = "ipv4";
template<> const char* ElemIPv4Range::id      = "ipv4range";
template<> const char* ElemIPv6::id	      = "ipv6";
template<> const char* ElemIPv6Range::id      = "ipv6range";
template<> const char* ElemIPv4Net::id	      = "ipv4net";
template<> const char* ElemIPv6Net::id	      = "ipv6net";
template<> const char* ElemU32Range::id	      = "u32range";
template<> const char* ElemASPath::id	      = "aspath";
template<> const char* ElemIPv4NextHop::id    = "ipv4nexthop";
template<> const char* ElemIPv6NextHop::id    = "ipv6nexthop";

Element::Hash		 ElemU32::_hash	        = HASH_ELEM_U32;
Element::Hash		 ElemInt32::_hash       = HASH_ELEM_INT32;
Element::Hash		 ElemCom32::_hash       = HASH_ELEM_COM32;
Element::Hash		 ElemStr::_hash	        = HASH_ELEM_STR;
Element::Hash		 ElemBool::_hash        = HASH_ELEM_BOOL;
Element::Hash		 ElemNull::_hash        = HASH_ELEM_NULL;
Element::Hash		 ElemFilter::_hash      = HASH_ELEM_FILTER;
template<> Element::Hash ElemIPv4::_hash        = HASH_ELEM_IPV4;
template<> Element::Hash ElemIPv4Range::_hash   = HASH_ELEM_IPV4RANGE;
template<> Element::Hash ElemIPv6::_hash        = HASH_ELEM_IPV6;
template<> Element::Hash ElemIPv6Range::_hash   = HASH_ELEM_IPV6RANGE;
template<> Element::Hash ElemIPv4Net::_hash     = HASH_ELEM_IPV4NET;
template<> Element::Hash ElemIPv6Net::_hash     = HASH_ELEM_IPV6NET;
template<> Element::Hash ElemU32Range::_hash    = HASH_ELEM_U32RANGE;
template<> Element::Hash ElemASPath::_hash      = HASH_ELEM_ASPATH;
template<> Element::Hash ElemIPv4NextHop::_hash = HASH_ELEM_IPV4NEXTHOP;
template<> Element::Hash ElemIPv6NextHop::_hash = HASH_ELEM_IPV6NEXTHOP;

/**
 * @short Well-known communities per RFC1997
 */
static struct { string text; uint32_t value; } com_aliases[] = {
    { "NO_EXPORT", 0xFFFFFF01 },
    { "NO_ADVERTISE", 0xFFFFFF02 },
    { "NO_EXPORT_SUBCONFED", 0xFFFFFF03 },
    { "", 0 }
};

/**
 * @short Element constructor with a parser for a BGP community syntax.
 *    "N" -> (uint32_t) N
 *   ":N" -> (uint16_t) N
 *  "N:"  -> ((uint16_t) N) << 16
 *  "N:M" -> (((uint16_t) N) << 16) + (uint16_t) M
 */
ElemCom32::ElemCom32(const char* c_str) : Element(_hash) {
    // Semantic checker needs this
    if(c_str == NULL) {
	_val = 0;
	return;
    }

    int len = strlen(c_str);
    char *colon = strstr(const_cast<char*>(c_str), ":");

    if(len > 0 && colon != NULL) {
	uint32_t msw, lsw;
	msw = strtoul(c_str, NULL, 0);
	lsw = strtoul(++colon, NULL, 0);
	if (msw > 0xffff || lsw > 0xffff)
	    xorp_throw(PolicyException, "uint16_t overflow for community " +
		       string(c_str));
	_val = (msw << 16) + lsw;
    } else {
	string x = string(c_str);

	_val = strtoul(c_str, NULL, 0);
	for(int i = 0; com_aliases[i].text.length(); i++)
	    if (com_aliases[i].text == x) {
		_val = com_aliases[i].value;
		break;
	    }
    }
}

string
ElemCom32::str() const {
    for(int i = 0; com_aliases[i].text.length(); i++)
	if (com_aliases[i].value == _val)
	    return com_aliases[i].text;
    ostringstream oss;
    oss << (_val >> 16) << ":" << (_val & 0xffff);
    return (oss.str());
}

template<class A>
ElemNet<A>::ElemNet() : Element(_hash), _net(NULL), _mod(MOD_NONE), _op(NULL)
{
    _net = new A();
}

template<class A>
ElemNet<A>::ElemNet(const char* str) : Element(_hash), _net(NULL),
				       _mod(MOD_NONE), _op(NULL)
{
    if (!str) {
	_net = new A();

	return;
    }

    // parse modifier
    string in = str;

    const char* p = strchr(str, ' ');
    if (p) {
	in = in.substr(0, p - str);

	_mod = str_to_mod(++p);
    }

    // parse net
    try {
	    _net = new A(in.c_str());
    } catch(...) {
	ostringstream oss;

	oss << "Can't init " << id << " using " << in;

	xorp_throw(PolicyException, oss.str());
    }
}

template<class A>
ElemNet<A>::ElemNet(const A& net) : Element(_hash), _net(NULL), _mod(MOD_NONE),
				    _op(NULL)
{
    _net = new A(net);
}

template<class A>
ElemNet<A>::ElemNet(const ElemNet<A>& net) : Element(_hash),
					     _net(net._net),
					     _mod(net._mod),
					     _op(NULL)
{
    if (_net)
	_net = new A(*_net);
}

template<class A>
ElemNet<A>::~ElemNet()
{
    delete _net;
}

template<class A>
string
ElemNet<A>::str() const
{
    string str = _net->str();

    if (_mod != MOD_NONE) {
	str += " ";
	str += mod_to_str(_mod);
    }

    return str;
}

template<class A>
const char*
ElemNet<A>::type() const
{
    return id;
}

template<class A>
const A&
ElemNet<A>::val() const
{
    return *_net;
}

template<class A>
bool
ElemNet<A>::operator<(const ElemNet<A>& rhs) const
{
    return *_net < *rhs._net;
}

template<class A>
bool
ElemNet<A>::operator==(const ElemNet<A>& rhs) const
{
    return *_net == *rhs._net;
}

template<class A>
typename ElemNet<A>::Mod
ElemNet<A>::str_to_mod(const char* p)
{
    string in = p;

    if (!in.compare("<=") || !in.compare("orlonger")) {
	return MOD_ORLONGER;

    } else if (!in.compare("<") || !in.compare("longer")) {
	return MOD_LONGER;

    } else if (!in.compare(">") || !in.compare("shorter")) {
	return MOD_SHORTER;

    } else if (!in.compare(">=") || !in.compare("orshorter")) {
	return MOD_ORSHORTER;

    } else if (!in.compare("!=") || !in.compare("not")) {
	return MOD_NOT;

    } else if (!in.compare("==") || !in.compare(":") || !in.compare("exact")) {
	return MOD_EXACT;
    
    } else {
	string err = "Can't parse modifier: " + in;

	xorp_throw(PolicyException, err);
    }

    // unreach
    abort();
}

template<class A>
string
ElemNet<A>::mod_to_str(Mod mod)
{
    switch (mod) {
    case MOD_NONE:
	return "";

    case MOD_EXACT:
	return "==";

    case MOD_SHORTER:
	return ">";

    case MOD_ORSHORTER:
	return ">=";

    case MOD_LONGER:
	return "<";

    case MOD_ORLONGER:
	return "<=";

    case MOD_NOT:
	return "!=";
    }

    // unreach
    abort();
}

template<class A>
BinOper&
ElemNet<A>::op() const
{
    static OpEq EQ;
    static OpNe NE;
    static OpLt LT;
    static OpLe LE;
    static OpGt GT;
    static OpGe GE;

    if (_op)
	return *_op;

    switch (_mod) {
    case MOD_NONE:
    case MOD_EXACT:
	_op = &EQ;
	break;

    case MOD_NOT:
	_op = &NE;
	break;

    case MOD_SHORTER:
	_op = &GT;
	break;

    case MOD_ORSHORTER:
	_op = &GE;
	break;

    case MOD_LONGER:
	_op = &LT;
	break;

    case MOD_ORLONGER:
	_op = &LE;
	break;
    }

    XLOG_ASSERT(_op);

    return op();
}

template <class A>
ElemNextHop<A>::ElemNextHop() : Element(_hash), _var(VAR_NONE)
{
}

template <class A>
ElemNextHop<A>::ElemNextHop(const A& nh) : Element(_hash), _var(VAR_NONE),
					   _addr(nh)
{
}

template <class A>
ElemNextHop<A>::ElemNextHop(const char* in) : Element(_hash), _var(VAR_NONE)
{
    if (!in)
	return;

    string s = in;

    if (s.compare("discard") == 0)
	_var = VAR_DISCARD;

    else if (s.compare("next-table") == 0)
	_var = VAR_NEXT_TABLE;

    else if (s.compare("peer-address") == 0)
	_var = VAR_PEER_ADDRESS;

    else if (s.compare("reject") == 0)
	_var = VAR_REJECT;

    else if (s.compare("self") == 0)
	_var = VAR_SELF;

    else {
	_var = VAR_NONE;
	_addr = A(in);
    }
}

template <class A>
string
ElemNextHop<A>::str() const
{
    switch (_var) {
    case VAR_NONE:
	return _addr.str();

    case VAR_DISCARD:
	return "discard";

    case VAR_NEXT_TABLE:
	return "next-table";

    case VAR_PEER_ADDRESS:
	return "peer-address";

    case VAR_REJECT:
	return "reject";
    
    case VAR_SELF:
	return "self";
    }

    // unreach
    XLOG_ASSERT(false);
    abort();
}

template <class A>
const char*
ElemNextHop<A>::type() const
{
    return id;
}

template <class A>
typename ElemNextHop<A>::Var
ElemNextHop<A>::var() const
{
    return _var;
}

template <class A>
const A&
ElemNextHop<A>::addr() const
{
    XLOG_ASSERT(_var == VAR_NONE);

    return _addr;
}

template <class A>
const A&
ElemNextHop<A>::val() const
{
    return addr();
}

// instantiate
template class ElemNet<IPv4Net>;
template class ElemNet<IPv6Net>;
template class ElemNextHop<IPv4>;
template class ElemNextHop<IPv6>;
