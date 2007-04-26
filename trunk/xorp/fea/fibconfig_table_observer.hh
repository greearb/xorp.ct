// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/fea/fticonfig_table_observer.hh,v 1.20 2007/02/16 22:45:40 pavlin Exp $

#ifndef __FEA_FIBCONFIG_TABLE_OBSERVER_HH__
#define __FEA_FIBCONFIG_TABLE_OBSERVER_HH__

#include "netlink_socket.hh"
#include "routing_socket.hh"
#include "win_rtm_pipe.hh"


class FtiConfig;
class FibTableObserverBase;

class FtiConfigTableObserver {
public:
    FtiConfigTableObserver(FtiConfig& ftic);
    
    virtual ~FtiConfigTableObserver();
    
    FtiConfig&	ftic() { return _ftic; }
    
    virtual void register_ftic_primary();
    virtual void register_ftic_secondary();
    virtual void set_primary() { _is_primary = true; }
    virtual void set_secondary() { _is_primary = false; }
    virtual bool is_primary() const { return _is_primary; }
    virtual bool is_secondary() const { return !_is_primary; }
    virtual bool is_running() const { return _is_running; }

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg) = 0;
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg) = 0;
    
    /**
     * Receive data from the underlying system.
     * 
     * @param buffer the buffer with the received data.
     */
    virtual void receive_data(const vector<uint8_t>& buffer) = 0;

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
    bool	_is_primary;	// True -> primary, false -> secondary method
};

class FtiConfigTableObserverDummy : public FtiConfigTableObserver {
public:
    FtiConfigTableObserverDummy(FtiConfig& ftic);
    virtual ~FtiConfigTableObserverDummy();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);
    
    /**
     * Receive data from the underlying system.
     * 
     * @param buffer the buffer with the received data.
     */
    virtual void receive_data(const vector<uint8_t>& buffer);
    
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
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);
    
    /**
     * Receive data from the underlying system.
     * 
     * @param buffer the buffer with the received data.
     */
    virtual void receive_data(const vector<uint8_t>& buffer);
    
    void rtsock_data(const vector<uint8_t>& buffer);
    
private:
    
};


class FtiConfigTableObserverRtmV2 : public FtiConfigTableObserver {
public:
    FtiConfigTableObserverRtmV2(FtiConfig& ftic);
    virtual ~FtiConfigTableObserverRtmV2();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);
    
    /**
     * Receive data from the underlying system.
     * 
     * @param buffer the buffer with the received data.
     */
    virtual void receive_data(const vector<uint8_t>& buffer);
    
private:
    class RtmV2Observer : public WinRtmPipeObserver {
    public:
    	RtmV2Observer(WinRtmPipe& rs, int af,
		      FtiConfigTableObserverRtmV2& rtmo)
	    : WinRtmPipeObserver(rs), _af(af), _rtmo(rtmo) {}
    	virtual ~RtmV2Observer() {}
	void rtsock_data(const vector<uint8_t>& buffer) {
	    _rtmo.receive_data(buffer);
	}
    private:
	int _af;
    	FtiConfigTableObserverRtmV2& _rtmo;
    };
private:
    WinRtmPipe*		_rs4;
    RtmV2Observer*	_rso4;
    WinRtmPipe*		_rs6;
    RtmV2Observer*	_rso6;
};

class FtiConfigTableObserverNetlink : public FtiConfigTableObserver,
				      public NetlinkSocket,
				      public NetlinkSocketObserver {
public:
    FtiConfigTableObserverNetlink(FtiConfig& ftic);
    virtual ~FtiConfigTableObserverNetlink();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);
    
    /**
     * Receive data from the underlying system.
     * 
     * @param buffer the buffer with the received data.
     */
    virtual void receive_data(const vector<uint8_t>& buffer);
    
    void nlsock_data(const vector<uint8_t>& buffer);
    
private:
    
};

class FtiConfigTableObserverIPHelper : public FtiConfigTableObserver {
public:
    FtiConfigTableObserverIPHelper(FtiConfig& ftic);
    virtual ~FtiConfigTableObserverIPHelper();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);
    
    /**
     * Receive data from the underlying system.
     * 
     * @param buffer the buffer with the received data.
     */
    virtual void receive_data(const vector<uint8_t>& buffer);
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

#endif // __FEA_FIBCONFIG_TABLE_OBSERVER_HH__
