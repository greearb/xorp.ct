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

#ident "$XORP: xorp/rtrmgr/xrldb.cc,v 1.1.1.1 2002/12/11 23:56:16 hodson Exp $"

#include <glob.h>
#include "config.h"
#include "rtrmgr_module.h"
#include "libxorp/exceptions.hh"
#include "libxipc/xrl_parser.hh"
#include "xrldb.hh"

XrlSpec::XrlSpec(const Xrl& xrl, const XrlArgs& rspec) 
    : _xrl(xrl), _rspec(rspec) {
    cout << "XrlSpec " + xrl.str() + " -> " + rspec.str() + "\n";
}


XRLMatchType
XrlSpec::matches(const Xrl& xrl, const XrlArgs& rspec) const {
#if 0
    cout << "------\n";
    cout << "TEST:" << xrl.str() << " -> " << rspec.str() << "\n";
    cout << "DB:  " << _xrl.str() << " -> " << _rspec.str() << "\n";
    if (xrl == _xrl) 
	cout << "XRL matches\n";
    if (rspec == _rspec) 
	cout << "RSPEC matches\n";
#endif
    XRLMatchType m = MATCH_FAIL;

    if (xrl == _xrl) {
	m = XRLMatchType(m | MATCH_XRL);
    }
    if (rspec == _rspec) {
	m = XRLMatchType(m | MATCH_RSPEC);
    }
    return m;
}

string 
XrlSpec::str() const {
    string s = _xrl.str() + " -> " + _rspec.str();
    return s;
}

XRLtarget::XRLtarget(const string& xrlfilename) 
    : _targetname(xrlfilename)
{
    cout << "Loading xrl file " << xrlfilename << endl;
    XrlParserFileInput xpi(xrlfilename.c_str());
    XrlParser parser(xpi);

    while (parser.start_next()) {
	string s;
	XrlArgs rargs;
	list<XrlAtomSpell> rspec;

	try {
	    if (parser.get(s)) {
		cout << "Xrl " + s + "\n";
		if (parser.get_return_specs(rspec)) {
		    cout << "Return Spec:" << endl;
		    list<XrlAtomSpell>::const_iterator si;
		    for (si = rspec.begin(); si != rspec.end(); si++) {
			rargs.add(si->atom());
			cout << "\t -> " 
			     << si->atom().str() << " - " << si->spell() 
			     << endl;
		    }
		} else {
		    cout << "No return spec for XRL " + s + "\n";
		}
		Xrl xrl(s.c_str());
		_xrlspecs.push_back(XrlSpec(xrl, rargs));
	    }
	} catch (const XrlParseError& xpe) {
	    cout << string(79, '-') << endl;
	    cout << xpe.pretty_print() << "\n";
	    cout << string(79, '=') << endl; 
	    cout << "Attempting resync...";
	    if (parser.resync()) 
		cout << "okay"; 
	    else 
		cout << "fail";
	    cout << endl;
	} catch (const InvalidString& is) {
	    cout << is.str() << endl;
	}
    }
}

XRLMatchType
XRLtarget::xrl_matches(const Xrl& xrl, const XrlArgs& rspec) const {
    list <XrlSpec>::const_iterator i;
    XRLMatchType match;
    XRLMatchType failure_type = MATCH_FAIL;
    for (i = _xrlspecs.begin(); i!= _xrlspecs.end(); ++i) {
	match = i->matches(xrl, rspec);
	if (match == MATCH_ALL) return MATCH_ALL;
	if (match == MATCH_XRL) failure_type = MATCH_XRL;
    }
    return failure_type;
}

string 
XRLtarget::str() const {
    string s = "  Target " + _targetname + ":\n";
    list <XrlSpec>::const_iterator i;
    for (i=_xrlspecs.begin(); i!=_xrlspecs.end(); ++i) {
	s += "    " + i->str() + "\n";
    }
    return s;
}

XRLdb::XRLdb(const string& xrldir) {
#if 0
    printf("Initializing XRLdb from %s\n", xrldir.c_str());
#endif

    list <string> files;

    struct stat dirdata;
    if (stat(xrldir.c_str(), &dirdata) < 0) {
	string errstr = "rtrmgr: error reading xrl directory "
	    + xrldir + "\n";
	errstr += strerror(errno); 
	errstr += "\n";
	fprintf(stderr, "%s", errstr.c_str());
	exit(1);
    }

    if ((dirdata.st_mode & S_IFDIR) == 0) {
	string errstr = "rtrmgr: error reading xrl directory "
	    + xrldir + "\n" + xrldir + " is not a directory\n";
	fprintf(stderr, "%s", errstr.c_str());
	exit(1);
    }
    
    string globname = xrldir + "/*.xrls";
    glob_t pglob;
    if (glob(globname.c_str(), 0, 0, &pglob) != 0) {
	fprintf(stderr, "rtrmgr failed to find xrl files in %s\n",
		xrldir.c_str());
	globfree(&pglob);
	exit(1);
    }

    if (pglob.gl_pathc == 0) {
	fprintf(stderr, "rtrmgr failed to find any xrl files in %s\n", 
		xrldir.c_str());
	globfree(&pglob);
	exit(1);
    }
    
    for (size_t i=0; i< (size_t)pglob.gl_pathc; i++) {
	_targets.push_back(XRLtarget(pglob.gl_pathv[i]));
    }
#if 0
    printf("XRLdb initialized\n");
    printf("%s\n", str().c_str());
#endif
    globfree(&pglob);
}

bool
XRLdb::check_xrl_syntax(const string &xrlstr) const {
    printf("XRLdb: checking xrl syntax: %s\n", xrlstr.c_str());
    try {
	string::size_type start = xrlstr.find("->");
	string rspec;
	string xrlspec;
	if (start == string::npos) {
	    rspec.empty();
	    xrlspec = xrlstr;
	} else {
	    xrlspec = xrlstr.substr(0, start);
	    rspec = xrlstr.substr(start+2, xrlstr.size()-(start+2));
	}
	Xrl test_xrl(xrlspec.c_str());
	XrlArgs test_atoms(rspec.c_str());
    } catch (const InvalidString&) {
	return false;
    }
    return true;
}

XRLMatchType
XRLdb::check_xrl_exists(const string &xrlstr) const {
    printf("XRLdb: checking xrl exists: %s\n", xrlstr.c_str());

    string::size_type start = xrlstr.find("->");
    string xrlspec;
    string rspec;
    if (start == string::npos) {
	xrlspec = xrlstr;
	rspec.empty();
    } else {
	xrlspec = xrlstr.substr(0, start);
	rspec = xrlstr.substr(start+2, xrlstr.size()-(start+2));
    }
    Xrl test_xrl(xrlspec.c_str());    
    XrlArgs test_atoms(rspec.c_str());
    XRLMatchType match;
    XRLMatchType failure_type = MATCH_FAIL;
    list <XRLtarget>::const_iterator i;
    for (i=_targets.begin(); i!=_targets.end(); ++i) {
	match = i->xrl_matches(test_xrl, test_atoms);
	if (match == MATCH_ALL) return MATCH_ALL;
	if (match == MATCH_XRL) failure_type = MATCH_XRL;
    }
    return failure_type;
}

string 
XRLdb::str() const {
    string s = "XRLdb contains\n";
    list <XRLtarget>::const_iterator i;
    for (i=_targets.begin(); i!=_targets.end(); ++i) {
	s += i->str() + "\n";
    }
    return s;
}
