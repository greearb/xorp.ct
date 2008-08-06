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

#ident "$XORP: xorp/policy/common/element.cc,v 1.13 2008/07/23 05:11:26 pavlin Exp $"

#include "element.hh"
#include "elem_null.hh"
#include "elem_filter.hh"
#include "elem_bgp.hh"
#include "policy_exception.hh"

// Initialization of static members.
// Remember to be unique in id's.
const char* ElemInt32::id = "i32";
Element::Hash ElemInt32::_hash = HASH_ELEM_INT32;

const char* ElemU32::id = "u32";
Element::Hash ElemU32::_hash = HASH_ELEM_U32;

const char* ElemCom32::id = "com32";
Element::Hash ElemCom32::_hash = HASH_ELEM_COM32;

const char* ElemStr::id = "txt";
Element::Hash ElemStr::_hash = HASH_ELEM_STR;

const char* ElemBool::id = "bool";
Element::Hash ElemBool::_hash = HASH_ELEM_BOOL;

template<> const char* ElemIPv4::id = "ipv4";
template<> Element::Hash ElemIPv4::_hash = HASH_ELEM_IPV4;

template<> const char* ElemIPv4Range::id = "ipv4range";
template<> Element::Hash ElemIPv4Range::_hash = HASH_ELEM_IPV4RANGE;

template<> const char* ElemIPv6::id = "ipv6";
template<> Element::Hash ElemIPv6::_hash = HASH_ELEM_IPV6;

template<> const char* ElemIPv6Range::id = "ipv6range";
template<> Element::Hash ElemIPv6Range::_hash = HASH_ELEM_IPV6RANGE;

template<> const char* ElemIPv4Net::id = "ipv4net";
template<> Element::Hash ElemIPv4Net::_hash = HASH_ELEM_IPV4NET;

template<> const char* ElemIPv6Net::id = "ipv6net";
template<> Element::Hash ElemIPv6Net::_hash = HASH_ELEM_IPV6NET;

template<> const char* ElemU32Range::id = "u32range";
template<> Element::Hash ElemU32Range::_hash = HASH_ELEM_U32RANGE;

template<> const char* ElemASPath::id = "aspath";
template<> Element::Hash ElemASPath::_hash = HASH_ELEM_ASPATH;

const char* ElemNull::id = "null";
Element::Hash ElemNull::_hash = HASH_ELEM_NULL;

Element::Hash ElemFilter::_hash = HASH_ELEM_FILTER;

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
    char *colon = strstr(c_str, ":");

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
