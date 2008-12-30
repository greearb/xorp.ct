// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/bgp/process_watch.hh,v 1.15 2008/07/23 05:09:35 pavlin Exp $

#ifndef __BGP_PROCESS_WATCH_HH__
#define __BGP_PROCESS_WATCH_HH__

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/eventloop.hh"
#include "libxorp/timer.hh"
#include "libxipc/xrl_std_router.hh"

class ProcessWatch {
public:
    typedef XorpCallback0<void>::RefPtr TerminateCallback;

    ProcessWatch(XrlStdRouter *xrl_router, EventLoop& eventloop,
		 const char *bgp_mib_name,
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
    void finder_death(const char *file, const int lineno);

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
    bool target_exists(const string& target) const;

protected:
    void interest_callback(const XrlError& error);
    void add_target(const string& target_class,
		     const string& target_instance);
    void remove_target(const string& target_class,
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
