// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/selector.cc,v 1.16 2003/06/11 19:15:19 jcardona Exp $"

#include "libxorp_module.h"
#include "xorp.h"
#include "debug.h"
#include "selector.hh"
#include "timeval.hh"
#include "utility.h"
#include "xlog.h"

// ----------------------------------------------------------------------------
// SelectorList::Node methods

inline
SelectorList::Node::Node()
{
    _mask[SEL_RD_IDX] = _mask[SEL_WR_IDX] = _mask[SEL_EX_IDX] = 0;
}

inline bool
SelectorList::Node::add_okay(SelectorMask m, const SelectorCallback& scb)
{
    int i;

    // always OK to try to register for nothing
    if (!m)
	return true;

    // Sanity Checks

    // 0. We understand all bits in 'mode'
    assert((m & (SEL_RD | SEL_WR | SEL_EX)) == m);

    // 1. Check that bits in 'mode' are not already registered
    for (i = 0; i < SEL_MAX_IDX; i++) {
	if (_mask[i] & m) {
	    return false;
	}
    }

    // 2. If not already registered, find empty slot and add entry.
    for (i = 0; i < SEL_MAX_IDX; i++) {
	if (!_mask[i]) {
	    _mask[i]	= m;
	    _cb[i]	= SelectorCallback(scb);
	    return true;
	}
    }

    assert(0);
    return false;
}

inline int
SelectorList::Node::run_hooks(SelectorMask m, int fd)
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
     * An alternate fix is to change the semantics of add_selector so
     * there is one callback for each i/o event.
     *
     * Yet another alternative is to have an object, let's call it a
     * Selector that is a handle state of a file descriptor: ie an fd, a
     * mask, a callback and an enabled flag.  We would process the Selector
     * state individually.
     */
    SelectorMask already_matched = SelectorMask(0);

    for (int i = 0; i < SEL_MAX_IDX; i++) {
	SelectorMask match = SelectorMask(_mask[i] & m & ~already_matched);
	if (match) {
	    assert(_cb[i].is_empty() == false);
	    _cb[i]->dispatch(fd, match);
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
	if (_mask[i] == 0)
	    _cb[i].release();
    }
}

inline bool
SelectorList::Node::is_empty()
{
    return (_mask[SEL_RD_IDX] == _mask[SEL_WR_IDX] == _mask[SEL_EX_IDX] == 0);
}

// ----------------------------------------------------------------------------
// SelectorList implementation

SelectorList::SelectorList()
    : _maxfd(0), _descriptor_count(0), _observer(NULL)
{
    static_assert(SEL_RD == (1 << SEL_RD_IDX) && SEL_WR == (1 << SEL_WR_IDX)
		  && SEL_EX == (1 << SEL_EX_IDX) && SEL_MAX_IDX == 3);
    for (int i = 0; i < SEL_MAX_IDX; i++)
	FD_ZERO(&_fds[i]);
}

bool
SelectorList::add_selector(int			   fd,
			   SelectorMask		   mask,
			   const SelectorCallback& scb)
{
    if (fd < 0) {
	// Probably want to make this XLOG_FATAL should this ever occur
	XLOG_ERROR("SelectorList::add_selector: attempt to add invalid file "
		   "descriptor (fd = %d)\n", fd);
	return -1;
    }

    bool resize = false;
    if (fd >= _maxfd) {
	_maxfd = fd;
	if ((size_t)fd >= _selector_entries.size()) {
	    _selector_entries.resize(fd + 32);
	    resize = true;
	}
    }

    bool no_selectors_with_fd = _selector_entries[fd].is_empty();
    if (_selector_entries[fd].add_okay(mask, scb) == false) {
	return false;
    }
    if (no_selectors_with_fd) _descriptor_count++;

    for (int i = 0; i < SEL_MAX_IDX; i++) {
	if (mask & (1 << i)) {
	    FD_SET(fd, &_fds[i]);
	    if (_observer) _observer->notify_added(fd, mask);
	}
    }

    return true;
}

void
SelectorList::remove_selector(int fd, SelectorMask mask)
{
    if (fd < 0 || fd >= (int)_selector_entries.size()) {
	XLOG_ERROR("Attempting to remove fd = %d that is outside range of file descriptors 0..%u", fd,  (uint32_t)_selector_entries.size());
	return;
    }

for (int i = 0; i < SEL_MAX_IDX; i++) {
	if (mask & (1 << i) && FD_ISSET(fd, &_fds[i])) {
	    FD_CLR(fd, &_fds[i]);
	    if (_observer) 
		_observer->notify_removed(fd, ((SelectorMask) (1 << i)));
	}
    }
    _selector_entries[fd].clear(mask);
    if (_selector_entries[fd].is_empty()) {
	_descriptor_count --;
    }
}

int
SelectorList::select(TimeVal* timeout)
{
    fd_set testfds[SEL_MAX_IDX];
    int n = 0;

 select_again:
    memcpy(testfds, _fds, sizeof(_fds));

    if (timeout == 0 || *timeout == TimeVal::MAXIMUM()) {
	n = ::select(_maxfd + 1,
		     &testfds[SEL_RD_IDX],
		     &testfds[SEL_WR_IDX],
		     &testfds[SEL_EX_IDX],
		     0);
    } else {
	struct timeval tv_to;
	timeout->copy_out(tv_to);
	n = ::select(_maxfd + 1,
		     &testfds[SEL_RD_IDX],
		     &testfds[SEL_WR_IDX],
		     &testfds[SEL_EX_IDX],
		     &tv_to);
    }

    if (n < 0) {
	if (errno == EINTR) {
	    // The system call was interrupted by a signal, hence restart it.
	    goto select_again;
	}
	switch (errno) {
	case EBADF:
	    callback_bad_descriptors();
	    break;
	case EINVAL:
	    XLOG_FATAL("Bad select argument (probably timeval)");
	    break;
	default:
	    XLOG_ERROR("SelectorList::select failed: %s", strerror(errno));
	    break;
	}
	return 0;
    }

    // Build list of all possible file descriptors
    static vector<int> fdtable(0);

    while (fdtable.size() < (size_t)_maxfd + 1)
	fdtable.push_back(fdtable.size());

    size_t sz = fdtable.size();
    for (int j = 0; sz != 0 && j != n; ) {
	// Pick a random file descriptor to check.  Store it and swap value
	// with old position in vector with last element in vector.  The
	// latter operation keeps the value in the vector but removes it
	// from consideration for this round.
	int x = random() % sz;
	int fd = fdtable[x];
	fdtable[x] = fdtable[--sz];
	fdtable[sz] = fd;

	int mask = 0;
	if (FD_ISSET(fd, &testfds[SEL_RD_IDX])) {
	    mask |= SEL_RD;
	    FD_CLR(fd, &testfds[SEL_RD_IDX]);	// paranoia
	}
	if (FD_ISSET(fd, &testfds[SEL_WR_IDX])) {
	    mask |= SEL_WR;
	    FD_CLR(fd, &testfds[SEL_WR_IDX]);	// paranoia
	}
	if (FD_ISSET(fd, &testfds[SEL_EX_IDX])) {
	    mask |= SEL_EX;
	    FD_CLR(fd, &testfds[SEL_EX_IDX]);	// paranoia
	}
	if (mask) {
	    _selector_entries[fd].run_hooks(SelectorMask(mask), fd);
	    j++;
	}
    }

    for (int i = 0; i <= _maxfd; i++) {
	assert(!FD_ISSET(i, &testfds[SEL_RD_IDX]));	// paranoia
	assert(!FD_ISSET(i, &testfds[SEL_WR_IDX]));	// paranoia
	assert(!FD_ISSET(i, &testfds[SEL_EX_IDX]));	// paranoia
    }
    return n;
}

int
SelectorList::select(int millisecs)
{
    TimeVal t(millisecs / 1000, (millisecs % 1000) * 1000);
    return select(&t);
}

void
SelectorList::get_fd_set(SelectorMask selected_mask, fd_set& fds) const
{
    if ((SEL_RD != selected_mask) && (SEL_WR != selected_mask) &&
	(SEL_EX != selected_mask)) return;
    if (SEL_RD == selected_mask) fds = _fds [SEL_RD_IDX];
    if (SEL_WR == selected_mask) fds = _fds [SEL_WR_IDX];
    if (SEL_EX == selected_mask) fds = _fds [SEL_EX_IDX];
    return;
}

int
SelectorList::get_max_fd() const
{
    return _maxfd;
}

void
SelectorList::callback_bad_descriptors()
{
    int bc = 0;	/* bad descriptor count */
    for(int fd = 0; fd <= _maxfd; fd++) {
	if (_selector_entries[fd].is_empty() == true)
	    continue;
	/* Check fd is valid.  NB we call fstat without a stat struct to
	 * avoid copy.  fstat will always fail as a result.
	 */
	fstat(fd, 0);
	if (errno == EBADF) {
	    /* Force callbacks, should force read/writes that fail and
	     * client should remove descriptor from list. */
	    XLOG_ERROR("SelectorList found file descriptor %d no longer "
		       "valid.", fd);
	    _selector_entries[fd].run_hooks(SEL_ALL, fd);
	    bc++;
	}
    }
    /* Assert should only fail if OS checks stat struct before fd (??) */
    assert(bc != 0);
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

