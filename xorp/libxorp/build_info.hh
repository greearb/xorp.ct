// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2012 XORP, Inc and Others
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


#ifndef __XORP_BUILD_INFO_INC__
#define __XORP_BUILD_INFO_INC__

class BuildInfo {
public:
    /** As in: 1.8.5-WIP */
#define DEFSTR1(a) #a
#define DEFSTR(a) DEFSTR1(a)
    static const char* getXorpVersion() {
	return DEFSTR(XORP_VERSION);
    }

#ifdef XORP_BUILDINFO
    /** git md5sum for HEAD */
    static const char* getGitVersion();
    /** Last 3 git change logs */
    static const char* getGitLog();

    static const char* getShortBuildDate();
    static const char* getBuildDate();
    static const char* getBuilder();
    static const char* getBuildMachine();
#endif
};

#endif
