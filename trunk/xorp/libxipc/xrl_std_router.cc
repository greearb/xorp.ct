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

#ident "$XORP: xorp/libxipc/xrl_std_router.cc,v 1.3 2003/09/10 21:42:05 hodson Exp $"

#include "xrl_std_router.hh"
#include "xrl_pf_stcp.hh"

// ----------------------------------------------------------------------------
// Helper methods

static XrlPFListener*
create_listener(EventLoop& e, XrlDispatcher* d)
{
    // Change type here for your protocol family of choice
    return new XrlPFSTCPListener(e, d);
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
			   const char*	class_name)
    : XrlRouter(eventloop, class_name)
{
    _l = create_listener(eventloop, this);
    add_listener(_l);
}

XrlStdRouter::XrlStdRouter(EventLoop&	eventloop,
			   const char*	class_name,
			   IPv4		finder_address)
    : XrlRouter(eventloop, class_name, finder_address)
{
    _l = create_listener(eventloop, this);
    add_listener(_l);
}

XrlStdRouter::XrlStdRouter(EventLoop&	eventloop,
			   const char*	class_name,
			   IPv4		finder_address,
			   uint16_t	finder_port)
    : XrlRouter(eventloop, class_name, finder_address, finder_port)
{
    _l = create_listener(eventloop, this);
    add_listener(_l);
}

XrlStdRouter::XrlStdRouter(EventLoop&	eventloop,
			   const char*	class_name,
			   const char*	finder_address)
    : XrlRouter(eventloop, class_name, finder_address)
{
    _l = create_listener(eventloop, this);
    add_listener(_l);
}

XrlStdRouter::XrlStdRouter(EventLoop&	eventloop,
			   const char*	class_name,
			   const char*	finder_address,
			   uint16_t	finder_port)
    : XrlRouter(eventloop, class_name, finder_address, finder_port)
{
    _l = create_listener(eventloop, this);
    add_listener(_l);
}

XrlStdRouter::~XrlStdRouter()
{
    // remove_listener(&_l);
    destroy_listener(_l);
}

