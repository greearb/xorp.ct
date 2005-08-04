// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
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

#ident "$XORP: xorp/policy/parser.cc,v 1.3 2005/07/12 00:47:51 abittau Exp $"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "policy_module.h"
#include "parser.hh"
#include "policy_parser.hh"
#include "policy/common/policy_utils.hh"

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
