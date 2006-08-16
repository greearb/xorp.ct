// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2006 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/crash_dump.cc,v 1.1 2006/08/15 23:04:40 mjh Exp $"

#include "bgp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/c_format.hh"

#include "crash_dump.hh"

#ifndef HOST_OS_WINDOWS
#include <pwd.h>
#endif

//make it real
CrashDumpManager CrashDumper::_mgr;


CrashDumpManager::CrashDumpManager()
{
}


void
CrashDumpManager::register_dumper(CrashDumper *dumper)
{
    _dumpers.push_back(dumper);
}

void
CrashDumpManager::unregister_dumper(CrashDumper *dumper)
{
    list <CrashDumper*>::iterator i;
    for (i = _dumpers.begin(); i != _dumpers.end(); ++i) {
	if (*i == dumper) {
	    _dumpers.erase(i);
	    return;
	}
    }
    XLOG_UNREACHABLE();
}

void
CrashDumpManager::crash_dump()
{
    FILE *dumpfile;

#ifndef HOST_OS_WINDOWS
    struct passwd *pwd = getpwuid(getuid());
    string filename = "/tmp/bgp_dump.";
    filename += pwd->pw_name;
#else
    char *tmppath = (char *)malloc(256);
    GetTempPathA(256, tmppath);
    string filename = string(tmppath) + "bgp_dump";
    free(tmppath);
#endif

    dumpfile = fopen(filename.c_str(), "w");
    if (dumpfile == NULL) {
	XLOG_WARNING(c_format("Failed to open dump file: %s\n",
			      filename.c_str()).c_str());
	return;
    }

    list <CrashDumper*>::iterator i;
    for (i = _dumpers.begin(); i != _dumpers.end(); i++) {
	string s = (*i)->dump_state();
	fwrite(s.c_str(), 1, s.size(), dumpfile);
    }

    fclose(dumpfile);
}

CrashDumper::CrashDumper()
{
    _mgr.register_dumper(this);
}

CrashDumper::~CrashDumper()
{
    _mgr.unregister_dumper(this);
}

void
CrashDumper::crash_dump() const
{
    _mgr.crash_dump();
}
