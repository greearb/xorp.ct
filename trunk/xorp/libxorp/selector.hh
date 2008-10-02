// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/libxorp/selector.hh,v 1.33 2008/09/23 19:57:39 abittau Exp $

#ifndef __LIBXORP_SELECTOR_HH__
#define __LIBXORP_SELECTOR_HH__

#ifdef HOST_OS_WINDOWS
#error "This file is not intended to be included on Windows."
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <vector>

#include "callback.hh"
#include "ioevents.hh"
#include "task.hh"

class ClockBase;
class SelectorList;
class TimeVal;

/**
 * Selector event type masks.
 */
enum SelectorMask {
    SEL_NONE	= 0x0,				// No events
    SEL_RD	= 0x01,				// Read events
    SEL_WR	= 0x02,				// Write events
    SEL_EX	= 0x04,				// Exception events
    SEL_ALL	= SEL_RD | SEL_WR | SEL_EX	// All events
};

class SelectorTag;

typedef ref_ptr<SelectorTag> Selector;

/**
 * @short Abstract class used to receive SelectorList notifications
 *
 * A SelectorListObserverBase abstract class can be subtyped to create classes
 * that will receive @ref SelectorList events.
 * All methods in this class are private, since they must only be invoked by
 * the friend class SelectorList
 */
class SelectorListObserverBase {
public:
    virtual ~SelectorListObserverBase();

private:
    /**
     * This function will get called when a new file descriptor is
     * added to the SelectorList.
     */
    virtual void notify_added(XorpFd fd, const SelectorMask& mask) = 0;

    /**
     * This function will get called when a new file descriptor is
     * removed from the SelectorList.
     */
    virtual void notify_removed(XorpFd fd, const SelectorMask& mask) = 0;

    SelectorList * _observed;

    friend class SelectorList;
};

/**
 * @short A class to provide an interface to I/O multiplexing.
 *
 * A SelectorList provides an entity where callbacks for pending I/O
 * operations on file descriptors may be registered.  The callbacks
 * are invoked when one of the @ref wait_and_dispatch methods is called
 * and I/O is pending on the particular descriptors.
 */
class SelectorList {
public:

    /**
     * Default constructor.
     */
    SelectorList(ClockBase* clock);

    /**
     * Destructor.
     */
    virtual ~SelectorList();

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
     bool add_ioevent_cb(XorpFd		fd, 
			 IoEventType	type, 
			 const IoEventCb& cb,
			 int		priority = XorpTask::PRIORITY_DEFAULT);

    /**
     * Remove hooks for pending I/O operations.
     *
     * @param fd the file descriptor.
     *
     * @param mask of event types to be removed, e.g. an OR'ed
     * combination of the available @ref SelectorMask values.
     */
    void remove_ioevent_cb(XorpFd fd, IoEventType type = IOT_ANY);

    /**
     * Find out if any of the selectors are ready.
     *
     * @return true if any selector is ready.
     */
    bool ready();

    /**
     * Find out the highest priority from the ready file descriptors.
     *
     * @return the priority of the highest priority ready file descriptor.
     */
    int get_ready_priority(bool force);

    /**
     * Wait for a pending I/O events and invoke callbacks when they
     * become ready.
     *
     * @param timeout the maximum period to wait for.
     *
     * @return the number of callbacks that were made.
     */
    int	wait_and_dispatch(TimeVal& timeout);

    /**
     * Wait for a pending I/O events and invoke callbacks when they
     * become ready.
     *
     * @param millisecs the maximum period in milliseconds to wait for.
     *
     * @return the number of callbacks that were made.
     */
    int	wait_and_dispatch(int millisecs);

    /**
     * Get the count of the descriptors that have been added.
     *
     * @return the count of the descriptors that have been added.
     */
    size_t descriptor_count() const { return _descriptor_count; }

    /**
     * Get a copy of the current list of monitored file descriptors in
     * Unix fd_set format
     *
     * @param the selected mask as @ref SelectorMask (SEL_RD, SEL_WR, or SEL_EX)
     *
     * @return the selected fd_set.
     */
     void get_fd_set(SelectorMask selected_mask, fd_set& fds) const;

     /**
      * Get a the value of the largest monitored file descriptor
      *
      * @return the maximum fd.
      */
     int get_max_fd() const;

     /**
     * Set the SelectorObserver object that will receive notifications
     *
     * @return void
     */
     void set_observer(SelectorListObserverBase& obs);

     /**
     * Remove the SelectorObserver object that receives notifications
     *
     * @return void
     */
     void remove_observer();


protected:
    void callback_bad_descriptors();

private:
    int do_select(struct timeval* to, bool force);

    SelectorList(const SelectorList&);			// not implemented
    SelectorList& operator=(const SelectorList&);	// not implemented

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
	int		_mask[SEL_MAX_IDX];
	IoEventCb	_cb[SEL_MAX_IDX];
	// Reverse mapping of legacy UNIX event to IoEvent
	IoEventType	_iot[SEL_MAX_IDX];
	int		_priority[SEL_MAX_IDX];

	Node();
	bool		add_okay(SelectorMask m, IoEventType type,
				 const IoEventCb& cb, int priority);
	int		run_hooks(SelectorMask m, XorpFd fd);
	void		clear(SelectorMask m);
	bool		is_empty();
    };

    ClockBase*		_clock;
    SelectorListObserverBase * _observer;
    fd_set		_fds[SEL_MAX_IDX];
    fd_set		_testfds[SEL_MAX_IDX];
    int			_testfds_n;
    int			_maxpri_fd;
    int			_maxpri_sel;

    vector<Node>	_selector_entries;
    int			_maxfd;
    size_t		_descriptor_count;
};

#endif // __LIBXORP_SELECTOR_HH__
