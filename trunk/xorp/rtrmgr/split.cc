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

#ident "$XORP: xorp/rtrmgr/split.cc,v 1.1.1.1 2002/12/11 23:56:16 hodson Exp $"

#include "rtrmgr_module.h"
#include "config.h"
#include "libxorp/xorp.h"
#include <list>

list<string> split(const string& s, char ch) {
    list <string> parts;
    string s2 = s;
    size_t ix;
    ix = s2.find(ch);
    while (ix != string::npos) {
	parts.push_back(s2.substr(0,ix));
	s2 = s2.substr(ix+1, s2.size()-ix);
	ix = s2.find(ch);
    }
    if (!s2.empty())
	parts.push_back(s2);
    return parts;
}

