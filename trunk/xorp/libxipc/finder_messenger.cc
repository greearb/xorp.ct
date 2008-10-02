// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/libxipc/finder_messenger.cc,v 1.15 2008/07/23 05:10:41 pavlin Exp $"

#include "finder_module.h"
#include "finder_messenger.hh"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

FinderMessengerBase::FinderMessengerBase(EventLoop&		 e,
					 FinderMessengerManager* fmm,
					 XrlCmdMap& 		 cmds)
    : _eventloop(e), _manager(fmm), _cmds(cmds)
{
    //    _manager.messenger_birth_event(this);
    debug_msg("Constructor for %p\n", this);
}

FinderMessengerBase::~FinderMessengerBase()
{
    //   _manager.messenger_death_event(this);
    debug_msg("Destructor for %p\n", this);
}

bool
FinderMessengerBase::dispatch_xrl_response(uint32_t	   seqno,
					   const XrlError& xe,
					   XrlArgs*	   args)
{
    SeqNoResponseMap::iterator i = _expected_responses.find(seqno);
    if (_expected_responses.end() == i) {
	debug_msg("Attempting to make response for invalid seqno\n");
	return false;
    }

    SendCallback scb = i->second.scb;
    _expected_responses.erase(i);
    scb->dispatch(xe, args);

    return true;
}

bool
FinderMessengerBase::store_xrl_response(uint32_t seqno,
					const SendCallback& scb)
{
    SeqNoResponseMap::const_iterator ci = _expected_responses.find(seqno);
    if (_expected_responses.end() != ci)
	return false;	// A callback appears to be registered with seqno

    _expected_responses.insert(
	SeqNoResponseMap::value_type(seqno, ResponseState(seqno, scb, this))
	);
    
    return true;
}

void
FinderMessengerBase::response_timeout(uint32_t seqno)
{
    // Assert that response existed to be dispatched: it shouldn't be able
    // to timeout otherwise.
    XLOG_ASSERT(dispatch_xrl_response(seqno, XrlError::REPLY_TIMED_OUT(), 0));
}

void
FinderMessengerBase::dispatch_xrl(uint32_t seqno, const Xrl& xrl)
{
    const XrlCmdEntry* ce = command_map().get_handler(xrl.command());
	
    if (0 == ce) {
	reply(seqno, XrlError::NO_SUCH_METHOD(), 0);
	return;
    }

    // Announce we're about to dispatch an xrl
    if (manager())
	manager()->messenger_active_event(this);
    
    XrlArgs reply_args;
    XrlError e = ce->dispatch(xrl.args(), &reply_args);
    if (XrlCmdError::OKAY() == e) {
	reply(seqno, e, &reply_args);
    } else {
	reply(seqno, e, 0);
    }

    // Announce we've dispatched xrl
    if (manager())
	manager()->messenger_inactive_event(this);
}

void
FinderMessengerBase::unhook_manager()
{
    _manager = 0;
}
