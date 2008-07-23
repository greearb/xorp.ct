// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2006-2008 XORP, Inc.
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

#ident "$XORP: xorp/bgp/crash_dump.cc,v 1.6 2008/03/10 20:25:00 pavlin Exp $"

#include "bgp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/c_format.hh"
#include "libxorp/timer.hh"

#include "crash_dump.hh"

#ifndef HOST_OS_WINDOWS
#include <pwd.h>
#endif

#define CRASHLOG_SIZE 100

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
	XLOG_WARNING("Failed to open dump file: %s", filename.c_str());
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
    _logfirst = 0;
    _loglast = 0;

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

void
CrashDumper::log(const string& msg)
{
    if (_logfirst == _loglast) {
	// first time we're called, allocate the storage
	_log.resize(CRASHLOG_SIZE);
	_logtimes.resize(CRASHLOG_SIZE);
    }

    if ( ((_loglast + 1) % CRASHLOG_SIZE) == _logfirst) {
	// need to overwrite an old entry
	_loglast = _logfirst;
	_logfirst = ((_logfirst + 1) % CRASHLOG_SIZE);
    } else {
	// there's still space
	_loglast = ((_loglast + 1) % CRASHLOG_SIZE);
    }

    _log[_loglast] = msg;

    TimeVal tv;
    TimerList::system_gettimeofday(&tv);
    _logtimes[_loglast] = tv;
}

string
CrashDumper::dump_state() const
{
    string s;
    if (_logfirst != _loglast) {
	s = "Audit Log:\n";
	int i = _logfirst;
	while (1) {
	    s += _logtimes[i].str() + " " + _log[i] + "\n";
	    if (i == _loglast)
		break;
	    i = ((i + 1) % CRASHLOG_SIZE);
	}
    }
    return s;
}
