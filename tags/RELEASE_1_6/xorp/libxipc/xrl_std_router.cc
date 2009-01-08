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

#ident "$XORP: xorp/libxipc/xrl_std_router.cc,v 1.22 2008/10/02 21:57:26 bms Exp $"

#include "xrl_module.h"
#include "xrl_std_router.hh"
#include "xrl_pf_inproc.hh"
#include "xrl_pf_stcp.hh"
#include "xrl_pf_sudp.hh"
#include "xrl_pf_unix.hh"
#include "libxorp/xlog.h"

// ----------------------------------------------------------------------------
// Helper methods

XrlPFListener*
XrlStdRouter::create_listener()
{
    const char* pf = getenv("XORP_PF");

    if (pf != NULL) {
	switch (pf[0]) {
	case 'i':
	    return new XrlPFInProcListener(_e, this);

	case 'u':
	    return new XrlPFSUDPListener(_e, this);

	case 'x':
	    XLOG_ASSERT(_unix == NULL);
	    return new XrlPFUNIXListener(_e, this);

	default:
	    XLOG_ERROR("Unknown PF %s\n", pf);
	    XLOG_ASSERT(false);
	    break;
	}
    }

    return new XrlPFSTCPListener(_e, this);
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
