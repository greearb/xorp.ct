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

// $XORP: xorp/libxipc/finder_tcp.hh,v 1.2 2003/01/24 01:45:39 hodson Exp $

#ifndef __LIBXIPC_FINDER_TCP_HH__
#define __LIBXIPC_FINDER_TCP_HH__

#include <sys/types.h>
#include <sys/uio.h>

#include <vector>

#include "libxorp/asyncio.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ref_ptr.hh"

static const uint16_t FINDER_TCP_DEFAULT_PORT = 19999;

class FinderTcpBase {
public:
    FinderTcpBase(EventLoop& e, int fd);

    virtual ~FinderTcpBase();

    /**
     * Method to be implemented by derived classes that is called when
     * data arrives or an error occurs when processing when data arrives.
     *
     * @param errval error code, values are equivalent to errno.
     * @param data pointer to data
     * @param data_bytes size of data.
     */
    virtual void read_event(int		   errval,
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
    
    void close();
    bool closed() const;

protected:
    FinderTcpBase(const FinderTcpBase&);		// Not implemented
    FinderTcpBase& operator=(const FinderTcpBase&);	// Not implemented

    void read_callback(AsyncFileOperator::Event,
		       const uint8_t*, size_t, size_t);
    void write_callback(AsyncFileOperator::Event,
			const uint8_t*, size_t, size_t);
    
protected:    
    int _fd;
    vector<uint8_t> _input_buffer;

    AsyncFileReader _reader;
    AsyncFileWriter _writer;
    
    uint32_t _isize;	// input buffer size as received.
    uint32_t _osize;	// output buffer size as advertised.

    // Maximum size of _input_buffer.
    static const uint32_t FINDER_TCP_BUFFER_BYTES = 8192;
};

class FinderTcpListenerBase {
public:
    typedef vector<IPv4> AddrList;
    typedef vector<IPv4Net> NetList;

public:
    FinderTcpListenerBase(EventLoop&	e,
			  IPv4		interface,
			  uint16_t	port)
	throw (InvalidPort);

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
    virtual bool connection_event(int fd) = 0;

    /**
     * Add host to permitted hosts list.
     * @return true if host is not on list already.
     */
    bool add_permitted_host(const IPv4& host);

    /**
     * Add list of permitted hosts.
     */
    bool add_permitted_hosts(const AddrList& hosts);
    
    /**
     * Add net to permitted nets list.
     * @return true if net is not on list already.
     */
    bool add_permitted_net(const IPv4Net& net);

    /**
     * Add list of permitted nets.
     */
    bool add_permitted_nets(const NetList& hosts);
    
    inline const AddrList& permitted_hosts() const { return _ok_addrs; }
    inline const NetList&  permitted_nets() const  { return _ok_nets; }
protected:
    /**
     * Accepts connection, checks source address, and then calls
     * connection_event() if source is valid.
     */
    void connect_hook(int fd, SelectorMask m);
    
    FinderTcpListenerBase(const FinderTcpListenerBase&);	    // Not impl
    FinderTcpListenerBase& operator=(const FinderTcpListenerBase&); // Not impl

    inline EventLoop& event_loop() const { return _e; }
    
protected:
    EventLoop&	_e;
    int		_lfd;

    AddrList	_ok_addrs;
    NetList	_ok_nets;
};

#endif // __LIBXIPC_FINDER_TCP_HH__
