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

// $XORP: xorp/libxorp/win_dispatcher.hh,v 1.23 2008/10/02 21:57:36 bms Exp $

#ifndef __LIBXORP_WIN_DISPATCHER_HH__
#define __LIBXORP_WIN_DISPATCHER_HH__

#include "libxorp/xorp.h"
#include "libxorp/xorpfd.hh"
#include "libxorp/timeval.hh"
#include "libxorp/ioevents.hh"
#include "libxorp/task.hh"

#include <vector>
#include <map>


#ifdef HOST_OS_WINDOWS

class ClockBase;

/**
 * A tuple specifying a Windows object and I/O event type.
 */
struct IoEventTuple {
private:
    XorpFd	_fd;
    IoEventType	_type;
public:
    IoEventTuple(XorpFd fd, IoEventType type) : _fd(fd), _type(type) {}

    XorpFd fd()		{ return (_fd); };
    IoEventType type()	{ return (_type); };

    bool operator ==(const IoEventTuple& rhand) const {
	return ((_fd == rhand._fd) && (_type == rhand._type));
    }

    bool operator !=(const IoEventTuple& rhand) const {
	return (!((_fd == rhand._fd) && (_type == rhand._type)));
    }

    bool operator >(const IoEventTuple& rhand) const {
	if (_fd != rhand._fd)
	    return ((int)_type > (int)rhand._type);
	else
	    return (_fd > rhand._fd);
    }

    bool operator <(const IoEventTuple& rhand) const {
	if (_fd != rhand._fd)
	    return (_fd < rhand._fd);
	else
	    return ((int)_type < (int)rhand._type);
    }
};

/**
 * A map of Windows object handle and I/O event type to an I/O callback for
 * each member.
 */
typedef map<IoEventTuple, IoEventCb>	IoEventMap;

/**
 * A map of Windows event handle to socket handle, used for faster
 * dispatch of socket events on return from WFMO().
 * Note that XorpFd is used as the value type.
 */
typedef map<HANDLE, XorpFd>		EventSocketMap;

/**
 * A value-type tuple of a Windows event handle and a WSA event mask.
 */
typedef std::pair<HANDLE, long>		MaskEventPair;

/**
 * A map of sockets to a MaskEventPair tuple.
 */
typedef map<SOCKET, MaskEventPair>	SocketEventMap;

/**
 * Mask of Winsock events for which the I/O event framework is able
 * to dispatch callbacks.
 */
#define	WSAEVENT_SUPPORTED	\
	(FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE)

/**
 * @short A class to provide an interface to Windows native I/O multiplexing.
 *
 * A WinDispatcher provides an entity where callbacks for pending I/O
 * operations on Windows sockets and other Windows system objects may
 * be registered. The callbacks are invoked when the @ref wait_and_dispatch
 * method is called, and I/O is pending on a descriptor, or a Windows
 * object's state has been set to signalled.
 *
 * The entire event loop is driven from the Win32 API call
 * WaitForMultipleObjects(), which means there is a hard 64-object limit.
 *
 * This class is the glue which binds XORP to Windows. There are tricks
 * in the AsyncFileOperator classes which are tightly coupled with what
 * happens here. Most of these tricks exist because systems-level
 * asynchronous I/O facilities cannot easily be integrated into XORP's
 * current design.
 *
 * There is no way of telling specific I/O events apart on objects other
 * than sockets without actually trying to service the I/O. The class
 * uses WSAEventSelect() internally to map socket handles to event
 * handles, and much of the complexity in this class is there to deal
 * with this schism in Windows low-level I/O.
 *
 * The class emulates the ability of UNIX-like systems to interrupt a
 * process waiting in select() for pending I/O on pipes, by using a
 * time-slice quantum to periodically poll the pipes. This quantum is
 * currently hard-coded to 250ms.
 *
 * Sockets are optimized to be the faster dispatch path, as this is where
 * the bulk of XORP I/O processing happens.
 *
 * WinDispatcher should only be exposed to @ref EventLoop.
 */
class WinDispatcher {
public:
    /**
     * Default constructor.
     */
    WinDispatcher(ClockBase *clock);

    /**
     * Destructor.
     */
    virtual ~WinDispatcher();

    /**
     * Add a hook for pending I/O operations on a callback.
     *
     * Only one callback may be registered for each possible IoEventType,
     * with the following exceptions.
     *
     * If the @ref XorpFd maps to a Windows socket handle, multiple
     * callbacks may be registered for different IoEventTypes, but one
     * and only one callback may be registered for the handle if a
     * callback is registered for the IOT_ACCEPT event.
     *
     * If the @ref XorpFd maps to a Windows pipe or console handle,
     * callbacks may only be registered for the IOT_READ and IOT_DISCONNECT
     * events.
     *
     * If the @ref XorpFd corresponds to any other kind of Windows object
     * handle, only a single callback may be registered, and the IoEventType
     * must be IOT_EXCEPTION. This mechanism is typically used to direct
     * the class to wait on Windows event or process handles.
     * 
     * @param fd a Windows object handle encapsulated in a @ref XorpFd .
     * @param type the @ref IoEventType which should trigger the callback.
     * @param cb callback object which shall be invoked when the event is
     * triggered.
     * @param priority the XorpTask priority at which this callback should
     *        be run.
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
     * @param type the @ref IoEventType to remove the callback for; the
     * special value IOT_ANY may be specified to remove all such callbacks.
     * @return true if function succeeds, false otherwise.
     */
    bool remove_ioevent_cb(XorpFd fd, IoEventType type);

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
    int get_ready_priority();

    /**
     * Wait for a pending I/O events and invoke callbacks when they
     * become ready.
     *
     * @param timeout the maximum period to wait for.
     */
    void wait_and_dispatch(TimeVal& timeout) {
	if (timeout == TimeVal::MAXIMUM())
	    wait_and_dispatch(INFINITE);
	else
	    wait_and_dispatch(timeout.to_ms());
    }

    /**
     * Wait for a pending I/O events and invoke callbacks when they
     * become ready.
     *
     * @param millisecs the maximum period in milliseconds to wait for.
     */
    void wait_and_dispatch(int ms);

    /**
     * Get the count of the descriptors that have been added.
     *
     * @return the count of the descriptors that have been added.
     */
    size_t descriptor_count() const { return _descriptor_count; }

protected:
    /**
     * No user-servicable parts here at the moment.
     */
    void dispatch_sockevent(HANDLE hevent, XorpFd fd);

private:

    /** Returns true if more work to be done. */
    bool do_wait_and_dispatch(vector<HANDLE>& handles, int ms, bool first);

    bool add_socket_cb(XorpFd& fd, IoEventType type, const IoEventCb& cb,
		       int priority);
    bool add_handle_cb(XorpFd& fd, IoEventType type, const IoEventCb& cb,
		       int priority);
    void callback_bad_handle();
    void callback_bad_socket(XorpFd& fd);
    bool remove_socket_cb(XorpFd& fd, IoEventType type);
    bool remove_handle_cb(XorpFd& fd, IoEventType type);
    void dispatch_pipe_reads();

private:
    static const int POLLED_INTERVAL_MS = 250;

private:
    ClockBase*		_clock;		    // System clock

    IoEventMap		_callback_map;	    // XorpFd + IoEventType -> Callback
    SocketEventMap	_socket_event_map;  // Socket -> Event + Mask
    EventSocketMap	_event_socket_map;  // Event -> Socket
    vector<HANDLE>	_handles;	    // All Win32 handles pending wait
    vector<HANDLE>	_polled_pipes;	    // Pipe handles pending poll
    size_t		_descriptor_count;  // Count for socket/event handlers
};

#endif // HOST_OS_WINDOWS

#endif // __LIBXORP_WIN_DISPATCHER_HH__
