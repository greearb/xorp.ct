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

// $XORP: xorp/devnotes/template.hh,v 1.2 2003/01/16 19:08:48 mjh Exp $

#ifndef __LIBXIPC_FINDER_NG_HH__
#define __LIBXIPC_FINDER_NG_HH__

#include "config.h"

#include "libxipc/xrl_cmd_map.hh"
#include "finder_tcp_messenger.hh"

class FinderNG;

class FinderNGMessenger : public FinderTcpMessenger {
public:
    FinderNGMessenger(EventLoop& e,
		      FinderNG&	 finder,
		      int	 fd,
		      XrlCmdMap& cmds)
	: FinderTcpMessenger(e, fd, cmds), _finder(finder)
    {}

    void pre_dispatch_xrl();

    void post_dispatch_xrl();

    void close_event();
    
protected:
    FinderNG& _finder;
};

class FinderNG : public FinderTcpListenerBase {
public:
    typedef FinderTcpListenerBase::AddrList Addr4List;
    typedef FinderTcpListenerBase::NetList Net4List;
    
public:
    FinderNG(EventLoop& e, IPv4 interface, uint16_t port)
	throw (InvalidPort);
    ~FinderNG();
    
    FinderNGMessenger* set_active_messenger(FinderNGMessenger*);

    void messenger_death_event(FinderNGMessenger*);

    bool connection_event(int fd);
    
protected:
    XrlCmdMap _cmds;
    FinderNGMessenger* _active_messenger;
    list<FinderNGMessenger*> _messengers;
};

#endif // __LIBXIPC_FINDER_NG_HH__
