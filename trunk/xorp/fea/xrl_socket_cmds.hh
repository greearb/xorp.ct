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

// $XORP: xorp/devnotes/template.hh,v 1.2 2003/01/16 19:08:48 mjh Exp $

#ifndef __FEA_XRL_SOCKET_CMDS_HH__
#define __FEA_XRL_SOCKET_CMDS_HH__

#include <vector>
#include <list>

#include "libxorp/ipv4.hh"

class XrlSocketServer;
class XrlSender;

class XrlSocketCommandBase {
public:
    typedef XorpCallback1<void, const XrlError&>::RefPtr CommandCallback;

public:
    XrlSocketCommandBase(const string& tgt);
    virtual ~XrlSocketCommandBase() = 0;

    virtual bool execute(XrlSender& xs, const CommandCallback&) = 0;
    virtual string str() const = 0;

    inline const char* target() const			{ return _t.c_str(); }

protected:
    string _t;
};

class Socket4UserSendRecvEvent : public XrlSocketCommandBase {
public:
    inline Socket4UserSendRecvEvent(const string& tgt, const string& sockid)
	: XrlSocketCommandBase(tgt), _sockid(sockid), _src_port(0xffffffff)
    {}
    inline void set_source_host(IPv4 host)		{ _src_host = host; }
    inline void set_source_port(uint16_t port)		{ _src_port = port; }
    inline vector<uint8_t>& data()			{ return _data; }
    inline const vector<uint8_t>& data() const		{ return _data; }

    bool execute(XrlSender& xs, const CommandCallback& cb);
    string str() const;

private:
    string		_sockid;
    IPv4		_src_host;
    uint32_t		_src_port;
    vector<uint8_t>	_data;
};

class Socket4UserSendConnectEvent : public XrlSocketCommandBase {
public:
    inline Socket4UserSendConnectEvent(XrlSocketServer* ss,
				       const string&	tgt,
				       const string&	sockid,
				       IPv4		host,
				       uint16_t		port,
				       const string&	new_sockid)
	: XrlSocketCommandBase(tgt), _ss(ss), _sockid(sockid),
	  _src_host(host), _src_port(port), _new_sockid(new_sockid)
    {}
    bool execute(XrlSender& xs, const CommandCallback& cb);
    void accept_or_reject_cb(const XrlError& e, const bool* accept);
    string str() const;

private:
    XrlSocketServer*	_ss;
    string		_sockid;
    IPv4		_src_host;
    uint32_t		_src_port;
    string		_new_sockid;
};

class Socket4UserSendErrorEvent : public XrlSocketCommandBase {
public:
    inline Socket4UserSendErrorEvent(const string&	tgt,
				     const string&	sockid,
				     const string&	error,
				     bool		fatal)
	: XrlSocketCommandBase(tgt), _sockid(sockid),
	  _error(error), _fatal(fatal)
    {}
    bool execute(XrlSender& xs, const CommandCallback& cb);
    string str() const;

private:
    string	_sockid;
    string	_error;
    bool	_fatal;
};

class Socket4UserSendCloseEvent : public XrlSocketCommandBase {
public:
    inline Socket4UserSendCloseEvent(const string&	tgt,
				     const string&	sockid,
				     const string&	reason)
	: XrlSocketCommandBase(tgt), _sockid(sockid), _reason(reason)
    {}
    bool execute(XrlSender& xs, const CommandCallback& cb);
    string str() const;

private:
    string	_sockid;
    string	_reason;
};

class XrlSocketCommandDispatcher {
public:
    typedef ref_ptr<XrlSocketCommandBase> Command;

public:
    XrlSocketCommandDispatcher(XrlSender& xs);
    virtual ~XrlSocketCommandDispatcher() = 0;

    /**
     * Enqueue an Xrl command for dispatch.  If queue is empty,
     * command is sent immediately for dispatch.  When dispatch
     * completes xrl_cb() is called with the response.
     */
    void enqueue(const Command& cmd);

    inline bool queue_empty() const		{ return _cmds.empty(); }
protected:
    /**
     * Send next Xrl enqueued (if any).
     *
     * @return true if next xrl sent, false if queue is empty or send failed.
     */
    bool send_next();

    /**
     * Method invoked when an Xrl dispatch is completed.
     * Implementations should call send_next() to trigger next queued
     * XrlSocketCommandBase object to be dispatched.
     *
     * @param xe Xrl error object.
     */
    virtual void xrl_cb(const XrlError& xe) = 0;

private:
    inline bool dispatch_head();

private:
    XrlSender&	  _xs;
    list<Command> _cmds;
};

#endif // __FEA_XRL_SOCKET_CMDS_HH__
