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

#ident "$XORP: xorp/libxipc/finder.cc,v 1.10 2003/05/09 21:00:51 hodson Exp $"

#include "finder_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "finder_tcp.hh"
#include "finder.hh"

#include "finder_xrl_queue.hh"

///////////////////////////////////////////////////////////////////////////////
//
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
		 const string& class_name,
		 const string& cookie,
		 FinderMessengerBase* fm)
	: _name(name), _class_name(class_name), _cookie(cookie),
	  _enabled(false), _messenger(fm)
    {}

    ~FinderTarget()
    {
	debug_msg("Destructing %s\n", name().c_str());
    }
    
    inline const string& name() const { return _name; }

    inline const string& class_name() const { return _class_name; }

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
	if (_resolutions.end() == i) {
	    debug_msg("Looking for \"%s\"\n", key.c_str());
	    for (i = _resolutions.begin(); i != _resolutions.end(); ++i) {
		debug_msg("Have \"%s\"\n", i->first.c_str());
	    }
	    return false;
	}
	return &i->second;
    }

protected:
    string			_name;		// name of target
    string			_class_name;	// name of target class
    string			_cookie;
    bool			_enabled;
    ResolveMap			_resolutions;	// items registered by target
    FinderMessengerBase*	_messenger;	// source of registrations
};


///////////////////////////////////////////////////////////////////////////////
//
// FinderClass
//
// Stores information about which classes exist and what instances
// exist.
//

class FinderClass
{
public:
    FinderClass(const string& name, bool singleton)
	: _name(name), _singleton(singleton)
    {}

    inline bool singleton() const { return _singleton; }

    inline const list<string>& instances() const { return _instances; }

    inline bool
    add_instance(const string& instance)
    {
	list<string>::const_iterator i = find(_instances.begin(),
					      _instances.end(),
					      instance);
	if (i != _instances.end()) {
	    debug_msg("instance %s already exists.\n",
		    instance.c_str());
	    return false;
	}
	debug_msg("added instance \"%s\".\n",
		instance.c_str());

	debug_msg("Adding instance %s to class %s\n",
		  instance.c_str(), _name.c_str());
	_instances.push_back(instance);
	return true;
    }

    inline bool
    remove_instance(const string& instance)
    {
	list<string>::iterator i = find(_instances.begin(), _instances.end(),
					instance);
	if (i == _instances.end()) {
	    debug_msg("Failed to remove unknown instance %s from class %s\n",
		      instance.c_str(), _name.c_str());

	    return false;
	}
	_instances.erase(i);
	debug_msg("Removed instance %s from class %s\n",
		  instance.c_str(), _name.c_str());

	return true;
    }
    
protected:
    string	 _name;
    list<string> _instances;
    bool	 _singleton;
};


///////////////////////////////////////////////////////////////////////////////
//
// Internal consistency check
//

static void
validate_finder_classes_and_instances(const Finder::ClassTable&  classes,
				      const Finder::TargetTable& targets)

{
    typedef Finder::ClassTable  ClassTable;
    typedef Finder::TargetTable TargetTable;

    //
    // Validate each instance associated with classes table exists in
    // TargetTable
    //
    for (ClassTable::const_iterator ci = classes.begin();
	 ci != classes.end(); ++ci) {
	XLOG_ASSERT(ci->second.instances().empty() == false);
	for (list<string>::const_iterator ii = ci->second.instances().begin();
	     ii != ci->second.instances().end(); ++ii) {
	    if (targets.find(*ii) == targets.end()) {
		for (TargetTable::const_iterator ti = targets.begin();
		     ti != targets.end(); ti++) {
		    debug_msg("Known target \"%s\"\n", ti->first.c_str());
		}
		XLOG_FATAL("Missing instance (%s) / %d\n", ii->c_str(),
			   targets.size());
	    }
	}
    }

    //
    // Validate each instance in the target table exists is associated
    // with a class entry.
    //
    for (TargetTable::const_iterator ti = targets.begin();
	 ti != targets.end(); ++ti) {
	XLOG_ASSERT(ti->first == ti->second.name());
	ClassTable::const_iterator ci = classes.find(ti->second.class_name());
	if (ci == classes.end()) {
	    XLOG_FATAL("Class (%s) associated with instance (%s) does not "
		       "appear in class table.\n",
		       ti->second.class_name().c_str(),
		       ti->second.name().c_str());
	    if (find(ci->second.instances().begin(),
		     ci->second.instances().end(),
		     ti->second.name()) == ci->second.instances().end()) {
		XLOG_FATAL("Instance (%s) is not associated with class in "
			   "expected class (%s).\n",
			   ti->second.name().c_str(),
			   ti->second.class_name().c_str());
	    }
	}
    }
}


///////////////////////////////////////////////////////////////////////////////
//
// Finder
//

Finder::Finder(EventLoop& e) : _e(e), _cmds("finder"), _active_messenger(0)
{
}

Finder::~Finder()
{
    validate_finder_classes_and_instances(_classes, _targets);
    _targets.clear();
    _classes.clear();
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
						FinderXrlCommandQueue(m)));
    if (hello_timer_running() == false)
	start_hello_timer();
}

void
Finder::messenger_death_event(FinderMessengerBase* m)
{
    validate_finder_classes_and_instances(_classes, _targets);

    debug_msg("Finder::messenger %p death\n", m);    

    //
    // 1. Remove from Messenger list
    //
    FinderMessengerList::iterator mi;
    mi = find(_messengers.begin(), _messengers.end(), m);
    XLOG_ASSERT(_messengers.end() != mi);
    _messengers.erase(mi);

    //
    // 2. Clear up queue associated with messenger
    //
    OutQueueTable::iterator oi;
    oi = _out_queues.find(m);
    XLOG_ASSERT(_out_queues.end() != oi);
    _out_queues.erase(oi);

    //
    // 3. Walk targets associated with messenger, remove them and announce fact
    //
    validate_finder_classes_and_instances(_classes, _targets);

    for (TargetTable::iterator ti = _targets.begin(); ti != _targets.end(); ) {
	FinderTarget& tgt = ti->second;
	if (tgt.messenger() != m) {
	    ti++;
	    continue;
	}
	remove_target(ti);
	break;
    }
    validate_finder_classes_and_instances(_classes, _targets);
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
		   bool		 singleton,
		   const string& cookie)
{
    validate_finder_classes_and_instances(_classes, _targets);
    //
    // Add instance
    //
    debug_msg("add_target %s / %s / %s\n", tgt.c_str(), cls.c_str(),
	      cookie.c_str());

    TargetTable::const_iterator ci = _targets.find(tgt);
    if (ci->second.messenger() == _active_messenger) {
	debug_msg("already registered by messenger.\n");
	return true;
    } else if (ci != _targets.end()) {
	debug_msg("Fail registered by another messenger.");
	return false;
    }

    pair<TargetTable::iterator, bool> r =
	_targets.insert(
	    TargetTable::value_type(tgt,
				    FinderTarget(tgt, cls, cookie,
						 _active_messenger)));
    if (r.second == false) {
	debug_msg("Failed to insert target.");
	return false;
    }

    //
    // Add class and instance to class
    //
    if (add_class_instance(cls, tgt, singleton) == false) {
	debug_msg("Failed to register class instance");
	_targets.erase(r.first);
	return false;
    }

    validate_finder_classes_and_instances(_classes, _targets);
    return true;
}

bool
Finder::active_messenger_represents_target(const string& tgt) const
{
    validate_finder_classes_and_instances(_classes, _targets);
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
    validate_finder_classes_and_instances(_classes, _targets);    
}

void
Finder::remove_target(TargetTable::iterator& i)
{
    validate_finder_classes_and_instances(_classes, _targets);    

    FinderTarget& t = i->second;
    debug_msg("Removing target %s / %s / %s\n",
	      t.name().c_str(),
	      t.class_name().c_str(),
	      t.cookie().c_str());

    announce_departure(t.name());
    if (primary_instance(t.class_name()) == t.name()) {
	announce_departure(t.class_name());
    }
    remove_class_instance(t.class_name(), t.name());
    _targets.erase(i);

    validate_finder_classes_and_instances(_classes, _targets);    
}

bool
Finder::remove_target_with_cookie(const string& cookie)
{
    validate_finder_classes_and_instances(_classes, _targets);

    TargetTable::iterator i;
    for (i = _targets.begin(); i != _targets.end(); ++i) {
	if (i->second.cookie() != cookie)
	    continue;
	remove_target(i);
	validate_finder_classes_and_instances(_classes, _targets);	
	return true;
    }
    debug_msg("Failed to find target with cookie %s\n", cookie.c_str());
    return false;
}

bool
Finder::remove_target(const string& target)
{
    validate_finder_classes_and_instances(_classes, _targets);

    TargetTable::iterator i = _targets.find(target);

    if (_targets.end() == i) {
	return false;
    }

    FinderTarget& tgt = i->second;
    
    if (tgt.messenger() != _active_messenger) {
	// XXX TODO walk list of targets and print out what offending
	// messenger is responsible for + string representation of messenger.
	XLOG_WARNING("Messenger illegally attempted to remove %s\n",
		     target.c_str());
	return false;
    }
    remove_target(i);
    
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

void
Finder::start_hello_timer()
{
    _hello = _e.new_periodic(1000, callback(this, &Finder::send_hello));
}

bool
Finder::send_hello()
{
    validate_finder_classes_and_instances(_classes, _targets);
    
    OutQueueTable::iterator oqi = _out_queues.begin();
    debug_msg("Send hello\n");

    if (oqi == _out_queues.end()) {
	return false;
    }
    
    do {
	FinderXrlCommandQueue& q = oqi->second;

	q.enqueue(new FinderSendHelloToClient(q, "oxo"));
	++oqi;
    } while (oqi != _out_queues.end());
    
    return true;
}

bool
Finder::class_exists(const string& class_name) const
{
    return _classes.find(class_name) != _classes.end();
}

bool
Finder::add_class_instance(const string& class_name,
			   const string& instance,
			   bool		 singleton)
{
    ClassTable::iterator i = _classes.find(class_name);
    if (i == _classes.end()) {
	pair<ClassTable::iterator, bool> r;
	ClassTable::value_type pv(class_name,
				  FinderClass(class_name, singleton));
	r = _classes.insert(pv);
	if (r.second == false) {
	    // Insertion failed
	    debug_msg("insert failed");
	    return false;
	}
	i = r.first;
    }

    if ((singleton || i->second.singleton()) &&
	i->second.instances().empty() == false) {
	debug_msg("blocked by singleton");
	return false;
    }

    return i->second.add_instance(instance);
}

bool
Finder::remove_class_instance(const string& class_name,
			      const string& instance)
{
    ClassTable::iterator i = _classes.find(class_name);
    if (i == _classes.end()) {
	debug_msg("Attempt to remove unknown class %s not found\n",
		  class_name.c_str());
	return false;
    } else if (i->second.remove_instance(instance) == false) {
	debug_msg("Attempt to remove unknown instance (%s) from class %s",
		  instance.c_str(), class_name.c_str());
	return false;
    }
    if (i->second.instances().empty()) {
	_classes.erase(i);
    }
    debug_msg("Removed class instance (%s) from class %s\n",
	      instance.c_str(), class_name.c_str());
    return true;
}

bool
Finder::class_default_instance(const string& class_name,
			       string&	     instance) const
{
    ClassTable::const_iterator ci = _classes.find(class_name);
    if (ci == _classes.end() || ci->second.instances().empty()) {
	return false;
    }
    instance = ci->second.instances().front();
    return true;
}

const string&
Finder::primary_instance(const string& instance_or_class) const
{
    validate_finder_classes_and_instances(_classes, _targets);
    ClassTable::const_iterator ci = _classes.find(instance_or_class);
    if (ci == _classes.end()) {
	return instance_or_class;
    }
    XLOG_ASSERT(ci->second.instances().empty() == false);
    return ci->second.instances().front();
}
