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

// $XORP$

#ifndef __POLICY_COMMON_DISPATCHER_HH__
#define __POLICY_COMMON_DISPATCHER_HH__

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <assert.h>

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
	OpNotFound(const string& err) : PolicyException(err) {}
    };

    /**
     * Method to register a binary operation callback with dispatcher.
     *
     * @param L concrete class of first argument
     * @param R concrete class of second argument
     * @param funct function to be called to perform operation.
     * @param op cinary operation to be registered.
     */
    template<class L, class R, Element* (*funct)(const L&,const R&)>
    void 
    add(const BinOper& op) {

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


    /**
     * Execute an n-ary operation.
     *
     * Throws an exception on failure.
     *
     * @return result of operation.
     * @param op operation to dispatch.
     * @param args arguments of operation.
     */
    Element* run(const Oper& op, const ArgList& args) const;

    /**
     * Execute an unary operation.
     *
     * @return Result of operation. Caller is responsible for delete.
     * @param op Operation to perform.
     * @param arg Argument of operation.
     */
    Element* run(const UnOper& op, const Element& arg) const ;
    
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

    // Key which relates to a callback
    typedef string Key;

    // A key relates to either a binary (x)or unary operation.
    typedef union {
	CB_un un;
        CB_bin bin;
    } Value;

    // Hashtable would be better
    typedef map<Key,Value> Map;



    /**
     * Create a key for the callback table based on operation and arguments.
     *
     * @return key used for callback lookup.
     * @param op requested operation.
     * @param args the arguments for the operation.
     */
    Key makeKey(const Oper& op, const ArgList& args) const;

    // XXX: definition moved in header file to allow compilation on 2.95.x
    /**
     * Lookup a callback for the requested operation and elements.
     * Throws exception if none is found.
     *
     * @return callback which will perform requested operation.
     * @param op operation to perform.
     * @param args the arguments of the operation.
     */
    Value lookup(const Oper& op, const ArgList& args) const {
	assert(op.arity() == args.size());
  
        // find callback
        Key key = makeKey(op,args);
        Map::iterator i = _map.find(key);


        // none found
        if(i == _map.end()) {
	    ostringstream oss;

	    oss << "No operation registered for OP: " << op.str()
		<< " Arity: " << op.arity() << " args:";

            for(ArgList::const_iterator i = args.begin();
                i != args.end(); ++i) {
        
	        const Element* arg = *i;
	        oss << " " << arg->type();
            }
            throw OpNotFound(oss.str());
        }                    

        return (*i).second;
    }




    // Only one global map. Creating multiple dispatcher is thus harmless.
    // However, we may not have different dispatchers.
    static Map _map;

    // Do initial registration of callbacks.
    static RegisterOperations _regops;

};


#endif // __POLICY_COMMON_DISPATCHER_HH__
