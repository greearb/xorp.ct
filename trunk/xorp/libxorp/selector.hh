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

// $XORP: xorp/libxorp/selector.hh,v 1.1.1.1 2002/12/11 23:56:05 hodson Exp $

#ifndef __LIBXORP_SELECTOR_HH__
#define __LIBXORP_SELECTOR_HH__

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include <vector>

#include "callback.hh"


class SelectorList;

/**
 * Selector event type masks.
 */
enum SelectorMask {
    SEL_RD	= 0x01,				// Read events
    SEL_WR	= 0x02,				// Write events
    SEL_EX	= 0x04,				// Exception events
    SEL_ALL	= SEL_RD | SEL_WR | SEL_EX	// All events
};

/**
 * SelectorList event hook type.
 */
typedef void (*SelectorHook)(int fd, SelectorMask mask, void* cookie);

typedef XorpCallback2<void,int,SelectorMask>::RefPtr SelectorCallback;

class SelectorTag;
typedef ref_ptr<SelectorTag> Selector;

/**
 * @short A class to provide an interface to I/O multiplexing.
 *
 * A SelectorList provides an entity where callbacks for pending I/O
 * operations on file descriptors may be registered.  The callbacks
 * are invoked when one of the @ref select methods is called and I/O
 * is pending on the particular descriptors.
 *
 */
class SelectorList { 
public:
    
    /**
     * Default constructor
     */
    SelectorList();

    /**
     * Add a hook for pending I/O operations on a callback.  
     *
     * Only one callback may be registered for each possible I/O event
     * type (read, write, exception).  
     *
     * Multiple event types may share the same callback. If multiple
     * event types share the same callback and multiple types of event
     * are pending, the callback is invoked just once and the mask
     * argument to the callback shows which events are pending.
     *
     * @param file descriptor.  
     *
     * @param mask mask of I/O event types
     * that should invoke callback.  An OR'ed combination of the
     * available @ref SelectorMask values.
     *
     * @param scb callback object that will be invoked when the
     * descriptor.
     *
     * @return true if function succeeds, false otherwise.  
     */
     bool add_selector(int fd, SelectorMask mask, 
		       const SelectorCallback& scb);

    /**
     * [Deprecated]
     * Historical add_selector method.
     */
    inline bool add_selector(int fd, SelectorMask mask, 
			     SelectorHook h, void* cookie) 
    { 
	return add_selector(fd, mask, callback(h, cookie)); 
    }

    // Not implemented - new interface
    inline Selector add_selector(int fd, SelectorMask mask);

    /**
     * Remove hooks for pending I/O operations.
     *
     * @param fd the file descriptor.
     *
     * @param mask of event types to be removed, e.g. an OR'ed
     * combination of the available @ref SelectorMask values.
     */
    void remove_selector(int fd, SelectorMask event_mask = SEL_ALL);

    /**
     * Wait for a pending I/O events and invoke callbacks when they
     * become ready.
     *
     * @param timeout the maximum period to wait for.
     *
     * @return the number of callbacks that were made.
     */
    int	select(timeval *timeout);

    /**
     * Wait for a pending I/O events and invoke callbacks when they
     * become ready.
     *
     * @param millisecs the maximum period in milliseconds to wait for.
     *
     * @return the number of callbacks that were made.
     */
    int	select(int millisecs);

    /**
     * Get the number of file descriptors with requested callbacks.
     * 
     * @return the number of file descriptors that currently have
     * callbacks registered.  
     */
    inline size_t descriptor_count() const { return _descriptor_count; }

protected:
    void callback_bad_descriptors();

private:
    enum {
	// correspond to SelectorMask; correspondence checked with
	// static_assert
	SEL_RD_IDX	= 0,
	SEL_WR_IDX	= 1,
	SEL_EX_IDX	= 2,
	SEL_MAX_IDX	= 3
    };
    struct Node {
	int			_mask[SEL_MAX_IDX];
	SelectorCallback	_cb[SEL_MAX_IDX];

	Node();
	inline bool	add_okay(SelectorMask m, const SelectorCallback& scb);
	inline int	run_hooks(SelectorMask m, int fd);
	inline void	clear(SelectorMask m);
	inline bool	is_empty();
    };

    vector<Node>	_selector_entries;
    fd_set		_fds[SEL_MAX_IDX];
    int			_maxfd;
    size_t		_descriptor_count;
};

class SelectorTag {
public:
    SelectorTag(SelectorList& l, int fd, SelectorMask mask) :
	_sl(l), _fd(fd), _mask(mask) {}
    ~SelectorTag() {
	remove_selector();
    }
    inline void remove_selector() {
	if (_fd >= 0) {
	    _sl.remove_selector(_fd, _mask);
	    _fd = -1;
	}
    }
    inline bool connected() const {
	return _fd == -1;
    }
private:
    SelectorList&	_sl;
    int			_fd;
    SelectorMask	_mask;
};

#endif // __LIBXORP_SELECTOR_HH__
