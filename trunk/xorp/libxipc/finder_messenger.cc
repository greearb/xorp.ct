// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/devnotes/template.cc,v 1.1.1.1 2002/12/11 23:55:54 hodson Exp $"

#include "finder_module.h"
#include "finder_messenger.hh"

#include "libxorp/xlog.h"

bool
FinderMessengerBase::dispatch_xrl_response(uint32_t	   seqno,
					   const XrlError& xe,
					   XrlArgs*	   args)
{
    SeqNoResponseMap::iterator i = _expected_responses.find(seqno);
    if (_expected_responses.end() == i)
	return false;

    i->second.scb->dispatch(xe, args);
    _expected_responses.erase(i);

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
FinderMessengerBase::pre_dispatch_xrl()
{
}

void
FinderMessengerBase::post_dispatch_xrl()
{
}

void
FinderMessengerBase::dispatch_xrl(uint32_t seqno, const Xrl& xrl)
{
    const XrlCmdEntry* ce = command_map().get_handler(xrl.command());
	
    if (0 == ce) {
	reply(seqno, XrlError::NO_SUCH_METHOD(), 0);
	return;
    }

    pre_dispatch_xrl();
    
    XrlArgs reply_args;
    XrlError e = ce->callback->dispatch(xrl, &reply_args);
    if (XrlCmdError::OKAY() == e) {
	reply(seqno, e, &reply_args);
    } else {
	reply(seqno, e, 0);
    }

    post_dispatch_xrl();
}
