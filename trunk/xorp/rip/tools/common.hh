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

// $XORP: xorp/devnotes/template.hh,v 1.2 2003/01/16 19:08:48 mjh Exp $

#ifndef __RIP_TOOLS_COMMON_HH__
#define __RIP_TOOLS_COMMON_HH__

#include <string>

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

    bool startup();
    bool shutdown();
    void dispatch_complete(const XrlError& xe, const XrlJobBase* cmd);

    inline XrlSender* sender()			{ return _rtr; }
    inline const string& target() const		{ return _tgt; }
    inline void enqueue(const Job& cmd)		{ _jobs.push_back(cmd); }

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
 */
class XrlJobBase : public CallbackSafeObject {
public:
    XrlJobBase(XrlJobQueue& q) : _q(q) {}

    virtual ~XrlJobBase() {}
    virtual bool dispatch() = 0;

protected:
    XrlJobQueue& queue() { return _q; }

private:
    XrlJobBase(const XrlJobBase&);		// Not implemented
    XrlJobBase& operator=(const XrlJobBase&);	// Not implemented

private:
    XrlJobQueue& _q;
};

#endif // __RIP_TOOLS_COMMON_HH__
