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

// $XORP: xorp/libxipc/finder_transport.hh,v 1.3 2003/01/28 00:42:24 hodson Exp $

#ifndef __LIBXORP_FINDER_TRANSPORT_HH__
#define __LIBXORP_FINDER_TRANSPORT_HH__

#include <list>

#include "libxorp/eventloop.hh"
#include "libxorp/callback.hh"
#include "finder_msg.hh"

/**
 * Base class for FinderMessage transport systems.
 */
class FinderTransport {
public:

    typedef ref_ptr<FinderTransport> RefPtr;

    /**
     * Type of callback to be invoked when a FinderMessage arrives.
     * The callback is invoked with the string representation of the
     * FinderMessage (as much information as can be gleaned from the
     * static FinderParser methods - FinderTransport is independent of
     * Parser).
     */
    typedef XorpCallback2<void, const FinderTransport&, const string&>::RefPtr ACallback;

    /**
     * Type of callback to be invoked when a FinderMessage has been sent.
     */
    typedef XorpCallback2<void,
	const FinderTransport&,
	const FinderMessage::RefPtr&>::RefPtr DCallback;

    /**
     * Type of callback to be invoked when the underlying transport fails
     * catastrophically.
     */
    typedef XorpCallback1<void, const FinderTransport&>::RefPtr FCallback;

    /**
     * Constructor
     *
     * @param arrive_cb callback to be invoked when a FinderMessage arrives.
     *
     * @param depart_cb callback to be invoked when a FinderMessage has been
     * successfully written.
     *
     * @param failure_cb callback to be invoked with underlying transport
     * mechanism fails.
     *
     * @param hmac @ref HMAC object to be used to sign messages.
     */
    FinderTransport(const ACallback& arrive_cb = callback(no_arrival_callback),
		    const DCallback& departure_cb = callback(no_departure_callback),
		    const FCallback& failure_cb = callback(no_failure_callback),
		    const HMAC* hmac = 0)
	: _acb(arrive_cb), _dcb(departure_cb), _fcb(failure_cb), _hmac(NULL)
    {
	set_hmac(hmac);
    }

    virtual ~FinderTransport()
    {
	set_hmac(0);
    }

    /**
     * Install a HMAC object for use when sending messages.
     */
    inline void set_hmac(const HMAC* hmac) {
	if (_hmac) delete _hmac;
	_hmac = hmac ? hmac->clone() : 0;
    }

    /**
     * Set arrival callback.
     */
    inline void set_arrival_callback(const ACallback& acb)	{ _acb = acb; }

    /**
     * Set departure callback.
     */
    inline void set_departure_callback(const DCallback& dcb)	{ _dcb = dcb; }

    /**
     * Set failure callback.
     */
    inline void set_failure_callback(const FCallback& fcb)	{ _fcb = fcb; }

    /**
     * Check if all the requisite event callbacks have been set.  The
     * transport cannot function correctly without the callbacks being set.
     *
     * @return true if arrival, departure, and failure callbacks have
     * been set (with set_arrival_callback, set_departure_callback,
     * set_failure_callback, or via constructor).
     */
    inline bool callbacks_set() const {
	return _acb != callback(no_arrival_callback)   &&
	       _dcb != callback(no_departure_callback) &&
	       _fcb != callback(no_failure_callback);
    }

    /**
     * Place FinderMessage on outbound message queue.
     *
     * @param m refpointer to the FinderMessage to be sent.
     *
     * @param dcb callback to be invoked when message successfully written.
     *
     * @return sequence number assigned to message.
     */
    inline uint32_t write(const FinderMessage::RefPtr& m) {
	m->set_seqno(_dseq++);
	_departures.push_back(m);
	push_departures();
	return m->seqno();
    }

    /**
     * Examine installed hmac.
     *
     * @return pointer HMAC if used, 0 otherwise.
     */
    inline const HMAC* hmac() const {
	return _hmac;
    }

protected:
    /**
     * Method to be implemented by sub-classes to push-start
     * departures.
     */
    virtual void push_departures() = 0;

    /**
     * Method that transport implementations call when a message
     * arrives.
     */
    inline void announce_arrival(const string& s) {
	_acb->dispatch(*this, s);
    }

    /**
     * Method that transport implementations call when a message departs.
     * Notifies client via departure callback that message has been
     * sent and removes message state.
     */
    inline void announce_departure(const FinderMessage::RefPtr& m) {
	assert(_departures.front() == m);
	_dcb->dispatch(*this, m);
	_departures.pop_front();
    }

    /**
     * @return message at head of departure list.
     */
    inline const FinderMessage::RefPtr departure_head() const {
	return _departures.front();
    }

    inline const int departures_waiting() const {
	return _departures.size();
    }

    /**
     * Method transport implmentations call when transport fails.
     */
    inline void fail() const {
	_fcb->dispatch(*this);
    }

    /**
     * Callback function invoked when no user specified arrival callback
     * provided.
     */
    static void no_arrival_callback(const FinderTransport&,
				    const string&);

    /**
     * Callback function invoked when no user specified departure callback
     * provided.
     */
    static void no_departure_callback(const FinderTransport&,
				      const FinderMessage::RefPtr&);

    /**
     * Callback function invoked when no user specified failure callback
     * provided.
     */
    static void no_failure_callback(const FinderTransport&);

private:
    ACallback			_acb;		// arrival callback
    DCallback			_dcb;		// departure callback
    FCallback			_fcb;		// failure callback
    list<FinderMessage::RefPtr>	_departures;	// departure list
    uint32_t			_dseq;		// departure sequence number
    HMAC*			_hmac;		// message hmac'er
};

/**
 * Base class for FinderTransport server factories.
 */
class FinderTransportServerFactory {
public:
    /**
     * Callback object type.  Callback takes a pointer to the newly
     * created FinderTransport object as an argument.
     */
    typedef XorpCallback1<void, FinderTransport::RefPtr&>::RefPtr ConnectCallback;

    /**
     * Constructor
     *
     * @param cb callback to be invoked when a new FinderTransport
     * object is instantiated by the factory.  The instantiation
     * callback MUST add arrival, departure, and failure callbacks
     * to the FinderTransport object passed.
     */
    FinderTransportServerFactory(const ConnectCallback& cb) :
	_connect_cb(cb) {}

    /**
     * Destructor
     */
    virtual ~FinderTransportServerFactory() {}

protected:
    ConnectCallback		_connect_cb;

    /**
     * Called by derived classed when a FinderTransport is created.
     */
    inline void announce_connect(FinderTransport::RefPtr& ftr) {
	// Create a ref_ptr to object so if user does create copy
	// of ref_ptr(new_ft) then it is automatically deleted here.
	_connect_cb->dispatch(ftr);
    }
};

static const uint16_t FINDER_TCP_DEFAULT_PORT = 19999;

/**
 * Factory class for FinderTransport objects on a server.
 */
class FinderTcpServerFactory : public FinderTransportServerFactory {
public:

    struct BadPort : public XorpReasonedException {
	BadPort(const char* file, size_t line, const string& why = "") :
	    XorpReasonedException("BadPort", file, line, why) {}
    };

    /**
     * Constructor
     *
     * @param e EventLoop used by system.
     *
     * @param port Port to listen for incoming connections on.
     *
     * @param Callback object to invoke when connection arrives.
     */
    FinderTcpServerFactory(EventLoop&				e,
			   int					port,
			   const ConnectCallback&		cb)
	throw (BadPort);

    /**
     * Destructor
     */
    ~FinderTcpServerFactory();

    /**
     * Not Implemented.
     */
    FinderTcpServerFactory(const FinderTcpServerFactory&);

    /**
     * Not Implemented.
     */
    FinderTcpServerFactory& operator=(const FinderTcpServerFactory&);

protected:
    EventLoop&	_event_loop;
    int		_fd;

    void connect(int fd, SelectorMask m);

    inline EventLoop& eventloop() const
    {
	return _event_loop;
    }
};

/**
 * Base class for FinderTransport client factories.  Client factories
 * are asynchronous objects that attempt to create a FinderTransport
 * instance when run() is called.  They continue to in the attempt
 * until a FinderTransport instance is successfully instantiated.  At
 * which point they invoke the ConnectCallback and stop future attempts.
 */
class FinderTransportClientFactory {
public:
    virtual ~FinderTransportClientFactory() {}

    /**
     * Callback object type.  Callback takes a pointer to the newly
     * created FinderTransport object as an argument.
     */
    typedef
    XorpCallback1<void, FinderTransport::RefPtr&>::RefPtr ConnectCallback;

    /**
     * Repeatedly attempt to instantiate a FinderTransport and call
     * ConnectCallback upon success.  Ceases to attempt to instantiate
     * FinderTransport when success achieved.  It is an error to call
     * run() if factory is already running.
     */
    virtual void run(const ConnectCallback& cb) = 0;

    /**
     * @return whether factory is attempting to instantiate a transport.
     */
    virtual bool running() const = 0;

    /**
     * Force factory to stop try to instantiate FinderTransport.
     */
    virtual void stop() = 0;
};

class FinderTcpClientFactory : public FinderTransportClientFactory {
public:
    struct BadDest : public XorpReasonedException {
	BadDest(const char* file, size_t line, const string& why = "") :
	    XorpReasonedException("BadDest", file, line, why) {}
    };

    FinderTcpClientFactory(EventLoop& e,
			   const char* addr = "localhost",
			   int port = FINDER_TCP_DEFAULT_PORT) throw (BadDest);

    void run(const ConnectCallback& cb);
    bool running() const;
    void stop();

private:
    EventLoop&	_e;
    const char*	_addr;
    int		_port;

    XorpTimer		_connect_timer;
    ConnectCallback	_connect_cb;

    bool connect();
};

#endif // __LIBXORP_FINDER_TRANSPORT_HH__

