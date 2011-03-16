// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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

#include "libxorp/xorp.h"

#ifndef HOST_OS_WINDOWS // Entire file is stubbed out on Windows.

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/xorpfd.hh"
#include "libxorp/timeval.hh"
#include "libxorp/clock.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/utility.h"

#include "selector.hh"


// ----------------------------------------------------------------------------
// Helper function to deal with translating between old and new
// I/O event notification mechanisms.

static SelectorMask
map_ioevent_to_selectormask(const IoEventType type)
{
    SelectorMask mask = SEL_NONE;

    // Convert new event type to legacy UNIX event mask used by SelectorList.
    switch (type) {
    case IOT_READ:
	mask = SEL_RD;
	break;
    case IOT_WRITE:
	mask = SEL_WR;
	break;
    case IOT_EXCEPTION:
	mask = SEL_EX;
	break;
    case IOT_ACCEPT:
	mask = SEL_RD;
	break;
    case IOT_CONNECT:
	mask = SEL_WR;
	break;
    case IOT_DISCONNECT:
	mask = SEL_EX;	// XXX: Disconnection isn't a distinct event in UNIX
	break;
    case IOT_ANY:
	mask = SEL_ALL;
	break;
    }
    return (mask);
}

// ----------------------------------------------------------------------------
// SelectorList::Node methods

inline
SelectorList::Node::Node() {
    magic = GOOD_NODE_MAGIC;
    for (int i = 0; i<SEL_MAX_IDX; i++) {
	_mask[i] = 0;
	_priority[i] = XorpTask::PRIORITY_INFINITY;
	_iot[i] = IOT_ANY;
	_cb[i] = NULL; // callbacks are ref_ptr objects.
    }
}

SelectorList::Node::~Node() {
    magic = 0xDEADBEEF;
}

SelectorList::Node::Node(const Node& rhs) {
    magic = GOOD_NODE_MAGIC;
    *this = rhs;
}

SelectorList::Node& SelectorList::Node::operator=(const Node& rhs) {
    if (this == &rhs) {
	return *this;
    }
    for (int i = 0; i<SEL_MAX_IDX; i++) {
	_mask[i] = rhs._mask[i];
	_priority[i] = rhs._priority[i];
	_iot[i] = rhs._iot[i];
	_cb[i] = rhs._cb[i];
    }
    return *this;
} 

inline bool
SelectorList::Node::add_okay(SelectorMask m, IoEventType type,
			     const IoEventCb& cb, int priority)
{
    int idx = -1;

    // always OK to try to register for nothing
    if (!m)
	return true;

    // Sanity Checks

    // 0. We understand all bits in 'mode'
    assert((m & (SEL_RD | SEL_WR | SEL_EX)) == m);

    // 1. Select event type index
    switch (m) {
    case SEL_RD:
	idx = SEL_RD_IDX;
	break;
    case SEL_WR:
	idx = SEL_WR_IDX;
	break;
    case SEL_EX:
	idx = SEL_EX_IDX;
	break;
    default:
	XLOG_FATAL("Cannot add selector mask 0x%x", m);
	return false;
    }
    XLOG_ASSERT((idx >= 0) && (idx < SEL_MAX_IDX));

    // 2. Check that bits in 'mode' are not already registered
    // TODO: This is an overkill: we don't need to check the whole array,
    // but only _mask[idx]
    for (int i = 0; i < SEL_MAX_IDX; i++) {
	if (_mask[i] & m) {
	    return false;
	}
    }

    // 2. Register in the selected slot and add entry.
    // XXX: TODO: Determine if the node we're about to add is for
    // an accept event, so we know how to map it back.
    if (!_mask[idx]) {
	_mask[idx]	= m;
	_cb[idx]	= IoEventCb(cb);
	_iot[idx]	= type;
	_priority[idx]	= priority;
	return true;
    }

    assert(0);
    return false;
}

inline int
SelectorList::Node::run_hooks(SelectorMask m, XorpFd fd)
{
    int n = 0;

    /*
     * This is nasty.  We dispatch the callbacks here associated with
     * the file descriptor fd.  Unfortunately these callbacks can
     * manipulate the mask and callbacks associated with the
     * descriptor, ie the data change beneath our feet.  At no time do
     * we want to call a callback that has been removed so we can't
     * just copy the data before starting the dispatch process.  We do
     * not want to perform another callback here on a masked bit that
     * we have already done a callback on.  We therefore keep track of
     * the bits already matched with the variable already_matched.
     *
     * An alternate fix is to change the semantics of add_ioevent_cb so
     * there is one callback for each I/O event.
     *
     * Yet another alternative is to have an object, let's call it a
     * Selector that is a handle state of a file descriptor: ie an fd, a
     * mask, a callback and an enabled flag.  We would process the Selector
     * state individually.
     */
    SelectorMask already_matched = SelectorMask(0);

    for (int i = 0; i < SEL_MAX_IDX; i++) {
	assert(magic == GOOD_NODE_MAGIC);
	SelectorMask match = SelectorMask(_mask[i] & m & ~already_matched);
	if (match) {
	    assert(_cb[i].is_empty() == false);
	    _cb[i]->dispatch(fd, _iot[i]);
	    assert(magic == GOOD_NODE_MAGIC);
	    n++;
	}
	already_matched = SelectorMask(already_matched | match);
    }
    return n;
}

inline void
SelectorList::Node::clear(SelectorMask zap)
{
    for (size_t i = 0; i < SEL_MAX_IDX; i++) {
	_mask[i] &= ~zap;
	if (_mask[i] == 0) {
	    _cb[i].release();
	    _priority[i] = XorpTask::PRIORITY_INFINITY;
	}
    }
}

inline bool
SelectorList::Node::is_empty()
{
    return ((_mask[SEL_RD_IDX] == 0) && (_mask[SEL_WR_IDX] == 0) &&
	    (_mask[SEL_EX_IDX] == 0));
}

// ----------------------------------------------------------------------------
// SelectorList implementation


// NOTE:  It is possible for callbacks to add an event, that that event can
// cause the selector_entries to be resized.  See:  add_ioevent_cb
// This in turn deletes the old memory in the vector.  This this causes
// Node::run_hooks to be accessing deleted memory (ie, 'this' was deleted during
// the call to dispatch().
// Seems like a lot of pain to fix this right, so in the meantime, will pre-allocate
// logs of space in the selector_entries vector in hopes we do not have to resize.
SelectorList::SelectorList(ClockBase *clock)
    : _clock(clock), _observer(NULL), _testfds_n(0), _last_served_fd(-1),
      _last_served_sel(-1),
      // XXX: Preallocate to work around use-after-free in Node::run_hooks().
      _selector_entries(1024),
      _maxfd(0), _descriptor_count(0), _is_debug(false)
{
    x_static_assert(SEL_RD == (1 << SEL_RD_IDX) && SEL_WR == (1 << SEL_WR_IDX)
		  && SEL_EX == (1 << SEL_EX_IDX) && SEL_MAX_IDX == 3);
    for (int i = 0; i < SEL_MAX_IDX; i++)
	FD_ZERO(&_fds[i]);
}

SelectorList::~SelectorList()
{
}

bool
SelectorList::add_ioevent_cb(XorpFd		fd,
			   IoEventType		type,
			   const IoEventCb&	cb,
			   int			priority)
{
    SelectorMask mask = map_ioevent_to_selectormask(type);

    if (mask == 0) {
	XLOG_FATAL("SelectorList::add_ioevent_cb: attempt to add invalid event "
		   "type (type = %d)\n", type);
    }

    if (!fd.is_valid()) {
	XLOG_FATAL("SelectorList::add_ioevent_cb: attempt to add invalid file "
		   "descriptor (fd = %s)\n", fd.str().c_str());
    }

    if (fd >= _maxfd) {
	_maxfd = fd;
	if ((size_t)fd >= _selector_entries.size()) {
	    _selector_entries.resize(fd + 32);
	}
    }

    bool no_selectors_with_fd = _selector_entries[fd].is_empty();
    if (_selector_entries[fd].add_okay(mask, type, cb, priority) == false) {
	return false;
    }
    if (no_selectors_with_fd)
	_descriptor_count++;

    for (int i = 0; i < SEL_MAX_IDX; i++) {
	if (mask & (1 << i)) {
	    FD_SET(fd, &_fds[i]);
	    if (_observer) _observer->notify_added(fd, mask);
	}
    }

    return true;
}

void
SelectorList::remove_ioevent_cb(XorpFd fd, IoEventType type)
{
    bool found = false;

    if (fd < 0 || fd >= (int)_selector_entries.size()) {
	XLOG_ERROR("Attempting to remove fd = %d that is outside range of "
		   "file descriptors 0..%u", (int)fd,
		   XORP_UINT_CAST(_selector_entries.size()));
	return;
    }

    SelectorMask mask = map_ioevent_to_selectormask(type);

    for (int i = 0; i < SEL_MAX_IDX; i++) {
	if (mask & (1 << i) && FD_ISSET(fd, &_fds[i])) {
	    found = true;
	    FD_CLR(fd, &_fds[i]);
	    if (_observer)
		_observer->notify_removed(fd, ((SelectorMask) (1 << i)));
	}
    }
    if (! found) {
	// XXX: no event that needs to be removed has been found
	return;
    }

    _selector_entries[fd].clear(mask);
    if (_selector_entries[fd].is_empty()) {
	assert(FD_ISSET(fd, &_fds[SEL_RD_IDX]) == 0);
	assert(FD_ISSET(fd, &_fds[SEL_WR_IDX]) == 0);
	assert(FD_ISSET(fd, &_fds[SEL_EX_IDX]) == 0);
	_descriptor_count--;
    }
}

bool
SelectorList::ready()
{
    fd_set testfds[SEL_MAX_IDX];
    int n = 0;

    memcpy(testfds, _fds, sizeof(_fds));
    struct timeval tv_zero;
    tv_zero.tv_sec = 0;
    tv_zero.tv_usec = 0;

    n = ::select(_maxfd + 1,
		 &testfds[SEL_RD_IDX],
		 &testfds[SEL_WR_IDX],
		 &testfds[SEL_EX_IDX],
		 &tv_zero);

    if (n < 0) {
	switch (errno) {
	case EBADF:
	    callback_bad_descriptors();
	    break;
	case EINVAL:
	    XLOG_FATAL("Bad select argument");
	    break;
	case EINTR:
	    // The system call was interrupted by a signal, hence return
	    // immediately to the event loop without printing an error.
	    debug_msg("SelectorList::ready() interrupted by a signal\n");
	    break;
	default:
	    XLOG_ERROR("SelectorList::ready() failed: %s", strerror(errno));
	    break;
	}
	return false;
    }
    if (n == 0)
	return false;
    else
	return true;
}

int
SelectorList::do_select(struct timeval* to, bool force)
{
    if (!force && _testfds_n > 0)
        return _testfds_n;

    _maxpri_fd = _maxpri_sel = -1;

    memcpy(_testfds, _fds, sizeof(_fds));

    _testfds_n = ::select(_maxfd + 1,
		          &_testfds[SEL_RD_IDX],
		          &_testfds[SEL_WR_IDX],
		          &_testfds[SEL_EX_IDX],
		          to);

    if (!to || to->tv_sec > 0)
	    _clock->advance_time();

    if (_testfds_n < 0) {
	switch (errno) {
	case EBADF:
	    callback_bad_descriptors();
	    break;
	case EINVAL:
	    XLOG_FATAL("Bad select argument");
	    break;
	case EINTR:
	    // The system call was interrupted by a signal, hence return
	    // immediately to the event loop without printing an error.
	    debug_msg("SelectorList::ready() interrupted by a signal\n");
	    break;
	default:
	    XLOG_ERROR("SelectorList::ready() failed: %s", strerror(errno));
	    break;
	}
    }

    return _testfds_n;
}

int
SelectorList::get_ready_priority(bool force)
{
    struct timeval tv_zero;
    tv_zero.tv_sec = 0;
    tv_zero.tv_usec = 0;

    if (do_select(&tv_zero, force) <= 0)
	return XorpTask::PRIORITY_INFINITY;

    if (_maxpri_fd != -1)
	return _selector_entries[_maxpri_fd]._priority[_maxpri_sel];

    int max_priority = XorpTask::PRIORITY_INFINITY;

    //
    // Test the priority of remaining events for the last served file
    // descriptor.
    //
    bool found_one = false;
    if ((_last_served_fd >= 0) && (_last_served_fd <= _maxfd)) {
	for (int sel_idx = _last_served_sel + 1;
	     sel_idx < SEL_MAX_IDX;
	     sel_idx++) {
	    if (FD_ISSET(_last_served_fd, &_testfds[sel_idx])) {
		int p = _selector_entries[_last_served_fd]._priority[sel_idx];
		if ((p < max_priority) || (!found_one)) {
		    found_one = true;
		    max_priority = p;
		    _maxpri_fd   = _last_served_fd;
		    _maxpri_sel  = sel_idx;
		}
	    }
	}
    }

    for (int i = 0; i <= _maxfd; i++) {
	//
	// Use (_last_served_fd + 1) as a starting offset in the round-robin
	// search for the next file descriptor with the best priority.
	//
	int fd = (i + _last_served_fd + 1) % (_maxfd + 1);
	for (int sel_idx = 0; sel_idx < SEL_MAX_IDX; sel_idx++) {
	    if (FD_ISSET(fd, &_testfds[sel_idx])) {
		int p = _selector_entries[fd]._priority[sel_idx];
		if ((p < max_priority) || (!found_one)) {
		    found_one = true;
		    max_priority = p;
		    _maxpri_fd   = fd;
		    _maxpri_sel  = sel_idx;
		}
	    }
	}
    }

    XLOG_ASSERT(_maxpri_fd != -1);

    return max_priority;
}

int
SelectorList::wait_and_dispatch(TimeVal& timeout)
{
    int n = 0;

    if (timeout == TimeVal::MAXIMUM())
	n = do_select(NULL, false);
    else {
	struct timeval tv_to;
	timeout.copy_out(tv_to);

	n = do_select(&tv_to, false);
    }

    if (n <= 0)
	return 0;

    get_ready_priority(false);

    XLOG_ASSERT(_maxpri_fd != -1);

    // I hit this assert while trying to add 5000 BGP originate-routes via call_xrl:
    // /usr/local/xorp/bgp/harness/originate_routes.pl ip=10.1.1.0 nh=10.1.1.1 sigb=4 amt=5000
    // I cannot figure out how this assert could happen..unless maybe there is some re-entry issue or
    // similar.  Going to deal with things as best as possible w/out asserting.
    // TODO:  Re-write this logic entirely to be less crufty all around.
    if (!(FD_ISSET(_maxpri_fd, &_testfds[_maxpri_sel]))) {
	_testfds_n = 0;
	_maxpri_fd = -1;
	_maxpri_sel = -1;
	return 0;
    }

    FD_CLR(_maxpri_fd, &_testfds[_maxpri_sel]);

    SelectorMask sm = SEL_NONE;

    switch (_maxpri_sel) {
    case SEL_RD_IDX:
	sm = SEL_RD;
	break;

    case SEL_WR_IDX:
	sm = SEL_WR;
	break;

    case SEL_EX_IDX:
	sm = SEL_EX;
	break;

    default:
	XLOG_ASSERT(false);
    }


    XLOG_ASSERT((_maxpri_fd >= 0) && (_maxpri_fd < (int)(_selector_entries.size())));
    XLOG_ASSERT(_selector_entries[_maxpri_fd].magic == GOOD_NODE_MAGIC);

    _selector_entries[_maxpri_fd].run_hooks(sm, _maxpri_fd);

    _last_served_fd = _maxpri_fd;
    _last_served_sel = _maxpri_sel;
    _maxpri_fd = -1;
    _testfds_n--;
    XLOG_ASSERT(_testfds_n >= 0);

    return 1; // XXX what does the return value mean?
}

int
SelectorList::wait_and_dispatch(int millisecs)
{
    TimeVal t(millisecs / 1000, (millisecs % 1000) * 1000);
    return wait_and_dispatch(t);
}

void
SelectorList::get_fd_set(SelectorMask selected_mask, fd_set& fds) const
{
    if (SEL_RD == selected_mask)
	fds = _fds [SEL_RD_IDX];
    if (SEL_WR == selected_mask)
	fds = _fds [SEL_WR_IDX];
    if (SEL_EX == selected_mask)
	fds = _fds [SEL_EX_IDX];
    return;
}

int
SelectorList::get_max_fd() const
{
    return _maxfd;
}

//
// Note that this method should be called only if there are bad file
// descriptors.
//
void
SelectorList::callback_bad_descriptors()
{
    int bc = 0;	/* bad descriptor count */

    for (int fd = 0; fd <= _maxfd; fd++) {
	if (_selector_entries[fd].is_empty() == true)
	    continue;
	/*
	 * Check whether fd is valid.
	 */
	struct stat sb;
	if ((fstat(fd, &sb) < 0) && (errno == EBADF)) {
	    //
	    // Force callbacks, should force read/writes that fail and
	    // client should remove descriptor from list.
	    //
	    XLOG_ERROR("SelectorList found file descriptor %d no longer "
		       "valid.", fd);
	    _selector_entries[fd].run_hooks(SEL_ALL, fd);
	    bc++;
	}
    }
    //
    // Assert should only fail if we called this method when there were
    // no bad file descriptors, or if fstat() didn't return the appropriate
    // error.
    //
    XLOG_ASSERT(bc != 0);
}

void
SelectorList::set_observer(SelectorListObserverBase& obs)
{
    _observer = &obs;
    _observer->_observed = this;
    return;
}

void
SelectorList::remove_observer()
{
    if (_observer) _observer->_observed = NULL;
    _observer = NULL;
    return;
}

SelectorListObserverBase::~SelectorListObserverBase()
{
    if (_observed) _observed->remove_observer();
}

#endif // !HOST_OS_WINDOWS
