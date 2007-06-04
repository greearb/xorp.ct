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

// $XORP: xorp/fea/fibconfig_entry_observer.hh,v 1.4 2007/05/01 01:42:37 pavlin Exp $

#ifndef __FEA_FIBCONFIG_ENTRY_OBSERVER_HH__
#define __FEA_FIBCONFIG_ENTRY_OBSERVER_HH__

#include "fea/data_plane/control_socket/netlink_socket.hh"
#include "fea/data_plane/control_socket/routing_socket.hh"


class FibConfig;

class FibConfigEntryObserver {
public:
    FibConfigEntryObserver(FibConfig& fibconfig)
	: _is_running(false),
	  _fibconfig(fibconfig),
	  _is_primary(true)
    {}
    virtual ~FibConfigEntryObserver() {}
    
    FibConfig&	fibconfig() { return _fibconfig; }
    
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
    
protected:
    // Misc other state
    bool	_is_running;

private:
    FibConfig&	_fibconfig;
    bool	_is_primary;	// True -> primary, false -> secondary method
};

class FibConfigEntryObserverDummy : public FibConfigEntryObserver {
public:
    FibConfigEntryObserverDummy(FibConfig& fibconfig);
    virtual ~FibConfigEntryObserverDummy();

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

class FibConfigEntryObserverRtsock : public FibConfigEntryObserver,
				     public RoutingSocket,
				     public RoutingSocketObserver {
public:
    FibConfigEntryObserverRtsock(FibConfig& fibconfig);
    virtual ~FibConfigEntryObserverRtsock();

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

class FibConfigEntryObserverNetlink : public FibConfigEntryObserver,
				      public NetlinkSocket,
				      public NetlinkSocketObserver {
public:
    FibConfigEntryObserverNetlink(FibConfig& fibconfig);
    virtual ~FibConfigEntryObserverNetlink();

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

class FibConfigEntryObserverIPHelper : public FibConfigEntryObserver {
public:
    FibConfigEntryObserverIPHelper(FibConfig& fibconfig);
    virtual ~FibConfigEntryObserverIPHelper();

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

class FibConfigEntryObserverRtmV2 : public FibConfigEntryObserver {
public:
    FibConfigEntryObserverRtmV2(FibConfig& fibconfig);
    virtual ~FibConfigEntryObserverRtmV2();

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
    
private:

};
    
#endif // __FEA_FIBCONFIG_ENTRY_OBSERVER_HH__
