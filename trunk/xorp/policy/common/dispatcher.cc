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

#ident "$XORP: xorp/policy/common/dispatcher.cc,v 1.16 2008/08/06 08:11:28 abittau Exp $"

#include "libxorp/xorp.h"

#include <typeinfo>

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
