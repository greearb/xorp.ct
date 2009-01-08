// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ident "$XORP: xorp/fea/data_plane/control_socket/windows_rtm_pipe.cc,v 1.5 2008/10/02 21:56:54 bms Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/utils.hh"

#include <algorithm>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <errno.h>

#ifdef HOST_OS_WINDOWS
#include "libxorp/win_io.h"
#include "windows_routing_socket.h"
#endif

#include "libcomm/comm_api.h"
#include "windows_rtm_pipe.hh"

uint16_t WinRtmPipe::_instance_cnt = 0;
pid_t WinRtmPipe::_pid = getpid();

//
// Routing Sockets communication with the kernel
//

WinRtmPipe::WinRtmPipe(EventLoop& eventloop)
    : _eventloop(eventloop),
      _seqno(0),
      _instance_no(_instance_cnt++)
{
    
}

WinRtmPipe::~WinRtmPipe()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the RTMv2 pipe: %s", error_msg.c_str());
    }

    XLOG_ASSERT(_ol.empty());
}

#ifndef HOST_OS_WINDOWS

int
WinRtmPipe::start(int af, string& error_msg)
{
    UNUSED(af);

    error_msg = c_format("The system does not support Router Manager V2");
    XLOG_UNREACHABLE();
    return (XORP_ERROR);
}

int
WinRtmPipe::stop(string& error_msg)
{
    UNUSED(error_msg);

    return (XORP_OK);
}

ssize_t
WinRtmPipe::write(const void* data, size_t nbytes)
{
    return (-1);
    UNUSED(data);
    UNUSED(nbytes);
}

int
WinRtmPipe::force_read(string& error_msg)
{
    XLOG_UNREACHABLE();

    error_msg = "method not supported";

    return (XORP_ERROR);
}

#else // HOST_OS_WINDOWS

int
WinRtmPipe::start(int af, string& error_msg)
{
    string pipename;
    DWORD result;

    if (af == AF_INET) {
	pipename = XORPRTM4_PIPENAME;
    } else if (af == AF_INET6) {
	pipename = XORPRTM6_PIPENAME;
    } else {
    	error_msg = c_format("Unknown address family %d.", af);
	return (XORP_ERROR);
    }

    if (!WaitNamedPipeA(pipename.c_str(), NMPWAIT_USE_DEFAULT_WAIT)) {
    	error_msg = c_format("No RTMv2 pipes available.");
	return (XORP_ERROR);
    }

    _fd = CreateFileA(pipename.c_str(), GENERIC_READ|GENERIC_WRITE,
			 0, NULL, OPEN_EXISTING, 0, NULL);
    if (!_fd.is_valid()) {
	result = GetLastError();
    	error_msg = c_format("Error opening RTMv2 pipe: %s.",
			     win_strerror(result));
	return (XORP_ERROR);
    }

    if (_eventloop.add_ioevent_cb(_fd, IOT_READ,
	    callback(this, &WinRtmPipe::io_event)) == false) {
	error_msg = c_format("Failed to add RTMv2 pipe read to EventLoop");
	CloseHandle(_fd);
	_fd.clear();
	return (XORP_ERROR);
    }
    if (_eventloop.add_ioevent_cb(_fd, IOT_DISCONNECT,
	    callback(this, &WinRtmPipe::io_event)) == false) {
	error_msg = c_format("Failed to add RTMv2 pipe close to EventLoop");
	CloseHandle(_fd);
	_fd.clear();
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
WinRtmPipe::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (_fd.is_valid()) {
	_eventloop.remove_ioevent_cb(_fd, IOT_READ);
	_eventloop.remove_ioevent_cb(_fd, IOT_DISCONNECT);
	CloseHandle(_fd);
	_fd.clear();
    }

    return (XORP_OK);
}

ssize_t
WinRtmPipe::write(const void* data, size_t nbytes)
{
    DWORD byteswritten;
    DWORD result;

    if (!_fd.is_valid())
	return (-1);

    _seqno++;
    result = WriteFile(_fd, data, nbytes, &byteswritten, NULL);

    return ((size_t)byteswritten);
    UNUSED(result);
}

int
WinRtmPipe::force_read(string& error_msg)
{
    vector<uint8_t> message;
    vector<uint8_t> buffer(ROUTING_SOCKET_BYTES);
    size_t off = 0;
    size_t last_mh_off = 0;
    DWORD nbytes;
    DWORD result;

    if (!_fd.is_valid())
	return (XORP_ERROR);
    
    for (;;) {
	do {
	    result = PeekNamedPipe(_fd, &buffer[0], buffer.size(), NULL,
				    NULL, &nbytes);
	    if (result == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		buffer.resize(buffer.size() + ROUTING_SOCKET_BYTES);
		continue;
	    } else {
		break;
	    }
	} while (true);

	result = ReadFile(_fd, &buffer[0], buffer.size(), &nbytes, NULL);
	if (result == 0) {
	    result = GetLastError();
	    error_msg = c_format("Rtmv2 pipe read error: %s",
				 win_strerror(result));
	    return (XORP_ERROR);
	}
	message.resize(message.size() + nbytes);
	memcpy(&message[off], &buffer[0], nbytes);
	off += nbytes;

	if ((off - last_mh_off)
	    < (ssize_t)(sizeof(u_short) + 2 * sizeof(u_char))) {
	    error_msg = c_format("Rtmv2 pipe read failed: "
				 "message truncated: "
				 "received %d bytes instead of (at least) %u "
				 "bytes",
				 XORP_INT_CAST(nbytes),
				 XORP_UINT_CAST(sizeof(u_short) + 2 * sizeof(u_char)));
	    return (XORP_ERROR);
	}

	//
	// Received message (probably) OK
	//
	AlignData<struct if_msghdr> align_data(message);
	const struct if_msghdr* mh = align_data.payload();
	XLOG_ASSERT(mh->ifm_msglen == message.size());
	XLOG_ASSERT(mh->ifm_msglen == nbytes);
	last_mh_off = off;
	break;
    }
    XLOG_ASSERT(last_mh_off == message.size());

    //
    // Notify observers
    //
    for (ObserverList::iterator i = _ol.begin(); i != _ol.end(); i++) {
	(*i)->routing_socket_data(message);
    }

    return (XORP_OK);
}

void
WinRtmPipe::io_event(XorpFd fd, IoEventType type)
{
    string error_msg;

    XLOG_ASSERT(fd == _fd);

    if (!_fd.is_valid()) {
	XLOG_ERROR("Error: io_event called when file descriptor is dead");
	error_msg = "RTMv2 pipe disconnected";
	stop(error_msg);
    }

    if (type == IOT_READ) {
	if (force_read(error_msg) != XORP_OK) {
	    XLOG_ERROR("Error force_read() from RTMv2 pipe: %s",
		       error_msg.c_str());
	}
    } else if (type == IOT_DISCONNECT) {
	error_msg = "RTMv2 pipe disconnected";
	stop(error_msg);
    } else {
	XLOG_UNREACHABLE();
    }
}

#endif // !HOST_OS_WINDOWS

//
// Observe routing sockets activity
//

struct WinRtmPipePlumber {
    typedef WinRtmPipe::ObserverList ObserverList;

    static void
    plumb(WinRtmPipe& r, WinRtmPipeObserver* o)
    {
	ObserverList& ol = r._ol;
	ObserverList::iterator i = find(ol.begin(), ol.end(), o);
	debug_msg("Plumbing WinRtmPipeObserver %p to WinRtmPipe%p\n",
		  o, &r);
	XLOG_ASSERT(i == ol.end());
	ol.push_back(o);
    }
    static void
    unplumb(WinRtmPipe& r, WinRtmPipeObserver* o)
    {
	ObserverList& ol = r._ol;
	debug_msg("Unplumbing WinRtmPipeObserver %p from "
		  "WinRtmPipe %p\n", o, &r);
	ObserverList::iterator i = find(ol.begin(), ol.end(), o);
	XLOG_ASSERT(i != ol.end());
	ol.erase(i);
    }
};

WinRtmPipeObserver::WinRtmPipeObserver(WinRtmPipe& rs)
    : _rs(rs)
{
    WinRtmPipePlumber::plumb(rs, this);
}

WinRtmPipeObserver::~WinRtmPipeObserver()
{
    WinRtmPipePlumber::unplumb(_rs, this);
}

WinRtmPipe&
WinRtmPipeObserver::routing_socket()
{
    return _rs;
}

