// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ident "$XORP: xorp/libxipc/test_xrl_parser.cc,v 1.13 2008/07/23 05:10:44 pavlin Exp $"

#include "xrl_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "xrl_parser.hh"


static const char QUOT='"';

/**
 * @return number of errors encountered.
 */
static uint32_t
parse_buffer(XrlParser& p)
{
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
main(int argc, char *argv[])
{
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
    try {
	for (int i = 1; i < argc; i++) {
	    XrlParserFileInput xpfi(argv[i]);
	    XrlParser xp(xpfi);
	    errcnt += parse_buffer(xp);
	}
    } catch (...) {
	xorp_catch_standard_exceptions();
	errcnt++;
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return errcnt;
}
