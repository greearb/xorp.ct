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

#ident "$XORP: xorp/policy/var_map.cc,v 1.5 2005/07/15 02:27:07 abittau Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "policy_module.h"
#include "libxorp/debug.h"
#include "var_map.hh"
#include "policy/common/policy_utils.hh"
#include "policy/common/element_factory.hh"

using namespace policy_utils;

const string VarMap::TRACE = "trace";

const VarMap::VariableMap&
VarMap::variablemap(const string& protocol) const
{

    ProtoMap::const_iterator i = _protocols.find(protocol);
    if(i == _protocols.end()) 
	throw VarMapErr("Unknown protocol: " + protocol);

    const VariableMap* vm = (*i).second;

    return *vm;
}

const VarMap::Variable&
VarMap::variable(const string& protocol, const string& varname) const
{
    const VariableMap& vmap = variablemap(protocol);

    VariableMap::const_iterator i = vmap.find(varname);

    if(i == vmap.end())
	throw VarMapErr("Unknown variable: " + varname + " in protocol " +
			protocol);

    const Variable* v = (*i).second;

    return *v;
}


VarMap::VarMap(ProcessWatchBase& pw) : _process_watch(pw) 
{
    add_metavariable(TRACE, "u32", WRITE);
}

VarMap::~VarMap()
{
    for(ProtoMap::iterator i = _protocols.begin();
	i != _protocols.end(); ++i) {
	
	VariableMap* vm = (*i).second;

	clear_map(*vm);	
    }
    clear_map(_protocols);
    clear_map(_metavars);
}


bool 
VarMap::protocol_known(const string& protocol)
{
    return _protocols.find(protocol) != _protocols.end();
}

void 
VarMap::add_variable(VariableMap& vm, const string& varname, 
		     const string& type, Access acc)
{

    VariableMap::iterator i = vm.find(varname);

    if(i != vm.end()) 
	throw VarMapErr("Variable " + varname + " exists already");
    
    Variable* var = new Variable(varname,type,acc);
    vm[varname] = var;
}

void 
VarMap::add_protocol_variable(const string& protocol, 
			      const string& varname, 
			      const string& type, Access acc)
{

    debug_msg("[POLICY] VarMap added proto: %s, var: %s, type: %s, R/W: %d\n",
	      protocol.c_str(), varname.c_str(), type.c_str(), acc);

    if (!ElementFactory::can_create(type)) {
	throw VarMapErr("Unable to create element of type: " + type
			+ " in proto: " + protocol + " varname: " + varname);
    }

    ProtoMap::iterator iter = _protocols.find(protocol);
    VariableMap* vm;

    // if no variablemap exists for the protocol exists, create one
    if(iter == _protocols.end()) {
        vm = new VariableMap();
        _protocols[protocol] = vm;
    
        _process_watch.add_interest(protocol);

	// add the metavars
	for (MetaVarContainer::iterator i = _metavars.begin(); i !=
	     _metavars.end(); ++i) {
	    
	    Variable* v = i->second;
	    add_variable(*vm, v->name, v->type, v->access);
	}
    }
    // or else just update existing one
    else 
        vm = (*iter).second;

    add_variable(*vm,varname,type,acc);

}

void
VarMap::add_metavariable(const string& variable, const string& type, Access acc)
{
    if (_metavars.find(variable) != _metavars.end()) {
	throw VarMapErr("Metavar: " + variable + " exists already");
    }

    Variable* v = new Variable(variable, type, acc);
    _metavars[variable] = v;
}

string
VarMap::str()
{
    ostringstream out;

    // go through protocols
    for (ProtoMap::iterator i = _protocols.begin(); 
	 i != _protocols.end(); ++i) {

	const string& proto = i->first;
	VariableMap* vm = i->second;

	for(VariableMap::iterator j = vm->begin(); j != vm->end(); ++j) {
	    Variable* v = j->second;

	    out << proto << " " << v->name << " " << v->type << " ";
	    if(v->access == READ)
		out << "r";
	    else
		out << "rw";
	    out << endl;	
	}
    }

    return out.str();
}
