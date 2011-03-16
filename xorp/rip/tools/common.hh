// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/rip/tools/common.hh,v 1.10 2008/10/02 21:58:21 bms Exp $

#ifndef __RIP_TOOLS_COMMON_HH__
#define __RIP_TOOLS_COMMON_HH__



#include "libxorp/xorp.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/service.hh"

class EventLoop;
class XrlJobBase;

// ----------------------------------------------------------------------------
// Utility funtions

/**
 * Utility function to return the default RIP XRL target according to
 * the IP version number.
 *
 * @param ip_version IP version number.
 * @return pointer to character string containing XRL target name on
 * success, NULL if an invalid IP version number.
*/
const char* default_xrl_target(uint32_t ip_version);

/**
 * Utility function to return IP version from named rip description string.
 *
 * @param rip_name string that is expected to be "rip" or "ripng".
 *
 * @return on success either of 4 or 6, on failure 0.
 */
uint32_t rip_name_to_ip_version(const char* rip_name);

/**
 * Utility function to break a colon separated host name and port into
 * separate variables.
 *
 * @param host_colon_port string containing colon separated host and port.
 * @param host string assigned with host name.
 * @param port unsigned integer assigned with port number.
 * @return true on success, false if host_colon_port is invalid.
 */
bool parse_finder_args(const string&	host_colon_port,
		       string&		host,
		       uint16_t&	port);


// ----------------------------------------------------------------------------
// Class declarations

/**
 * Class for buffering and dispatching XRLs.
 */
class XrlJobQueue : public ServiceBase
{
public:
    typedef ref_ptr<XrlJobBase> Job;

public:
    XrlJobQueue(EventLoop& 	e,
		const string& 	finder_host,
		uint16_t 	finder_port,
		const string& 	tgtname);
    ~XrlJobQueue();

    int startup();
    int shutdown();
    void dispatch_complete(const XrlError& xe, const XrlJobBase* cmd);

    XrlSender* sender()			{ return _rtr; }
    const string& target() const	{ return _tgt; }
    void enqueue(const Job& cmd)	{ _jobs.push_back(cmd); }

protected:
    bool xrl_router_ready_poll();

    void process_next_job();

protected:
    EventLoop&		_e;
    string 		_fhost;	// Finder host
    uint16_t 		_fport;	// Finder port
    string		_tgt; 	// Xrl target to for jobs

    list<Job> 		_jobs;
    XrlStdRouter* 	_rtr;
    XorpTimer		_rtr_poll;	// Timer used to poll XrlRouter::ready
    uint32_t		_rtr_poll_cnt;	// Number of timer XrlRouter polled.
};

/**
 * Base class for Xrl Jobs that are invoked by classes derived
 * from XrlJobQueue.
 * Non-copyable due to inheriting from CallbackSafeObject.
 */
class XrlJobBase :
    public CallbackSafeObject
{
public:
    XrlJobBase(XrlJobQueue& q) : _q(q) {}

    virtual ~XrlJobBase() {}
    virtual bool dispatch() = 0;

protected:
    XrlJobQueue& queue() { return _q; }

private:
    XrlJobQueue& _q;
};

#endif // __RIP_TOOLS_COMMON_HH__
