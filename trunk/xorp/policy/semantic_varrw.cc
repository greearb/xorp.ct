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

#include "policy_module.h"
#include "config.h"

#include "semantic_varrw.hh"
#include "policy/common/policy_utils.hh"

SemanticVarRW::SemanticVarRW(VarMap& vars) : 
		    _vars(vars) 
{
}

SemanticVarRW::~SemanticVarRW() {
    policy_utils::clear_container(_trash);
}

const Element& 
SemanticVarRW::read(const string& id) {
    const VarMap::Variable& var = _vars.variable(_protocol,id);

    Element* e =  _ef.create(var.type,NULL);

    _trash.insert(e);
    return *e;
}

void 
SemanticVarRW::write(const string& id, const Element& elem) {
    // this will throw exception on unknown variable
    const VarMap::Variable& var = _vars.variable(_protocol,id);

    // check the rw access
    if(var.access != VarMap::READ_WRITE) {
	string error = "Trying to write on read-only variable ";
        error += id;
        throw var_error(error);
    }    

    // type checking
    if(var.type != elem.type()) {
	ostringstream err;

        err << "Trying to assign value of type " << elem.type() << " to " <<
	var.type << " variable " << id;

        throw var_error(err.str());
    }
}

void 
SemanticVarRW::set_protocol(const string& proto) {
    _protocol = proto;
}

void
SemanticVarRW::sync() {
    policy_utils::clear_container(_trash);
}
