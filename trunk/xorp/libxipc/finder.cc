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

#ident "$XORP: xorp/libxipc/finder.cc,v 1.7 2003/04/23 20:50:45 hodson Exp $"

#include "finder_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "finder_tcp.hh"
#include "finder.hh"

#include "finder_xrl_queue.hh"

///////////////////////////////////////////////////////////////////////////////
// FinderTarget
//
// This class is a container for values associated with a particular
// target.  It contains a one-to-many mapping for values associated with
// the target, eg unresolved xrl to resolved xrls, name-to-values, etc...
//

class FinderTarget {
public:
    typedef Finder::Resolveables Resolveables;
    typedef map<string, Resolveables> ResolveMap;
public:
    FinderTarget(const string& name,
		 const string& classname,
		 const string& cookie,
		 FinderMessengerBase* fm)
	: _name(name), _classname(classname), _cookie(cookie),
	  _enabled(false), _messenger(fm)
    {}

    inline const string& name() const { return _name; }

    inline const string& classname() const { return _classname; }

    inline const string& cookie() const { return _cookie; }

    inline bool enabled() const { return _enabled; }

    inline void set_enabled(bool en) { _enabled = en; }
    
    inline const FinderMessengerBase* messenger() const
    {
	return _messenger;
    }

    inline const ResolveMap& resolve_map() const
    {
	return _resolutions;
    }

    bool add_resolution(const string& key, const string& value)
    {
	Resolveables& r = _resolutions[key];
	if (find(r.begin(), r.end(), value) == r.end()) 
	    r.push_back(value);
	return true;
    }

    bool remove_resolutions(const string& key)
    {
	ResolveMap::iterator i = _resolutions.find(key);
	if (_resolutions.end() != i) {
	    _resolutions.erase(i);
	    return true;
	}
	return false;
    }

    const Resolveables*
    resolveables(const string& key) const
    {
	ResolveMap::const_iterator i = _resolutions.find(key);
	if (_resolutions.end() == i)
	    return false;
	return &i->second;
    }

protected:
    string			_name;		// name of target
    string			_classname;	// name of target class
    string			_cookie;
    bool			_enabled;
    ResolveMap			_resolutions;	// items registered by target
    FinderMessengerBase*	_messenger;	// source of registrations
};

///////////////////////////////////////////////////////////////////////////////
//
// Finder
//

Finder::Finder(EventLoop& e) : _e(e), _cmds("finder"), _active_messenger(0)
{
}

Finder::~Finder()
{
    _targets.clear();
    while (false == _messengers.empty()) {
	FinderMessengerBase* old_front = _messengers.front();
	delete _messengers.front();
	// Expect death event for messenger to remove item from list
	assert(_messengers.front() != old_front);
    }
}

void
Finder::messenger_active_event(FinderMessengerBase* m)
{
    XLOG_ASSERT(0 == _active_messenger);
    debug_msg("messenger %p active\n", m);
    _active_messenger = m;
}

void
Finder::messenger_inactive_event(FinderMessengerBase* m)
{
    XLOG_ASSERT(m == _active_messenger);
    debug_msg("messenger %p inactive\n", m);
    _active_messenger = 0;
}

void
Finder::messenger_stopped_event(FinderMessengerBase* m)
{
    debug_msg("Messenger %p stopped.", m);
    delete m;
}

void
Finder::messenger_birth_event(FinderMessengerBase* m)
{
    XLOG_ASSERT(
	_messengers.end() == find(_messengers.begin(), _messengers.end(), m)
	);
    _messengers.push_back(m);

    debug_msg("messenger %p birth\n", m);    
    XLOG_ASSERT(_out_queues.end() == _out_queues.find(m));
    _out_queues.insert(OutQueueTable::value_type(m,
						FinderXrlCommandQueue(*m)));
}

void
Finder::messenger_death_event(FinderMessengerBase* m)
{
    debug_msg("Finder::messenger %p death\n", m);    
    // 1. Remove from Messenger list
    FinderMessengerList::iterator mi;

    mi = find(_messengers.begin(), _messengers.end(), m);
    XLOG_ASSERT(_messengers.end() != mi);
    _messengers.erase(mi);

    // 2. Clear up queue associated with messenger
    OutQueueTable::iterator oi;
    oi = _out_queues.find(m);
    XLOG_ASSERT(_out_queues.end() != oi);
    _out_queues.erase(oi);

    // 3. Walk targets associated with messenger, remove them and announce fact
    TargetTable::iterator ti;
    for (ti = _targets.begin(); ti != _targets.end(); ++ti) {
	FinderTarget& tgt = ti->second;
	if (tgt.messenger() == m) {
	    announce_departure(tgt.name());
	    _targets.erase(ti);
	}
    }
}

bool
Finder::manages(const FinderMessengerBase* m) const
{
    return _messengers.end() !=
	find(_messengers.begin(), _messengers.end(), m);
}

size_t
Finder::messengers() const
{
    return _messengers.size();
}

XrlCmdMap&
Finder::commands()
{
    return _cmds;
}

bool
Finder::add_target(const string& tgt,
		   const string& cls,
		   const string& cookie)
{
    debug_msg("add_target %s / %s / %s\n", tgt.c_str(), cls.c_str(),
	      cookie.c_str());

    TargetTable::const_iterator existing = _targets.find(tgt);
    if (existing->second.messenger() == _active_messenger) {
	debug_msg("already registered by messenger.\n");
	return true;
    }

    if (_targets.end() == existing) {
	_targets.insert(
			TargetTable::value_type(tgt,
				FinderTarget(tgt, cls, cookie,
					       _active_messenger))
			);
	return true;
    }
    debug_msg("FAIL registered by another messenger.");
    return false;

}

bool
Finder::active_messenger_represents_target(const string& tgt) const
{
    TargetTable::const_iterator i = _targets.find(tgt);
    if (_targets.end() == i) {
	debug_msg("Failed to find target %s\n", tgt.c_str());
	for (TargetTable::const_iterator ci = _targets.begin();
	     ci != _targets.end(); ++ci) {
	    debug_msg("Target \"%s\"\n", ci->first.c_str());
	}
	return false;
    } else {
	return i->second.messenger() == _active_messenger;
    }
}

bool
Finder::remove_target_with_cookie(const string& cookie)
{
    TargetTable::iterator i;
    for (i = _targets.begin(); i != _targets.end(); ++i) {
	if (i->second.cookie() == cookie) {
	    debug_msg("Removing target %s / %s / %s\n",
		      i->second.name().c_str(),
		      i->second.classname().c_str(),
		      i->second.cookie().c_str());
	    announce_departure(i->second.name());
	    _targets.erase(i);

	    return true;
	}
    }
    debug_msg("Failed to find target with cookie %s\n", cookie.c_str());
    return false;
}

bool
Finder::remove_target(const string& tgt)
{
    TargetTable::iterator i = _targets.find(tgt);

    if (_targets.end() == i) {
	return false;
    }

    if (i->second.messenger() != _active_messenger) {
	// XXX TODO walk list of targets and print out what offending
	// messenger is responsible for + string representation of messenger.
	XLOG_WARNING("Messenger illegally attempted to remove %s\n",
		     tgt.c_str());
	return false;
    }
    
    _targets.erase(i);
    announce_departure(tgt);

    return true;
}

bool
Finder::set_target_enabled(const string& tgt, bool en)
{
    TargetTable::iterator i = _targets.find(tgt);
    if (_targets.end() == i) {
	return false;
    }
    i->second.set_enabled(en);
    return true;
}

bool
Finder::target_enabled(const string& tgt, bool& en) const
{
    TargetTable::const_iterator i = _targets.find(tgt);
    if (_targets.end() == i) {
	return false;
    }
    en = i->second.enabled();
    return true;
}

bool
Finder::add_resolution(const string& tgt,
		       const string& key,
		       const string& value)
{
    TargetTable::iterator i = _targets.find(tgt);

    if (_targets.end() == i) {
	return false;
    }

    if (i->second.messenger() != _active_messenger) {
	XLOG_WARNING("Messenger illegally attempted to add to %s\n",
		     tgt.c_str());
	return false;
    }

    FinderTarget& t = i->second;
    return t.add_resolution(key, value);
}

bool
Finder::remove_resolutions(const string& tgt,
			   const string& key)
{
    TargetTable::iterator i = _targets.find(tgt);

    if (_targets.end() == i) {
	return false;
    }

    if (i->second.messenger() != _active_messenger) {
	XLOG_WARNING("Messenger illegally attempted to add to %s\n",
		     tgt.c_str());
	return false;
    }

    FinderTarget& t = i->second;
    if (t.remove_resolutions(key)) {
	announce_departure(tgt, key);
	return true;
    }
    return false;
}

const Finder::Resolveables*
Finder::resolve(const string& tgt, const string& key)
{
    TargetTable::iterator i = _targets.find(tgt);
    if (_targets.end() == i) {
	return 0;
    }
    return i->second.resolveables(key);
}

void
Finder::announce_departure(const string& tgt)
{
    FinderMessengerList::iterator i;

    for (i = _messengers.begin(); i != _messengers.end(); ++i) {
	OutQueueTable::iterator qi = _out_queues.find(*i);
	XLOG_ASSERT(_out_queues.end() != qi);
	FinderXrlCommandQueue& q = qi->second;
	q.enqueue(new FinderSendRemoveXrls(q, tgt));
    }
}

void
Finder::announce_departure(const string& tgt, const string& key)
{
    FinderMessengerList::iterator i;

    for (i = _messengers.begin(); i != _messengers.end(); ++i) {
	OutQueueTable::iterator qi = _out_queues.find(*i);
	XLOG_ASSERT(_out_queues.end() != qi);
	FinderXrlCommandQueue& q = qi->second;
	q.enqueue(new FinderSendRemoveXrl(q, tgt, key));
    }
}

bool
Finder::fill_target_list(list<string>& tgt_list) const
{
    TargetTable::const_iterator ci;
    for (ci = _targets.begin(); ci != _targets.end(); ++ci) {
	tgt_list.push_back(ci->first);
    }
    return true;
}

bool
Finder::fill_targets_xrl_list(const string& target,
			      list<string>& xrl_list) const
{
    TargetTable::const_iterator ci = _targets.find(target);
    if (_targets.end() == ci) {
	return false;
    }

    FinderTarget::ResolveMap::const_iterator
	cmi = ci->second.resolve_map().begin();

    const FinderTarget::ResolveMap::const_iterator
	end = ci->second.resolve_map().end();
    
    while (end != cmi) {
	xrl_list.push_back(cmi->first);
	++cmi;
    }
    
    return true;
}
