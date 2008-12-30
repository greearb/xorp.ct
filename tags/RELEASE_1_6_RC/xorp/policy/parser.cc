// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/policy/parser.cc,v 1.9 2008/07/23 05:11:19 pavlin Exp $"

#include "policy_module.h"

#include "libxorp/xorp.h"

#include "policy/common/policy_utils.hh"

#include "parser.hh"
#include "policy_parser.hh"


Parser::Nodes* 
Parser::parse(const Term::BLOCKS& block, const string& text)
{
    Nodes* nodes = new Nodes();
    
    // there was an error
    if(policy_parser::policy_parse(*nodes, block, text, _last_error)) {
	
	// delete semi-parsed tree  
	policy_utils::delete_vector(nodes);
	return NULL;
    }	    

    return nodes;
}

string Parser::last_error()
{
    return _last_error;
}
