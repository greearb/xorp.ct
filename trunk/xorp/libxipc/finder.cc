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

#ident "$XORP: xorp/devnotes/template.cc,v 1.2 2003/01/16 19:08:48 mjh Exp $"

#include "finder_module.h"

#include "libxorp/xlog.h"
#include "finder_ng.hh"

///////////////////////////////////////////////////////////////////////////////
//
// FinderNGMessenger methods
//

void
FinderNGMessenger::pre_dispatch_xrl()
{
    FinderNGMessenger* old = _finder.set_active_messenger(this);
    XLOG_ASSERT(0 == old);
}

void
FinderNGMessenger::post_dispatch_xrl()
{
    FinderNGMessenger* old = _finder.set_active_messenger(0);
    XLOG_ASSERT(this == old);
}

void
FinderNGMessenger::close_event()
{
    _finder.messenger_death_event(this);
}

///////////////////////////////////////////////////////////////////////////////
//
// FinderNG
//

FinderNG::FinderNG(EventLoop& e, IPv4 interface, uint16_t port)
	throw (InvalidPort)
    : FinderTcpListenerBase(e, interface, port)
{   
}

FinderNG::~FinderNG()
{
    while (false == _messengers.empty()) {
	delete _messengers.front();
	_messengers.pop_front();
    }
}

FinderNGMessenger*
FinderNG::set_active_messenger(FinderNGMessenger* m)
{
    FinderNGMessenger* old = _active_messenger;
    _active_messenger = m;
    return old;
}

bool
FinderNG::connection_event(int fd)
{
    FinderNGMessenger* m = new FinderNGMessenger(event_loop(), *this,
						 fd, _cmds);
    _messengers.push_back(m);
    return true;
}

void
FinderNG::messenger_death_event(FinderNGMessenger* m)
{
    // The messenger is telling us it's about to die, ie we're losing
    // the connection to a particular process and it can be considered
    // dead.  We need to do something here
    XLOG_INFO("FinderNGMessenger dead (instance @ 0x%p)\n", m);
}
