// -*- c-basic-offset: 4 -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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
#include "xrlcmdmap.hh"
#include "xrlpf.hh"
#include "finder-client.hh"

class DispatchState;

/**
 * @short XRL transmission and reception point.
 *
 * An XrlRouter is responsible for sending and receiving XRL requests
 * for entities in a XORP Router.  A single process may have multiple
 * XrlRouter objects.  In this case each XrlRouter object corresponds
 * to an independent entity within the process.  
 */
class XrlRouter : public XrlCmdMap {
public:
    /**
     * Constructor for when the Finder is running on the local host.
     *
     * @param event_loop the EventLoop that the XrlRouter should associate 
     * itself with.
     * @param entity_name the name this router will register with the Finder.
     */
    XrlRouter(EventLoop& event_loop, const char* entity_name)
	: XrlCmdMap(entity_name), _event_loop(event_loop), _fc(event_loop), 
	_sends_pending(0), _finder_lookups_pending(0) {}

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
	: XrlCmdMap(entity_name), _event_loop(event_loop),
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
	: XrlCmdMap(entity_name), _event_loop(event_loop), 
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

    typedef XorpCallback4<void, const XrlError&, XrlRouter&, const Xrl&, XrlArgs*>::RefPtr XrlCallback;

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


    // Deprecated callback typedef
    typedef void (*ResponseCallback)(const XrlError& 	e,
				     XrlRouter&		router,
				     const Xrl&		request,
				     XrlArgs*		response,
				     void*		cookie);
    /**
     * [Deprecated] Dispatch an Xrl.
     *
     * NB A callback must be supplied if the Xrl returns a value.  If
     * the Xrl returns nothing, a callback is only necessary if the
     * dispatcher wants to know that the Xrl dispatch was successful
     * and completed.
     *
     * @param xrl The Xrl to be dispatched.  
     * @param user_callback invoked when the Xrl has returns or fails.  
     * @param cookie to be passed to the user_callback.  
     *
     * @return true if sufficient resources available, false otherwise.
     */
    inline bool
    send (const Xrl& 		xrl,
	  ResponseCallback	user_callback, 
	  void*			cookie = 0)
    {
	return send(xrl, callback(user_callback, cookie));
    }

    /**
     * Returns true if this router has any pending actions.
     */
    bool pending() const {
	return 0 != _sends_pending || 0 != _finder_lookups_pending;
    }

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

    static void		resolve_callback(FinderClientError, 
					 const char*, const char*, void*);

    static void 	send_callback(const XrlError&, const Xrl&, 
				      XrlArgs*, void*);

    static void		finder_register_callback(FinderClientError,
						 const char*, const char*,
						 void *thunk);
};

#endif // __XRLROUTER_HH__
