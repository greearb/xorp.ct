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

// $XORP$

#ifndef __LIBXORP_SERVICE_HH__
#define __LIBXORP_SERVICE_HH__

/**
 * Enumeration of states objects derived from ServiceBase may be in.
 */
enum ServiceStatus {
    READY		= 0x001,	// Ready for startup
    STARTING		= 0x002,	// Starting up
    RUNNING		= 0x004,	// Running, service operational
    PAUSING		= 0x008,	// Transitioning to paused state
    PAUSED		= 0x010,	// Paused, non-operational
    RESUMING		= 0x020,	// Resuming from pause
    SHUTTING_DOWN	= 0x040,	// Transitioning to shutdown
    SHUTDOWN		= 0x080,	// Shutdown, non-operational
    FAILED		= 0x100,	// Failed, non-operational
    ALL			= READY | STARTING | RUNNING |
    			  PAUSING | PAUSED | RESUMING |
			  SHUTTING_DOWN | SHUTDOWN | FAILED
};

/**
 * Get text description of enumerated service status.
 *
 * @param s service status to recover name for.
 */
const char* service_status_name(ServiceStatus s);

class ServiceChangeObserverBase;

/**
 * @short Base class for Services.
 *
 * This class provides a base for services within Xorp processes.  A
 * service instance is an entity that can logically started and
 * stopped and typically needs some asynchronous processing in order
 * to start and stop.  An example service within a routing process
 * would be a RIB communicator service, which needs to co-ordinate
 * with the RIB which is within a different process and may be on a
 * different machine.
 *
 * A service may be started and shutdown by calling @ref startup() and
 * @ref shutdown().  The status of a service may be determined by
 * calling @ref status().  Additional notes on the current status may be
 * obtained by calling @ref status_note().
 *
 * Synchronous service status changes may be received through the @ref
 * ServiceChangeObserverBase class.  Instances of objects derived from
 * this class can register for status change notifications in a
 * Service instance by calling @ref set_observer().
 */
class ServiceBase {
public:
    ServiceBase();

    virtual ~ServiceBase() = 0;

    /**
     * Start service.  Service should transition from READY to
     * STARTING immediately and onto RUNNING or FAILED in the near
     * future.
     */
    virtual void startup() = 0;

    /**
     * Shutdown service.  Service should transition from RUNNING to
     * SHUTTING_DOWN immediately and onto SHUTDOWN or FAILED in the
     * near future.
     */
    virtual void shutdown() = 0;

    /**
     * Reset service.  Service should transition in READY from
     * whichever state it is in.
     *
     * The default implementation always returns false as there is no
     * default behaviour.
     *
     * @return true on success, false on failure.
     */
    virtual bool reset();

    /**
     * Pause service.  Service should transition from RUNNING to
     * PAUSING and asynchronously into PAUSED.
     *
     * The default implementation always returns false as there is no
     * default behaviour.
     *
     * @return true on success, false on failure.
     */
    virtual bool pause();

    /**
     * Resume paused service.  Service should transition from PAUSED
     * to PAUSING and asynchronously into RUNNING.
     *
     * The default implementation always returns false as there is no
     * default behaviour.
     *
     * @return true on success, false on failure.
     */
    virtual bool resume();

    /**
     * Get the current status.
     */
    inline ServiceStatus status() const			{ return _status; }

    /**
     * Get annotation associated with current status.  The annotation when
     * set is an explanation of the state, ie "waiting for Y"
     */
    inline const string& status_note() const		{ return _note; }

    /**
     * Get a character representation of the current service status.
     */
    const char* status_name() const;

    /**
     * Set service status change observer.  The observer will receive
     * synchronous notifications of changes in service state.
     *
     * @param so service change observer to add.
     * @return true on success, false if an observer is already set.
     */
    bool set_observer(ServiceChangeObserverBase* so);

    /**
     * Remove service status change observer.
     *
     * @param so observer to remove.
     * @return true on success, false if supplied observer does match
     * the last set observer.
     */
    bool unset_observer(ServiceChangeObserverBase* so);

protected:
    /**
     * Set current status.
     *
     * @param status new status.
     * @param note comment on new service status.
     */
    void set_status(ServiceStatus status, const string& note);

    /**
     * Set current status and clear status note.
     *
     * @param status new status.
     */
    void set_status(ServiceStatus status);

protected:
    ServiceStatus	 	_status;
    string		 	_note;
    ServiceChangeObserverBase*	_observer;
};

/**
 * @short Base class for service status change observer.
 */
class ServiceChangeObserverBase {
public:
    virtual ~ServiceChangeObserverBase() = 0;

    virtual void status_change(ServiceBase*  service,
			       ServiceStatus old_status,
			       ServiceStatus new_status) = 0;
};

/**
 * @short Selective Change Observer.
 *
 * Forwards limited subset of status changes to a status change observer.
 */
class ServiceFilteredChangeObserver
    : public ServiceChangeObserverBase {
public:
    /**
     * Constructor.
     *
     * Only changes from the states represented in @ref from_mask to
     * the states represented in @ref to_mask are reported.
     *
     * @param child recipient of status changes.
     * @param from_mask mask of states left to trigger changes.
     * @param to_mask mask of states entered to trigger changes.
     */
    ServiceFilteredChangeObserver(ServiceChangeObserverBase*	child,
				  ServiceStatus			from_mask,
				  ServiceStatus			to_mask);

protected:
    void status_change(ServiceBase*	service,
		       ServiceStatus	old_status,
		       ServiceStatus	new_status);

protected:
    ServiceChangeObserverBase*	_child;
    ServiceStatus		_from_mask;
    ServiceStatus		_to_mask;
};

#endif // __LIBXORP_SERVICE_HH__
