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

// $XORP: xorp/libxipc/finder_xrl_queue.hh,v 1.3 2003/04/23 20:50:48 hodson Exp $

#ifndef __LIBXIPC_FINDER_NG_XRL_QUEUE_HH__
#define __LIBXIPC_FINDER_NG_XRL_QUEUE_HH__

#include <list>
#include "libxorp/ref_ptr.hh"

class FinderXrlCommandBase;

/**
 * @short Xrl Queue for Finder.
 *
 * The FinderXrlCommandQueue holds and dispatches Xrls for the Finder.
 * Commands are added with the @ref FinderXrlCommandQueue::enqueue
 * method and are serially dispatched without any additional
 * intervention.  During the completion of each Xrl in the queue triggers
 * the sending of the next Xrl.
 */
class FinderXrlCommandQueue {
public:
    typedef ref_ptr<FinderXrlCommandBase> Command;

public:
    FinderXrlCommandQueue(FinderMessengerBase& messenger) :
	_m(messenger), _pending(false) {}

    inline FinderMessengerBase& messenger() { return _m; }

    void enqueue(const Command& cmd);

protected:
    void push();

protected:
    friend class FinderXrlCommandBase;
    void crank();

private:
    FinderMessengerBase& _m;
    list<Command>	 _cmds;
    bool		 _pending;
};

/**
 * @short Base class for Xrls sent from Finder.
 */
class FinderXrlCommandBase {
public:
    FinderXrlCommandBase(FinderXrlCommandQueue& q) : _queue(q) {}
    virtual ~FinderXrlCommandBase() {}

    inline FinderXrlCommandQueue& queue()	{ return _queue; }
    inline FinderMessengerBase& messenger()	{ return _queue.messenger(); }

    virtual bool dispatch() = 0;

    void dispatch_cb(const XrlError& e)
    {
	if (e != XrlCmdError::OKAY())
	    XLOG_ERROR("Sent xrl got response %s\n", e.str().c_str());
	queue().crank();
    }

protected:
    FinderXrlCommandQueue& _queue;
};

#include "finder_client_xif.hh"

/**
 * @short Send "hello" Xrl to Client.
 */
class FinderSendHelloToClient : public FinderXrlCommandBase {
public:
    FinderSendHelloToClient(FinderXrlCommandQueue& q,
			    const string&	   tgtname)
	: FinderXrlCommandBase(q), _tgtname(tgtname)
    {
    }

    bool dispatch()
    {
	XrlFinderClientV0p1Client client(&(queue().messenger()));
	return client.send_hello(_tgtname.c_str(),
		  callback(static_cast<FinderXrlCommandBase*>(this),
			   &FinderXrlCommandBase::dispatch_cb));
    }
    
protected:
    string _tgtname;
};

/**
 * @short Send "remove xrl" to client.
 */
class FinderSendRemoveXrl : public FinderXrlCommandBase {
public:
    FinderSendRemoveXrl(FinderXrlCommandQueue& q,
			const string&	       tgtname,
			const string&	       xrl)
	: FinderXrlCommandBase(q), _tgtname(tgtname), _xrl(xrl)
    {
    }

    ~FinderSendRemoveXrl()
    {
	_tgtname = _xrl = "croak";
    }
    
    bool dispatch()
    {
	XrlFinderClientV0p1Client client(&(queue().messenger()));
	return client.send_remove_xrl_from_cache(_tgtname.c_str(), _xrl,
		  callback(static_cast<FinderXrlCommandBase*>(this),
			   &FinderXrlCommandBase::dispatch_cb));
    }
    
protected:
    string _tgtname;
    string _xrl;
};

/**
 * @short Send "remove xrls for target" to client.
 */
class FinderSendRemoveXrls : public FinderXrlCommandBase {
public:
    FinderSendRemoveXrls(FinderXrlCommandQueue& q,
			 const string&		tgtname)
	: FinderXrlCommandBase(q), _tgtname(tgtname)
    {
    }

    ~FinderSendRemoveXrls()
    {
	_tgtname = "croak";
    }

    bool dispatch()
    {
	XrlFinderClientV0p1Client client(&(queue().messenger()));
	return client.send_remove_xrls_for_target_from_cache(_tgtname.c_str(),
							     _tgtname,
		  callback(static_cast<FinderXrlCommandBase*>(this),
			   &FinderXrlCommandBase::dispatch_cb));
    }
    
protected:
    string _tgtname;
};

#endif // __LIBXIPC_FINDER_NG_XRL_QUEUE_HH__
