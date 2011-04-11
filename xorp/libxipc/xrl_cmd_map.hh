// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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


#ifndef __LIBXIPC_XRL_CMD_MAP_HH__
#define __LIBXIPC_XRL_CMD_MAP_HH__






#include "libxorp/callback.hh"
#include "xrl.hh"
#include "xrl_error.hh"





// This the the original, synchronous, command callback.
// Out-arguments and error code must be set by the time the function
// returns.
typedef
XorpCallback2<const XrlCmdError, const XrlArgs&, XrlArgs*>::RefPtr
XrlRecvSyncCallback;

// This is the type of callback that a command implementation must
// call if it wants to operate asynchronously.
typedef
XorpCallback2<void, const XrlCmdError &, const XrlArgs *>::RefPtr
XrlRespCallback;

// This is the new, asynchronous, command callback.  No errors are
// returned, nor out-arguments passed.  Instead, the XrlRespCallback
// is invoked with the results.
typedef
XorpCallback2<void, const XrlArgs&, XrlRespCallback>::RefPtr
XrlRecvAsyncCallback;



// We define some types depending on whether asynchronous
// implementations are allowed.
#ifdef XORP_ENABLE_ASYNC_SERVER

// Asynchronous implementations do not return anything.
typedef void XrlCmdRT;

// To report an error, invoke the callback, and stop.
#define XRL_CMD_RETURN_ERROR(OUT, ERR)	\
    do {				\
	(OUT)->dispatch((ERR), NULL);	\
	return;				\
    } while (0)

// Responses are expressed through a callback.
typedef XrlRespCallback XrlCmdOT;

// The normal command implementation type is the asynchronous version.
typedef XrlRecvAsyncCallback XrlRecvCallback;


#else


// Synchronous implementations return the error code.
typedef const XrlCmdError XrlCmdRT;

// To report an error, return it.
#define XRL_CMD_RETURN_ERROR(OUT, ERR)	\
    do {				\
	return (ERR);			\
    } while (0)

// Placeholders for out-arguments are supplied to the command
// implementation.
typedef XrlArgs* XrlCmdOT;

// The normal command implementation type is the synchronous version.
typedef XrlRecvSyncCallback XrlRecvCallback;

#endif




class XrlCmdEntry {
    // An asynchronous command implementation that calls a synchronous
    // one
    static void invoke_sync(const XrlArgs& in, XrlRespCallback out,
			    XrlRecvSyncCallback impl);

public:
    // Make an asynchronous command implementation out of a
    // synchronous one.
    static XrlRecvAsyncCallback make_async_cb(const XrlRecvSyncCallback& cb) {
	return callback(&XrlCmdEntry::invoke_sync, cb);
    }

    XrlCmdEntry(const string& s, XrlRecvAsyncCallback cb) :
	    _name(s), _cb(cb) {}
    XrlCmdEntry(const string& s, XrlRecvSyncCallback cb) :
	    _name(s), _cb(make_async_cb(cb)) {}

#ifdef XORP_USE_USTL
    XrlCmdEntry() { }
#endif

    const string& name() const { return _name; }

    void dispatch(const XrlArgs& inputs, XrlRespCallback outputs) const {
	return _cb->dispatch(inputs, outputs);
    }

protected:
    string			_name;
    XrlRecvAsyncCallback	_cb;
};

class XrlCmdMap :
    public NONCOPYABLE
{
public:
    typedef map<string, XrlCmdEntry> CmdMap;

    XrlCmdMap(const string& name = "anonymous") : _name(name) {}
    virtual ~XrlCmdMap() {}

    const string& name() const { return _name; }

    virtual bool add_handler_internal(const string& cmd,
				      const XrlRecvAsyncCallback& rcb);

    bool add_handler(const string& cmd, const XrlRecvAsyncCallback& rcb) {
	return add_handler_internal(cmd, rcb);
    }

    bool add_handler(const string& cmd, const XrlRecvSyncCallback& rcb) {
	return add_handler_internal(cmd, XrlCmdEntry::make_async_cb(rcb));
    }

    virtual bool remove_handler (const string& name);

    const XrlCmdEntry* get_handler(const string& name) const;

    uint32_t count_handlers() const;

    const XrlCmdEntry* get_handler(uint32_t index) const;

    void get_command_names(list<string>& names) const;

    /**
     * Mark command map as finished.
     */
    virtual void finalize();

protected:
    bool add_handler (const XrlCmdEntry& c);

protected:
    const string _name;

    CmdMap _cmd_map;
};

#endif // __LIBXIPC_XRL_CMD_MAP_HH__
