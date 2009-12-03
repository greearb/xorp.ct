// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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



#include "libxorp_module.h"
#include "xorp.h"
#include "debug.h"
#include "xlog.h"

#include "buffered_asyncio.hh"

extern bool is_pseudo_error(const char* name, XorpFd fd, int error_num);

BufferedAsyncReader::BufferedAsyncReader(EventLoop& 		e,
					 XorpFd 		fd,
					 size_t 		reserve_bytes,
					 const Callback& 	cb,
					 int			priority)
    : _eventloop(e), _fd(fd), _cb(cb), _buffer(reserve_bytes),
      _last_error(0),
      _priority(priority)
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

    size_t head_off = _config.head - &_buffer[0];
    _buffer.resize(bytes);

    _config.head = &_buffer[0] + head_off;
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
					   &BufferedAsyncReader::io_event),
				  _priority) ==
	false) {
	XLOG_ERROR("BufferedAsyncReader: failed to add I/O event callback.");
    }

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

    _eventloop.remove_ioevent_cb(_fd, IOT_READ);
    _ready_timer.unschedule();
}

void
BufferedAsyncReader::io_event(XorpFd fd, IoEventType type)
{
    assert(fd == _fd);
    assert(type == IOT_READ);

    uint8_t* 	tail 	   = _config.head + _config.head_bytes;
    size_t 	tail_bytes = _buffer.size() - (tail - &_buffer[0]);

    assert(tail_bytes >= 1);
    assert(tail + tail_bytes == &_buffer[0] + _buffer.size());

    ssize_t read_bytes = -1;

    errno = 0;
    _last_error = 0;
    read_bytes = ::read(fd, tail, tail_bytes);
    if (read_bytes < 0)
	_last_error = errno;
    errno = 0;

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
