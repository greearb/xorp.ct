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

// $XORP: xorp/libxipc/finder_tcp_messenger.hh,v 1.9 2003/06/19 00:44:42 hodson Exp $

#ifndef __LIBXIPC_FINDER_TCP_MESSENGER_HH__
#define __LIBXIPC_FINDER_TCP_MESSENGER_HH__

#include <list>

#include "libxorp/ref_ptr.hh"

#include "finder_tcp.hh"
#include "finder_msgs.hh"
#include "finder_messenger.hh"

class FinderTcpMessenger
    : public FinderMessengerBase, protected FinderTcpBase
{
public:
    FinderTcpMessenger(EventLoop&		e,
		       FinderMessengerManager*	mm,
		       int			fd,
		       XrlCmdMap&		cmds);

    virtual ~FinderTcpMessenger();

    bool send(const Xrl& xrl, const SendCallback& scb);

    bool pending() const;

    inline void close() { FinderTcpBase::close(); }
    
protected:
    // FinderTcpBase methods
    void read_event(int		   errval,
		    const uint8_t* data,
		    uint32_t	   data_bytes);

    void write_event(int	    errval,
		     const uint8_t* data,
		     uint32_t	    data_bytes);

    void close_event();

    void error_event();
    
protected:
    void reply(uint32_t seqno, const XrlError& xe, const XrlArgs* reply_args);

protected:
    void push_queue();
    void drain_queue();
    
    /* High water-mark to disable reads, ie reading faster than writing */
    static const uint32_t OUTQUEUE_BLOCK_READ_HI_MARK = 6;

    /* Low water-mark to enable reads again */
    static const uint32_t OUTQUEUE_BLOCK_READ_LO_MARK = 4;    

    typedef list<const FinderMessageBase*> OutputQueue;
    OutputQueue _out_queue;
};

/**
 * Class that creates FinderMessengers for incoming connections.
 */
class FinderTcpListener : public FinderTcpListenerBase {
public:
    typedef FinderTcpListenerBase::AddrList Addr4List;
    typedef FinderTcpListenerBase::NetList Net4List;

    FinderTcpListener(EventLoop&		e,
			FinderMessengerManager& mm,
			XrlCmdMap&		cmds,
			IPv4			interface,
			uint16_t		port,
			bool			enabled = true)
	throw (InvalidPort);

    ~FinderTcpListener();

    /**
     * Instantiate a Messenger instance for fd.
     * @return true on success, false on failure.
     */
    bool connection_event(int fd);

protected:
    FinderMessengerManager& _mm;
    XrlCmdMap& _cmds;
};

class FinderTcpConnector {
public:
    FinderTcpConnector(EventLoop&		e,
		       FinderMessengerManager&	mm,
		       XrlCmdMap&		cmds,
		       IPv4			host,
		       uint16_t			port);

    /**
     * Connect to host specified in constructor.
     *
     * @param created_messenger pointer to be assigned messenger created upon
     * successful connect.
     * @return 0 on success, errno on failure.
     */
    int connect(FinderTcpMessenger*& created_messenger);

    IPv4 finder_address() const;
    uint16_t finder_port() const;
    
protected:
    EventLoop&		    _e;
    FinderMessengerManager& _mm;
    XrlCmdMap&		    _cmds;
    IPv4		    _host;
    uint16_t		    _port;
};

/**
 * Class to establish and manage a single connection to a FinderTcpListener.
 * Should the connection fail after being established a new connection is
 * started.
 */
class FinderTcpAutoConnector
    : public FinderTcpConnector, public FinderMessengerManager
{
public:
    FinderTcpAutoConnector(EventLoop&		     e,
			     FinderMessengerManager& mm,
			     XrlCmdMap&		     cmds,
			     IPv4		     host,
			     uint16_t		     port,
			     bool		     enabled = true);
    virtual ~FinderTcpAutoConnector();

    void set_enabled(bool en);
    bool enabled() const;
    bool connected() const;

protected:
    void do_auto_connect();
    void start_timer(uint32_t ms = 0);
    void stop_timer();

protected:
    /*
     * Implement FinderMessengerManager interface to catch death of
     * active messenger to trigger auto-reconnect.  All methods are
     * forwarded to _real_manager.
     */
    void messenger_birth_event(FinderMessengerBase*);
    void messenger_death_event(FinderMessengerBase*);
    void messenger_active_event(FinderMessengerBase*);
    void messenger_inactive_event(FinderMessengerBase*);
    void messenger_stopped_event(FinderMessengerBase*);
    bool manages(const FinderMessengerBase*) const;

protected:
    FinderMessengerManager& _real_manager;
    bool		    _connected;
    bool		    _enabled;
    bool		    _once_active;
    XorpTimer		    _retry_timer;
    int			    _last_error;
    size_t		    _consec_error;

    static const uint32_t CONNECT_RETRY_PAUSE_MS = 100;
    static const uint32_t CONNECT_FAILS_BEFORE_LOGGING = 10;
};

#endif // __LIBXIPC_FINDER_TCP_MESSENGER_HH__
