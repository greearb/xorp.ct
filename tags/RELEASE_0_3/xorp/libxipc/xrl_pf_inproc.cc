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

#ident "$XORP: xorp/libxipc/xrl_pf_inproc.cc,v 1.9 2003/05/09 19:36:16 hodson Exp $"

#include <sys/param.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

#include <string>
#include <map>

#include "xrl_module.h"
#include "config.h"

#include "libxorp/debug.h"
#include "libxorp/c_format.hh"

#include "xrl_error.hh"
#include "xrl_pf_inproc.hh"
#include "xrl_dispatcher.hh"

// ----------------------------------------------------------------------------
// InProc is "intra-process" - a minimalist and simple direct call transport
// mechanism for XRLs.
//
// Resolved names have protocol name "inproc" and specify addresses as
// "host/pid.iid" where pid is process id and iid is instance id (number).
// The latter being used to differentiate when multiple listeners exist
// within a single process.
//

// ----------------------------------------------------------------------------
// Constants

static char INPROC_PROTOCOL_NAME[] = "inproc";

const char* XrlPFInProcSender::_protocol = INPROC_PROTOCOL_NAME;
const char* XrlPFInProcListener::_protocol = INPROC_PROTOCOL_NAME;

// Glue for Sender and Listener rendez-vous
static XrlPFInProcListener* get_inproc_listener(uint32_t instance_no);

// ----------------------------------------------------------------------------
// Utility Functions

static const string
this_host()
{
    char buffer[MAXHOSTNAMELEN + 1];
    buffer[MAXHOSTNAMELEN] ='\0';
    gethostname(buffer, MAXHOSTNAMELEN);
    return string(buffer);
}

bool
split_inproc_address(const char* address,
		     string& host, uint32_t& pid, uint32_t& iid)
{
    const char *p;

    p = address;
    for (;;) {
	if (*p == '\0') {
	    // unexpected end of input
	    return false;
	} else if (*p == '/') {
	    break;
	}
	p++;
    }
    host = string(address, p - address);
    p++;

    pid = 0;
    while (isdigit(*p)) {
	pid *= 10;
	pid += *p - '0';
	p++;
    }

    if (*p != '.')
	return false;
    p++;

    iid = 0;
    while (isdigit(*p)) {
	iid *= 10;
	iid += *p - '0';
	p++;
    }
    return (*p == '\0');
}

// ----------------------------------------------------------------------------
// XrlPFInProcSender

XrlPFInProcSender::XrlPFInProcSender(EventLoop& e, const char* address)
    throw (XrlPFConstructorError)
    : XrlPFSender(e, address)
{
    string hname;
    uint32_t pid, iid;

    if (split_inproc_address(address, hname, pid, iid) == false) {
	xorp_throw(XrlPFConstructorError,
		   c_format("Invalid address: %s", address));
    } else if (hname != this_host()) {
	xorp_throw(XrlPFConstructorError,
		   c_format("Wrong host: %s != %s",
			    hname.c_str(), this_host().c_str()));
    } else if (pid != (uint32_t)getpid()) {
	xorp_throw(XrlPFConstructorError, "Bad process id");
    }
    _listener_no = iid;
}

XrlPFInProcSender::~XrlPFInProcSender()
{}

void
XrlPFInProcSender::send(const Xrl& x, const SendCallback& cb)
{
    XrlPFInProcListener *l = get_inproc_listener(_listener_no);

    if (l) {
	XrlArgs reply;
	const XrlError e = l->dispatch(x, reply);
	cb->dispatch(e, x, (e == XrlError::OKAY()) ? &reply : 0);
    } else {
	debug_msg("XrlPFInProcSender::send() no listener (id %d)\n",
		  _listener_no);
	cb->dispatch(XrlError::SEND_FAILED(), x, 0);
    }
}

// ----------------------------------------------------------------------------
// XrlPFInProcListener instance management

static map<uint32_t, XrlPFInProcListener*> listeners;

static XrlPFInProcListener*
get_inproc_listener(uint32_t instance_no)
{
    map<uint32_t, XrlPFInProcListener*>::iterator i;
    debug_msg("getting -> size %u\n", (uint32_t)listeners.size());
    i = listeners.find(instance_no);
    return (i == listeners.end()) ? 0 : i->second;
}

static void
add_inproc_listener(uint32_t instance_no, XrlPFInProcListener* l)
{
    assert(get_inproc_listener(instance_no) == 0);
    debug_msg("adding no %d size %u\n", instance_no,
	      (uint32_t)listeners.size());
    listeners[instance_no] = l;
}

static void
remove_inproc_listener(uint32_t instance_no)
{
    assert(get_inproc_listener(instance_no) != 0);
    listeners.erase(instance_no);
    debug_msg("Removing listener %d\n", instance_no);
}

// ----------------------------------------------------------------------------
// XrlPFInProcListener

uint32_t XrlPFInProcListener::_next_instance_no;

XrlPFInProcListener::XrlPFInProcListener(EventLoop& e, XrlDispatcher* xr)
    throw (XrlPFConstructorError)
    : XrlPFListener(e, xr)
{
    _instance_no = _next_instance_no ++;

    _address = this_host();

    char buffer[256];
    snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]),
	     "/%d.%d", getpid(), _instance_no);
    _address += buffer;

    add_inproc_listener(_instance_no, this);
}

XrlPFInProcListener::~XrlPFInProcListener()
{
    remove_inproc_listener(_instance_no);
}

const XrlError
XrlPFInProcListener::dispatch(const Xrl& request, XrlArgs& reply)
{
    const XrlDispatcher* xr = dispatcher();
    assert(xr != 0);
    return xr->dispatch_xrl(request, reply);
}
