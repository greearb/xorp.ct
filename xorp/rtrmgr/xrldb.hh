// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2010 XORP, Inc and Others
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

// $XORP: xorp/rtrmgr/xrldb.hh,v 1.14 2008/10/02 21:58:27 bms Exp $

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
#ifdef VALIDATE_XRLDB
    bool check_xrl_syntax(const string& xrl) const;
    XRLMatchType check_xrl_exists(const string& xrl) const;
#endif
    string str() const;

private:
    list<XRLtarget> _targets;
    bool	_verbose;	// Set to true if output is verbose
};

#endif // __RTRMGR_XRLDB_HH__
