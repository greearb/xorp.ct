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
#include "register_elements.hh"
#include "element_factory.hh"
#include "element.hh"
#include "elem_set.hh"
#include "elem_null.hh"



namespace elem_create {

Element* create_i32(const char* x) {
    return new ElemInt32(x);
}

Element* create_u32(const char* x) {
    return new ElemU32(x);
}

Element* create_str(const char* x) {
    return new ElemStr(x);
}

Element* create_bool(const char* x) {
    return new ElemBool(x);
}

Element* create_set(const char* x) {
    return new ElemSet(x);
}

Element* create_ipv4(const char* x) {
    return new ElemIPv4(x);
}


Element* create_ipv6(const char* x) {
    return new ElemIPv6(x);
}

Element* create_null(const char* x) {
    return new ElemNull(x);
}

Element* create_ipv4net(const char* x) {
    return new ElemIPv4Net(x);
}

Element* create_ipv6net(const char* x) {
    return new ElemIPv6Net(x);
}

} // namespace

RegisterElements::RegisterElements() {
    ElementFactory ef;

    ef.add(ElemInt32::id,elem_create::create_i32);
    ef.add(ElemU32::id,elem_create::create_u32);
    ef.add(ElemStr::id,elem_create::create_str);
    ef.add(ElemBool::id,elem_create::create_bool);

    ef.add(ElemSet::id,elem_create::create_set);

    ef.add(ElemIPv4::id,elem_create::create_ipv4);
    ef.add(ElemIPv6::id,elem_create::create_ipv6);

    ef.add(ElemNull::id,elem_create::create_null);

    ef.add(ElemIPv4Net::id,elem_create::create_ipv4net);
    ef.add(ElemIPv6Net::id,elem_create::create_ipv6net);
}
