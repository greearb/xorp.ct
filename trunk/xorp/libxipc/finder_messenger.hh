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

// $XORP: xorp/libxipc/finder_messenger.hh,v 1.7 2003/04/22 23:27:18 hodson Exp $

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

    inline XrlCmdMap& command_map();
    inline EventLoop& eventloop();

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
