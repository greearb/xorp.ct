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

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "policy_module.h"
#include "config.h"

#include "libxorp/debug.h"
#include "var_map.hh"
#include "policy/common/policy_utils.hh"

using namespace policy_utils;

const VarMap::VariableMap&
VarMap::variablemap(const string& protocol) const {

    ProtoMap::const_iterator i = _protocols.find(protocol);
    if(i == _protocols.end()) 
	throw VarMapErr("Unknown protocol: " + protocol);

    const VariableMap* vm = (*i).second;

    return *vm;
}

const VarMap::Variable&
VarMap::variable(const string& protocol, const string& varname) const {
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
}

VarMap::~VarMap() {
    for(ProtoMap::iterator i = _protocols.begin();
	i != _protocols.end(); ++i) {
	
	VariableMap* vm = (*i).second;

	clear_map(*vm);	
    }
    clear_map(_protocols);
}


bool 
VarMap::protocol_known(const string& protocol) {
    return _protocols.find(protocol) != _protocols.end();
}

void 
VarMap::add_variable(VariableMap& vm, const string& varname, 
		     const string& type, Access acc) {

    VariableMap::iterator i = vm.find(varname);

    if(i != vm.end()) 
	throw VarMapErr("Variable " + varname + " exists already");
    
    Variable* var = new Variable(varname,type,acc);
    vm[varname] = var;
}

void 
VarMap::add_protocol_variable(const string& protocol, 
			      const string& varname, 
			      const string& type, Access acc) {

    debug_msg("[POLICY] VarMap added proto: %s, var: %s, type: %s, R/W: %d\n",
	      protocol.c_str(), varname.c_str(), type.c_str(), acc);


    ProtoMap::iterator iter = _protocols.find(protocol);
    VariableMap* vm;

    // if no variablemap exists for the protocol exists, create one
    if(iter == _protocols.end()) {
        vm = new VariableMap();
        _protocols[protocol] = vm;
    
        _process_watch.add_interest(protocol);
    }
    // or else just update existing one
    else 
        vm = (*iter).second;

    add_variable(*vm,varname,type,acc);

}

void 
VarMap::configure(const string& conf) {
    istringstream iss(conf);

    unsigned state = 0;

    // protocol, variable, type, access
    string tokens[4];

    while(!iss.eof()) {
	string token;

	// lex =D
	iss >> token;

	if(!token.length())
	    continue;
	
	tokens[state] = token;

	state++;
	
	// yacc =D
	if(state == 4) {
	    Access a = READ;

	    if(tokens[3] == "rw")
		a = READ_WRITE;
	
	    add_protocol_variable(tokens[0],tokens[1],tokens[2],a);
	    state = 0;
	}
    }


}
