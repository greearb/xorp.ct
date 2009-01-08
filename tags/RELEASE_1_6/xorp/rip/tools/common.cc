// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

#ident "$XORP: xorp/rip/tools/common.cc,v 1.13 2008/10/02 21:58:21 bms Exp $"

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
    static const char m[] = "ripng";
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
            XLOG_ERROR("Finder port %u is not in range 1--65535.\n",
		       XORP_UINT_CAST(t_port));
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
    set_status(SERVICE_READY);
}

XrlJobQueue::~XrlJobQueue()
{
    delete _rtr;
}

int
XrlJobQueue::startup()
{
    string cls = c_format("%s-%u\n", xlog_process_name(),
			  XORP_UINT_CAST(getpid()));
    _rtr = new XrlStdRouter(_e, cls.c_str(), _fhost.c_str(), _fport);
    _rtr->finalize();
    set_status(SERVICE_STARTING);
    _rtr_poll = _e.new_periodic_ms(100,
				   callback(this, &XrlJobQueue::xrl_router_ready_poll));
    return (XORP_OK);
}

int
XrlJobQueue::shutdown()
{
    while (_jobs.empty() == false) {
	_jobs.pop_front();
    }
    set_status(SERVICE_SHUTDOWN);
    return (XORP_OK);
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
	set_status(SERVICE_FAILED, "Could not contact XORP Finder process");
    }
    return true;
}

void
XrlJobQueue::process_next_job()
{
    if (_jobs.front()->dispatch() == false) {
	set_status(SERVICE_FAILED, "Could not dispatch xrl");
    }
}
