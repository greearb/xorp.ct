// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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

#ident "$XORP: xorp/policy/semantic_varrw.cc,v 1.14 2008/10/02 21:58:00 bms Exp $"

#include "policy_module.h"

#include "libxorp/xorp.h"

#include "policy/common/policy_utils.hh"

#include "semantic_varrw.hh"


SemanticVarRW::SemanticVarRW(VarMap& vars) : 
		    _vars(vars) 
{
}

SemanticVarRW::~SemanticVarRW() {
    policy_utils::clear_container(_trash);
}

const Element& 
SemanticVarRW::read(const Id& id) {
    const VarMap::Variable& var = _vars.variable(_protocol,id);

    Element* e =  _ef.create(var.type,NULL);

    _trash.insert(e);
    return *e;
}

void 
SemanticVarRW::write(const Id& id, const Element& elem) {
    // this will throw exception on unknown variable
    const VarMap::Variable& var = _vars.variable(_protocol,id);

    // check the rw access
    if(!var.writable()) {
	string error = "Trying to write on read-only variable ";
        error += id;
        xorp_throw(var_error, error);
    }    

    // type checking
    if(var.type != elem.type()) {
	ostringstream err;

        err << "Trying to assign value of type " << elem.type() << " to " <<
	var.type << " variable " << id;

        xorp_throw(var_error, err.str());
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
