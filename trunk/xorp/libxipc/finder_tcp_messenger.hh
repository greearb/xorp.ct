// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/devnotes/template.hh,v 1.1.1.1 2002/12/11 23:55:54 hodson Exp $

#ifndef __LIBXIPC_FINDER_TCP_MESSENGER_HH__
#define __LIBXIPC_FINDER_TCP_MESSENGER_HH__

#include <list>

#include "libxorp/ref_ptr.hh"

#include "finder_tcp.hh"
#include "finder_msgs.hh"
#include "finder_messenger.hh"

class FinderTcpMessenger : public FinderMessengerBase, protected FinderTcpBase
{
public:
    FinderTcpMessenger(EventLoop& e, int fd, XrlCmdMap& cmds) :
	FinderMessengerBase(e, cmds), FinderTcpBase(e, fd) {}

    ~FinderTcpMessenger();

    bool send(const Xrl& xrl, const SendCallback& scb);

    bool pending() const;
    
protected:
    // FinderTcpBase methods
    void read_event(int		   errval,
		    const uint8_t* data,
		    uint32_t	   data_bytes);

    void write_event(int	    errval,
		     const uint8_t* data,
		     uint32_t	    data_bytes);

    void close_event();

protected:
    void reply(uint32_t seqno, const XrlError& xe, const XrlArgs* reply_args);

protected:
    void push_queue();
    void drain_queue();
    
    /* High water-mark to disable reads, ie reading faster than writing */
    static const uint32_t OUTQUEUE_BLOCK_READ_HI_MARK = 6;

    /* Low water-mark to enable reads again */
    static const uint32_t OUTQUEUE_BLOCK_READ_LO_MARK = 4;    

    typedef list<const FinderMessageBase*> OutputQueue;
    OutputQueue _out_queue;
};

#endif // __LIBXIPC_FINDER_TCP_MESSENGER_HH__
