// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/profile.cc,v 1.3 2004/09/21 21:44:35 atanu Exp $"

#include "libxorp_module.h"
#include "xorp.h"

#include <sys/time.h>

#include "xlog.h"
#include "debug.h"
#include "profile.hh"

Profile::Profile()
    : _profile_cnt(0)
{
}

inline
void
zap(const pair<string, ref_ptr<Profile::ProfileState> >& p)
{
    p.second->zap();
}

Profile::~Profile()
{
    for_each(_profiles.begin(), _profiles.end(), ptr_fun(zap));
}

void
Profile::create(const string& pname, const string& comment)
    throw(PVariableExists)
{
    // Catch initialization problems.
    if (_profiles.count(pname))
	xorp_throw(PVariableExists, pname.c_str());

    ProfileState *p = new ProfileState(comment, false, false, new logentries);
    _profiles[pname] = ref_ptr<ProfileState>(p);
}

void
Profile::log(const string& pname, string comment)
    throw(PVariableUnknown,PVariableNotEnabled)
{
    profiles::iterator i = _profiles.find(pname);

    // Catch any mispelt pnames.
    if (i == _profiles.end())
	xorp_throw(PVariableUnknown, pname.c_str());

    // In order to be logging, we must be enabled.
    if (!i->second->enabled())
	xorp_throw(PVariableNotEnabled, pname.c_str());
    
#if	0
    // Make sure that this variable is not locked.
    if (!i->second->locked())
	xorp_throw(PVariableLocked, pname.c_str());
#endif
    
    struct timeval tv;
    if (0 != gettimeofday(&tv, 0))
	XLOG_FATAL("gettimeofday failed");
    TimeVal t(tv);
    i->second->logptr()->push_back(ProfileLogEntry(t, comment));
}

void
Profile::enable(const string& pname) throw(PVariableUnknown,PVariableLocked)
{
    profiles::iterator i = _profiles.find(pname);

    // Catch any mispelt pnames.
    if (i == _profiles.end())
	xorp_throw(PVariableUnknown, pname.c_str());

    // If this profile name is already enabled, get out of here
    // without updating the counter.
    if (i->second->enabled())
	return;

    // Don't allow a locked entry to be enabled.
    if (i->second->locked())
	xorp_throw(PVariableLocked, pname.c_str());
    
    i->second->set_enabled(true);
    _profile_cnt++;
}

void
Profile::disable(const string& pname) throw(PVariableUnknown)
{
    profiles::iterator i = _profiles.find(pname);

    // Catch any mispelt pnames.
    if (i == _profiles.end())
	xorp_throw(PVariableUnknown, pname.c_str());

    // If this profile name is already disabled, get out of here
    // without updating the counter.
    if (!i->second->enabled())
	return;
    i->second->set_enabled(false);
    _profile_cnt--;
}

void
Profile::lock_log(const string& pname) throw(PVariableUnknown,PVariableLocked)
{
    profiles::iterator i = _profiles.find(pname);

    // Catch any mispelt pnames.
    if (i == _profiles.end())
	xorp_throw(PVariableUnknown, pname.c_str());

    // Don't allow a locked entry to be locked again.
    if (i->second->locked())
	xorp_throw(PVariableLocked, pname.c_str());

    // Disable logging.
    disable(pname);

    // Lock the entry
    i->second->set_locked(true);

    i->second->set_iterator(i->second->logptr()->begin());
}

bool 
Profile::read_log(const string& pname, ProfileLogEntry& entry) 
    throw(PVariableUnknown,PVariableNotLocked)
{
    profiles::iterator i = _profiles.find(pname);

    // Catch any mispelt pnames.
    if (i == _profiles.end())
	xorp_throw(PVariableUnknown, pname.c_str());

    // Verify that the log entry is locked
    if (!i->second->locked())
	xorp_throw(PVariableNotLocked, pname.c_str());

    logentries::iterator li;
    i->second->get_iterator(li);
    if (li == i->second->logptr()->end())
	return false;

    entry = *li;
    i->second->set_iterator(++li);

    return true;
}

void
Profile::release_log(const string& pname) 
    throw(PVariableUnknown,PVariableNotLocked)
{
    profiles::iterator i = _profiles.find(pname);

    // Catch any mispelt pnames.
    if (i == _profiles.end())
	xorp_throw(PVariableUnknown, pname.c_str());

    // Verify that the log entry is locked
    if (!i->second->locked())
	xorp_throw(PVariableNotLocked, pname.c_str());

    // Unlock the entry
    i->second->set_locked(false);
}

void
Profile::clear(const string& pname) throw(PVariableUnknown,PVariableLocked)
{
    profiles::iterator i = _profiles.find(pname);

    // Catch any mispelt pnames.
    if (i == _profiles.end())
	xorp_throw(PVariableUnknown, pname.c_str());

    // Don't allow a locked entry to be cleared.
    if (i->second->locked())
	xorp_throw(PVariableLocked, pname.c_str());

    i->second->logptr()->clear();
}

class List: public unary_function<pair<string, 
				       ref_ptr<Profile::ProfileState> >,
				  void> {
 public:
    void operator()(const pair<string, ref_ptr<Profile::ProfileState> >& p) {
	_result += p.first;
	_result += "\t";
	_result += c_format("%d", p.second->size());
	_result += "\t";
	_result += p.second->enabled() ? "enabled" : "disabled";
	_result += "\t";
	_result += p.second->comment();
	_result += "\n";
    }

    string result() const {
	return _result;
    }
 private:
    string _result;
};

string
Profile::list() const
{
    return for_each(_profiles.begin(), _profiles.end(), List()).result();
}

