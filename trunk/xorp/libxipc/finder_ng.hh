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

// $XORP: xorp/libxipc/finder_ng.hh,v 1.1 2003/01/24 02:48:22 hodson Exp $

#ifndef __LIBXIPC_FINDER_NG_HH__
#define __LIBXIPC_FINDER_NG_HH__

#include "config.h"

#include "libxipc/xrl_cmd_map.hh"
#include "finder_messenger.hh"

class FinderNG {
public:
    
public:
    FinderNG();
    ~FinderNG();

    void messenger_active_event(FinderMessengerBase*);
    void messenger_inactive_event(FinderMessengerBase*);
    void messenger_stopped_event(FinderMessengerBase*);
    void messenger_birth_event(FinderMessengerBase*);
    void messenger_death_event(FinderMessengerBase*);

    XrlCmdMap& commands();
    
protected:
    XrlCmdMap _cmds;
    FinderMessengerBase* _active_messenger;
    list<FinderMessengerBase*> _messengers;
};

#include "finder_tcp.hh"
#include "finder_tcp_messenger.hh"

class FinderTcpMessenger : public FinderTcpMessengerBase {
public:
    FinderTcpMessenger(EventLoop& e,
		       FinderNG&  finder,
		       int	  fd,
		       XrlCmdMap& cmds);

    FinderTcpMessenger(EventLoop& e,
		       int	  fd,
		       XrlCmdMap& cmds);

    ~FinderTcpMessenger();
    
    void pre_dispatch_xrl();

    void post_dispatch_xrl();

    void close_event();
    
protected:
    FinderNG* _finder;
};

/**
 * Class that creates FinderMessengers for incoming connections.
 *
 * New FinderMessengers are announced to the Finder via
 * FinderNG::messenger_birth_event, and the Finder becomes responsible
 * for the memory management of the messengers.
 */
class FinderNGTcpListener : public FinderTcpListenerBase {
public:
    typedef FinderTcpListenerBase::AddrList Addr4List;
    typedef FinderTcpListenerBase::NetList Net4List;

    FinderNGTcpListener(FinderNG&  finder,
			EventLoop& e,
			IPv4	   interface,
			uint16_t   port)
	throw (InvalidPort);

    ~FinderNGTcpListener();

    /**
     * Instantiate a Messenger instance for fd.
     * @return true on success, false on failure.
     */
    bool connection_event(int fd);
    
protected:
    FinderNG&			_finder;
    list<FinderTcpMessenger*> _messengers;
};

#endif // __LIBXIPC_FINDER_NG_HH__
