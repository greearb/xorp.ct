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

// $XORP: xorp/libxorp/asyncio.hh,v 1.9 2004/09/24 04:52:21 pavlin Exp $

#ifndef __LIBXORP_BUFFERED_ASYNCIO_HH__
#define __LIBXORP_BUFFERED_ASYNCIO_HH__

#include <list>
#include <fcntl.h>

#include "libxorp/callback.hh"
#include "libxorp/eventloop.hh"

class BufferedAsyncReader {
public:
    enum Event {
	DATA 		  = 1,
	ERROR_CHECK_ERRNO = 2,
	END_OF_FILE 	  = 3
    };

    /*
     * Callback has arguments:
     *		BufferedAsyncReader*	caller,
     * 		ErrorCode		e,
     *		uint8_t*		buffer,
     * 		size_t 			buffer_bytes,
     *          size_t 			offset
     *
     * The callback is invoked when data arrives, when an error
     * occurs, or the end of file is detected.  For data, the callback is
     * only invoked when the threshold is crossed.
     */
    typedef XorpCallback4<void, BufferedAsyncReader*, Event, uint8_t*, size_t>::RefPtr Callback;

    /**
     * Constructor.
     *
     * @param e the eventloop.
     * @param fd the file descriptor.
     * @param reserve_bytes the number of bytes to reserve in the data buffer.
     * @param cb the callback to invoke.
     */
    BufferedAsyncReader(EventLoop& 	e,
			int 		fd,
			size_t 		reserve_bytes,
			const Callback& cb);

    ~BufferedAsyncReader();

    /**
     * Set threshold for event notification.  Only when this threshold
     * is reached the consumers callback invoked.  If more data is already
     * available, then the event notification will be triggered
     * through a 0 second timer.  This provides an opportunity for
     * other tasks to run.
     *
     * Calling this method may cause the internal buffer state to
     * change.  If it is called from within a consumer callback, then
     * the buffer pointer may become invalid and dereferencing the
     * pointer should be avoided.
     *
     * @param bytes the number of threshold bytes.
     * @return true on success, false if bytes is larger than reserve.
     */
    bool set_trigger_bytes(size_t bytes);

    /**
     * Get the current threshold for event notification.
     */
    size_t trigger_bytes() const;

    /**
     * Acknowledge data at the start of the buffer is finished with.
     *
     * Typically, a consumer would call this from within their
     * callback to say this number of bytes has been processed and can
     * be discarded.
     *
     * @param bytes the number of bytes to dispose.
     * @return true on success, false if bytes is larger than the number
     * of available bytes.
     */
    bool dispose(size_t bytes);

    /**
     * Set reserve for maximum amount of data to receive.
     *
     * @param bytes the number of bytes to reserve.
     * @return true on success, false if error.
     */
    bool set_reserve_bytes(size_t bytes);

    /**
     * Get reserve for maximum amount of data to receive.
     */
    size_t reserve_bytes() const;

    /**
     * Get the number of currently available bytes.
     */
    size_t available_bytes() const;

    /**
     * Start.
     *
     * Start asynchrous reads.
     */
    void start();

    /**
     * Stop.
     *
     * Stop asynchrous reading.
     */
    void stop();

private:
    BufferedAsyncReader();				// Not implemented
    BufferedAsyncReader(const BufferedAsyncReader&);	// Not implemented
    BufferedAsyncReader& operator=(const BufferedAsyncReader&); // Not implemented

private:
    void selector_event(int fd, SelectorMask m);
    void announce_event(Event e);
    inline void provision_trigger_bytes();

    struct Config {
	uint8_t* head;		// The beginning of data
	size_t   head_bytes;	// The number of available bytes with data
	size_t	 trigger_bytes;	// The number of bytes to trigger cb delivery
	size_t	 reserve_bytes;	// The number of bytes to reserve for data
    };

    Config		_config;

    EventLoop&		_eventloop;
    int			_fd;
    Callback		_cb;

    vector<uint8_t>	_buffer;
    XorpTimer		_ready_timer;
};

#endif // __LIBXORP_BUFFERED_ASYNCIO_HH__
