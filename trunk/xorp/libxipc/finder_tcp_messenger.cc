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

#ident "$XORP: xorp/libxipc/finder_tcp_messenger.cc,v 1.1 2003/01/21 18:51:36 hodson Exp $"

#include "finder_module.h"
#include "libxorp/xlog.h"

#include "finder_tcp_messenger.hh"

FinderTcpMessengerBase::~FinderTcpMessengerBase()
{
    drain_queue();
}

void
FinderTcpMessengerBase::read_event(int	      errval,
			       const uint8_t* data,
			       uint32_t	      data_bytes)
{
    if (errval != 0) {
	/* An error has occurred, the FinderTcpBase class will close
	 * connection following this notification.
	 */
	return;
    }

    string s(data, data + data_bytes);
    string ex;
    try {
	try {
	    ParsedFinderXrlMessage fm(s.c_str());
	    dispatch_xrl(fm.seqno(), fm.xrl());
	    return;
	} catch (const WrongFinderMessageType&) {
	    ParsedFinderXrlResponse fm(s.c_str());
	    dispatch_xrl_response(fm.seqno(), fm.xrl_error(), fm.xrl_args());
	    return;
	}
    } catch (const InvalidString& e) {
	ex = e.str();
    } catch (const BadFinderMessageFormat& e) {
	ex = e.str();
    } catch (const WrongFinderMessageType& e) {
	ex = e.str();
    }
    XLOG_ERROR("Got exception %s, closing connection", ex.c_str());
    close();
}

bool
FinderTcpMessengerBase::send(const Xrl& xrl, const SendCallback& scb)
{
    if (pending())
	return false;

    FinderXrlMessage* msg = new FinderXrlMessage(xrl);

    if (store_xrl_response(msg->seqno(), scb) == false) {
	XLOG_ERROR("Could not store xrl response\n");
	delete msg;
	return false;
    }

    if (_out_queue.empty()) {
	_out_queue.push_back(msg);
	push_queue();
    } else {
	_out_queue.push_back(msg);
    }
    
    return true;
}

bool
FinderTcpMessengerBase::pending() const
{
    return (false == _out_queue.empty());
}

void
FinderTcpMessengerBase::reply(uint32_t	  seqno,
			  const XrlError& xe,
			  const XrlArgs*  args)
{
    FinderXrlResponse* msg = new FinderXrlResponse(seqno, xe, args);
    if (_out_queue.empty()) {
	_out_queue.push_back(msg);
	push_queue();
    } else {
	_out_queue.push_back(msg);
    }
}

inline static const uint8_t*
get_data(const FinderMessageBase& fm)
{
    const char* p = fm.str().c_str();
    return reinterpret_cast<const uint8_t*>(p);
}

inline static uint32_t
get_data_bytes(const FinderMessageBase& fm)
{
    return fm.str().size();
}

void
FinderTcpMessengerBase::push_queue()
{
    XLOG_ASSERT(false == _out_queue.empty());
    const FinderMessageBase* fm = _out_queue.front();

    assert(0 != fm);
    write_data(get_data(*fm), get_data_bytes(*fm));

    /*
     * Block read queue if output queue has grown too large.  This
     * stops new requests and responses and is intended to allow output
     * queue to drain.
     */
    const size_t qs = _out_queue.size();
    if (qs >= OUTQUEUE_BLOCK_READ_HI_MARK && true == read_enabled()) {
	set_read_enabled(false);
	XLOG_WARNING("Blocking input queue, output queue hi water mark "
		     "reached.");
    } else if (qs == OUTQUEUE_BLOCK_READ_LO_MARK && false == read_enabled()) {
	set_read_enabled(true);
	XLOG_WARNING("Unblocking input queue, output queue lo water mark "
		     "reached.");
    }
}

void
FinderTcpMessengerBase::drain_queue()
{
    while (false == _out_queue.empty()) {
	delete _out_queue.front();
    }
}

void
FinderTcpMessengerBase::write_event(int 		errval,
				const uint8_t*	data,
				uint32_t	data_bytes)
{
    XLOG_ASSERT(false == _out_queue.empty());
    if (errval != 0) {
	/* Tcp connection will be closed shortly */
	return;
    }
    assert(data == get_data(*_out_queue.front()));
    assert(data_bytes == get_data_bytes(*_out_queue.front()));
    delete _out_queue.front();
    _out_queue.pop_front();
    if (false == _out_queue.empty())
	push_queue();
}

void
FinderTcpMessengerBase::close_event()
{
    // XXX TODO 
}

