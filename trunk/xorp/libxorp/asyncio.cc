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

#ident "$XORP: xorp/libxorp/asyncio.cc,v 1.9 2004/09/24 04:52:21 pavlin Exp $"

#include <signal.h>

#include "libxorp_module.h"
#include "config.h"
#include "xorp.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"
#include "asyncio.hh"

// ----------------------------------------------------------------------------
// Utility

bool
is_pseudo_error(const char* name, int fd, int error_num)
{
    switch (error_num) {
    case EINTR:
	XLOG_WARNING("%s (fd = %d) got EINTR, continuing.", name, fd);
	return true;
    case EAGAIN:
	XLOG_WARNING("%s (fd = %d) got EAGAIN, continuing.", name, fd);
	return true;
    }
    return false;
}

// ----------------------------------------------------------------------------
AsyncFileOperator::~AsyncFileOperator()
{
    _fd = -9999;
}

// ----------------------------------------------------------------------------
// AsyncFileReader read method and entry hook

void
AsyncFileReader::add_buffer(uint8_t* b, size_t b_bytes, const Callback& cb)
{
    assert(b_bytes != 0);
    _buffers.push_back(BufferInfo(b, b_bytes, cb));
}

void
AsyncFileReader::add_buffer_with_offset(uint8_t*	b, 
					size_t		b_bytes, 
					size_t		off,
					const Callback&	cb) 
{
    assert(off < b_bytes);
    _buffers.push_back(BufferInfo(b, b_bytes, off, cb));
}

void
AsyncFileReader::read(int fd, SelectorMask m) {
    assert(m == SEL_RD);
    assert(fd == _fd);
    assert(_buffers.empty() == false);

    debug_msg("Buffer count %u\n", (uint32_t)_buffers.size());

    BufferInfo& head = _buffers.front();

    errno = 0;
    ssize_t done = ::read(_fd, head._buffer + head._offset,
			      head._buffer_bytes - head._offset);

    if (done < 0 && is_pseudo_error("AsyncFileReader", _fd, errno)) {
	errno = 0;
	return;
    }
    complete_transfer(errno, done);
}

// transfer_complete() invokes callbacks if necessary and updates buffer
// variables and buffer list.
void
AsyncFileReader::complete_transfer(int err, ssize_t done) {
    // XXX careful after callback is invoked: "this" maybe deleted, so do
    // not reference any object state after callback.

    if (done > 0) {
	BufferInfo& head = _buffers.front();
	head._offset += done;
	if (head._offset == head._buffer_bytes) {
	    BufferInfo copy = head; 		// copy head
	    _buffers.erase(_buffers.begin());	// remove head
	    if (_buffers.empty()) {
		stop();
	    }
	    copy.dispatch_callback(DATA);
	} else {
	    head.dispatch_callback(DATA);
	}
	return;
    }

    BufferInfo& head = _buffers.front();
    if (err != 0 || done < 0) {
	stop();
	head.dispatch_callback(ERROR_CHECK_ERRNO);
    } else {
	head.dispatch_callback(END_OF_FILE);
    }
}

bool 
AsyncFileReader::start() {
    if (_running)
	return true;

    if (_buffers.empty() == true) {
	XLOG_WARNING("Could not start reader - no buffers available");
	return false;
    }

    EventLoop& e = _eventloop;
    if (e.add_selector(_fd, SEL_RD, 
		       callback(this, &AsyncFileReader::read)) == false) {
	XLOG_ERROR("Async reader failed to add_selector.");
	return false;
    }

    _running = true;
    return _running;
}

void
AsyncFileReader::stop() {
    _eventloop.remove_selector(_fd, SEL_RD);
    _running = false;
}

void
AsyncFileReader::flush_buffers() {
    stop();
    while (_buffers.empty() == false) {
	// Copy out buffer so flush buffers can be called re-entrantly (even
	// if we happen to think this is bad coding style :-).
	BufferInfo head = _buffers.front();
	_buffers.erase(_buffers.begin());
	head.dispatch_callback(FLUSHING);
    }
}

// ----------------------------------------------------------------------------
// AsyncFileWriter write method and entry hook

void
AsyncFileWriter::add_buffer(const uint8_t*	b, 
			    size_t		b_bytes, 
			    const Callback&	cb) 
{
    assert(b_bytes != 0);
    _buffers.push_back(BufferInfo(b, b_bytes, cb));
}

void
AsyncFileWriter::add_buffer_with_offset(const uint8_t*	b, 
					size_t		b_bytes, 
					size_t		off,
					const Callback&	cb) 
{
    assert(off < b_bytes);
    _buffers.push_back(BufferInfo(b, b_bytes, off, cb));
}

static int write_call = 0;

void
AsyncFileWriter::write(int fd, SelectorMask m) 
{
    write_call++;
    assert(_buffers.empty() == false);
    assert(m == SEL_WR);
    assert(fd == _fd);
    BufferInfo& head = _buffers.front();

    size_t out_bytes = head._buffer_bytes - head._offset;
 retry:

    sig_t saved_sigpipe = signal(SIGPIPE, SIG_IGN);
    ssize_t done = ::write(_fd, head._buffer + head._offset, out_bytes);
    signal(SIGPIPE, saved_sigpipe);
    if (done < 0 && is_pseudo_error("AsyncFileWriter", _fd, errno)) {
	errno = 0;
	return;
    }
    if (done < 0 && errno == ENOBUFS) {
	out_bytes /= 2;
	if (out_bytes > 0)
	    goto retry;
	else {
	    return;
	}
    }

    complete_transfer(done);
}

// transfer_complete() invokes callbacks if necessary and updates buffer
// variables and buffer list.
void
AsyncFileWriter::complete_transfer(ssize_t done) 
{
    // XXX careful after callback is invoked: "this" maybe deleted, so do
    // not reference any object state after callback.

    if (done >= 0) {
	BufferInfo& head = _buffers.front();
	head._offset += done;
	if (head._offset == head._buffer_bytes) {
	    BufferInfo copy = head; 		// copy head
	    _buffers.erase(_buffers.begin());	// remove head
	    if (_buffers.empty()) {
		stop();
	    }
	    copy.dispatch_callback(DATA);
	} else {
	    head.dispatch_callback(DATA);
	}
    } else {
	stop();
	BufferInfo& head = _buffers.front();
	head.dispatch_callback(ERROR_CHECK_ERRNO);
    }
}

bool 
AsyncFileWriter::start() {
    if (_running)
	return true;

    if (_buffers.empty() == true) {
	XLOG_WARNING("Could not start writer - no buffers available");
	return false;
    }

    EventLoop& e = _eventloop;
    if (e.add_selector(_fd, SEL_WR, 
		       callback(this, &AsyncFileWriter::write)) == false) {
	XLOG_ERROR("Async reader failed to add_selector.");
    }
    _running = true;
    return _running;
}

void
AsyncFileWriter::stop() {
    _eventloop.remove_selector(_fd, SEL_WR);
    _running = false;
}

void
AsyncFileWriter::flush_buffers() {
    stop();
    while (_buffers.empty() == false) {
	// Copy out buffer so flush buffers can be called re-entrantly (even
	// if we happen to think this is bad coding style :-).
	BufferInfo head = _buffers.front();
	_buffers.erase(_buffers.begin());
	head.dispatch_callback(FLUSHING);
    }
}
