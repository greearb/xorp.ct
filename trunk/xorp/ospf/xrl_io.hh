// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ifndef __OSPF_XRL_IO_HH__
#define __OSPF_XRL_IO_HH__

#include "io.hh"

class EventLoop;

/**
 * Concrete implementation of IO using XRLs.
 */
class XrlIO : public IO {
 public:
    XrlIO(EventLoop& eventloop, string ribname)
	: _eventloop(eventloop), _ribname(ribname)
    {}


    /**
     * Send Raw frames.
     */
    bool send(const string& interface, const string& vif, 
	      uint8_t* data, uint32_t len);

    /**
     * Register for receiving raw frames.
     */
    bool register_receive(ReceiveCallback cb);

    /**
     * Add route to RIB.
     */
    bool add_route();

    /**
     * Delete route from RIB
     */
    bool delete_route();

 private:
    EventLoop& _eventloop;
    string _ribname;

    ReceiveCallback _cb;
};
#endif // __OSPF_XRL_IO_HH__
