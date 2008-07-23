// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/libxorp/service.hh,v 1.12 2008/01/04 03:16:39 pavlin Exp $

#ifndef __LIBXORP_SERVICE_HH__
#define __LIBXORP_SERVICE_HH__

/**
 * Enumeration of states objects derived from ServiceBase may be in.
 */
enum ServiceStatus {
    SERVICE_READY	= 0x001,	// Ready for startup
    SERVICE_STARTING	= 0x002,	// Starting up
    SERVICE_RUNNING	= 0x004,	// Running, service operational
    SERVICE_PAUSING	= 0x008,	// Transitioning to paused state
    SERVICE_PAUSED	= 0x010,	// Paused, non-operational
    SERVICE_RESUMING	= 0x020,	// Resuming from pause
    SERVICE_SHUTTING_DOWN = 0x040,	// Transitioning to shutdown
    SERVICE_SHUTDOWN	= 0x080,	// Shutdown, non-operational
    SERVICE_FAILED	= 0x100,	// Failed, non-operational
    SERVICE_ALL		= SERVICE_READY |
			  SERVICE_STARTING |
			  SERVICE_RUNNING |
    			  SERVICE_PAUSING |
			  SERVICE_PAUSED |
			  SERVICE_RESUMING |
			  SERVICE_SHUTTING_DOWN |
			  SERVICE_SHUTDOWN |
			  SERVICE_FAILED
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
    ServiceBase(const string& name = "Unknown");

    virtual ~ServiceBase() = 0;

    /**
     * Start service.  Service should transition from SERVICE_READY to
     * SERVICE_STARTING immediately and onto SERVICE_RUNNING or
     * SERVICE_FAILED in the near future.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int startup() = 0;

    /**
     * Shutdown service.  Service should transition from SERVICE_RUNNING to
     * SERVICE_SHUTTING_DOWN immediately and onto SERVICE_SHUTDOWN or
     * SERVICE_FAILED in the near future.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int shutdown() = 0;

    /**
     * Reset service.  Service should transition in SERVICE_READY from
     * whichever state it is in.
     *
     * The default implementation always returns false as there is no
     * default behaviour.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int reset();

    /**
     * Pause service.  Service should transition from SERVICE_RUNNING to
     * SERVICE_PAUSING and asynchronously into SERVICE_PAUSED.
     *
     * The default implementation always returns false as there is no
     * default behaviour.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int pause();

    /**
     * Resume paused service.  Service should transition from SERVICE_PAUSED
     * to SERVICE_PAUSING and asynchronously into SERVICE_RUNNING.
     *
     * The default implementation always returns false as there is no
     * default behaviour.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int resume();

    /**
     * Get name of service.
     *
     * @return name of service.  May be empty if not set in constructor.
     */
    const string& service_name() const		{ return _name; }

    /**
     * Get the current status.
     */
    ServiceStatus status() const		{ return _status; }

    /**
     * Get annotation associated with current status.  The annotation when
     * set is an explanation of the state, ie "waiting for Y"
     */
    const string& status_note() const		{ return _note; }

    /**
     * Get a character representation of the current service status.
     */
    const char* status_name() const;

    /**
     * Set service status change observer.  The observer will receive
     * synchronous notifications of changes in service state.
     *
     * @param so service change observer to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_observer(ServiceChangeObserverBase* so);

    /**
     * Remove service status change observer.
     *
     * @param so observer to remove.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unset_observer(ServiceChangeObserverBase* so);

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
    string			_name;
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
