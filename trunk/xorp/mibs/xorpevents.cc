
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

#ident "$Header$"

#define XORP_MODULE_NAME "netsnmpxorp"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "xorpevents.hh"
#include "libxorp/xlog.h"


// definition and initialization of static members
const char * SnmpEventLoop::_log_name = "SnmpEventLoop";
SnmpEventLoop SnmpEventLoop::_sel;

//
// run_fd_callbacks and run_timer_callbacks must be callable from C modules,
// this is why they are not members of SnmpEventLoop.  In principle they are
// only needed in this module, but not declared static so they can be invoked
// from test programs
//
void
run_fd_callbacks (int, void *)
{
    SnmpEventLoop& e = SnmpEventLoop::the_instance();
    /*
     * we know the next call will not block since one of XORP's monitored file
     * descriptors showed some activity, thus the 0 ms timeout
     */
    DEBUGMSGTL((e._log_name, "run all xorp file descriptor callbacks\n"));
    if (!e.selector_list().select(0))
	snmp_log(LOG_WARNING, "call to run_fd_callbacks did nothing\n");
}

void
run_timer_callbacks(u_int alarm_id, void *)
{
    SnmpEventLoop& e = SnmpEventLoop::the_instance();
    DEBUGMSGTL((e._log_name, "run all xorp timers\n"));
    DEBUGMSGTL((e._log_name, "# of timers: %d\n", e.timer_list().size()));
    e.timer_list().run();
    SnmpEventLoop::AlarmMap::iterator p;
    for (p = e._pending_alarms.begin(); p != e._pending_alarms.end(); ++p) {
	if (alarm_id == (*p).second) {
	    e._pending_alarms.erase(p);
	    // exported alarms are all 'one-time', so no need to unregister them
	    // with calls to snmp_alarm_unregister here.
	    break;
	}
    }
}

SnmpEventLoop&
SnmpEventLoop::the_instance()
{
    return _sel;
}

SnmpEventLoop::SnmpEventLoop() : EventLoop(), SelectorListObserverBase(),
    TimerListObserverBase()
{
    timer_list().set_observer(*this);
    selector_list().set_observer(*this);
    DEBUGMSGTL((_log_name, "new shared event loop %p...\n", this));
}

SnmpEventLoop::~SnmpEventLoop()
{
    DEBUGMSGTL((_log_name, "shared event loop freed...!\n"));
    clear_pending_alarms();
    clear_monitored_fds();
}

void 
SnmpEventLoop::notify_added(int fd, const SelectorMask& mask) 
{
  
    switch (mask) {
    case SEL_RD:
	if (_exported_readfds.end() != _exported_readfds.find(fd)) break; 
	if (FD_REGISTERED_OK != register_readfd(fd, run_fd_callbacks, NULL))
	    snmp_log(LOG_WARNING, "unable to import xorp fd %d\n", fd);
	else {
	    DEBUGMSGTL((_log_name, "imported xorp rdfd:%d\n", fd));
	    _exported_readfds.insert(fd);
	}
	break;
    case SEL_WR:
	if (_exported_writefds.end() != _exported_writefds.find(fd)) break; 
	if (FD_REGISTERED_OK != register_writefd(fd, run_fd_callbacks, NULL))
	    snmp_log(LOG_WARNING, "unable to import xorp fd %d\n", fd);
	else {
	    DEBUGMSGTL((_log_name, "imported xorp wrfd:%d \n", fd));
	    _exported_writefds.insert(fd);
	}
	break;
    case SEL_EX:
	if (_exported_exceptfds.end() != _exported_exceptfds.find(fd)) break; 
	if (FD_REGISTERED_OK != register_exceptfd(fd, run_fd_callbacks, NULL))
	    snmp_log(LOG_WARNING, "unable to import xorp fd %d\n", fd);
	else {
	    DEBUGMSGTL((_log_name, "imported xorp exfd:%d \n", fd));
	    _exported_exceptfds.insert(fd);
	}
	break;
   default:
	snmp_log(LOG_WARNING, "invalid mask %d for fd %d\n", mask, fd);
    }
    
}

void 
SnmpEventLoop::notify_removed(int fd, const SelectorMask& mask) 
{
    SnmpEventLoop::FdSet::iterator p;
    switch (mask) {
    case SEL_RD:
	p = _exported_readfds.find(fd);
	if (p == _exported_readfds.end()) break; 
	unregister_readfd(*p);
	_exported_readfds.erase(p);
	break;	
    case SEL_WR:
	p =_exported_writefds.find(fd);
	if (p == _exported_writefds.end()) break;
	unregister_writefd(*p);
	_exported_writefds.erase(p);
	break;	
    case SEL_EX: 
	p = _exported_exceptfds.find(fd);
	if (p == _exported_exceptfds.end()) break;
	unregister_exceptfd(*p);
	_exported_exceptfds.erase(p);
	break;	
    default:
	snmp_log(LOG_WARNING, "invalid mask %d for fd %d\n", mask, fd);
    }
}

void 
SnmpEventLoop::notify_scheduled(const TimeVal& abs_tv) 
{   
    TimeVal del_tv, now_tv;
    timeval snmp_tv;
    unsigned int alarm_id;

    if (_pending_alarms.end() != _pending_alarms.find(abs_tv))
	return;  // timer already exported

    the_instance().timer_list().current_time(now_tv);

    if ((abs_tv < now_tv) || (abs_tv == now_tv)) {
	// if already expired, do not export, just execute callbacks 
	DEBUGMSGTL((_log_name, "running callbacks of expired timer(s)\n"));
	the_instance().timer_list().run();
	return;
    }

    del_tv = abs_tv - now_tv; 
    del_tv.copy_out(snmp_tv);
    alarm_id = snmp_alarm_register_hr(snmp_tv, 0, run_timer_callbacks, NULL);
    if (!alarm_id) snmp_log(LOG_WARNING, "unable to import xorp timeout");
    else {
	SnmpEventLoop::AlarmMap::value_type al(abs_tv, alarm_id);
	_pending_alarms.insert(al);
    }
}

void 
SnmpEventLoop::notify_unscheduled(const TimeVal& abs_tv) 
{
    SnmpEventLoop::AlarmMap::iterator p = _pending_alarms.find(abs_tv);
    if (p == _pending_alarms.end()) return;
    snmp_alarm_unregister((*p).second);
    _pending_alarms.erase(p);
     
}

void
SnmpEventLoop::clear_pending_alarms ()
{
    SnmpEventLoop::AlarmMap::iterator p;
    for (p = _pending_alarms.begin(); p != _pending_alarms.end(); p++)
	snmp_alarm_unregister((*p).second);
    _pending_alarms.erase(_pending_alarms.begin(), _pending_alarms.end());
}

void
SnmpEventLoop::clear_monitored_fds ()
{
    SnmpEventLoop::FdSet::iterator p;
    for (p = _exported_readfds.begin(); p != _exported_readfds.end(); p++)
	unregister_readfd(*p);
    for (p = _exported_writefds.begin(); p != _exported_writefds.end(); p++)
	unregister_writefd(*p);
    for (p = _exported_exceptfds.begin(); p != _exported_exceptfds.end(); p++)
	unregister_exceptfd(*p);
    _exported_readfds.erase(_exported_readfds.begin(),_exported_readfds.end());
    _exported_writefds.erase(
	_exported_writefds.begin(), _exported_writefds.end());
    _exported_exceptfds.erase(
	_exported_exceptfds.begin(), _exported_exceptfds.end());
}
