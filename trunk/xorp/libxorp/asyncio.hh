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

// $XORP: xorp/libxorp/asyncio.hh,v 1.7 2004/06/10 22:41:14 hodson Exp $

#ifndef __LINXORP_ASYNCIO_HH__
#define __LINXORP_ASYNCIO_HH__

#include <list>
#include <fcntl.h>

#include "libxorp/callback.hh"
#include "libxorp/eventloop.hh"

// Asynchronous file transfer classes.  These utilize XORP EventLoop
// and it's SelectorList to read / write files asynchronously.  The
// user creates and AsyncFile{Reader,Writer} and adds a buffer for
// reading / writing with add_buffer().  A callback is provided with
// each buffer is called every time I/O happens on the buffer.  
//
// Reading/Writing only begins when start() is called, and normally
// continues until there are no buffers left.

// ----------------------------------------------------------------------------
// AsyncFileOperator - Abstract base class for asynchronous file operators.

/**
 * @short Base class for asynchronous file transfer operations.
 *
 * Asynchronous file transfer operations allow data to be transferred
 * to or from user supplied buffers.  A callback is invoked on each
 * transfer.  Transfer stops when the available buffers are exhausted.
 */
class AsyncFileOperator {
public:
    enum Event {
	DATA = 1,		// I/O occured
	FLUSHING = 2,		// Buffer is being flushed
	ERROR_CHECK_ERRNO = 4,	// I/O Error has occurred, check errno 
	END_OF_FILE = 8		// End of file reached (applies to read only)
    };

    /**
     * Callback type user provides when adding buffers to sub-classes
     * AsyncFileOperator.  Callback's are on a per buffer basis and
     * invoked any time some I/O is performed.  The offset field
     * refers to the offset of the last byte read, or written, from
     * the start of the buffer.
     * 
     * Callback has arguments:
     * 		ErrorCode	e, 
     *		uint8_t*	buffer, 
     * 		size_t 		buffer_bytes, 
     *          size_t 		offset
     */

    typedef XorpCallback4<void, Event, const uint8_t*, size_t, size_t>::RefPtr Callback;
public:
    /**
     * @return the number of buffers available.
     */
    virtual size_t 	buffers_remaining() const = 0;

    /**
     * Stop asynchronous operation and clear list of buffers.
     */
    virtual void	flush_buffers() = 0;

    /**
     * Start asynchronous operation.
     * 
     * @return true on success, false if no buffers are available.
     */
    virtual bool	start() = 0;

    /**
     * Stop asynchronous operation.
     */
    virtual void	stop()  = 0;

    /**
     * Resume stopped asynchronous operation.
     * 
     * @return true on success, false if no buffers are available.
     */
    inline bool		resume()		{ return start(); }

    /**
     * @return true if asynchronous I/O is started.
     */
    inline bool		running() const 	{ return _running; }

    /**
     * @return file descriptor undergoing asynchronous operation.
     */
    inline int		fd() const		{ return _fd; }

protected:
    AsyncFileOperator(EventLoop& e, int fd)
	: _eventloop(e), _fd(fd), _running(false) {
	int fl = fcntl(fd, F_GETFL);
	assert(fl & O_NONBLOCK);
    }
    virtual ~AsyncFileOperator();

    EventLoop&		_eventloop;
    int			_fd;
    bool		_running;
};

/**
 * @short Read asynchronously from a file.
 */
class AsyncFileReader : public AsyncFileOperator {
public:
    /**
     * @param e EventLoop that object should associate itself with.
     * @param fd a file descriptor to read from.
     */
    AsyncFileReader(EventLoop& e, int fd) : AsyncFileOperator(e, fd) {}
    ~AsyncFileReader() { stop(); }

    /**
     * Add an additional buffer for reading to.
     * 
     * @param buffer pointer to buffer.
     * @param buffer_bytes size of buffer in bytes.
     * @param cb Callback object to invoke when I/O is performed.
     */
    void add_buffer(uint8_t* buffer, size_t buffer_bytes, const Callback& cb);

    /**
     * Add an additional buffer for reading to.
     * 
     * @param buffer pointer to buffer.
     * @param buffer_bytes size of buffer in bytes.
     * @param offset starting point for read operation.
     * @param cb Callback object to invoke when I/O is performed.
     */
    void add_buffer_with_offset(uint8_t* buffer, size_t buffer_bytes,
				size_t offset, const Callback& cb);

    /**
     * Start asynchronous operation.
     * 
     * @return true on success, false if no buffers are available.
     */
    bool start();

    /**
     * Stop asynchronous operation.
     */
    void stop();

    /**
     * @return the number of buffers available.
     */
    size_t buffers_remaining() const { return _buffers.size(); }

    /**
     * Stop asynchronous operation and clear list of buffers.
     */
    void flush_buffers();
    
protected:
    struct BufferInfo {
	BufferInfo(uint8_t* b, size_t bb, Callback cb)
	    : _buffer(b), _buffer_bytes(bb), _offset(0), _cb(cb) {}
	BufferInfo(uint8_t* b, size_t bb, size_t off, Callback cb) 
	    : _buffer(b), _buffer_bytes(bb), _offset(off), _cb(cb) {} 
	inline void dispatch_callback(AsyncFileOperator::Event e) {
	    _cb->dispatch(e, _buffer, _buffer_bytes, _offset);
	}
	uint8_t*	_buffer;
	size_t		_buffer_bytes;
	size_t		_offset;
	Callback	_cb;
    };

    void read(int fd, SelectorMask m);
    void complete_transfer(int err, ssize_t done);

    list<BufferInfo> _buffers;
};


/**
 * @short Write asynchronously to non-blocking file.
 */
class AsyncFileWriter : public AsyncFileOperator {
public:
    /**
     * @param e EventLoop that object should associate itself with.
     * @param fd a file descriptor marked as non-blocking to write to.
     */
    AsyncFileWriter(EventLoop& e, int fd) : AsyncFileOperator(e, fd),
					    _immediate_ctr(0)
    {}

    ~AsyncFileWriter() { stop(); }

    /**
     * Add an additional buffer for writing from.
     * 
     * @param buffer pointer to buffer.
     * @param buffer_bytes size of buffer in bytes.
     * @param cb Callback object to invoke when I/O is performed.
     */
    void add_buffer(const uint8_t*	buffer,
		    size_t		buffer_bytes,
		    const Callback&	cb);

    /**
     * Add an additional buffer for writing from.
     * 
     * @param buffer pointer to buffer.
     * @param buffer_bytes size of buffer in bytes.
     * @param offset the starting point to write from in the buffer.
     * @param cb Callback object to invoke when I/O is performed.
     */
    void add_buffer_with_offset(const uint8_t*	buffer,
				size_t		buffer_bytes,
				size_t		offset,
				const Callback&	cb);

    /**
     * Start asynchronous operation.
     * 
     * @return true on success, false if no buffers are available.
     */
    bool start();

    /**
     * Stop asynchronous operation.
     */
    void stop();

    /**
     * @return the number of buffers available.
     */
    size_t buffers_remaining() const { return _buffers.size(); }

    /**
     * Stop asynchronous operation and clear list of buffers.
     */
    void flush_buffers();
    
protected:
    struct BufferInfo {
	BufferInfo(const uint8_t* b, size_t bb, const Callback& cb) 
	    : _buffer(b), _buffer_bytes(bb), _offset(0), _cb(cb) {}
	BufferInfo(const uint8_t* b, size_t bb, size_t off, 
		   const Callback& cb) 
	    : _buffer(b), _buffer_bytes(bb), _offset(off), _cb(cb)
	{}
	inline void dispatch_callback(AsyncFileOperator::Event e) {
	    _cb->dispatch(e, _buffer, _buffer_bytes, _offset);
	}

	const uint8_t*	_buffer;
	size_t		_buffer_bytes;
	size_t		_offset;
	Callback	_cb;
    };

    void write(int, SelectorMask);
    void immediate_write();
    bool complete_transfer(ssize_t done);

    list<BufferInfo> _buffers;
    int _immediate_ctr;
};

#endif // __LIBXORP_ASYNCIO_HH__
