// -*- c-basic-offset: 4 -*-

// Copyright (c) 2001-2003 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the Xorp LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the Xorp LICENSE file; the license in that file is
// legally binding.

// $Id$

#ifndef __XRLSTDROUTER_HH__
#define __XRLSTDROUTER_HH__

#include "xrl_router.hh"
#include "xrl_pf_sudp.hh"

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
    XrlStdRouter(EventLoop&	event_loop,
		 const char*	entity_name)
	: XrlRouter(event_loop, entity_name), _sudp(event_loop, this)
    {
	add_listener(&_sudp);
    }

    XrlStdRouter(EventLoop&	event_loop,
		 const char*	entity_name,
		 const char*	finder_address)
	: XrlRouter(event_loop, entity_name, finder_address),
	  _sudp(event_loop, this)
    {
	add_listener(&_sudp);
    }

    XrlStdRouter(EventLoop&	event_loop,
		 const char*	entity_name,
		 const char*	finder_address,
		 uint16_t	finder_port)
	: XrlRouter(event_loop, entity_name, finder_address, finder_port),
	  _sudp(event_loop, this)
    {
	add_listener(&_sudp);
    }

    ~XrlStdRouter()
    {
	// remove_listener(&_sudp);
    }

private:
    XrlPFSUDPListener _sudp;
};

#endif // __XRLROUTER_HH__
