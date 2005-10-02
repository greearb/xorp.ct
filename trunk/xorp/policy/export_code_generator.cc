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

#ident "$XORP: xorp/policy/export_code_generator.cc,v 1.5 2005/08/04 15:26:55 bms Exp $"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "policy_module.h"
#include "libxorp/xlog.h"
#include "export_code_generator.hh"

ExportCodeGenerator::ExportCodeGenerator(
			const string& proto, 
			const SourceMatchCodeGenerator::Tags& t,
			const VarMap& varmap) : 
	CodeGenerator(proto, filter::EXPORT, varmap), _tags(t)
{
    _tags_iter = _tags.begin();
}

const Element* 
ExportCodeGenerator::visit_term(Term& term)
{
    XLOG_ASSERT(_tags_iter != _tags.end());

    // ignore source [done by source match]
    Term::Nodes& dest = term.dest_nodes();
    Term::Nodes& actions = term.action_nodes();

    Term::Nodes::iterator i;

    _os << "TERM_START " << term.name() << endl ;

    // make sure source block was not empty:
    // tags are linear.. for each term, match the tag in the source block.
    const SourceMatchCodeGenerator::Taginfo& ti = *_tags_iter;
    if (ti.first) {
        _os << "LOAD " << VarRW::VAR_POLICYTAGS << "\n";
        _os << "PUSH u32 " << (ti.second) << endl;
        _os << "<=\n";
        _os << "ONFALSE_EXIT" << endl;
    
	// update tags used by the code
	_code._tags.insert(ti.second);
    }

    // do dest block
    for(i = dest.begin(); i != dest.end(); ++i) {
        (i->second)->accept(*this);
        _os << "ONFALSE_EXIT" << endl;
    }

    // do actions
    for(i = actions.begin(); i != actions.end(); ++i) {
        (i->second)->accept(*this);
    }

    _os << "TERM_END\n";

    // go to next tag information
    ++_tags_iter;

    return NULL;
}
