// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2008 International Computer Science Institute
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

// $XORP: xorp/fea/fea_io.hh,v 1.4 2007/12/12 03:50:08 pavlin Exp $


#ifndef __FEA_FEA_IO_HH__
#define __FEA_FEA_IO_HH__


//
// FEA (Forwarding Engine Abstraction) I/O interface.
//

#include <list>

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
