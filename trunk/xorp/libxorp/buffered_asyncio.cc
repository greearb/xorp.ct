// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/buffered_asyncio.cc,v 1.12 2007/02/16 22:46:15 pavlin Exp $"

#include "libxorp_module.h"
#include "xorp.h"
#include "debug.h"
#include "xlog.h"

#include "buffered_asyncio.hh"

extern bool is_pseudo_error(const char* name, XorpFd fd, int error_num);

BufferedAsyncReader::BufferedAsyncReader(EventLoop& 		e,
					 XorpFd 			fd,
					 size_t 		reserve_bytes,
					 const Callback& 	cb)
    : _eventloop(e), _fd(fd), _cb(cb), _buffer(reserve_bytes),
      _last_error(0)
{
    _config.head 	  = &_buffer[0];
    _config.head_bytes 	  = 0;
    _config.trigger_bytes = 1;
    _config.reserve_bytes = reserve_bytes;
}

BufferedAsyncReader::~BufferedAsyncReader()
{
    stop();
}

inline void
BufferedAsyncReader::provision_trigger_bytes()
{
    size_t post_head_bytes = _buffer.size() - (_config.head - &_buffer[0]);

    if (_config.head + _config.head_bytes == &_buffer[0] + _buffer.size() ||
	_config.trigger_bytes >= post_head_bytes ||
	post_head_bytes < _buffer.size() / 2) {
	memmove(&_buffer[0], _config.head, _config.head_bytes);
	_config.head = &_buffer[0];
    }
}

bool
BufferedAsyncReader::set_trigger_bytes(size_t bytes)
{
    if (bytes > _config.reserve_bytes)
	return false;

    _config.trigger_bytes = bytes;
    provision_trigger_bytes();

    return true;
}

size_t
BufferedAsyncReader::trigger_bytes() const
{
    return _config.trigger_bytes;
}

bool
BufferedAsyncReader::dispose(size_t bytes)
{
    if (_config.head_bytes < bytes)
	return false;

    _config.head += bytes;
    _config.head_bytes -= bytes;

    return true;
}

bool
BufferedAsyncReader::set_reserve_bytes(size_t bytes)
{
    if (_config.reserve_bytes > bytes)
	return false;

    _buffer.resize(bytes);
    _config.reserve_bytes = bytes;

    return true;
}

size_t
BufferedAsyncReader::reserve_bytes() const
{
    return _config.reserve_bytes;
}

size_t
BufferedAsyncReader::available_bytes() const
{
    return _config.head_bytes;
}

void
BufferedAsyncReader::start()
{
    if (_eventloop.add_ioevent_cb(_fd, IOT_READ,
				  callback(this,
					   &BufferedAsyncReader::io_event)) ==
	false) {
	XLOG_ERROR("BufferedAsyncReader: failed to add I/O event callback.");
    }
#ifdef HOST_OS_WINDOWS
    if (_eventloop.add_ioevent_cb(_fd, IOT_DISCONNECT,
				  callback(this,
					   &BufferedAsyncReader::io_event)) ==
	false) {
	XLOG_ERROR("BufferedAsyncReader: failed to add I/O event callback.");
    }
#endif

    if (_config.head_bytes >= _config.trigger_bytes) {
	_ready_timer =
	     _eventloop.new_oneoff_after_ms(0,
		callback(this, &BufferedAsyncReader::announce_event, DATA));
    }

    debug_msg("%p start\n", this);
}

void
BufferedAsyncReader::stop()
{
    debug_msg("%p stop\n", this);

#ifdef HOST_OS_WINDOWS
    _eventloop.remove_ioevent_cb(_fd, IOT_DISCONNECT);
#endif
    _eventloop.remove_ioevent_cb(_fd, IOT_READ);
    _ready_timer.unschedule();
}

void
BufferedAsyncReader::io_event(XorpFd fd, IoEventType type)
{
    assert(fd == _fd);
#ifndef HOST_OS_WINDOWS
    assert(type == IOT_READ);
#else
    // Explicitly handle disconnection events
    if (type == IOT_DISCONNECT) {
	XLOG_ASSERT(fd.is_socket());
	stop();
	announce_event(END_OF_FILE);
	return;
    }
#endif

    uint8_t* 	tail 	   = _config.head + _config.head_bytes;
    size_t 	tail_bytes = _buffer.size() - (tail - &_buffer[0]);

    assert(tail_bytes >= 1);
    assert(tail + tail_bytes == &_buffer[0] + _buffer.size());

    ssize_t read_bytes = -1;

#ifdef HOST_OS_WINDOWS
    if (fd.is_socket()) {
	read_bytes = ::recvfrom(fd, (char *)tail, tail_bytes, 0,
		       NULL, 0);
	_last_error = WSAGetLastError();
	WSASetLastError(ERROR_SUCCESS);
    } else {
	(void)ReadFile(fd, (LPVOID)tail, (DWORD)tail_bytes,
		       (LPDWORD)&read_bytes, NULL);
	_last_error = GetLastError();
	SetLastError(ERROR_SUCCESS);
    }
#else
    errno = 0;
    _last_error = 0;
    read_bytes = ::read(fd, tail, tail_bytes);
    if (read_bytes < 0)
	_last_error = errno;
    errno = 0;
#endif

    if (read_bytes > 0) {
	_config.head_bytes += read_bytes;
	if (_config.head_bytes >= _config.trigger_bytes) {
	    debug_msg("YES notify - buffered I/O %u / %u\n",
		      XORP_UINT_CAST(_config.head_bytes),
		      XORP_UINT_CAST(_config.trigger_bytes));
	    announce_event(DATA);
	} else {
	    debug_msg("NO notify - buffered I/O %u / %u read %d\n",
		      XORP_UINT_CAST(_config.head_bytes),
		      XORP_UINT_CAST(_config.trigger_bytes),
		      XORP_INT_CAST(read_bytes));
	}
    } else if (read_bytes == 0) {
	announce_event(END_OF_FILE);
    } else {
	if (is_pseudo_error("BufferedAsyncReader", fd, _last_error))
	    return;
	XLOG_ERROR("read error %d", _last_error);
	stop();
	announce_event(OS_ERROR);
    }
}

void
BufferedAsyncReader::announce_event(Event ev)
{
    if (ev == DATA && _config.head_bytes < _config.trigger_bytes) {
	//
	// We might get here because a read returns more data than a user
	// wants to process.  They exit the callback with more data in their
	// buffer than the threshold event so we schedule a timer to
	// prod them again, but in the meantime an I/O event occurs
	// and pre-empts the timer callback.
	// Another example could be when a previous callback modifies
	// the threshold value.
	// Basically, we don't want to call the user below threshold.
	//
	debug_msg("announce_event: DATA (head_bytes = %u, trigger_bytes = %u)",
		  XORP_UINT_CAST(_config.head_bytes),
		  XORP_UINT_CAST(_config.trigger_bytes));
	return;
    }

    //
    // Take a reference to callback and a copy of it's count.  If it's
    // count falls between here and when the callback dispatch returns
    // the current instance has been deleted and we should return
    // without accessing any member state.
    //
    assert(_cb.is_only() == true);
    Callback cb = _cb;

    cb->dispatch(this, ev, _config.head, _config.head_bytes);

    if (cb.is_only() == true)
	return;	// We've been deleted!  Just leave

    provision_trigger_bytes();

    if (_config.head_bytes >= _config.trigger_bytes) {
	_ready_timer =
	    _eventloop.new_oneoff_after_ms(0,
		callback(this, &BufferedAsyncReader::announce_event, DATA));
    }
}
