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

#ident "$XORP: xorp/libxipc/finder_ng.cc,v 1.1 2003/01/24 02:48:21 hodson Exp $"

#include "finder_module.h"

#include "libxorp/xlog.h"

#include "finder_tcp.hh"
#include "finder_ng.hh"

///////////////////////////////////////////////////////////////////////////////
//
// FinderNG
//

FinderNG::FinderNG()
{}

FinderNG::~FinderNG()
{
    while (false == _messengers.empty()) {
	delete _messengers.front();
	_messengers.pop_front();
    }
}

void
FinderNG::messenger_active_event(FinderMessengerBase* m)
{
    XLOG_ASSERT(0 == _active_messenger);
    _active_messenger = m;
}

void
FinderNG::messenger_inactive_event(FinderMessengerBase* m)
{
    XLOG_ASSERT(m == _active_messenger);
    _active_messenger = 0;
}

void
FinderNG::messenger_stopped_event(FinderMessengerBase* m)
{
    XLOG_INFO("Messenger %p stopped.", m);
}

void
FinderNG::messenger_birth_event(FinderMessengerBase* m)
{
    _messengers.push_back(m);
}

void
FinderNG::messenger_death_event(FinderMessengerBase* m)
{
    // The messenger is telling us it's about to die, ie we're losing
    // the connection to a particular process and it can be considered
    // dead.  We need to do something here
    XLOG_INFO("FinderMessenger dead (instance @ 0x%p)\n", m);
}

XrlCmdMap&
FinderNG::commands()
{
    return _cmds;
}

///////////////////////////////////////////////////////////////////////////////
//
// FinderTcpMessenger methods
//

FinderTcpMessenger::FinderTcpMessenger(EventLoop& e,
				       FinderNG&  finder,
				       int	  fd,
				       XrlCmdMap& cmds)
    : FinderTcpMessengerBase(e, fd, cmds), _finder(&finder)
{
    _finder->messenger_birth_event(this);
}

FinderTcpMessenger::FinderTcpMessenger(EventLoop& e,
				       int	  fd,
				       XrlCmdMap& cmds)
    : FinderTcpMessengerBase(e, fd, cmds), _finder(0)
{
    if (_finder)
	_finder->messenger_birth_event(this);
}

FinderTcpMessenger::~FinderTcpMessenger()
{
    if (_finder)
	_finder->messenger_death_event(this);
}

void
FinderTcpMessenger::pre_dispatch_xrl()
{
    if (_finder)
	_finder->messenger_active_event(this);
}

void
FinderTcpMessenger::post_dispatch_xrl()
{
    if (_finder)
	_finder->messenger_inactive_event(this);
}

void
FinderTcpMessenger::close_event()
{
    if (_finder)
	_finder->messenger_stopped_event(this);
}

///////////////////////////////////////////////////////////////////////////////
//
// FinderNGTcpListener methods
//

FinderNGTcpListener::FinderNGTcpListener(FinderNG&  finder,
					 EventLoop& e,
					 IPv4	    interface,
					 uint16_t   port)
    throw (InvalidPort)
    : FinderTcpListenerBase(e, interface, port), _finder(finder)
{
}
					 
FinderNGTcpListener::~FinderNGTcpListener()
{
    while (false == _messengers.empty()) {
	delete _messengers.front();
	_messengers.pop_front();
    }
}

bool
FinderNGTcpListener::connection_event(int fd)
{
    FinderTcpMessenger* m = new FinderTcpMessenger(event_loop(),
						   _finder, fd,
						   _finder.commands());
    _messengers.push_back(m);
    return true;
}
