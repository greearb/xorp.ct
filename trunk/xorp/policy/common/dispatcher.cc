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

#ident "$XORP: xorp/policy/common/dispatcher.cc,v 1.2 2005/03/25 02:54:15 pavlin Exp $"

#include "config.h"
#include "dispatcher.hh"
#include "elem_null.hh"
#include "policy_utils.hh"

// init static members
Dispatcher::Map Dispatcher::_map;
RegisterOperations Dispatcher::_regops;

Dispatcher::Dispatcher()
{
}

Dispatcher::Key
Dispatcher::makeKey(const Oper& op, const ArgList& args) const
{
    XLOG_ASSERT(op.arity() == args.size());
   
    // XXX: key has to be fast to compute!!!
    string key("");

    // make key unique based on operation
    key += op.str();

    for(ArgList::const_iterator i = args.begin();
	i != args.end(); ++i) {
    
	const Element* arg = *i;

	// delimiter
	key += "_";

	// unique based on type / arg position
	key += arg->type();
    }	
                    
    return key;
}   

Element*
Dispatcher::run(const Oper& op, const ArgList& args) const
{
    unsigned arity = op.arity();

    // make sure we got correct # of args
    if(arity != args.size()) {
	ostringstream oss;

	oss << "Wrong number of args. Arity: " << arity 
	    << " Args supplied: " << args.size();

	throw OpNotFound(oss.str());
    }

    // check for null arguments and special case them: return null
    for(ArgList::const_iterator i = args.begin();
	i != args.end(); ++i) {
    
	const Element* arg = *i;

	if(arg->type() == ElemNull::id)
	    return new ElemNull();
    }

    // find function
    Value funct = lookup(op,args);

    
    // expand args and execute function
    switch(arity) {
	case 1:
	    return funct.un(*(args[0]));
	
	case 2:
	    return funct.bin(*(args[0]),*(args[1]));

	// the infrastructure is ready however.
	default:
	    throw OpNotFound("Operations of arity: " +
			     policy_utils::to_str(arity) + 
			     " not supported");
    }
    // unreach
}


Element* 
Dispatcher::run(const UnOper& op, const Element& arg) const
{
    // prepare arglist
    ArgList args;
    args.push_back(&arg);

    // execute generic run
    return run(op,args);
}

Element* 
Dispatcher::run(const BinOper& op, 
		const Element& left, 
		const Element& right) const
{
    // prepare arglist and execute generic run
    ArgList args;

    args.push_back(&left);
    args.push_back(&right);

    return run(op,args);
}

template<class L, class R, Element* (*funct)(const L&,const R&)>
void
Dispatcher::add(const BinOper& op)
{
    // XXX: do it in a better way
    L arg1;
    R arg2;

    ArgList args;

    args.push_back(&arg1);
    args.push_back(&arg2);

    Key key = makeKey(op,args);

    struct Local {
        static Element* Trampoline(const Element& left, const Element& right) {
            return funct(dynamic_cast<const L&>(left),
                         dynamic_cast<const R&>(right));
        }
    };

    _map[key].bin = &Local::Trampoline;
}

template<class T, Element* (*funct)(const T&)>
void 
Dispatcher::add(const UnOper& op)
{
    // XXX: ugly
    T arg;

    ArgList args;

    args.push_back(&arg);

    Key key = makeKey(op,args);

    struct Local {
	static Element* Trampoline(const Element& arg) {
	    return funct(dynamic_cast<const T&>(arg));
        }
    };

    _map[key].un = &Local::Trampoline;
}
