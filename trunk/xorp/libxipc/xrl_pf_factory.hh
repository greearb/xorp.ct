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

// $XORP: xorp/libxipc/xrl_pf_factory.hh,v 1.7 2003/04/22 23:27:19 hodson Exp $

#ifndef __LIBXIPC_XRL_PF_FACTORY_HH__
#define __LIBXIPC_XRL_PF_FACTORY_HH__

#include "xrl_error.hh"
#include "xrl_pf.hh"

class XrlPFSenderFactory {
public:
    static void	 	startup();
    static void	 	shutdown();

    static XrlPFSender* create_sender(EventLoop&	eventloop,
				      const char*	proto_colon_addr);

    static XrlPFSender* create_sender(EventLoop&	e,
				      const char*	protocol,
				      const char*	address);

    static void		destroy_sender(XrlPFSender*	s);
};

#endif // __LIBXIPC_XRL_PF_FACTORY_HH__
