// -*- c-basic-offset: 4 -*-

// Copyright (c) 2001,2002 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the Xorp LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the Xorp LICENSE file; the license in that file is
// legally binding.

// $Id$

#ifndef __XRL_CMD_MAP_HH__
#define __XRL_CMD_MAP_HH__

#include <map>
#include <string>

#include "libxorp/callback.hh"
#include "xrl.hh"
#include "xrl_error.hh"

typedef
XorpCallback2<const XrlCmdError, const Xrl&, XrlArgs*>::RefPtr XrlRecvCallback;

struct XrlCmdEntry {
    XrlCmdEntry(const char* s, XrlRecvCallback cb) :
	name(s), callback(cb) {}
    //    XrlCmdEntry() : name("") {} // this has to be here for map<>.
    string		name;
    XrlRecvCallback	callback;
};

class XrlCmdMap {
public:
    XrlCmdMap(const char* name = "anonymous") : _name(name) {}

    const string& name() { return _name; }
    
    inline bool add_handler(const char* cmd, const XrlRecvCallback& rcb)
    {
	return add_handler(XrlCmdEntry(cmd, rcb));
    }

    bool add_handler (const XrlCmdEntry& c);

    bool remove_handler (const char* name);

    const XrlCmdEntry* get_handler(const char* name) const;

    uint32_t count_handlers() const;

    const XrlCmdEntry* get_handler(uint32_t index) const;

protected:
    const string _name;

    typedef map<string, XrlCmdEntry> CmdMap;
    typedef CmdMap::const_iterator CMI;
    typedef CmdMap::iterator MI;

    CmdMap _cmd_map;
};

#endif // __XRL_CMD_MAP_HH__
