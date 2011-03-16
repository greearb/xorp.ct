// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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

#ifndef XORP_USE_USTL
#include <typeinfo>
#endif

#include "dispatcher.hh"
#include "elem_null.hh"
#include "policy_utils.hh"
#include "operator.hh"
#include "element.hh"
#include "register_operations.hh"

// init static members
Dispatcher::Value Dispatcher::_map[32768];

Dispatcher::Dispatcher()
{
}

Element*
Dispatcher::run(const Oper& op, unsigned argc, const Element** argv) const
{
    XLOG_ASSERT(op.arity() == argc);

    unsigned key = 0;
    key |= op.hash();
    XLOG_ASSERT(key);

    // check for null arguments and special case them: return null
    for (unsigned i = 0; i < argc; i++) {
    
	const Element* arg = argv[i];
	unsigned char h = arg->hash();

	XLOG_ASSERT(h);

	if(h == ElemNull::_hash)
	    return new ElemNull();

	key |= h << (5*(argc-i));
    }

    // check for constructor
    if (argc == 2 && typeid(op) == typeid(OpCtr)) {
	string arg1type = argv[1]->type();

	if (arg1type != ElemStr::id)
	    xorp_throw(OpNotFound,
		       "First argument of ctr must be txt type, but is: " 
		       + arg1type);

	const ElemStr& es = dynamic_cast<const ElemStr&>(*argv[1]);

	return operations::ctr(es, *(argv[0]));
    }

    // find function
    Value funct = _map[key];

    // expand args and execute function
    switch(argc) {
	case 1:
	    XLOG_ASSERT(funct.un);
	    return funct.un(*(argv[0]));
	
	case 2:
	    XLOG_ASSERT(funct.bin);
	    return funct.bin(*(argv[1]),*(argv[0]));

	// the infrastructure is ready however.
	default:
	    xorp_throw(OpNotFound, "Operations of arity: " +
		       policy_utils::to_str(argc) + 
		       " not supported");
    }
    // unreach
}


Element* 
Dispatcher::run(const UnOper& op, const Element& arg) const
{
    static const Element* argv[1];

    argv[0] = &arg;
    // execute generic run

    return run(op, 1, argv);
}

Element* 
Dispatcher::run(const BinOper& op, 
		const Element& left, 
		const Element& right) const
{
    static const Element* argv[2];

    argv[0] = &right;
    argv[1] = &left;

    return run(op, 2, argv);
}
