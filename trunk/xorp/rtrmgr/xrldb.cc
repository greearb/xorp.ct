// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/rtrmgr/xrldb.cc,v 1.8 2004/05/28 18:26:29 pavlin Exp $"


#include <glob.h>

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/exceptions.hh"

#include "libxipc/xrl_parser.hh"

#include "xrldb.hh"


XrlSpec::XrlSpec(const Xrl& xrl, const XrlArgs& rspec, bool verbose)
    : _xrl(xrl),
      _rspec(rspec),
      _verbose(verbose)
{
    debug_msg("XrlSpec %s -> %s\n", xrl.str().c_str(), rspec.str().c_str());
}

XRLMatchType
XrlSpec::matches(const Xrl& xrl, const XrlArgs& rspec) const
{
    debug_msg("------\n");
    debug_msg("TEST: %s -> %s\n", xrl.str().c_str(), rspec.str().c_str());
    debug_msg("DB:   %s -> %s\n", _xrl.str().c_str(), _rspec.str().c_str());
    if (xrl == _xrl) 
	debug_msg("XRL matches\n");
    if (rspec == _rspec) 
	debug_msg("RSPEC matches\n");
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
XrlSpec::str() const
{
    string s = _xrl.str() + " -> " + _rspec.str();
    return s;
}

XRLtarget::XRLtarget(const string& xrlfilename, bool verbose)
    : _targetname(xrlfilename),
      _verbose(verbose)
{
    debug_msg("Loading xrl file %s\n", xrlfilename.c_str());

    XrlParserFileInput xpi(xrlfilename.c_str());
    XrlParser parser(xpi);

    while (parser.start_next()) {
	string s;
	XrlArgs rargs;
	list<XrlAtomSpell> rspec;

	try {
	    if (parser.get(s)) {
		debug_msg("Xrl %s\n", s.c_str());
		if (parser.get_return_specs(rspec)) {
		    debug_msg("Return Spec:\n");
		    list<XrlAtomSpell>::const_iterator si;
		    for (si = rspec.begin(); si != rspec.end(); si++) {
			rargs.add(si->atom());
			debug_msg("\t -> %s - %s\n",
				  si->atom().str().c_str(),
				  si->spell().c_str());
		    }
		} else {
		    debug_msg("No return spec for XRL %s\n", s.c_str());
		}
		Xrl xrl(s.c_str());
		_xrlspecs.push_back(XrlSpec(xrl, rargs, _verbose));
	    }
	} catch (const XrlParseError& xpe) {
	    s = string(79, '-') + "\n";
	    s += xpe.pretty_print() + "\n";
	    s += string(79, '=') + "\n"; 
	    s += "Attempting resync...";
	    if (parser.resync()) 
		s += "okay"; 
	    else 
		s += "fail";
	    s += "\n";
	    XLOG_ERROR("%s", s.c_str());
	} catch (const InvalidString& is) {
	    XLOG_ERROR("%s\n", is.str().c_str());
	}
    }
}

XRLMatchType
XRLtarget::xrl_matches(const Xrl& xrl, const XrlArgs& rspec) const
{
    list<XrlSpec>::const_iterator iter;
    XRLMatchType match;
    XRLMatchType failure_type = MATCH_FAIL;

    for (iter = _xrlspecs.begin(); iter != _xrlspecs.end(); ++iter) {
	match = iter->matches(xrl, rspec);
	if (match == MATCH_ALL)
	    return MATCH_ALL;
	if (match == MATCH_XRL)
	    failure_type = MATCH_XRL;
    }
    return failure_type;
}

string
XRLtarget::str() const
{
    string s = "  Target " + _targetname + ":\n";
    list<XrlSpec>::const_iterator iter;

    for (iter = _xrlspecs.begin(); iter != _xrlspecs.end(); ++iter) {
	s += "    " + iter->str() + "\n";
    }
    return s;
}

XRLdb::XRLdb(const string& xrldir, bool verbose) throw (InitError)
    : _verbose(verbose)
{
    string errmsg;
    list<string> files;

    struct stat dirdata;
    if (stat(xrldir.c_str(), &dirdata) < 0) {
	errmsg = c_format("Error reading XRL directory %s: %s",
			  xrldir.c_str(), strerror(errno));
	xorp_throw(InitError, errmsg);
    }

    if ((dirdata.st_mode & S_IFDIR) == 0) {
	errmsg = c_format("Error reading XRL directory %s: not a directory",
			  xrldir.c_str());
	xorp_throw(InitError, errmsg);
    }

    // TODO: file suffix is hardcoded here!
    string globname = xrldir + "/*.xrls";
    glob_t pglob;
    if (glob(globname.c_str(), 0, 0, &pglob) != 0) {
	globfree(&pglob);
	errmsg = c_format("Failed to find XRL files in %s", xrldir.c_str());
	xorp_throw(InitError, errmsg);
    }

    if (pglob.gl_pathc == 0) {
	globfree(&pglob);
	errmsg = c_format("Failed to find any XRL files in %s",
			  xrldir.c_str());
	xorp_throw(InitError, errmsg);
    }
    
    for (size_t i = 0; i < (size_t)pglob.gl_pathc; i++) {
	_targets.push_back(XRLtarget(pglob.gl_pathv[i], _verbose));
    }

    globfree(&pglob);

    debug_msg("XRLdb initialized\n");
    debug_msg("%s\n", str().c_str());
}

bool
XRLdb::check_xrl_syntax(const string& xrlstr) const
{
    debug_msg("XRLdb: checking xrl syntax: %s\n", xrlstr.c_str());

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
XRLdb::check_xrl_exists(const string& xrlstr) const
{
    debug_msg("XRLdb: checking xrl exists: %s\n", xrlstr.c_str());

    string::size_type start = xrlstr.find("->");
    string xrlspec;
    string rspec;

    if (start == string::npos) {
	xrlspec = xrlstr;
	rspec.empty();
    } else {
	xrlspec = xrlstr.substr(0, start);
	rspec = xrlstr.substr(start + 2, xrlstr.size() - (start + 2));
    }

    Xrl test_xrl(xrlspec.c_str());    
    XrlArgs test_atoms(rspec.c_str());
    XRLMatchType match;
    XRLMatchType failure_type = MATCH_FAIL;
    list<XRLtarget>::const_iterator iter;
    for (iter = _targets.begin(); iter != _targets.end(); ++iter) {
	match = iter->xrl_matches(test_xrl, test_atoms);
	if (match == MATCH_ALL)
	    return MATCH_ALL;
	if (match == MATCH_XRL)
	    failure_type = MATCH_XRL;
    }
    return failure_type;
}

string 
XRLdb::str() const
{
    string s = "XRLdb contains\n";
    list<XRLtarget>::const_iterator iter;

    for (iter = _targets.begin(); iter != _targets.end(); ++iter) {
	s += iter->str() + "\n";
    }
    return s;
}
