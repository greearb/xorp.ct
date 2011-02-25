// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "xrl_module.h"
#include "xrl_std_router.hh"
#include "xrl_pf_stcp.hh"
#include "xrl_pf_unix.hh"
#include "libxorp/xlog.h"


// ----------------------------------------------------------------------------
// Helper methods

#if !defined(XRL_PF)
#error "A default transport for XRL must be defined using the preprocessor."
#endif

static const char default_pf[] = { XRL_PF, '\0' };

string XrlStdRouter::toString() const {
    ostringstream oss;
    oss << XrlRouter::toString();
    oss << "\n_unix: ";

    if (_unix) {
	oss << _unix->toString() << endl;
    }
    else {
	oss << "NULL\n";
    }

    if (_l) {
	oss << "LISTENER: " << _l->toString() << endl;
    }
    else {
	oss << "LISTENER: NULL\n";
    }
    return oss.str();
}


XrlPFListener*
XrlStdRouter::create_listener()
{
    const char* pf = getenv("XORP_PF");
    if (pf == NULL)
	pf = default_pf;

    switch (pf[0]) {
	// For the benefit of bench_ipc.sh.
	case 't':
	    return new XrlPFSTCPListener(_e, this);
	    break;
	case 'x':
#ifndef	HOST_OS_WINDOWS
#if XRL_PF != 'x'
	    XLOG_ASSERT(_unix == NULL);
#endif
	    return new XrlPFUNIXListener(_e, this);
#else
	    XLOG_ERROR("PFUnix listener not available on windows builds.\n");
#endif
	default:
	    XLOG_ERROR("Unknown PF %s\n", pf);
	    XLOG_ASSERT(false);
	    break;
    }

    XLOG_UNREACHABLE();
    return 0;
}

static void
destroy_listener(XrlPFListener*& l)
{
    delete l;
    l = 0;
}

// ----------------------------------------------------------------------------
// XrlStdRouter implementation

XrlStdRouter::XrlStdRouter(EventLoop&	eventloop,
			   const char*	class_name,
			   bool		unix_socket)
    : XrlRouter(eventloop, class_name,  FinderConstants::FINDER_DEFAULT_HOST(),
		FinderConstants::FINDER_DEFAULT_PORT())
{
    construct(unix_socket);
}

XrlStdRouter::XrlStdRouter(EventLoop&	eventloop,
			   const char*	class_name,
			   IPv4		finder_address,
			   bool		unix_socket)
    : XrlRouter(eventloop, class_name, finder_address,
		FinderConstants::FINDER_DEFAULT_PORT())
{
    construct(unix_socket);
}

XrlStdRouter::XrlStdRouter(EventLoop&	eventloop,
			   const char*	class_name,
			   IPv4		finder_address,
			   uint16_t	finder_port,
			   bool		unix_socket)
    : XrlRouter(eventloop, class_name, finder_address, finder_port)
{
    construct(unix_socket);
}

XrlStdRouter::XrlStdRouter(EventLoop&	eventloop,
			   const char*	class_name,
			   const char*	finder_address,
			   bool		unix_socket)
    : XrlRouter(eventloop, class_name, finder_address,
		FinderConstants::FINDER_DEFAULT_PORT())
{
    construct(unix_socket);
}

XrlStdRouter::XrlStdRouter(EventLoop&	eventloop,
			   const char*	class_name,
			   const char*	finder_address,
			   uint16_t	finder_port,
			   bool		unix_socket)
    : XrlRouter(eventloop, class_name, finder_address, finder_port)
{
    construct(unix_socket);
}

void
XrlStdRouter::construct(bool unix_socket)
{
    _unix = _l = NULL;

    // We need to check the environment otherwise
    // we get the compiled-in default.
    const char* pf = getenv("XORP_PF");
    if (pf == NULL)
	pf = default_pf;
    if (pf[0] != 'x')
	unix_socket = false;

    if (unix_socket)
	create_unix_listener();

    _l = create_listener();
    add_listener(_l);
}

void
XrlStdRouter::create_unix_listener()
{
#ifndef HOST_OS_WINDOWS
    _unix = new XrlPFUNIXListener(_e, this);
    add_listener(_unix);
#endif // ! HOST_OS_WINDOWS
}

XrlStdRouter::~XrlStdRouter()
{
    if (_unix)
	destroy_listener(_unix);

    destroy_listener(_l);
}
