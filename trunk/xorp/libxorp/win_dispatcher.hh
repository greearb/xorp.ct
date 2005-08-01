// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

// $XORP$

#ifndef __LIBXORP_WIN_DISPATCHER_HH__
#define __LIBXORP_WIN_DISPATCHER_HH__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libxorp/xorp.h"
#include "libxorp/xorpfd.hh"
#include "libxorp/timeval.hh"
#include "libxorp/ioevents.hh"

#include <vector>
#include <map>

#ifdef HOST_OS_WINDOWS

class ClockBase;

/**
 * A map of sockets to the mask of events for which notifications have
 * been requested for each member.
 */
typedef map<SOCKET, long>		SocketWSAEventMap;

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
#endif

#ifdef HOST_OS_WINDOWS
/**
 * Pick a safe range for our private messages which 1) doesn't collide
 * with anything else and 2) may be used with the GetMessage() API's
 * message filtering capabilities.
 */
#define	XM_USER_BASE	(WM_USER + 16384)
#define	XM_SOCKET	(XM_USER_BASE + 1)

/**
 * Mask of Winsock events for which the I/O event framework is able
 * to dispatch callbacks.
 */
#define	WSAEVENT_SUPPORTED	\
	(FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE)

#endif // HOST_OS_WINDOWS

/**
 * @short A class to provide an interface to Windows I/O multiplexing.
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
#ifdef HOST_OS_WINDOWS
public:
    /**
     * Default constructor.
     */
    WinDispatcher(ClockBase *clock);

    /**
     * Destructor.
     */
    virtual ~WinDispatcher();

    /*
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
    bool add_ioevent_cb(XorpFd fd, IoEventType type, const IoEventCb& cb);

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

protected:
    /**
     * Window procedure for dispatching Windows messages.
     * Currently this is a member of the class itself. Should it become
     * necessary to use TranslateMessage()/DispatchMessage() in future,
     * you will need to invoke this through a C callback and pass the
     * instance pointer to Windows using SetWindowLongEx(). You will
     * also need to fix the calling convention and prototype.
     */
    int window_procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    bool add_socket_cb(XorpFd& fd, IoEventType type, const IoEventCb& cb);
    bool add_handle_cb(XorpFd& fd, IoEventType type, const IoEventCb& cb);
    bool remove_socket_cb(XorpFd& fd, IoEventType type);
    bool remove_handle_cb(XorpFd& fd, IoEventType type);
    void dispatch_socket_writes();
    void dispatch_pipe_reads();

private:
    static const char *WNDCLASS_STATIC;
    static const char *WNDNAME_XORP;

    // NT forces us to poll for certain kinds of events when we're using
    // a single process, because the programming model is multithreaded.
    static const int POLLED_INTERVAL_MS = 250;

private:
    ClockBase*		_clock;
    IoEventMap		_ioevent_map;

    // Window handle and message map for WSAAsyncSelect() dispatch.
    HWND		_hwnd;
    SocketWSAEventMap	_socket_wsaevent_map;

    // Win32 object handles which are set to the 'signalled' state on events.
    vector<HANDLE>	_handles;

    // Sockets requiring select() polling for writability events.
    vector<SOCKET>	_writesockets4;
    vector<SOCKET>	_writesockets6;
    fd_set		_writefdset4;
    fd_set		_writefdset6;

    // Pipe handles requiring PeekNamedPipe() polling for readability.
    vector<HANDLE>	_polled_pipes;
#endif
};

#endif // __LIBXORP_WIN_DISPATCHER_HH__
