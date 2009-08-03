// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/libxipc/finder_messenger.hh,v 1.22 2008/10/02 21:57:20 bms Exp $

#ifndef __LIBXIPC_FINDER_MESSENGER_HH__
#define __LIBXIPC_FINDER_MESSENGER_HH__

#include <map>

#include "libxorp/eventloop.hh"
#include "xrl_cmd_map.hh"
#include "xrl_sender.hh"

class FinderMessengerBase;

/**
 * Base class for classes managing descendents of FinderMessengerBase.
 */
class FinderMessengerManager {
public:
    /**
     * Empty virtual destructor.
     */
    virtual ~FinderMessengerManager() {}

    /**
     * Method called by messenger constructor.
     */
    virtual void messenger_birth_event(FinderMessengerBase*) = 0;

    /**
     * Method called by messenger destructor.
     */
    virtual void messenger_death_event(FinderMessengerBase*) = 0;

    /**
     * Method called before Xrl is dispatched.
     */
    virtual void messenger_active_event(FinderMessengerBase*) = 0;

    /**
     * Method called immediately after Xrl is dispatched.
     */
    virtual void messenger_inactive_event(FinderMessengerBase*) = 0;

    /**
     * Method called when Messenger is unable to continue.  For instance,
     * network connection lost.
     */
    virtual void messenger_stopped_event(FinderMessengerBase*) = 0;

    /**
     * Method called to tell if FinderMessengerManager instance manages
     * a particular messenger.
     */
    virtual bool manages(const FinderMessengerBase*) const = 0;
};

/**
 * @short Base class for FinderMessenger classes.
 *
 * FinderMessenger classes are expected to handle the transport and
 * dispatch of Xrl's and their responses.  This base class provides a
 * common code for actually doing the Xrl dispatch and handling the
 * state associated with their responses.
 */
class FinderMessengerBase : public XrlSender
{
public:
    typedef XrlSender::Callback SendCallback;

public:
    FinderMessengerBase(EventLoop& e,
			FinderMessengerManager* fmm,
			XrlCmdMap& cmds);
    
    virtual ~FinderMessengerBase();

    virtual bool send(const Xrl& xrl, const SendCallback& scb) = 0;
    virtual bool pending() const = 0;

    XrlCmdMap& command_map();
    EventLoop& eventloop();

    void unhook_manager();
    FinderMessengerManager* manager();
    
protected:
    /**
     * Find command associated with Xrl and dispatch it.  pre_dispatch_xrl()
     * and post_dispatch_xrl() are called either side of Xrl.  
     */
    void dispatch_xrl(uint32_t seqno, const Xrl& x);
    
    bool dispatch_xrl_response(uint32_t seqno,
			       const XrlError& e,
			       XrlArgs*);

    bool store_xrl_response(uint32_t seqno, const SendCallback& scb);
    
    virtual void reply(uint32_t seqno,
		       const XrlError& e,
		       const XrlArgs* reply_args) = 0;

    void response_timeout(uint32_t seqno);

private:
    struct ResponseState {
	ResponseState(uint32_t		   seqno,
		      const SendCallback&  cb,
		      FinderMessengerBase* fmb)
	    : scb(cb)
	{
	    expiry = fmb->eventloop().new_oneoff_after_ms(RESPONSE_TIMEOUT_MS,
			callback(fmb,  &FinderMessengerBase::response_timeout,
				 seqno));
	}

	SendCallback scb;
	XorpTimer    expiry;

	static const uint32_t RESPONSE_TIMEOUT_MS = 30000;
    };
    typedef map<uint32_t, ResponseState> SeqNoResponseMap;

    friend class ResponseState;

private:
    EventLoop&				_eventloop;
    FinderMessengerManager*		_manager;
    SeqNoResponseMap		 	_expected_responses;
    XrlCmdMap&			 	_cmds;
};

///////////////////////////////////////////////////////////////////////////////
//
// Inline methods

inline XrlCmdMap&
FinderMessengerBase::command_map()
{
    return _cmds;
}

inline EventLoop&
FinderMessengerBase::eventloop()
{
    return _eventloop;
}

inline FinderMessengerManager*
FinderMessengerBase::manager()
{
    return _manager;
}

#endif // __LIBXIPC_FINDER_MESSENGER_HH__
