// -*- c-basic-offset: 4 -*-

// Copyright (c) 2001-2003 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the Xorp LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the Xorp LICENSE file; the license in that file is
// legally binding.

// $Id$

#ifndef __XRLROUTER_HH__
#define __XRLROUTER_HH__

#include "config.h"
#include "libxorp/callback.hh"

#include "xrl.hh"
#include "xrl_sender.hh"
#include "xrl_cmd_map.hh"
#include "xrl_pf.hh"

class DispatchState;

class XrlCmdDispatcher : public XrlCmdMap {
public:
    XrlCmdDispatcher(const char* entity_name) : XrlCmdMap(entity_name)
    {}
    virtual ~XrlCmdDispatcher() {}
    virtual XrlError dispatch_xrl(const Xrl& xrl, XrlArgs& ret_vals) const;
};

#ifdef ORIGINAL_FINDER
#include "finder_client.hh"

/**
 * @short XRL transmission and reception point.
 *
 * An XrlRouter is responsible for sending and receiving XRL requests
 * for entities in a XORP Router.  A single process may have multiple
 * XrlRouter objects.  In this case each XrlRouter object corresponds
 * to an independent entity within the process.
 */
class XrlRouter : public XrlCmdDispatcher, public XrlSender {
public:
    /**
     * Constructor for when the Finder is running on the local host.
     *
     * @param event_loop the EventLoop that the XrlRouter should associate
     * itself with.
     * @param entity_name the name this router will register with the Finder.
     */
    XrlRouter(EventLoop& event_loop, const char* entity_name)
	: XrlCmdDispatcher(entity_name), _event_loop(event_loop),
	  _fc(event_loop), _sends_pending(0), _finder_lookups_pending(0) {}

    /**
     * Constructor for when the Finder is running on another host on
     * the default Finder port.
     *
     * @param event_loop the EventLoop that the XrlRouter should associate
     * itself with.
     * @param entity_name the name this router will register with the Finder.
     * @param finder_address the hostname running the Finder process.
     */
    XrlRouter(EventLoop& event_loop,
	      const char* entity_name,
	      const char* finder_address)
	: XrlCmdDispatcher(entity_name), _event_loop(event_loop),
	  _fc(event_loop, finder_address),
	  _sends_pending(0), _finder_lookups_pending(0) {}

    /**
     * Constructor for when the Finder is running on another host on
     * and listening on a custom port.
     *
     * @param event_loop the EventLoop that the XrlRouter should associate
     * itself with.
     * @param entity_name the name this router will register with the Finder.
     * @param finder_address the hostname running the Finder process.
     * @param finder_port the port the Finder process is listening on.
     */
    XrlRouter(EventLoop& event_loop,
	      const char* entity_name,
	      const char* finder_address, uint16_t finder_port)
	: XrlCmdDispatcher(entity_name), _event_loop(event_loop),
	_fc(event_loop, finder_address, finder_port),
	_sends_pending(0), _finder_lookups_pending(0) {}

    virtual ~XrlRouter();

    /**
     * Add a protocol family listener.
     *
     * This call associates the entity name of the XrlRouter with the
     * protocol family listener at the Finder.  Other processes may
     * then contact this XrlRouter through the Listener by resolving
     * the entity name.
     *
     * Registration may fail if the entity name of the XrlRouter
     * object is already in use at the Finder, or if the Finder is not
     * contactable.
     *
     * @return true if registration succeeds, false otherwise.
     */
    bool add_listener (XrlPFListener* pf);

    /**
     * Signal end of listener and additions to Xrl Command map.
     */
    void finalize();
    
    typedef XrlSender::Callback XrlCallback;

    /**
     * Dispatch an Xrl.
     *
     * NB A callback must be supplied if the Xrl returns a value.  If
     * the Xrl returns nothing, a callback is only necessary if the
     * dispatcher wants to know that the Xrl dispatch was successful
     * and completed.
     *
     * @param xrl The Xrl to be dispatched.
     * @param callback invoked when the Xrl has returns or fails.
     *
     * @return true if sufficient resources available, false otherwise.
     */
    bool send (const Xrl& xrl, const XrlCallback& callback);

    /**
     * Returns true if this router has any pending actions.
     */
    bool pending() const; 

    /**
     * Assignment operator (unimplemented and compiler generated not wanted).
     */
    XrlRouter& operator=(const XrlRouter&);

    /**
     * Copy constructor (unimplemented and compiler generated not wanted).
     */
    XrlRouter(const XrlRouter&);

    /**
     * Check if XrlRouter has connection to finder, ie is operational.
     *
     * @return true if connected to finder, false if attempting to connect
     *         to finder.
     */
    inline bool connected() const {
	return _fc.connected();
    }
    
private:
    EventLoop&		_event_loop;

    list <XrlPFListener*> _listeners;
    typedef list<XrlPFListener*>::iterator LI;

    FinderClient 	_fc;

    // The number of incomplete send transactions in progress
    int32_t 		_sends_pending;

    // The number of unanswered resolve requests
    int32_t 		_finder_lookups_pending;

    static void		resolve_callback(FinderClient::Error,
					 const char*, const char*,
					 DispatchState*);

    static void 	send_callback(const XrlError&,
				      const Xrl&,
				      XrlArgs*,
				      DispatchState*);

    static void		finder_register_callback(FinderClient::Error,
						 const char*, const char*,
						 int *success);
};
#else

class FinderNGClient;
class FinderNGClientXrlTarget;
class FinderNGTcpAutoConnector;
class FinderDBEntry;
class XrlRouterDispatchState;

class XrlRouter : public XrlCmdDispatcher, public XrlSender {
public:
    typedef XrlSender::Callback XrlCallback;    
    typedef XrlRouterDispatchState DispatchState;
    
public:
    XrlRouter(EventLoop&	e,
	      const char*	entity_name,
	      const char*	finder_address = "localhost",
	      uint16_t		finder_port = 0)
	throw (InvalidAddress);

    virtual ~XrlRouter();
    
    bool add_listener(XrlPFListener* listener);

    void finalize();
    
    bool connected() const;

    bool pending() const;

    bool send(const Xrl& xrl, const XrlCallback& cb);

    bool add_handler(const string& cmd, const XrlRecvCallback& rcb);

protected:
    void resolve_callback(const XrlError&		e,
			  const FinderDBEntry*		dbe,
			  XrlRouterDispatchState*	ds);

    void send_callback(const XrlError&		e,
		       const Xrl&		xrl,
		       XrlArgs*			reply,
		       XrlRouterDispatchState*	ds);

    void dispose(XrlRouterDispatchState*);
    
protected:
    EventLoop&			_e;
    FinderNGClient*		_fc;
    FinderNGClientXrlTarget*	_fxt;
    FinderNGTcpAutoConnector*	_fac;
    uint32_t			_id;

    uint32_t			_rpend, _spend;

    list<XrlPFListener*>	_listeners;
    list<XrlRouterDispatchState*> _dsl;
};

#endif // ORIGINAL_FINDER

#endif // __XRLROUTER_HH__
