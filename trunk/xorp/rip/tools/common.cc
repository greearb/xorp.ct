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

#ident "$XORP: xorp/rip/tools/common.cc,v 1.1 2004/06/03 00:41:34 hodson Exp $"

#include <string>

#include "rip/rip_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "libxorp/eventloop.hh"
#include "libxipc/xrl_std_router.hh"

#include "common.hh"

const char*
default_xrl_target(uint32_t ip_version)
{
    if (ip_version == 6) {
	return "ripng";
    } else if (ip_version == 4) {
	return "rip";
    } else {
	return 0;
    }
}

uint32_t
rip_name_to_ip_version(const char* rip_name)
{
    static const char* m = "ripng";
    size_t n_m = sizeof(m) / sizeof(m[0]);

    uint16_t i = 0;
    while (rip_name[i] == m[i]) {
	i++;
	if (i == n_m)
	    break;
    }
    if (i == 3) return 4;
    if (i == n_m) return 6;
    return 0;
}

bool
parse_finder_args(const string& host_colon_port, string& host, uint16_t& port)
{
    string::size_type sp = host_colon_port.find(":");
    if (sp == string::npos) {
        host = host_colon_port;
        // Do not set port, by design it has default finder port value.
    } else {
        host = string(host_colon_port, 0, sp);
        string s_port = string(host_colon_port, sp + 1, 14);
        uint32_t t_port = atoi(s_port.c_str());
        if (t_port == 0 || t_port > 65535) {
            XLOG_ERROR("Finder port %d is not in range 1--65535.\n", t_port);
            return false;
        }
        port = (uint16_t)t_port;
    }
    return true;
}

/**
 * Xrl Job Queue
 */
XrlJobQueue::XrlJobQueue(EventLoop& 	e,
	    const string& 		finder_host,
	    uint16_t 			finder_port,
	    const string& 		tgtname)
    : _e(e), _fhost(finder_host), _fport(finder_port), _tgt(tgtname),
      _rtr(0), _rtr_poll_cnt(0)
{
    set_status(READY);
}

XrlJobQueue::~XrlJobQueue()
{
    delete _rtr;
}

bool
XrlJobQueue::startup()
{
    string cls = c_format("%s-%u\n", xlog_process_name(), (uint32_t)getpid());
    _rtr = new XrlStdRouter(_e, cls.c_str(), _fhost.c_str(), _fport);
    _rtr->finalize();
    set_status(STARTING);
    _rtr_poll = _e.new_periodic(100,
			callback(this, &XrlJobQueue::xrl_router_ready_poll));
    return true;
}

bool
XrlJobQueue::shutdown()
{
    while (_jobs.empty() == false) {
	_jobs.pop_front();
    }
    set_status(SHUTDOWN);
    return true;
}

void
XrlJobQueue::dispatch_complete(const XrlError& xe, const XrlJobBase* cmd)
{
    XLOG_ASSERT(_jobs.empty() == false);
    XLOG_ASSERT(_jobs.front().get() == cmd);

    if (xe != XrlError::OKAY()) {
	if (xe == XrlError::COMMAND_FAILED()) {
	    cout << "Error: " << xe.note() << endl;
	} else {
	    cout << xe.str() << endl;
	}
	shutdown();
	return;
    }
    _jobs.pop_front();
    if (_jobs.empty() == false) {
	process_next_job();
    } else {
	shutdown();
    }
}

bool
XrlJobQueue::xrl_router_ready_poll()
{
    if (_rtr->ready()) {
	process_next_job();
	return false;
    }
    if (_rtr_poll_cnt++ > 50) {
	set_status(FAILED, "Could not contact XORP Finder process");
    }
    return true;
}

void
XrlJobQueue::process_next_job()
{
    if (_jobs.front()->dispatch() == false) {
	set_status(FAILED, "Could not dispatch xrl");
    }
}
