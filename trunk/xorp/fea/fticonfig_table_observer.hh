// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/fea/fticonfig_table_observer.hh,v 1.8 2004/06/02 22:52:37 pavlin Exp $

#ifndef __FEA_FTICONFIG_TABLE_OBSERVER_HH__
#define __FEA_FTICONFIG_TABLE_OBSERVER_HH__

#include "netlink_socket.hh"
#include "routing_socket.hh"


class FtiConfig;
class FibTableObserverBase;

class FtiConfigTableObserver {
public:
    FtiConfigTableObserver(FtiConfig& ftic);
    
    virtual ~FtiConfigTableObserver();
    
    FtiConfig&	ftic() { return _ftic; }
    
    virtual void register_ftic();

    /**
     * Start operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start() = 0;
    
    /**
     * Stop operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop() = 0;
    
    /**
     * Receive data from the underlying system.
     * 
     * @param data the buffer with the received data.
     * @param nbytes the number of bytes in the data buffer @ref data.
     */
    virtual void receive_data(const uint8_t* data, size_t nbytes) = 0;

    /**
     * Add a FIB table observer.
     * 
     * @param fib_table_observer the FIB table observer to add.
     */
    void add_fib_table_observer(FibTableObserverBase* fib_table_observer);

    /**
     * Delete a FIB table observer.
     * 
     * @param fib_table_observer the FIB table observer to delete.
     */
    void delete_fib_table_observer(FibTableObserverBase* fib_table_observer);

protected:
    /**
     * Propagate FIB changes to all FIB table observers.
     * 
     * @param fte_list the list with the FIB changes.
     */
    void propagate_fib_changes(const list<FteX>& fte_list);

    list<FibTableObserverBase* > _fib_table_observers;

    // Misc other state
    bool	_is_running;

private:
    FtiConfig&	_ftic;
};

class FtiConfigTableObserverDummy : public FtiConfigTableObserver {
public:
    FtiConfigTableObserverDummy(FtiConfig& ftic);
    virtual ~FtiConfigTableObserverDummy();

    /**
     * Start operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start();
    
    /**
     * Stop operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop();
    
    /**
     * Receive data from the underlying system.
     * 
     * @param data the buffer with the received data.
     * @param nbytes the number of bytes in the data buffer @ref data.
     */
    virtual void receive_data(const uint8_t* data, size_t nbytes);
    
private:
    
};

class FtiConfigTableObserverRtsock : public FtiConfigTableObserver,
				     public RoutingSocket,
				     public RoutingSocketObserver {
public:
    FtiConfigTableObserverRtsock(FtiConfig& ftic);
    virtual ~FtiConfigTableObserverRtsock();

    /**
     * Start operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start();
    
    /**
     * Stop operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop();
    
    /**
     * Receive data from the underlying system.
     * 
     * @param data the buffer with the received data.
     * @param nbytes the number of bytes in the data buffer @ref data.
     */
    virtual void receive_data(const uint8_t* data, size_t nbytes);
    
    void rtsock_data(const uint8_t* data, size_t nbytes);
    
private:
    
};

class FtiConfigTableObserverNetlink : public FtiConfigTableObserver,
				      public NetlinkSocket4,
				      public NetlinkSocket6,
				      public NetlinkSocketObserver {
public:
    FtiConfigTableObserverNetlink(FtiConfig& ftic);
    virtual ~FtiConfigTableObserverNetlink();

    /**
     * Start operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start();
    
    /**
     * Stop operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop();
    
    /**
     * Receive data from the underlying system.
     * 
     * @param data the buffer with the received data.
     * @param nbytes the number of bytes in the data buffer @ref data.
     */
    virtual void receive_data(const uint8_t* data, size_t nbytes);
    
    void nlsock_data(const uint8_t* data, size_t nbytes);
    
private:
    
};

/**
 * A base class that can be used by clients interested in observing
 * changes in the Forwarding Information Base.
 */
class FibTableObserverBase {
public:
    FibTableObserverBase() {}
    virtual ~FibTableObserverBase() {}

    /**
     * Process a list of IPv4 FIB route changes.
     * 
     * The FIB route changes come from the underlying system.
     * 
     * @param fte_list the list of Fte entries to add or delete.
     */
    virtual void process_fib_changes(const list<Fte4>& fte_list) = 0;

    /**
     * Process a list of IPv6 FIB route changes.
     * 
     * The FIB route changes come from the underlying system.
     * 
     * @param fte_list the list of Fte entries to add or delete.
     */
    virtual void process_fib_changes(const list<Fte6>& fte_list) = 0;

private:
};

#endif // __FEA_FTICONFIG_TABLE_OBSERVER_HH__
