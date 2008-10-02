// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/libxipc/xrl_std_router.hh,v 1.25 2008/09/23 20:50:59 abittau Exp $

#ifndef __LIBXIPC_XRL_STD_ROUTER_HH__
#define __LIBXIPC_XRL_STD_ROUTER_HH__

#include "xrl_router.hh"
#include "xrl_pf.hh"

#define UNIX_SOCKET_DEFAULT false

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
