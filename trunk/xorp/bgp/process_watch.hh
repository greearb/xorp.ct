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

// $XORP: xorp/bgp/process_watch.hh,v 1.5 2003/08/21 19:00:30 atanu Exp $

#ifndef __BGP_PROCESS_WATCH_HH__
#define __BGP_PROCESS_WATCH_HH__

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/eventloop.hh"
#include "libxorp/timer.hh"
#include "libxipc/xrl_std_router.hh"
#include "xrl/interfaces/finder_event_notifier_xif.hh"

class ProcessWatch {
public:
    typedef XorpCallback0<void>::RefPtr TerminateCallback;

    ProcessWatch(XrlStdRouter *xrl_router, EventLoop& eventloop,
		 TerminateCallback cb);

    /**
     * Method to call when the birth of a process has been detected.
     */
    void birth(const string& target_class, const string& target_instance);

    /**
     * Method to call when the death of a process has been detected.
     */
    void death(const string& target_class, const string& target_instance);

    /**
     * Method to call if the finder dies.
     */
    void finder_death();

    /**
     * Start a timer to kill this process if for some reason we get
     * hung up.
     */
    void start_kill_timer();

    /**
     * @return Return true when all the processes that BGP requires
     * for correct operation are running.
     */
    bool ready() const;

    /**
     * @return true if the target process exists.
     */
    bool process_exists(const string& target) const;

protected:
    void interest_callback(const XrlError& error);
    void add_process(const string& target_class,
		     const string& target_instance);
    void remove_process(const string& target_class,
			const string& target_instance);

private:
    EventLoop& _eventloop;
    TerminateCallback _shutdown;

    bool _fea;
    bool _rib;

    string _fea_instance;
    string _rib_instance;

    XorpTimer _shutdown_timer;

    class Process {
    public:
	Process(string c, string i) : _target_class(c), _target_instance(i)
	{}
	string _target_class;
	string _target_instance;
    };

    list<Process> _processes;
};

#endif // __BGP_PROCESS_WATCH_HH__
