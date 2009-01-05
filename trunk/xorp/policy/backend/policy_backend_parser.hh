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

// $XORP: xorp/policy/backend/policy_backend_parser.hh,v 1.9 2008/10/02 21:58:04 bms Exp $

#ifndef __POLICY_BACKEND_POLICY_BACKEND_PARSER_HH__
#define __POLICY_BACKEND_POLICY_BACKEND_PARSER_HH__

#include <string>
#include <vector>

#include "policy/common/element_base.hh"
#include "policy_instr.hh"
#include "term_instr.hh"
#include "instruction_base.hh"

typedef map<string, PolicyInstr*>   SUBR;

/**
 * @short Minimises global namespace pollution of yacc/lex variables.
 *
 * The nature of lex and yacc causes global variables / functions to be present.
 * Here such methods and functions are grouped under one namespace.
 */
namespace policy_backend_parser {

typedef vector<PolicyInstr*>	    POLICIES;

/**
 * Parses a backend policy configuration.
 *
 * Caller is responsible for deleting partially parsed policies and sets.
 *
 * @return 0 on success. Otherwise, outerr is filled with error message.
 * @param outpolicies the parse tree of all policies.
 * @param outsets the pair of set-name / content.
 * @param conf the configuration to parse.
 * @param outerr string filled with parse error message, on error.
 */
int policy_backend_parse(vector<PolicyInstr*>& outpolicies,
                         map<string,Element*>& outsets,
			 SUBR& outsubr,
                         const string& conf,
                         string& outerr);

extern vector<PolicyInstr*>*	_yy_policies;
extern map<string,Element*>*	_yy_sets;
extern vector<TermInstr*>*	_yy_terms;
extern vector<Instruction*>*	_yy_instructions;
extern bool			_yy_trace;
extern SUBR*			_yy_subr;

} // namespace

#endif // __POLICY_BACKEND_POLICY_BACKEND_PARSER_HH__
