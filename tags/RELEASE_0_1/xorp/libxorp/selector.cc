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

#ident "$XORP: xorp/libxorp/selector.cc,v 1.24 2002/12/09 18:29:13 hodson Exp $"

#include "libxorp_module.h"
#include "xorp.h"
#include "debug.h"
#include "selector.hh"
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
    for (int i = 0; i < SEL_MAX_IDX; i++) {
	SelectorMask match = SelectorMask(_mask[i] & m);
	if (match) {
	    assert(_cb[i].is_empty() == false);
	    _cb[i]->dispatch(fd, match);
	    n++;
	}
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
    : _maxfd(0), _descriptor_count(0)
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
    debug_msg("fd = %d\n", fd);

    if (fd < 0) {
	return -1; 
    }

    if (fd >= _maxfd) {
	_maxfd = fd;
	if ((size_t)fd >= _selector_entries.capacity()) {
	    _selector_entries.resize(fd + 32);
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
	}
    }

    return true;
}

void
SelectorList::remove_selector(int fd, SelectorMask mask) 
{
    if (fd < 0 || fd >= (int)_selector_entries.size()) {
	XLOG_ERROR("Attempting to remove fd = %d that is outside range of file descriptors 0..%d", fd,  _selector_entries.size());
	return;
    }

    for (int i = 0; i < SEL_MAX_IDX; i++) {
	if (mask & (1 << i)) {
	    FD_CLR(fd, &_fds[i]);
	}
    }
    _selector_entries[fd].clear(mask);
    if (_selector_entries[fd].is_empty())
	_descriptor_count --;

}

int
SelectorList::select(timeval* timeout)
{
    fd_set testfds[SEL_MAX_IDX];
    memcpy(testfds, _fds, sizeof(_fds));
    
    int n = ::select(_maxfd + 1,
		     &testfds[SEL_RD_IDX],
		     &testfds[SEL_WR_IDX],
		     &testfds[SEL_EX_IDX],
		     timeout);
    if (n < 0) {
	if (errno == EBADF) {
	    callback_bad_descriptors();
	} else {
	    XLOG_ERROR("SelectorList::select failed: %s", strerror(errno));
	}
	return 0;
    }

    int i, j;
    for (i = 0, j = 0; i <= _maxfd; i++) {
	int mask = 0;
	if (FD_ISSET(i, &testfds[SEL_RD_IDX])) {
	    mask |= SEL_RD;
	    FD_CLR(i, &testfds[SEL_RD_IDX]);	// paranoia
	    j++;
	}
	if (FD_ISSET(i, &testfds[SEL_WR_IDX])) {
	    mask |= SEL_WR;
	    FD_CLR(i, &testfds[SEL_WR_IDX]);	// paranoia
	    j++;
	}
	if (FD_ISSET(i, &testfds[SEL_EX_IDX])) {
	    mask |= SEL_EX;
	    FD_CLR(i, &testfds[SEL_EX_IDX]);	// paranoia
	    j++;
	}
	if (mask) {
	    _selector_entries[i].run_hooks(SelectorMask(mask), i);
	    if (j == n) 
		break;	/* All fd's with data done */
	}
    }
    assert(j == n);
    for (int i = 0; i < _maxfd; i++) {
	assert(!FD_ISSET(i, &testfds[SEL_RD_IDX]));	// paranoia
	assert(!FD_ISSET(i, &testfds[SEL_WR_IDX]));	// paranoia
	assert(!FD_ISSET(i, &testfds[SEL_EX_IDX]));	// paranoia
    }
    return n;
}

int
SelectorList::select(int millisecs)
{
        timeval t;
        t.tv_sec  = millisecs / 1000;
        t.tv_usec = (millisecs - t.tv_sec * 1000) * 1000;
        return select(&t);
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
