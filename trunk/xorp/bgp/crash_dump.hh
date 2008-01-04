// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2006-2008 International Computer Science Institute
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

// $XORP: xorp/bgp/crash_dump.hh,v 1.4 2007/02/16 22:45:11 pavlin Exp $

#ifndef __BGP_CRASH_DUMP_HH__
#define __BGP_CRASH_DUMP_HH__

#include <list>
#include <vector>

#include "libxorp/timeval.hh"

class CrashDumper;

class CrashDumpManager {
public:
    CrashDumpManager();
    void register_dumper(CrashDumper *dumper);
    void unregister_dumper(CrashDumper *dumper);
    void crash_dump();

private:
    list <CrashDumper*> _dumpers;
};


class CrashDumper {
public:
    CrashDumper();
    virtual ~CrashDumper();
    virtual void crash_dump() const;
    virtual string dump_state() const;
    void log(const string& msg);
private:
    static CrashDumpManager _mgr;
    vector <string> _log;
    vector <TimeVal> _logtimes;
    int _logfirst, _loglast;
};

#endif // __BGP_CRASH_DUMP_HH__
