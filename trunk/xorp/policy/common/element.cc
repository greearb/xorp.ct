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

#include "element.hh"
#include "elem_null.hh"

// Initialization of static members.
// Remember to be unique in id's.
const char* ElemInt32::id = "i32";
const char* ElemU32::id = "u32";
const char* ElemStr::id = "str";
const char* ElemBool::id = "bool";

template<> const char* ElemIPv4::id = "ipv4";
template<> const char* ElemIPv6::id = "ipv6";
template<> const char* ElemIPv4Net::id = "ipv4net";
template<> const char* ElemIPv6Net::id = "ipv6net";

const char* ElemNull::id = "null";
