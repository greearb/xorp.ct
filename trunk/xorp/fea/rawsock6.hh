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

// $XORP: xorp/fea/rawsock6.hh,v 1.1 2004/11/19 10:54:28 bms Exp $

#ifndef __FEA_RAWSOCK6_HH__
#define __FEA_RAWSOCK6_HH__

#include <list>
#include <vector>

#include "libxorp/exceptions.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/eventloop.hh"

/** Exception class for RawSocket Constructors */
struct RawSocket6Exception : public XorpReasonedException {
    RawSocket6Exception(const char* file, size_t line,
		       const char* cmd, int error_no) :
	XorpReasonedException("RawSocket6Exception", file, line,
			      c_format("%s: %s", cmd, strerror(error_no))) {}
};

/**
 * Base class for raw sockets.  Supports write only.
 */

class RawSocket6 {
public:
    RawSocket6(uint32_t protocol) throw (RawSocket6Exception);

    virtual ~RawSocket6();

    inline uint32_t protocol() const { return _pf; }

    /**
     * Write data to raw socket.
     *
     * @param buf pointer to raw IPv6 packet. Packet fields are expected
     * to be in network order.
     *
     * @param bufbytes number of bytes in packet pointed to by @ref buf.
     *
     * @return number of bytes written on success.  If return value is
     * negative check errno for system errors.  Invalid IPv6 fields
     * may cause packet to be rejected before being passed to system,
     * in which case errno will not indicate an error.  The error is
     * recorded in the xlog.
     */
    ssize_t write(const IPv6& src, const IPv6& dst, const uint8_t* buf,
		  size_t bufbytes) const;

private:
    RawSocket6(const RawSocket6&);		// Not implemented.
    RawSocket6 operator=(const RawSocket6&);	// Not implemented.

protected:
    int32_t  _fd;
    uint32_t _pf;
};

/**
 * Raw socket class supporting input and output.  Output is handled
 * with write() and writev() methods inherited from RawSocket.  Reads
 * are handled asynchronously.  When data arrives on the underlying
 * socket, recv is called and reads it into a per instance buffer.
 * The abstract method process_recv_data is then called with a
 * reference to a buffer containing the data.
 */

class IoRawSocket6 : public RawSocket6
{
public:
    IoRawSocket6(EventLoop& eventloop, uint32_t protocol, bool autohook = true)
	throw (RawSocket6Exception);

    ~IoRawSocket6();

protected:
    virtual void process_recv_data(const vector<uint8_t>& buf) = 0;

protected:
    void recv(int fd, SelectorMask m);

    bool eventloop_hook();

    void eventloop_unhook();

private:
    IoRawSocket6(const IoRawSocket6&);			// Not implemented.
    IoRawSocket6 operator=(const IoRawSocket6&);	// Not implemented.

private:
    enum { RECVBUF_BYTES = 131072 };
    EventLoop&		_eventloop;
    bool		_autohook;
    vector<uint8_t>	_recvbuf;
};

/**
 * A RawSocketClass that allows arbitrary filters to receive the data
 * associated with a raw socket.
 */
class FilterRawSocket6 : public IoRawSocket6
{
public:
    /**
     * Filter class.
     */
    class InputFilter {
    public:
	virtual ~InputFilter() {}
	/**
	 * Method invoked when data arrives on associated FilterRawSocket6
	 * instance.
	 */
	virtual void recv(const vector<uint8_t>& data) = 0;

	/**
	 * Method invoked by the destructor of the associated
	 * FilterRawSocket6 instance.  This method provides the
	 * InputFilter with the opportunity to delete itself or update
	 * it's state.  The input filter does not need to call
	 * FilterRawSocket6::remove_filter() since filter removal is
	 * automatically conducted.
	 */
	virtual void bye() = 0;
    };

public:
    FilterRawSocket6(EventLoop& eventloop, int protocol)
	throw (RawSocket6Exception);
    ~FilterRawSocket6();

    /** Add a filter to list of input filters.  The FilterRawSocket6 class
     * assumes that the callee will be responsible for managing the memory
     * associated with the filter and will call remove_filter() if the
     * filter is deleted or goes out of scope.
     */
    bool add_filter(InputFilter* filter);

    /** Remove filter from list of input filters. */
    bool remove_filter(InputFilter* filter);

    /** @return true if there are no filters associated with this instance. */
    bool empty() const { return _filters.empty(); }

protected:
    void process_recv_data(const vector<uint8_t>& buf);

private:
    FilterRawSocket6(const FilterRawSocket6&);		 // Not implemented.
    FilterRawSocket6 operator=(const FilterRawSocket6&); // Not implemented.

protected:
    list<InputFilter*> _filters;
};

#endif // __FEA_RAWSOCK6_HH__
