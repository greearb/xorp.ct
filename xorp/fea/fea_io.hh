// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2011 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#ifndef __FEA_FEA_IO_HH__
#define __FEA_FEA_IO_HH__


//
// FEA (Forwarding Engine Abstraction) I/O interface.
//



class EventLoop;
class FeaNode;
class InstanceWatcher;

/**
 * @short FEA (Forwarding Engine Abstraction) I/O class.
 */
class FeaIo {
public:
    /**
     * Constructor.
     *
     * @param eventloop the event loop to use.
     */
    FeaIo(EventLoop& eventloop);

    /**
     * Destructor
     */
    virtual	~FeaIo();

    /**
     * Startup the service operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		startup();

    /**
     * Shutdown the service operation.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		shutdown();

    /**
     * Test whether the service is running.
     *
     * @return true if the service is still running, otherwise false.
     */
    bool	is_running() const;

    /**
     * Get the event loop this service is added to.
     *
     * @return the event loop this service is added to.
     */
    EventLoop& eventloop() { return (_eventloop); }

    /**
     * Add a watcher for the status of a component instance.
     *
     * @param instance_name the name of the instance to watch.
     * @param instance_watcher the watcher that tracks the status of the
     * instance.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_instance_watch(const string& instance_name,
				   InstanceWatcher* instance_watcher,
				   string& error_msg);

    /**
     * Delete a watcher for the status of a component instance.
     *
     * @param instance_name the name of the instance to stop watching.
     * @param instance_watcher the watcher that tracks the status of the
     * instance.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_instance_watch(const string& instance_name,
				      InstanceWatcher* instance_watcher,
				      string& error_msg);

    /**
     * A component instance has been born.
     *
     * @param instance_name the name of the instance that is born.
     */
    virtual void instance_birth(const string& instance_name);

    /**
     * A component instance has died.
     *
     * @param instance_name the name of the instance that has died.
     */
    virtual void instance_death(const string& instance_name);

protected:
    /**
     * Register interest in events relating to a particular instance.
     *
     * @param instance_name name of target instance to receive event
     * notifications for.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int register_instance_event_interest(const string& instance_name,
						 string& error_msg) = 0;

    /**
     * Deregister interest in events relating to a particular instance.
     *
     * @param instance_name name of target instance to stop event
     * notifications for.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int deregister_instance_event_interest(const string& instance_name,
						   string& error_msg) = 0;

private:
    EventLoop&	_eventloop;	// The event loop to use
    bool	_is_running;	// True if the service is running

    list<pair<string, InstanceWatcher *> > _instance_watchers;
};

//

/**
 * @short Instance watcher base class.
 *
 * This is a base class used by other components to add/delete interest
 * in watching the status of a component instance.
 */

class InstanceWatcher {
public:
    virtual ~InstanceWatcher() {}

    /**
     * Inform the watcher that a component instance is alive.
     *
     * @param instance_name the name of the instance that is alive.
     */
    virtual void instance_birth(const string& instance_name) = 0;

    /**
     * Inform the watcher that a component instance is dead.
     *
     * @param instance_name the name of the instance that is dead.
     */
    virtual void instance_death(const string& instance_name) = 0;
};

#endif // __FEA_FEA_IO_HH__
