// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/libxipc/test_xrl_parser.cc,v 1.1.1.1 2002/12/11 23:56:04 hodson Exp $"

#include <stdio.h>
#include <string>

#include "xrl_module.h"
#include "config.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "xrl_parser.hh"

static const char QUOT='"';

/**
 * @return number of errors encountered.
 */
static uint32_t
parse_buffer(XrlParser& p) {
    uint32_t errcnt = 0;
    try {
	while (p.start_next() == true) {
	    string protocol, target, command;
	    XrlArgs args;
	    list<XrlAtomSpell> spells;
	    list<XrlAtomSpell> rspec;

	    try {
		cout << string(80, '-') << endl; 
		cout << "Input:    " << QUOT << p.input() << QUOT << endl;
		cout << string(80, '=') << endl;
		p.get(protocol, target, command, args, spells);

		cout << "Protocol: " << QUOT << protocol  << QUOT << endl 
		     << "Target:   " << QUOT << target    << QUOT << endl 
		     << "Command:  " << QUOT << command   << QUOT << endl;

		if (!args.empty()) {
		    cout << "Arguments: " << endl;
		    for (XrlArgs::const_iterator i = args.begin();
			 i != args.end(); i++) {
			cout << "\t" << i->str() << endl;
		    }
		}

		if (!spells.empty()) {
		    cout << "Input variable assignments: " << endl;
		    for (list<XrlAtomSpell>::const_iterator si = spells.begin(); 
			 si != spells.end(); si++) {
			cout << "\t" 
			     << si->atom().str() << " - " << si->spell() 
			     << endl;
		    }
		}
		if (p.get_return_specs(rspec)) {
		    cout << "Return Specification:" << endl;
		    for (list<XrlAtomSpell>::const_iterator si = rspec.begin(); 
			 si != rspec.end(); si++) {
			cout << "\t" 
			     << si->atom().str() << " - " << si->spell() 
			     << endl;
		    }
		}
	    } catch (const XrlParseError& xpe) {
		cout << string(79, '-') << endl;
		cout << xpe.pretty_print() << "\n";
		cout << string(79, '=') << endl; 
		cout << "Attempting resync...";
		if (p.resync()) 
		    cout << "okay"; 
		else 
		    cout << "fail";
		cout << endl;
		errcnt++;
	    }
	    cout << endl;
	    flush(cout);
	}
    } catch (const XrlParserInputException& xe) {
	cout << string(79, '!') << endl;
	cout << "Parser input failed: " << endl << xe.str() << endl;
	cout << p.parser_input().stack_trace() << endl;
	errcnt++;
    }
    return errcnt;
}

int
main(int argc, char *argv[]) {
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    uint32_t errcnt = 0;
    
    for (int i = 1; i < argc; i++) {
	XrlParserFileInput xpfi(argv[i]);
	XrlParser xp(xpfi);
	errcnt += parse_buffer(xp);
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return errcnt;
}
