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

#ifndef __POLICY_BACKEND_POLICY_BACKEND_PARSER_HH__
#define __POLICY_BACKEND_POLICY_BACKEND_PARSER_HH__


#include "policy/common/element_base.hh"

#include "policy_instr.hh"
#include "term_instr.hh"
#include "instruction_base.hh"

#include <string>
#include <vector>


/**
 * @short Minimises global namespace pollution of yacc/lex variables.
 *
 * The nature of lex and yacc causes global variables / functions to be present.
 * Here such methods and functions are grouped under one namespace.
 */
namespace policy_backend_parser {


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
                         const string& conf,
                         string& outerr);

// DO NOT TOUCH!
extern vector<PolicyInstr*>*	_yy_policies;
extern map<string,Element*>*	_yy_sets;

extern vector<TermInstr*>*	_yy_terms;
extern vector<Instruction*>*	_yy_instructions;

} // namespace

#endif // __POLICY_BACKEND_POLICY_BACKEND_PARSER_HH__
