// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2006 International Computer Science Institute
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

// $XORP: xorp/libxorp/win_dispatcher.hh,v 1.11 2006/08/11 00:57:43 pavlin Exp $

#ifndef __LIBXORP_WIN_DISPATCHER_HH__
#define __LIBXORP_WIN_DISPATCHER_HH__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libxorp/xorp.h"
#include "libxorp/xorpfd.hh"
#include "libxorp/timeval.hh"
#include "libxorp/ioevents.hh"
#include "libxorp/priority.hh"

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

    inline XorpFd fd()		{ return (_fd); };
    inline IoEventType type()	{ return (_type); };

    inline bool operator ==(const IoEventTuple& rhand) const {
	return ((_fd == rhand._fd) && (_type == rhand._type));
    }

    inline bool operator !=(const IoEventTuple& rhand) const {
	return (!((_fd == rhand._fd) && (_type == rhand._type)));
    }

    inline bool operator >(const IoEventTuple& rhand) const {
	if (_fd != rhand._fd)
	    return ((int)_type > (int)rhand._type);
	else
	    return (_fd > rhand._fd);
    }

    inline bool operator <(const IoEventTuple& rhand) const {
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
     * Only one callback may be registered for each possible IoEventType.
     *
     * If the @ref XorpFd corresponds to a Windows socket handle, multiple
     * callbacks may be registered for different IoEventTypes, but one
     * and only one callback may be registered for the handle if a
     * callback is registered for the IOT_ACCEPT event.
     *
     * If the @ref XorpFd corresponds to any other kind of Windows object
     * handle, only a single callback may be registered, and the IoEventType
     * must be IOT_READ. This is because Windows object handles can have
     * a signalled or non-signalled state, and there is no way of telling
     * specific I/O events apart without actually trying to service the I/O,
     * which is beyond the scope of this class's responsibilities.
     *
     * @param fd a Windows object handle encapsulated in a @ref XorpFd .
     * @param type the @ref IoEventType which should trigger the callback.
     * @param cb callback object which shall be invoked when the event is
     * triggered.
     * @return true if function succeeds, false otherwise.
     */
    bool add_ioevent_cb(XorpFd fd, 
			IoEventType type, 
			const IoEventCb& cb,
			int priority = XorpTask::PRIORITY_DEFAULT);

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
     * Find out if any of the selectors are ready
     *
     * @return true if any selector is ready
     */
    bool ready();

    /**
     * Find out the highest priority from the ready file descriptors
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
    inline void wait_and_dispatch(TimeVal* timeout) {
	if (timeout == NULL || *timeout == TimeVal::MAXIMUM())
	    wait_and_dispatch(INFINITE);
	else
	    wait_and_dispatch(timeout->to_ms());
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
    void dispatch_sockevent(HANDLE hevent, XorpFd fd);

private:
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
