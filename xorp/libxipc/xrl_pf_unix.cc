// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008-2011 XORP, Inc and Others
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

#include "xrl_module.h"
#include "xrl_pf_unix.hh"
#include "libcomm/comm_api.h"
#include "sockutil.hh"

#ifndef	HOST_OS_WINDOWS

#include <pwd.h>
#include <grp.h>


const char* XrlPFUNIXListener::_protocol = "unix";

XrlPFUNIXListener::XrlPFUNIXListener(EventLoop& e, XrlDispatcher* xr)
    : XrlPFSTCPListener(&e, xr)
{
    string path = get_sock_path();

    _sock = comm_bind_unix(path.c_str(), COMM_SOCK_NONBLOCKING);
    if (!_sock.is_valid())
	xorp_throw(XrlPFConstructorError, comm_get_last_error_str());

    if (comm_listen(_sock, COMM_LISTEN_DEFAULT_BACKLOG) != XORP_OK) {
	comm_close(_sock);
	_sock.clear();
        xorp_throw(XrlPFConstructorError, comm_get_last_error_str());
    }

    struct group *grp = getgrnam("xorp");
    if (grp) {
	/* Change the group to be 'xorp', leave owner as is. */
	if (chown(path.c_str(), -1, grp->gr_gid)) {
	    cerr << "ERROR: Failed chown on path: " << path << " error: " << strerror(errno) << endl;
	}
    }
    else {
	// Something is wrong, probably no xorp user.  This is not necessarily
	// a real problem, so don't want to fill up logs.  Might be worth
	// doing a similar check in xorp_rtrmgr startup and warn once there...
    }

    /* Owner read/write, group read/write, other read -JC */
    if (chmod(path.c_str(), S_IWUSR| S_IRUSR| S_IWGRP| S_IRGRP| S_IROTH)) {
	cerr << "ERROR: Failed chmod on path: " << path << " error: " << strerror(errno) << endl;
    }

    _address_slash_port = path;
    encode_address(_address_slash_port);

    _eventloop.add_ioevent_cb(_sock, IOT_ACCEPT,
         callback(dynamic_cast<XrlPFSTCPListener*>(this),
                  &XrlPFSTCPListener::connect_hook));
}

string
XrlPFUNIXListener::get_sock_path()
{
    // XXX race
    string path;
    string err;

    FILE* f = xorp_make_temporary_file("/var/tmp", "xrl", path, err);
    if (!f)
	xorp_throw(XrlPFConstructorError, err);

    fclose(f);

    // XXX we shouldn't be compiling this under windows
#ifndef HOST_OS_WINDOWS
    unlink(path.c_str());
#endif

    return path;
}

XrlPFUNIXListener::~XrlPFUNIXListener()
{
#ifndef HOST_OS_WINDOWS
    // XXX this probably isn't the right place for this.  Perhaps libcomm should
    // sort this out.
    string path = _address_slash_port;
    decode_address(path);
    unlink(path.c_str());
#endif
}

const char*
XrlPFUNIXListener::protocol() const
{
    return _protocol;
}

static void
replace(string& in, char a, char b)
{
    for (string::iterator i = in.begin(); i != in.end(); ++i) {
	char& x = *i;

	if (x == a)
	    x = b;
    }
}

void
XrlPFUNIXListener::encode_address(string& address)
{
    replace(address, '/', ':');
}

void
XrlPFUNIXListener::decode_address(string& address)
{
    replace(address, ':', '/');
}

////////////////////////
//
// XrlPFUNIXSender
//
////////////////////////

XrlPFUNIXSender::XrlPFUNIXSender(EventLoop& e, const char* addr)
    : XrlPFSTCPSender(&e, addr)
{
    string address = addr;
    XrlPFUNIXListener::decode_address(address);

    _sock = comm_connect_unix(address.c_str(), COMM_SOCK_NONBLOCKING);

    if (!_sock.is_valid())
	xorp_throw(XrlPFConstructorError,
		   c_format("Could not connect to %s\n", address.c_str()));

    // Set the receiving socket buffer size in the kernel
    if (comm_sock_set_rcvbuf(_sock, SO_RCV_BUF_SIZE_MAX, SO_RCV_BUF_SIZE_MIN)
        < SO_RCV_BUF_SIZE_MIN) {
        comm_close(_sock);
        _sock.clear();

	xorp_throw(XrlPFConstructorError, "Can't set receive buffer size");
    }
    
    // Set the sending socket buffer size in the kernel
    if (comm_sock_set_sndbuf(_sock, SO_SND_BUF_SIZE_MAX, SO_SND_BUF_SIZE_MIN)
        < SO_SND_BUF_SIZE_MIN) {
        comm_close(_sock);
        _sock.clear();

	xorp_throw(XrlPFConstructorError, "Can't set send buffer size");
    }

    construct();
}

const char*
XrlPFUNIXSender::protocol_name()
{
    return XrlPFUNIXListener::_protocol;
}

const char*
XrlPFUNIXSender::protocol() const
{
    return protocol_name();
}

#endif
