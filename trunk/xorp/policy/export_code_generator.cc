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

#include "export_code_generator.hh"

ExportCodeGenerator::ExportCodeGenerator(const string& proto, 
					 uint32_t tagstart) : 
	CodeGenerator(proto, filter::EXPORT), _currtag(tagstart) {
}

const Element* 
ExportCodeGenerator::visit_term(Term& term) {
    // ignore source [done by source match]
    vector<Node*>& dest = term.dest_nodes();
    vector<Node*>& actions = term.action_nodes();

    vector<Node*>::iterator i;

    // tags are linear.. for each term, match the tag in the source block.
    _os << "TERM_START " << term.name() << endl ;

    _os << "LOAD policytags\n";
    _os << "PUSH u32 " << _currtag << endl;
    _os << "<=\n";
    _os << "ONFALSE_EXIT" << endl;

    // do dest block
    for(i = dest.begin(); i != dest.end(); ++i) {
        (*i)->accept(*this);
        _os << "ONFALSE_EXIT" << endl;
    }

    // do actions
    for(i = actions.begin(); i != actions.end(); ++i) {
        (*i)->accept(*this);
    }

    _os << "TERM_END\n";

    // update tags used by the code
    _code._tags.insert(_currtag);

    // FIXME: integer overflow hopefully will never occur.
    _currtag++;
    return NULL;
}

uint32_t 
ExportCodeGenerator::currtag() { 
    return _currtag; 
} 
