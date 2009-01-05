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

// $XORP: xorp/policy/export_code_generator.hh,v 1.10 2008/10/02 21:57:58 bms Exp $

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
			const VarMap& varmap,
			PolicyMap& pmap);

    const Element* visit_term(Term& term);

private:
    const SourceMatchCodeGenerator::Tags& _tags;
    SourceMatchCodeGenerator::Tags::const_iterator _tags_iter;
};

#endif // __POLICY_EXPORT_CODE_GENERATOR_HH__
