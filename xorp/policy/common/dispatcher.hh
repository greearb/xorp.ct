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


#ifndef __POLICY_COMMON_DISPATCHER_HH__
#define __POLICY_COMMON_DISPATCHER_HH__

#include "policy/policy_module.h"
#include "libxorp/xlog.h"
#include "element_base.hh"
#include "operator_base.hh"
#include "register_operations.hh"
#include "policy_exception.hh"

/**
 * @short Link between elements and operations. Executes operations on elments.
 *
 * Implementation of multimethods.
 * Insipred/copied from Alexandrescu [Modern C++ Design].
 *
 * By taking base element arguments and an operation, it will execute the
 * correct operation based on the concrete type of the arguments.
 *
 * Similar to an ElementFactory.
 */
class Dispatcher {
public:
    typedef vector<const Element*> ArgList;

    Dispatcher();

    /**
     * @short Exception thrown if no operation is found for given arguments.
     *
     * If there is no combination for the given operation and element types.
     */
    class OpNotFound : public PolicyException {
    public:
	OpNotFound(const char* file, size_t line, const string& init_why = "")   
	    : PolicyException("OpNotFound", file, line, init_why) {}
    };

    /**
     * Method to register a binary operation callback with dispatcher.
     *
     * @param L concrete class of first argument
     * @param R concrete class of second argument
     * @param funct function to be called to perform operation.
     * @param op binary operation to be registered.
     */
    template<class L, class R, Element* (*funct)(const L&,const R&)>
    void add(const BinOper& op) {
	// XXX: do it in a better way
	L arg1;
	R arg2;

	const Element* args[] = { &arg1, &arg2 };

	unsigned int key = makeKey(op, 2, args);

	struct Local {
	    static Element* Trampoline(const Element& left, const Element& right) {
		return funct(static_cast<const L&>(left),
			     static_cast<const R&>(right));
	    }
	};

	_map[key].bin = &Local::Trampoline;
	logAdd(op, key, &arg1, &arg2);
    }

    /**
     * Method to register a unary operation callback with dispatcher.
     *
     * @param T concrete class of argument
     * @param funct function to be called to perform operation.
     * @param op unary operation to be registered.
     */
    template<class T, Element* (*funct)(const T&)>
    void add(const UnOper& op) {
	// XXX: ugly
	T arg;
	const Element* args[] = { &arg };

	unsigned int key = makeKey(op, 1, args);

	struct Local {
	    static Element* Trampoline(const Element& arg) {
		return funct(static_cast<const T&>(arg));
	    }
	};

	_map[key].un = &Local::Trampoline;

	logAdd(op, key, &arg, NULL);
    }

    /* NOTE:  add() is called before logging framework is online. */
    void logAdd(const Oper& op, unsigned int k, const Element* e1, const Element* e2) const;

    void logRun(const Oper& op, unsigned argc, const Element** argv, int key, const char* dbg) const;

    /**
     * Execute an n-ary operation.
     *
     * Throws an exception on failure.
     *
     * @return result of operation.
     * @param op operation to dispatch.
     * @param args arguments of operation.
     */
    Element* run(const Oper& op, unsigned argc, const Element** argv) const;

    /**
     * Execute an unary operation.
     *
     * @return Result of operation. Caller is responsible for delete.
     * @param op Operation to perform.
     * @param arg Argument of operation.
     */
    Element* run(const UnOper& op, const Element& arg) const;
    
    /**
     * Execute a binary operation.
     *
     * @return result of operation. Caller is responsible for delete.
     * @param op Operation to perform.
     * @param left first argument.
     * @param right second argument.
     */
    Element* run(const BinOper& op, 
		 const Element& left, 
		 const Element& right) const;

private:
    // Callback for binary operation
    typedef Element* (*CB_bin)(const Element&, const Element&);
    
    // Callback for unary operation
    typedef Element* (*CB_un)(const Element&);

    // A key relates to either a binary (x)or unary operation.
    typedef union {
	CB_un un;
        CB_bin bin;
    } Value;

    // Hashtable would be better
    typedef map<unsigned int, Value> Map;

    /**
     * Create a key for the callback table based on operation and arguments.
     *
     * @return key used for callback lookup.
     * @param op requested operation.
     * @param args the arguments for the operation.
     */
    unsigned int makeKey(const Oper& op, unsigned argc, const Element** argv) const;

    // XXX: definition moved in header file to allow compilation on 2.95.x
    /**
     * Lookup a callback for the requested operation and elements.
     * Throws exception if none is found.
     *
     * @return callback which will perform requested operation.
     * @param op operation to perform.
     * @param args the arguments of the operation.
     */
    Value lookup(const Oper& op, unsigned argc, const Element** argv) const;

    // Only one global map. Creating multiple dispatcher is thus harmless.
    // However, we may not have different dispatchers.
#define DISPATCHER_MAP_SZ 32768
    static Value _map[DISPATCHER_MAP_SZ];
};

#endif // __POLICY_COMMON_DISPATCHER_HH__
