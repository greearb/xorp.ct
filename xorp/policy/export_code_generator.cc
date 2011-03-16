// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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



#include "policy_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "export_code_generator.hh"


ExportCodeGenerator::ExportCodeGenerator(
			const string& proto, 
			const SourceMatchCodeGenerator::Tags& t,
			const VarMap& varmap,
			PolicyMap& pmap) : 
	CodeGenerator(proto, filter::EXPORT, varmap, pmap), _tags(t)
{
    _tags_iter = _tags.begin();
}

const Element* 
ExportCodeGenerator::visit_term(Term& term)
{
    XLOG_ASSERT(_tags_iter != _tags.end());

    // ignore source [done by source match]
    // XXX but there could be a from policy subroutine that has a dest block.
    // Currently ignored.  -sorbo
    Term::Nodes& dest = term.dest_nodes();
    Term::Nodes& actions = term.action_nodes();

    Term::Nodes::iterator i;

    _os << "TERM_START " << term.name() << endl ;

    // make sure source block was not empty:
    // tags are linear.. for each term, match the tag in the source block.
    const SourceMatchCodeGenerator::Taginfo& ti = *_tags_iter;
    if (ti.first) {
        _os << "LOAD " << (int)(VarRW::VAR_POLICYTAGS) << "\n";
        _os << "PUSH u32 " << (ti.second) << endl;
        _os << "<=\n";
        _os << "ONFALSE_EXIT" << endl;

	bool is_redist_tag = true;
	if (term.from_protocol() == protocol()) {
	    //
	    // XXX: If we have an export policy that exports routes
	    // from a protocol to itself, then don't tag those routes
	    // for redistribution from the RIB back to the protocol.
	    //
	    is_redist_tag = false;
	}

	// update tags used by the code
	_code.add_tag(ti.second, is_redist_tag);
    }

    // do dest block
    for(i = dest.begin(); i != dest.end(); ++i) {
        (i->second)->accept(*this);
        _os << "ONFALSE_EXIT" << endl;
    }

    //
    // Do the action block.
    // XXX: We generate last the code for the "accept" or "reject" statements.
    //
    for(i = actions.begin(); i != actions.end(); ++i) {
	if ((i->second)->is_accept_or_reject())
	    continue;
        (i->second)->accept(*this);
    }
    for(i = actions.begin(); i != actions.end(); ++i) {
	if ((i->second)->is_accept_or_reject())
	    (i->second)->accept(*this);
    }

    _os << "TERM_END\n";

    // go to next tag information
    ++_tags_iter;

    return NULL;
}
