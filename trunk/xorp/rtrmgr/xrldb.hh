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

// $XORP: xorp/rtrmgr/xrldb.hh,v 1.6 2004/05/28 18:26:29 pavlin Exp $

#ifndef __RTRMGR_XRLDB_HH__
#define __RTRMGR_XRLDB_HH__


#include <list>

#include "libxipc/xrl_router.hh"

#include "rtrmgr_error.hh"


enum XRLMatchType {
    MATCH_FAIL  = 0x0,
    MATCH_XRL   = 0x1,
    MATCH_RSPEC = 0x2,
    MATCH_ALL   = MATCH_XRL | MATCH_RSPEC
};

class XrlSpec {
public:
    XrlSpec(const Xrl& xrl, const XrlArgs& rspec, bool verbose);
    XRLMatchType matches(const Xrl& xrl, const XrlArgs& rspec) const;
    string str() const;

private:
    Xrl		_xrl;		// The XRL itself
    XrlArgs	_rspec;		// The return spec
    bool	_verbose;	// Set to true if output is verbose
};

class XRLtarget {
public:
    XRLtarget(const string& xrlfilename, bool verbose);

    XRLMatchType xrl_matches(const Xrl& test_xrl, const XrlArgs& rspec) const;
    string str() const;

private:
    string	_targetname;
    list<XrlSpec> _xrlspecs;
    bool	_verbose;	// Set to true if output is verbose
};

class XRLdb {
public:
    XRLdb(const string& xrldir, bool verbose) throw (InitError);
    bool check_xrl_syntax(const string& xrl) const;
    XRLMatchType check_xrl_exists(const string& xrl) const;
    string str() const;

private:
    list<XRLtarget> _targets;
    bool	_verbose;	// Set to true if output is verbose
};

#endif // __RTRMGR_XRLDB_HH__
