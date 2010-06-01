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

// $XORP: xorp/libxipc/finder_tcp.hh,v 1.25 2008/10/02 21:57:20 bms Exp $

#ifndef __LIBXIPC_FINDER_TCP_HH__
#define __LIBXIPC_FINDER_TCP_HH__

#include "libxorp/xorp.h"
#include "libxorp/asyncio.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ref_ptr.hh"

#include <vector>



#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif


#define MAX_XRL_INPUT_SIZE	65536	// maximum total XRL input buffer
					// size on the network.

class FinderTcpBase :
    public NONCOPYABLE
{
public:
    FinderTcpBase(EventLoop& e, XorpFd fd);

    virtual ~FinderTcpBase();

    /**
     * Method to be implemented by derived classes that is called when
     * data arrives or an error occurs when processing when data arrives.
     *
     * @param errval error code, values are equivalent to errno.
     * @param data pointer to data
     * @param data_bytes size of data.
     * @return true if the data was processed without an error and the
     * connection was kept, otherwise false. 
     */
    virtual bool read_event(int		   errval,
			    const uint8_t* data,
			    uint32_t	   data_bytes) = 0;

    /**
     * Control whether read events can occur.
     * @param en boolean value with true corresponding to read enabled.
     */
    void set_read_enabled(bool en);

    /**
     * Determine whether read events can occur.
     * @return true if read events are enabled.
     */
    bool read_enabled() const;

    /**
     * Write data on TCP connection.  To avoid an unnecessary data
     * copy, the client is expected to ensure the data is valid until
     * departure_event is called with the corresponding data pointer.
     *
     * @param data pointer to data.
     * @param data_bytes size data pointed to in bytes.
     *
     * @return true if data accepted for writing, false if no buffer
     * space is available at this time.
     */
    bool write_data(const uint8_t* data, uint32_t data_bytes);

    bool write_data(const iovec* iov, uint32_t iovcnt);

    /**
     * Method to be implemented by derived classes that is called when
     * data writing completes or an error occurs when processing when write.
     *
     * @param errval error code, values are equivalent to errno.
     */
    virtual void write_event(int	    errval,
			     const uint8_t* data,
			     uint32_t	    data_bytes) = 0;

    /**
     * Method that may be implemented by derived classes for detecting when
     * the underlying socket is closed.
     */
    virtual void close_event();

    virtual void error_event();

    void close();
    bool closed() const;

protected:
    void read_callback(AsyncFileOperator::Event,
		       const uint8_t*, size_t, size_t);
    void write_callback(AsyncFileOperator::Event,
			const uint8_t*, size_t, size_t);

protected:
    XorpFd _sock;
    vector<uint8_t> _input_buffer;

    AsyncFileReader _reader;
    AsyncFileWriter _writer;

    uint32_t _isize;	// input buffer size as received.
    uint32_t _osize;	// output buffer size as advertised.
};

class FinderTcpListenerBase :
    public NONCOPYABLE
{
public:
    typedef vector<IPv4> AddrList;
    typedef vector<IPv4Net> NetList;

public:
    FinderTcpListenerBase(EventLoop&	e,
			  IPv4		interface,
			  uint16_t	port,
			  bool		en = true)
	throw (InvalidAddress, InvalidPort);

    virtual ~FinderTcpListenerBase();

    /**
     * Method called when a connection is accepted and matches permitted
     * access conditions.
     *
     * @param fd file descriptor associated with new connection.
     *
     * @return true if instance agrees to take responsibility for file
     * descriptor, false otherwise.
     */
    virtual bool connection_event(XorpFd fd) = 0;

    /**
     * Determine whether listener is enabled.
     */
    bool enabled() const;

    /**
     * Control whether listener is enabled.
     */
    void set_enabled(bool en);

    /**
     * Get interface address listener is operating on.
     */
    IPv4 address() const		{ return _addr; }

    /**
     * Get port listener is bound to.
     */
    uint16_t port() const		{ return _port; }

protected:
    /**
     * Accepts connection, checks source address, and then calls
     * connection_event() if source is valid.
     */
    void connect_hook(XorpFd fd, IoEventType type);

    EventLoop& eventloop() const { return _e; }

protected:
    EventLoop&	_e;
    XorpFd	_lsock;
    bool	_en;

    IPv4	_addr;
    uint16_t	_port;

    AddrList	_ok_addrs;
    NetList	_ok_nets;
};

#endif // __LIBXIPC_FINDER_TCP_HH__
