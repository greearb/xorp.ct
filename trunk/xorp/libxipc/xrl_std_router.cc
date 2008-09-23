// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/libxipc/xrl_std_router.cc,v 1.19 2008/09/23 19:58:27 abittau Exp $"

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
