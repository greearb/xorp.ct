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

// $XORP: xorp/libxipc/finder_ng_xrl_queue.hh,v 1.1 2003/02/24 19:39:19 hodson Exp $

#ifndef __LIBXIPC_FINDER_NG_XRL_QUEUE_HH__
#define __LIBXIPC_FINDER_NG_XRL_QUEUE_HH__

#include <list>
#include "libxorp/ref_ptr.hh"

class FinderNGXrlCommandBase;

class FinderNGXrlCommandQueue {
public:
    typedef ref_ptr<FinderNGXrlCommandBase> Command;

public:
    FinderNGXrlCommandQueue(FinderMessengerBase& messenger) :
	_m(messenger), _pending(false) {}

    inline FinderMessengerBase& messenger() { return _m; }

    void enqueue(const Command& cmd);

protected:
    void push();

protected:
    friend class FinderNGXrlCommandBase;
    void crank();

private:
    FinderMessengerBase& _m;
    list<Command>	 _cmds;
    bool		 _pending;
};

class FinderNGXrlCommandBase {
public:
    FinderNGXrlCommandBase(FinderNGXrlCommandQueue& q) : _queue(q) {}
    virtual ~FinderNGXrlCommandBase() {}

    inline FinderNGXrlCommandQueue& queue() { return _queue; }
    inline FinderMessengerBase& messenger() { return _queue.messenger(); }

    virtual bool dispatch() = 0;

    void dispatch_cb(const XrlError& e) {
	if (e != XrlCmdError::OKAY())
	    XLOG_ERROR("Sent xrl got response %s\n", e.str().c_str());
	queue().crank();
    }

protected:
    FinderNGXrlCommandQueue& _queue;
};

#include "finder_client_xif.hh"

class FinderNGSendHelloToClient : public FinderNGXrlCommandBase {
public:
    FinderNGSendHelloToClient(FinderNGXrlCommandQueue& q,
			      const string&	       tgtname)
	: FinderNGXrlCommandBase(q), _tgtname(tgtname)
    {
    }

    bool dispatch()
    {
	XrlFinderClientV0p1Client client(&(queue().messenger()));
	return client.send_hello(_tgtname.c_str(),
		  callback(static_cast<FinderNGXrlCommandBase*>(this),
			   &FinderNGXrlCommandBase::dispatch_cb));
    }
    
protected:
    string _tgtname;
};

class FinderNGSendRemoveXrl : public FinderNGXrlCommandBase {
public:
    FinderNGSendRemoveXrl(FinderNGXrlCommandQueue& q, const string& tgtname, const string& xrl)
	: FinderNGXrlCommandBase(q), _tgtname(tgtname), _xrl(xrl)
    {
    }

    ~FinderNGSendRemoveXrl()
    {
	_tgtname = _xrl = "croak";
    }
    
    bool dispatch()
    {
	XrlFinderClientV0p1Client client(&(queue().messenger()));
	return client.send_remove_xrl_from_cache(_tgtname.c_str(), _xrl,
		  callback(static_cast<FinderNGXrlCommandBase*>(this),
			   &FinderNGXrlCommandBase::dispatch_cb));
    }
    
protected:
    string _tgtname;
    string _xrl;
};

class FinderNGSendRemoveXrls : public FinderNGXrlCommandBase {
public:
    FinderNGSendRemoveXrls(FinderNGXrlCommandQueue& q, const string& tgtname)
	: FinderNGXrlCommandBase(q), _tgtname(tgtname)
    {
    }

    ~FinderNGSendRemoveXrls()
    {
	_tgtname = "croak";
    }

    bool dispatch()
    {
	XrlFinderClientV0p1Client client(&(queue().messenger()));
	return client.send_remove_xrls_for_target_from_cache(_tgtname.c_str(),
							     _tgtname,
		  callback(static_cast<FinderNGXrlCommandBase*>(this),
			   &FinderNGXrlCommandBase::dispatch_cb));
    }
    
protected:
    string _tgtname;
};

#endif // __LIBXIPC_FINDER_NG_XRL_QUEUE_HH__
