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

#ident "$XORP: xorp/libxipc/finder_tcp_messenger.cc,v 1.13 2003/06/02 17:22:21 hodson Exp $"

#include "config.h"
#include "finder_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libcomm/comm_api.h"

#include "finder_tcp_messenger.hh"

FinderTcpMessenger::FinderTcpMessenger(EventLoop&		e,
				       FinderMessengerManager*	mm,
				       int			fd,
				       XrlCmdMap&		cmds)
    : FinderMessengerBase(e, mm, cmds), FinderTcpBase(e, fd)
{
    if (manager())
	manager()->messenger_birth_event(this);
}

FinderTcpMessenger::~FinderTcpMessenger()
{
    if (manager())
	manager()->messenger_death_event(this);
    drain_queue();
}

void
FinderTcpMessenger::read_event(int	      errval,
			       const uint8_t* data,
			       uint32_t	      data_bytes)
{
    if (errval != 0) {
	/* An error has occurred, the FinderTcpBase class will close
	 * connection following this notification.
	 */
	debug_msg("Got errval %d, data %p, data_bytes %u\n",
		  errval, data, data_bytes);
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
    } catch (const XorpException& e) {
	ex = e.str();
    } catch (...) {
	ex = "Unexpected ?";
    }
    XLOG_ERROR("Got exception %s, closing connection", ex.c_str());
    close();
}

bool
FinderTcpMessenger::send(const Xrl& xrl, const SendCallback& scb)
{
#if 0
    if (pending()) {
	XLOG_FATAL("Hit pending");
	return false;
    }
#endif
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
FinderTcpMessenger::pending() const
{
    return (false == _out_queue.empty());
}

void
FinderTcpMessenger::reply(uint32_t	  seqno,
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
FinderTcpMessenger::push_queue()
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
FinderTcpMessenger::drain_queue()
{
    while (false == _out_queue.empty()) {
	delete _out_queue.front();
	_out_queue.pop_front();
    }
}

void
FinderTcpMessenger::write_event(int 		errval,
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
FinderTcpMessenger::close_event()
{
    if (manager())
	manager()->messenger_stopped_event(this);
}

void
FinderTcpMessenger::error_event()
{
    delete this;
}

///////////////////////////////////////////////////////////////////////////////
//
// FinderTcpListener methods
//

FinderTcpListener::FinderTcpListener(EventLoop&		     e,
				     FinderMessengerManager& mm,
				     XrlCmdMap&		     cmds,
				     IPv4		     interface,
				     uint16_t		     port,
				     bool		     en)
    throw (InvalidPort)
    : FinderTcpListenerBase(e, interface, port, en), _mm(mm), _cmds(cmds)
{
}

FinderTcpListener::~FinderTcpListener()
{
}

bool
FinderTcpListener::connection_event(int fd)
{
    FinderTcpMessenger* m =
	new FinderTcpMessenger(eventloop(), &_mm, fd, _cmds);
    // Check if manager has taken responsibility for messenger and clean up if
    // not.
    if (_mm.manages(m) == false)
	delete m;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
// FinderTcpConnector methods
//

FinderTcpConnector::FinderTcpConnector(EventLoop&		e,
				       FinderMessengerManager&	mm,
				       XrlCmdMap&		cmds,
				       IPv4			host,
				       uint16_t 		port)
    : _e(e), _mm(mm), _cmds(cmds), _host(host), _port(port)
{}

int
FinderTcpConnector::connect(FinderTcpMessenger*& created_messenger)
{
    struct in_addr host_ia;
    host_ia.s_addr = _host.addr();

    int fd = comm_connect_tcp4(&host_ia, htons(_port));
    if (fd < 0) {
	created_messenger = 0;
	return errno;
    }

    created_messenger = new FinderTcpMessenger(_e, &_mm, fd, _cmds);
    debug_msg("Created messenger %p\n", created_messenger);
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// FinderTcpAutoConnector methods
//

FinderTcpAutoConnector::FinderTcpAutoConnector(
				   EventLoop&		   e,
				   FinderMessengerManager& real_manager,
				   XrlCmdMap&		   cmds,
				   IPv4			   host,
				   uint16_t		   port,
				   bool			   en)
    : FinderTcpConnector(e, *this, cmds, host, port),
      _real_manager(real_manager), _connected(false), _enabled(en),
      _last_error(0), _consec_error(0)
{
    if (en) {
	start_timer();
    }
}

FinderTcpAutoConnector::~FinderTcpAutoConnector()
{
    // Any existing messenger will die and we need to not restart the timer
    set_enabled(false);
}

void
FinderTcpAutoConnector::set_enabled(bool en)
{
    if (_enabled == en) {
	return;
    }

    _enabled = en;
    if (_connected) {
	// timer better not be running since we're connected
	XLOG_ASSERT(false == _retry_timer.scheduled());
	return;
    }

    if (false == _enabled) {
	stop_timer();
    } else {
	start_timer();
    }
}

bool
FinderTcpAutoConnector::enabled() const
{
    return _enabled;
}

bool
FinderTcpAutoConnector::connected() const
{
    return _connected;
}

void
FinderTcpAutoConnector::do_auto_connect()
{
    XLOG_ASSERT(false == _connected);

    FinderTcpMessenger* fm;
    int r = connect(fm);
    if (r == 0) {
	XLOG_ASSERT(fm != 0);
	_consec_error = 0;
	_connected = true;
    } else {
	XLOG_ASSERT(fm == 0);
	if (r != _last_error) {
	    XLOG_ERROR("Failed to connect to %s/%u: %s",
		       _host.str().c_str(), _port, strerror(r));
	    _consec_error = 0;
	} else if ((++_consec_error % CONNECT_FAILS_BEFORE_LOGGING) == 0) {
	    XLOG_ERROR("Failed %u times to connect to %s/%u: %s",
		       (uint32_t)_consec_error, _host.str().c_str(), _port,
		       strerror(r));
	    _consec_error = 0;
	}
	_connected = false;
	start_timer(CONNECT_RETRY_PAUSE_MS);
    }
    _last_error = r;
}

void
FinderTcpAutoConnector::start_timer(uint32_t ms)
{
    XLOG_ASSERT(false == _retry_timer.scheduled());
    XLOG_ASSERT(false == _connected);
    _retry_timer =
	_e.new_oneoff_after_ms(
	    ms, callback(this, &FinderTcpAutoConnector::do_auto_connect));
}

void
FinderTcpAutoConnector::stop_timer()
{
    _retry_timer.unschedule();
}

void
FinderTcpAutoConnector::messenger_birth_event(FinderMessengerBase* m)
{
    _real_manager.messenger_birth_event(m);
    set_enabled(false);
}

void
FinderTcpAutoConnector::messenger_death_event(FinderMessengerBase* m)
{
    _real_manager.messenger_death_event(m);
    _connected = false;
    if (_enabled)
	start_timer(CONNECT_RETRY_PAUSE_MS);
}

void
FinderTcpAutoConnector::messenger_active_event(FinderMessengerBase* m)
{
    _real_manager.messenger_active_event(m);

}
void
FinderTcpAutoConnector::messenger_inactive_event(FinderMessengerBase* m)
{
    _real_manager.messenger_inactive_event(m);
}

void
FinderTcpAutoConnector::messenger_stopped_event(FinderMessengerBase* m)
{
    _real_manager.messenger_stopped_event(m);
}

bool
FinderTcpAutoConnector::manages(const FinderMessengerBase* m) const
{
    return _real_manager.manages(m);
}

