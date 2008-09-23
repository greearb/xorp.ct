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

// $XORP: xorp/libxipc/xrl_std_router.hh,v 1.23 2008/09/23 19:58:33 abittau Exp $

#ifndef __LIBXIPC_XRL_STD_ROUTER_HH__
#define __LIBXIPC_XRL_STD_ROUTER_HH__

#include "xrl_router.hh"
#include "xrl_pf.hh"

#define UNIX_SOCKET_DEFAULT true

/**
 * @short Standard XRL transmission and reception point.
 *
 * Derived from XrlRouter, this class has the default protocol family
 * listener types associated with it at construction time.  Allows
 * for simple use of XrlRouter for common cases.
 * for entities in a XORP Router.  A single process may have multiple
 */
class XrlStdRouter : public XrlRouter {
public:
    XrlStdRouter(EventLoop& eventloop, const char* class_name,
		 bool unix_socket = UNIX_SOCKET_DEFAULT);

    XrlStdRouter(EventLoop&	eventloop,
		 const char*	class_name,
		 IPv4		finder_address,
		 bool		unix_socket = UNIX_SOCKET_DEFAULT);

    XrlStdRouter(EventLoop&	eventloop,
		 const char*	class_name,
		 IPv4		finder_address,
		 uint16_t	finder_port,
		 bool		unix_socket = UNIX_SOCKET_DEFAULT);

    XrlStdRouter(EventLoop&	eventloop,
		 const char*	class_name,
		 const char*	finder_address,
		 bool		unix_socket = UNIX_SOCKET_DEFAULT);

    XrlStdRouter(EventLoop&	eventloop,
		 const char*	class_name,
		 const char*	finder_address,
		 uint16_t	finder_port,
		 bool		unix_socket = UNIX_SOCKET_DEFAULT);

    ~XrlStdRouter();

private:
    void	   construct(bool unix_socket);
    void	   create_unix_listener();
    XrlPFListener* create_listener();

    XrlPFListener* _unix;
    XrlPFListener* _l;
};

#endif // __LIBXIPC_XRL_STD_ROUTER_HH__
