// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/policy/export_code_generator.hh,v 1.6 2007/02/16 22:46:53 pavlin Exp $

#ifndef __POLICY_EXPORT_CODE_GENERATOR_HH__
#define __POLICY_EXPORT_CODE_GENERATOR_HH__

#include "code_generator.hh"
#include "source_match_code_generator.hh"

/**
 * @short Generates export filter code from a node structure.
 *
 * This is a specialized version of the CodeGenerator used for import filters.
 * It skips the source section of terms and replaces it with tag matching
 * instead.
 *
 */
class ExportCodeGenerator : public CodeGenerator {
public:
    /**
     * @param proto Protocol for which code will be generated.
     * @param t information on which tags should be used.
     * @param varmap varmap.
     */
    ExportCodeGenerator(const string& proto, 
			const SourceMatchCodeGenerator::Tags& t,
			const VarMap& varmap);

    const Element* visit_term(Term& term);

private:
    const SourceMatchCodeGenerator::Tags& _tags;
    SourceMatchCodeGenerator::Tags::const_iterator _tags_iter;
};

#endif // __POLICY_EXPORT_CODE_GENERATOR_HH__
