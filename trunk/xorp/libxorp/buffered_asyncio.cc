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

#ident "$XORP: xorp/libxorp/asyncio.cc,v 1.10 2004/09/27 01:07:39 pavlin Exp $"

#include "libxorp_module.h"
#include "xorp.h"
#include "debug.h"
#include "xlog.h"

#include "buffered_asyncio.hh"

BufferedAsyncReader::BufferedAsyncReader(EventLoop& 		e,
					 int 			fd,
					 size_t 		reserve_bytes,
					 const Callback& 	cb)
    : _eventloop(e), _fd(fd), _cb(cb), _buffer(reserve_bytes)
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
    _eventloop.add_selector(_fd, SEL_RD,
			    callback(this,
				     &BufferedAsyncReader::selector_event));
    if (_config.head_bytes >= _config.trigger_bytes) {
	_ready_timer =
	    _eventloop.new_oneoff_after_ms(0,
		callback(this, &BufferedAsyncReader::announce_event, DATA));

    }
}

void
BufferedAsyncReader::stop()
{
    _eventloop.remove_selector(_fd);
    _ready_timer.unschedule();
}

void
BufferedAsyncReader::selector_event(int fd, SelectorMask m)
{
    assert((m & SEL_RD) == SEL_RD);
    assert(fd == _fd);

    uint8_t* 	tail 	   = _config.head + _config.head_bytes;
    size_t 	tail_bytes = _buffer.size() - (tail - &_buffer[0]);

    assert(tail_bytes >= 1);
    assert(tail + tail_bytes == &_buffer[0] + _buffer.size());

    ssize_t 	read_bytes = ::read(fd, tail, tail_bytes);

    if (read_bytes > 0) {
	_config.head_bytes += read_bytes;
	if (_config.head_bytes >= _config.trigger_bytes) {
	    debug_msg("YES notify - buffered i/o %u / %u\n",
		      static_cast<uint32_t>(_config.head_bytes),
		      static_cast<uint32_t>(_config.trigger_bytes));
	    announce_event(DATA);
	} else {
	    debug_msg("NO notify - buffered i/o %u / %u read %d\n",
		      static_cast<uint32_t>(_config.head_bytes),
		      static_cast<uint32_t>(_config.trigger_bytes),
		      read_bytes);
	}
    } else if (read_bytes == 0) {
	announce_event(END_OF_FILE);
    } else {
	if (errno == EINTR || errno == EAGAIN)
	    return;
	XLOG_ERROR("read error %d %s", errno, strerror(errno));
	stop();
	announce_event(ERROR_CHECK_ERRNO);
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
	// prod them again, but in the meantime selector event occurs
	// and pre-empts the timer callback.
	// Another example could be when a previous callback modifies
	// the threshold value.
	// Basically, we don't want to call the user below threshold.
	//
	debug_msg("announce_event: DATA (head_bytes = %u, trigger_bytes = %u)",
		  static_cast<uint32_t>(_config.head_bytes),
		  static_cast<uint32_t>(_config.trigger_bytes));
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
