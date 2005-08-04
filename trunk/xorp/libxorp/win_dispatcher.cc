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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HOST_OS_WINDOWS	    // This entire file is for Windows only.

#include "libxorp/xorp.h"

#include "libxorp/libxorp_module.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxorp/xorpfd.hh"
#include "libxorp/timeval.hh"
#include "libxorp/clock.hh"
#include "libxorp/callback.hh"
#include "libxorp/ioevents.hh"
#include "libxorp/eventloop.hh"

#include "libxorp/win_dispatcher.hh"

#include "libxorp/win_io.h"

// Some definitions may be missing from MinGW. Provide them here.
#ifdef __MINGW32__
#define	HWND_MESSAGE		((HWND)(-3))
#define	QS_ALLPOSTMESSAGE	256
#define	MWMO_ALERTABLE		2
#endif

const char *WinDispatcher::WNDCLASS_STATIC = "STATIC";
const char *WinDispatcher::WNDNAME_XORP = "XorpSocketWindow";

#ifndef XORP_FD_CAST
#define	XORP_FD_CAST(x)	((u_int)(x))
#endif

inline const char *
_get_event_string(long event)
{
    switch (event) {
    case FD_READ:
	return ("FD_READ");
    case FD_WRITE:
	return ("FD_WRITE");
    case FD_OOB:
	return ("FD_OOB");
    case FD_ACCEPT:
	return ("FD_ACCEPT");
    case FD_CONNECT:
	return ("FD_CONNECT");
    case FD_CLOSE:
	return ("FD_CLOSE");
    default:
	return ("unknown");
    }
}

inline int
_get_sock_family(XorpFd fd)
{
    WSAPROTOCOL_INFO wspinfo;
    int err, len;

    len = sizeof(wspinfo);
    err = getsockopt(fd, SOL_SOCKET, SO_PROTOCOL_INFO,
                         XORP_SOCKOPT_CAST(&wspinfo), &len);
    if (err == 0)
       return (wspinfo.iAddressFamily);

    return -1;
}

inline int
_map_wsaevent_to_ioevent(const long wsaevent)
{
    switch (wsaevent) {
    case FD_READ:
	return IOT_READ;
    //case FD_WRITE:
//	return IOT_WRITE;
    case FD_OOB:
	return IOT_EXCEPTION;
    case FD_ACCEPT:
	return IOT_ACCEPT;
    case FD_CONNECT:
	return IOT_CONNECT;
    case FD_CLOSE:
	return IOT_DISCONNECT;
    default:
	return -1;
    }
}

inline long
_map_ioevent_to_wsaevent(const IoEventType ioevent)
{
    switch (ioevent) {
    case IOT_READ:
	return FD_READ;
    //case IOT_WRITE:
//	return FD_WRITE;
    case IOT_EXCEPTION:
	return FD_OOB;
    case IOT_ACCEPT:
	return FD_ACCEPT;
    case IOT_CONNECT:
	return FD_CONNECT;
    case IOT_DISCONNECT:
	return FD_CLOSE;
    default:
	return 0;
    }
}

WinDispatcher::WinDispatcher(ClockBase* clock)
  : _clock(clock)
{
    // Avoid introducing a circular dependency upon libcomm by calling
    // WSAStartup() and WSACleanup() ourselves. It's safe for the same
    // thread within the same process to call it more than once.
    {
	WSADATA wsadata;
	WORD version = MAKEWORD(2, 2);
	int retval = WSAStartup(version, &wsadata);
	if (retval != 0) {
	    XLOG_FATAL("WSAStartup() error: %lu", GetLastError());
	}
    }

    _handles.reserve(MAXIMUM_WAIT_OBJECTS);

    FD_ZERO(&_writefdset4);
    FD_ZERO(&_writefdset6);

    _hwnd = ::CreateWindowExA(0, WNDCLASS_STATIC, WNDNAME_XORP,
			      0, 0, 0, 0, 0, HWND_MESSAGE, 0,
			      GetModuleHandle(NULL), 0);
    if (_hwnd == NULL) {
	XLOG_FATAL("CreateWindowExA error: %lu", GetLastError());
    }
}

WinDispatcher::~WinDispatcher()
{
    // Purge all pending socket event notifications from Winsock.
    for (SocketWSAEventMap::iterator ii = _socket_wsaevent_map.begin();
	 ii != _socket_wsaevent_map.end(); ++ii) {
	(void)::WSAEventSelect(ii->first, NULL, 0);
    }
    (void)::DestroyWindow(_hwnd);
    (void)::WSACleanup();
}

bool
WinDispatcher::add_socket_cb(XorpFd& fd, IoEventType type, const IoEventCb& cb)
{
	// XXX: Currently we only support 1 callback per socket/event tuple.
	// Check that the socket does not already have a callback
	// registered for it.
	IoEventMap::iterator ii = _ioevent_map.find(IoEventTuple(fd, type));
	if (ii != _ioevent_map.end()) {
	    debug_msg("callback already registered.\n");
	    return false;
	}

	// Writability events are a special case. Whilst WSAAsyncSelect()
	// supports an FD_WRITE event, it's edge triggered, not level
	// triggered; you only get it when the socket *transitions* into
	// an unblocked state, not whilst the socket is *in* such a state.
	// Because we cheat and use select() with a zero value timeval upon
	// return from MsgWaitForMultipleObjectsEx(), we have to make sure
	// the socket address family is a supported one.
	if (type == IOT_WRITE) {
	    int family = _get_sock_family(fd);

	    if (family == AF_INET) {
		_writesockets4.push_back(fd);
		FD_SET(XORP_FD_CAST(fd), &_writefdset4);
	    } else if (family == AF_INET6) {
		_writesockets6.push_back(fd);
		FD_SET(XORP_FD_CAST(fd), &_writefdset6);
	    } else {
		debug_msg("Attempt to register for IOT_WRITE with a "
			     "socket of unsupported address family %d.\n",
			     family);
		return false;
	    }

	    _ioevent_map.insert(std::make_pair(IoEventTuple(fd, type), cb));
	    return true;
	}

	long newmask, oldmask;

	oldmask = 0;
	newmask = _map_ioevent_to_wsaevent(type);

	if (newmask == 0 || (newmask & ~WSAEVENT_SUPPORTED) != 0) {
	    debug_msg("IoEventType does not map to a valid Winsock async "
			 "event mask.\n");
	    return false;
	}

	// Check if there are notifications already requested for
	// this socket.
	// If either old or new event masks for notification on the same
	// socket  contain other bits in addition to FD_ACCEPT in the mask,
	// reject the request.
	SocketWSAEventMap::iterator jj = _socket_wsaevent_map.find(fd);
	if (jj != _socket_wsaevent_map.end()) {
	    oldmask = jj->second;
	    if (((oldmask & ~FD_ACCEPT) && (newmask &  FD_ACCEPT)) ||
		((oldmask &  FD_ACCEPT) && (newmask & ~FD_ACCEPT))) {
		debug_msg("attempt to register a socket for other events "
			     "in addition to IOT_ACCEPT.\n");
		return false;
	    }
	}

	// Push the event mapping to Winsock. Wire up the callback.
	newmask |= oldmask;
	int retval = ::WSAAsyncSelect(fd, _hwnd, XM_SOCKET, newmask);
	if (retval != 0) {
	    debug_msg("WSAAsyncSelect(): %d\n", WSAGetLastError());
	    return false;
	}

	if (jj != _socket_wsaevent_map.end()) {
	    jj->second = newmask;
	} else {
	    _socket_wsaevent_map.insert(std::make_pair(fd, newmask));
	}

	_ioevent_map.insert(std::make_pair(IoEventTuple(fd, type), cb));
	return true;
}

bool
WinDispatcher::add_handle_cb(XorpFd& fd, IoEventType type, const IoEventCb& cb)
{
	// XXX: You cannot currently register for anything other
	// than an IOT_READ event on a Windows object handle because
	// there is no way of telling why an object was signalled.
	if (type != IOT_READ) {
	    debug_msg("attempt to register a Windows object handle for "
			 "event other than IOT_READ\n");
	    return false;
	}

	// Check that we haven't exceeded the handle limit.
	if (_handles.size() == MAXIMUM_WAIT_OBJECTS) {
	    debug_msg("exceeded Windows object handle count\n");
	    return false;
	}

	// XXX: Currently we only support 1 callback per Windows object/event.
	// Check that the object does not already have a callback
	// registered for it.
	IoEventMap::iterator ii = _ioevent_map.find(IoEventTuple(fd, type));
	if (ii != _ioevent_map.end()) {
	    debug_msg("callback already registered\n");
	    return false;
	}

	// Register the callback. Add fd to the object handle array.
	_ioevent_map.insert(std::make_pair(IoEventTuple(fd, type), cb));

	if (fd.is_pipe()) {
	    _polled_pipes.push_back(fd);
	} else {
	    _handles.push_back(fd);
	}

	return true;
}

bool
WinDispatcher::add_ioevent_cb(XorpFd fd, IoEventType type, const IoEventCb& cb)
{
    debug_msg("Adding event %d to object %s\n", type, fd.str().c_str());

    switch (fd.type()) {
    case XorpFd::FDTYPE_SOCKET:
	return add_socket_cb(fd, type, cb);
	break;
    case XorpFd::FDTYPE_PIPE:
    case XorpFd::FDTYPE_CONSOLE:
    case XorpFd::FDTYPE_ERROR:	    // XXX Process handles don't show up
	return add_handle_cb(fd, type, cb);
	break;
    default:
	break;
    }

    return false;
}

bool
WinDispatcher::remove_socket_cb(XorpFd& fd, IoEventType type)
{
    bool unregistered = false;

    // XXX: writability is a special case, so deal with it first.
    // XXX: We have a bit of a quandary here in that if the socket
    // has already been closed, we can't determine its family. This
    // is a problem for us because the handle may still be lying
    // around in an I/O dispatch map; therefore we need to take a
    // laborious detour through both lists.
    if (type == IOT_WRITE) {
	//int family = _get_sock_family(fd);

	//if (family == AF_INET) {
	    FD_CLR(XORP_FD_CAST(fd), &_writefdset4);
	    for (vector<SOCKET>::iterator ii = _writesockets4.begin();
		ii != _writesockets4.end(); ii++) {
		if (*ii == fd) {
		    ii = _writesockets4.erase(ii);
		    _ioevent_map.erase(IoEventTuple(fd, type));
		    unregistered = true;
		    break;
		}
	    }
	//} else if (family == AF_INET6) {
	    FD_CLR(XORP_FD_CAST(fd), &_writefdset6);
	    for (vector<SOCKET>::iterator ii = _writesockets6.begin();
		ii != _writesockets6.end(); ii++) {
		if (*ii == fd) {
		    _writesockets6.erase(ii);
		    _ioevent_map.erase(IoEventTuple(fd, type));
		    unregistered = true;
		    break;
		}
	    }
	//} else {
	 //   debug_msg("socket is neither AF_INET or AF_INET6, "
	//	      "or couldn't determine family.\n");
	//}

	if (unregistered == false)
	    debug_msg("socket %s not found for IOT_WRITE\n", fd.str().c_str());

	return unregistered;
    } /* IOT_WRITE */

	long newmask, delmask;

	// Look for the socket in the Winsock event map.
	SocketWSAEventMap::iterator ii = _socket_wsaevent_map.find(fd);
	if (ii == _socket_wsaevent_map.end()) {
	    debug_msg("Attempt to remove unmapped callback of type %d "
			 "from socket %s.\n", type, fd.str().c_str());
	    return false;
	}

	// Compute the new event mask. Set it to zero if we were asked to
	// deregister this socket completely.
	delmask = _map_ioevent_to_wsaevent(type);
	XLOG_ASSERT(delmask != -1);
	newmask = ii->second & ~delmask;

	int retval;

	// If the event mask for this socket is now empty, remove it
	// from Winsock's notification map. Otherwise, modify the map
	// to reflect the new event mask.
	if (newmask == 0) {
	    retval = ::WSAEventSelect(fd, NULL, 0);
	    _socket_wsaevent_map.erase(ii);	    // invalidates iterator
	} else {
	    retval = ::WSAAsyncSelect(fd, _hwnd, XM_SOCKET, newmask);
	    ii->second = newmask;
	}

	if (retval != 0)
	    debug_msg("WSAAsyncSelect(): %d\n", WSAGetLastError());

	IoEventMap::iterator jj = _ioevent_map.find(IoEventTuple(fd, type));
	if (jj != _ioevent_map.end()) {
	    // Unwire the callback if it exists.
	    _ioevent_map.erase(jj);
	    unregistered = true;
	} else {
	    debug_msg("Attempt to remove a nonexistent callback of "
			 "type %d from socket %s.\n", type, fd.str().c_str());
	}

    return unregistered;
}

bool
WinDispatcher::remove_handle_cb(XorpFd& fd, IoEventType type)
{
    XLOG_ASSERT(type == IOT_READ);

    if (fd.is_pipe()) {
	for (vector<HANDLE>::iterator ii = _polled_pipes.begin();
	    ii != _polled_pipes.end(); ++ii) {
	    if (*ii == fd) {
		ii = _polled_pipes.erase(ii);
		_ioevent_map.erase(IoEventTuple(fd, IOT_READ));
		return true;
	    }
	}
    } else {
	for (vector<HANDLE>::iterator ii = _handles.begin();
	    ii != _handles.end(); ++ii) {
	    if (*ii == fd) {
		ii = _handles.erase(ii);
		_ioevent_map.erase(IoEventTuple(fd, IOT_READ));
		return true;
	    }
	}
    }

    return false;
}

bool
WinDispatcher::remove_ioevent_cb(XorpFd fd, IoEventType type)
{
    debug_msg("Removing event %d from object %s\n", type, fd.str().c_str());

    switch (fd.type()) {
    case XorpFd::FDTYPE_SOCKET:
	return remove_socket_cb(fd, type);
    case XorpFd::FDTYPE_PIPE:
    case XorpFd::FDTYPE_CONSOLE:
	return remove_handle_cb(fd, type);
	break;
    default:
	break;
    }
    return false;
}

//
// Handle Winsock events received by our invisible window.
//
// TODO: Do something with the Winsock error code, as retrieved by
// the WSAGETSELECTERROR(lParam) macro.
//
int
WinDispatcher::window_procedure(HWND hwnd, UINT uMsg,
				WPARAM wParam, LPARAM lParam)
{
    XLOG_ASSERT(hwnd == _hwnd);

    if (uMsg == XM_SOCKET) {
	XorpFd fd((SOCKET)wParam);
	long event = WSAGETSELECTEVENT(lParam);

	debug_msg("Event %ld (%s) on socket %s\n", event,
		  _get_event_string(event), fd.str().c_str());

	// Sanity check: received event is for a socket we're aware of.
	SocketWSAEventMap::iterator ii = _socket_wsaevent_map.find(fd);
	if (ii == _socket_wsaevent_map.end()) {
	    debug_msg("Socket isn't registered for events.\n");
	    return -1;
	}

	// Sanity check: received event is in mask of expected events.
	if ((ii->second & WSAGETSELECTEVENT(lParam)) == 0) {
	    debug_msg("Event isn't registered for socket.\n");
	    return -1;
	}

#if 0
	// XXX: Temporarily block and restore FD_WRITE if enabled, to
	// try to persuade Winsock to give us level-triggered events!
	// XXX: This doesn't work. We need to do this the long way round;
	// we need to know when a socket is blocked for furhter writes.
	if (ii->second & FD_WRITE) {
	    ::WSAEventSelect(fd, NULL, 0);
	    ::WSAAsyncSelect(fd, hwnd, XM_SOCKET, ii->second);
	}
#endif
	
	// Sanity check: received event maps to a valid IoEventType.
	int itype = _map_wsaevent_to_ioevent(event);
	if (itype == -1) {
	    debug_msg("Event doesn't map to a valid IoEventType.\n");
	    return -1;
	}
	IoEventType type = static_cast<IoEventType>(itype);

	// Look up the callback.
	IoEventMap::iterator jj = _ioevent_map.find(IoEventTuple(fd, type));
	if (jj == _ioevent_map.end()) {
	    debug_msg("IoEvent %d on socket %s does not map to a callback.\n",
			 itype, fd.str().c_str());
	    return -1;
	}

	jj->second->dispatch(fd, type);
	return 0;

    } else {
	debug_msg("Unknown message type %u\n", XORP_UINT_CAST(uMsg));
	return -1;
    }

    return -1;
}

void
WinDispatcher::wait_and_dispatch(int ms)
{
    DWORD retval;

    if ((!_writesockets4.empty() || !_writesockets6.empty() ||
	 !_polled_pipes.empty()) && (ms > POLLED_INTERVAL_MS))
	ms = POLLED_INTERVAL_MS;

    retval = ::MsgWaitForMultipleObjectsEx(
		_handles.empty() ? 0 : _handles.size(),
		_handles.empty() ? NULL : &_handles[0],
		ms, QS_POSTMESSAGE|QS_SENDMESSAGE, 0);

    _clock->advance_time();

    // Handle polled readable objects.
    // Reads need to be handled first because they may come from
    // dying processes picked up by the handle poll.
    dispatch_pipe_reads();

    // The order of the if clauses here is important.
    if (retval == WAIT_FAILED) {
	DWORD lasterr = GetLastError();
	if (lasterr == ERROR_INVALID_HANDLE && !_handles.empty()) {
	    // There's a bad handle in the handles vector. Find it.
	    DWORD dwFlags;
	    vector<HANDLE>::iterator kk = _handles.begin();
	    while (kk != _handles.end()) {
		if (GetHandleInformation(*kk, &dwFlags) == 0) {
		    XLOG_ERROR("handle %p is bad, removing it.", *kk);
		    kk = _handles.erase(kk);
		} else {
		    ++kk;
		}
	    }
	} else {
	    XLOG_FATAL("MsgWaitForMultipleObjectsEx() failed, code %lu."
		       "This should never happen.", lasterr);
	}
    } else if (retval == WAIT_TIMEOUT) {
	// The timeout period elapsed. This is normal.
    } else if (retval == WAIT_IO_COMPLETION) {
	XLOG_ERROR("A User APC was called. This should never happen.");
    } else if (retval == WAIT_OBJECT_0 + _handles.size()) {
	//
	// One or more window messages were received. Dispatch them.
	// Use a filter to only look for XM_SOCKET messages.
	//
	MSG msg;
	BOOL ret;
	while (0 != (ret = PeekMessage(&msg, _hwnd, XM_SOCKET, XM_SOCKET,
				       PM_REMOVE|PM_NOYIELD))) {
	    if (ret == -1) {
		XLOG_ERROR("PeekMessage() failed, code %lu. "
			   "This should never happen.", GetLastError());
		break;
	    }
	    (void)window_procedure(_hwnd, msg.message, msg.wParam, msg.lParam);
	}
    } else if ( //retval >= WAIT_OBJECT_0 && // always true for unsigned
	       retval <= (WAIT_OBJECT_0 + _handles.size() - 1)) {
	//
	// An object in _handles was signalled. Dispatch its callback.
	//
	XorpFd fd(_handles[retval - WAIT_OBJECT_0]);

	debug_msg("Dispatching IOT_READ on handle %s\n", fd.str().c_str());

	IoEventMap::iterator jj = _ioevent_map.find(IoEventTuple(fd, IOT_READ));
	XLOG_ASSERT(jj != _ioevent_map.end());
	jj->second->dispatch(fd, IOT_READ);

    } else if (retval >= WAIT_ABANDONED_0 &&
	       retval <= (WAIT_ABANDONED_0 + _handles.size() - 1)) {
	XLOG_ERROR("A mutex was abandoned. This should never happen.");
    } else {
	XLOG_FATAL("An unknown Windows system event occurred.");
    }

    // Handle polled writable objects.
    dispatch_socket_writes();
}

// Deal with IOT_WRITE events on sockets. We realistically won't be dealing
// with sockets other than AF_INET and AF_INET6 at the moment, so we
// don't check for writability on anything else.
void
WinDispatcher::dispatch_socket_writes()
{
    struct timeval tv_zero = { 0, 0 };
    size_t i;
    int nfds;

    // Dispatch pending AF_INET IOT_WRITE callbacks if any.
    if (!_writesockets4.empty()) {
	fd_set writefds4 = _writefdset4;
	nfds = ::select(-1, NULL, &writefds4, NULL, &tv_zero);
	if (nfds > 0) {
	    for (i = 0; i < _writesockets4.size(); ++i) {
		if (FD_ISSET(_writesockets4[i], &writefds4)) {
		    IoEventMap::iterator jj =
_ioevent_map.find(IoEventTuple(_writesockets4[i], IOT_WRITE));
		    XLOG_ASSERT(jj != _ioevent_map.end());
		    jj->second->dispatch(_writesockets4[i], IOT_WRITE);
		}
	    }
       }
    }

    // Dispatch pending AF_INET6 IOT_WRITE callbacks if any.
    if (!_writesockets6.empty()) {
	fd_set writefds6 = _writefdset6;
	nfds = ::select(-1, NULL, &writefds6, NULL, &tv_zero);
	if (nfds > 0) {
	    for (i = 0; i < _writesockets6.size(); ++i) {
		if (FD_ISSET(_writesockets6[i], &writefds6)) {
		    IoEventMap::iterator jj =
_ioevent_map.find(IoEventTuple(_writesockets6[i], IOT_WRITE));
		    XLOG_ASSERT(jj != _ioevent_map.end());
		    jj->second->dispatch(_writesockets6[i], IOT_WRITE);
		}
	    }
	}
    }
}

// Deal with polling pipes for readability; their handles cannot be
// used with the Win32 wait functions.
void
WinDispatcher::dispatch_pipe_reads()
{
    ssize_t result;

    for (vector<HANDLE>::iterator ii = _polled_pipes.begin();
	ii != _polled_pipes.end(); ii++) {
	result = win_pipe_read(*ii, NULL, 0);
	if (result == WINIO_ERROR_HASINPUT) {
	    debug_msg("dispatching IOT_READ on pipe %p\n", *ii);
	    IoEventMap::iterator jj =
_ioevent_map.find(IoEventTuple(*ii, IOT_READ));
	    XLOG_ASSERT(jj != _ioevent_map.end());
	    jj->second->dispatch(*ii, IOT_READ);
	}
    }
}

#endif // HOST_OS_WINDOWS
