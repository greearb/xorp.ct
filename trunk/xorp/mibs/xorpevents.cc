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

#ident "$Header$"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "xorpevents.hh"

// definition and initialization of static members
const char * SnmpEventLoop::_log_name = "SnmpEventLoop";
SnmpEventLoop * SnmpEventLoop::_sel = NULL;

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
    e.export_fds();
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
    e.export_timers();
}

SnmpEventLoop&
SnmpEventLoop::the_instance()
{
    if (!_sel) {
	_sel = new SnmpEventLoop;
	DEBUGMSGTL((_log_name, "new shared event loop...\n"));
    }
    return *_sel;
}

void
SnmpEventLoop::export_events()
{
    export_timers();
    export_fds();
}

void
SnmpEventLoop::export_fds()
{
    fd_set read_fds, write_fds, except_fds;
    FD_ZERO(&read_fds); FD_ZERO(&write_fds); FD_ZERO(&except_fds);
    SelectorList & s = the_instance().selector_list();
    s.get_fd_set(SEL_RD, read_fds);
    s.get_fd_set(SEL_WR, write_fds);
    s.get_fd_set(SEL_EX, except_fds);
    for (int fd = 0; fd<s.get_max_fd()+1; fd++) {
    // the check to see if the fd is already in _exported_*fds is needed
    // because the snmpd call register_*fd does not check for duplicate fd's.
	if (FD_ISSET(fd, &read_fds) &&
	    (_exported_readfds.end() == _exported_readfds.find(fd))) {
	    if (FD_REGISTERED_OK != register_readfd(fd, run_fd_callbacks, NULL))
		snmp_log(LOG_WARNING, "unable to import xorp fd %d\n", fd);
	    else {
		DEBUGMSGTL((_log_name, "imported xorp rdfd:%d\n", fd));
		_exported_readfds.insert(fd);
	    }
	}
	if (FD_ISSET(fd, &write_fds) &&
	    (_exported_writefds.end() == _exported_writefds.find(fd))) {
	    if (FD_REGISTERED_OK != register_writefd(fd, run_fd_callbacks, NULL))
                snmp_log(LOG_WARNING, "unable to import xorp fd %d\n", fd);
	    else {
		DEBUGMSGTL((_log_name, "imported xorp wrfd:%d \n", fd));
		_exported_writefds.insert(fd);
	    }
	}
	if (FD_ISSET(fd, &except_fds) &&
	    (_exported_exceptfds.end() == _exported_exceptfds.find(fd))) {
	    if (FD_REGISTERED_OK != register_exceptfd(fd, run_fd_callbacks, NULL))
                snmp_log(LOG_WARNING, "unable to import xorp fd %d\n", fd);
	    else {
		DEBUGMSGTL((_log_name, "imported xorp exfd:%d \n", fd));
		_exported_exceptfds.insert(fd);
	    }
	}
    }
    // and finally, remove all the fd's that are no longer in use

    SnmpEventLoop::FdSet::iterator c, p;
    for (c = _exported_readfds.begin(); c != _exported_readfds.end();) {
	p = c++;		     // p can now be safely erased if needed
	if (!FD_ISSET(*p, &read_fds)) {
	    DEBUGMSGTL((_log_name, "erased xorp rdfd:%d\n",*p));
	    unregister_readfd(*p);
	    _exported_readfds.erase(p);
	}
    }
    for (c = _exported_writefds.begin(); c != _exported_writefds.end();) {
	p = c++;		     // p can now be safely erased if needed
	if (!FD_ISSET(*p, &write_fds)) {
	    DEBUGMSGTL((_log_name, "erased xorp wrfd:%d\n",*p));
	    unregister_writefd(*p);
	    _exported_writefds.erase(p);
	}
    }
    for (c = _exported_exceptfds.begin(); c != _exported_exceptfds.end();) {
	p = c++;		     // p can now be safely erased if needed
	if (!FD_ISSET(*p, &except_fds)) {
	    DEBUGMSGTL((_log_name, "erased xorp exfd:%d\n",*p));
	    unregister_exceptfd(*p);
	    _exported_exceptfds.erase(p);
	}
    }
}

void
SnmpEventLoop::export_timers()
{
    TimerList & t = the_instance().timer_list();
    TimeVal del_tv, abs_tv;
    timeval snmp_tv;
    unsigned int alarm_id;
    if (!t.get_next_delay(del_tv)) return;
    t.get_next_expire(abs_tv);
    if (_pending_alarms.end() != _pending_alarms.find(abs_tv))
	return;  // timer already exported
    DEBUGMSGTL((_log_name, "imported xorp timeout:%d s %d us \n",
			    del_tv.sec(), del_tv.usec()));
    if ((0 == del_tv.sec()) && (0 == del_tv.usec()))
	{
	DEBUGMSGTL((_log_name, "running callbacks of expired timer(s)\n"));
	SnmpEventLoop& e = SnmpEventLoop::the_instance();
	e.timer_list().run();
	e.export_events();
	return;
	}
    del_tv.copy_out(snmp_tv);
    alarm_id = snmp_alarm_register_hr(snmp_tv, 0, run_timer_callbacks, NULL);
    if (!alarm_id) snmp_log(LOG_WARNING, "unable to import xorp timeout");
    else {
	SnmpEventLoop::AlarmMap::value_type al(abs_tv, alarm_id);
	_pending_alarms.insert(al);
    }
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
    _exported_readfds.erase(_exported_readfds.begin(), _exported_readfds.end());
    _exported_writefds.erase(_exported_writefds.begin(), _exported_writefds.end());
    _exported_exceptfds.erase(_exported_exceptfds.begin(), _exported_exceptfds.end());
}
