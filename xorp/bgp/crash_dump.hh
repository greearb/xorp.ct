// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2006-2011 XORP, Inc and Others
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


#ifndef __BGP_CRASH_DUMP_HH__
#define __BGP_CRASH_DUMP_HH__




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
