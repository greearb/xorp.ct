// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/fea/xrl_socket_cmds.hh,v 1.4 2004/03/10 23:33:00 hodson Exp $

#ifndef __FEA_XRL_SOCKET_CMDS_HH__
#define __FEA_XRL_SOCKET_CMDS_HH__

#include <vector>
#include <list>

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

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

template <typename A>
class SocketUserSendRecvEvent : public XrlSocketCommandBase {
public:
    inline SocketUserSendRecvEvent(const string& tgt, const string& sockid)
	: XrlSocketCommandBase(tgt), _sockid(sockid), _src_port(0xffffffff)
    {}
    inline void set_source_host(const A& host)		{ _src_host = host; }
    inline void set_source_port(uint16_t port)		{ _src_port = port; }
    inline void set_source(const typename A::SockAddrType& sa,
			   socklen_t salen);

    inline vector<uint8_t>& data()			{ return _data; }
    inline const vector<uint8_t>& data() const		{ return _data; }

    bool execute(XrlSender& xs, const CommandCallback& cb);
    string str() const;

private:
    string		_sockid;
    A			_src_host;
    uint32_t		_src_port;
    vector<uint8_t>	_data;
};

template <>
inline void
SocketUserSendRecvEvent<IPv4>::set_source(const IPv4::SockAddrType& sin,
					  socklen_t sin_len)
{
    XLOG_ASSERT(sin_len == sizeof(sockaddr_in));

    _src_host = IPv4(sin);
    _src_port = ntohs(sin.sin_port);
}

template <>
inline void
SocketUserSendRecvEvent<IPv6>::set_source(const IPv6::SockAddrType& sin6,
					  socklen_t sin6_len)
{
    XLOG_ASSERT(sin6_len == sizeof(sockaddr_in6));

    _src_host = IPv6(sin6);
    _src_port = ntohs(sin6.sin6_port);
}

template <typename A>
class SocketUserSendConnectEvent : public XrlSocketCommandBase {
public:
    inline SocketUserSendConnectEvent(XrlSocketServer* ss,
				      const string&	tgt,
				      const string&	sockid,
				      const A&		host,
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
    A			_src_host;
    uint32_t		_src_port;
    string		_new_sockid;
};

template <typename A>
class SocketUserSendErrorEvent : public XrlSocketCommandBase {
public:
    inline SocketUserSendErrorEvent(const string&	tgt,
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

template <typename A>
class SocketUserSendCloseEvent : public XrlSocketCommandBase {
public:
    inline SocketUserSendCloseEvent(const string&	tgt,
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
