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

unsigned int Dispatcher::makeKey(const Oper& op, unsigned argc, const Element** argv) const
{
    XLOG_ASSERT(op.arity() == argc);
    XLOG_ASSERT(argc <= 2);

    unsigned int key = op.hash();
    XLOG_ASSERT(key);

    for (unsigned i = 0; i < argc; i++) {
	const Element* arg = argv[i];
	unsigned int eh = arg->hash();

	XLOG_ASSERT(eh);
	key |= eh << (5*(i+1));
    }

    XLOG_ASSERT(key < DISPATCHER_MAP_SZ);
    return key;
}

Dispatcher::Value Dispatcher::lookup(const Oper& op, unsigned argc, const Element** argv) const
{
    XLOG_ASSERT(op.arity() == argc);

    // find callback
    unsigned int key = makeKey(op, argc, argv);
    return _map[key];
}

/* NOTE:  add() is called before logging framework is online. */
void Dispatcher::logAdd(const Oper& op, unsigned int k, const Element* arg1, const Element* arg2) const {
// Enable this if debugging is needed.
#if 0
    printf("Adding operator: %s to dispatcher map[%d] %p %p\n",
	   op.str().c_str(), k, arg1, arg2);
    if (arg1) {
	printf("arg1: %s\n", arg1->dbgstr().c_str());
    }
    if (arg2) {
	printf("arg2: %s\n", arg2->dbgstr().c_str());
    }
#else
    UNUSED(op);
    UNUSED(k);
    UNUSED(arg1);
    UNUSED(arg2);
#endif
}

void Dispatcher::logRun(const Oper& op, unsigned argc, const Element** argv, int key, const char* dbg) const {
    /* We are about to crash, make sure this goes out immediately. */
    printf("operation: %s  key: %d  argc: %d  dbg: %s\n",
	   op.str().c_str(), key, argc, dbg);
    for (unsigned int i = 0; i<argc; i++) {
	const Element* arg = argv[i];
	printf("argv[%d]: %s\n", i, arg->dbgstr().c_str());
    }
}

/* If argc is 2, then argv[1] is left, argv[0] is right argument. */
Element*
Dispatcher::run(const Oper& op, unsigned argc, const Element** argv) const
{
    XLOG_ASSERT(op.arity() == argc);
    Element* rv;
    unsigned int key = op.hash();
    XLOG_ASSERT(key);

    // check for null arguments and special case them: return null
    for (unsigned i = 0; i < argc; i++) {
    
	const Element* arg = argv[i];
	unsigned int h = arg->hash();

	XLOG_ASSERT(h);

	if (h == ElemNull::_hash)
	    return new ElemNull();

	// NOTE:  Args are backwards from how makeKey goes, but code
	// must be kept in sync if makeKey changes.
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

    XLOG_ASSERT(key < DISPATCHER_MAP_SZ);

    // find function
    Value funct = _map[key];

    // expand args and execute function
    switch (argc) {
	case 1:
	    if (!funct.un) {
		logRun(op, argc, argv, key, "funct.un is NULL");
		XLOG_ASSERT(funct.un);
	    }
	    rv = funct.un(*(argv[0]));
	    //XLOG_WARNING("running unary operation: %s  key: %d arg: %s result: %s\n",
	    //	 op.str().c_str(), key, argv[0]->dbgstr().c_str(), rv->dbgstr().c_str());
	    return rv;
	
	case 2:
	    if (!funct.bin) {
		logRun(op, argc, argv, key, "funct.bin is NULL");
		XLOG_ASSERT(funct.bin);
	    }
	    rv = funct.bin(*(argv[1]), *(argv[0]));
	    //XLOG_WARNING("running binary operation: %s  key: %d left-arg1: %s  right: %s result: %s\n",
	    //	 op.str().c_str(), key, argv[1]->dbgstr().c_str(), argv[0]->dbgstr().c_str(),
	    //	 rv->dbgstr().c_str());
	    return rv;
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
