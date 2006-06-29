// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2006 International Computer Science Institute
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

// $XORP: xorp/fea/fticonfig_entry_observer.hh,v 1.16 2006/03/30 08:32:11 pavlin Exp $

#ifndef __FEA_FTICONFIG_ENTRY_OBSERVER_HH__
#define __FEA_FTICONFIG_ENTRY_OBSERVER_HH__

#include "netlink_socket.hh"
#include "routing_socket.hh"


class FtiConfig;

class FtiConfigEntryObserver {
public:
    FtiConfigEntryObserver(FtiConfig& ftic);
    
    virtual ~FtiConfigEntryObserver();
    
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
     * @param data the buffer with the received data.
     * @param nbytes the number of bytes in the data buffer @ref data.
     */
    virtual void receive_data(const uint8_t* data, size_t nbytes) = 0;
    
protected:
    // Misc other state
    bool	_is_running;

private:
    FtiConfig&	_ftic;
    bool	_is_primary;	// True -> primary, false -> secondary method
};

class FtiConfigEntryObserverDummy : public FtiConfigEntryObserver {
public:
    FtiConfigEntryObserverDummy(FtiConfig& ftic);
    virtual ~FtiConfigEntryObserverDummy();

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
     * @param data the buffer with the received data.
     * @param nbytes the number of bytes in the data buffer @ref data.
     */
    virtual void receive_data(const uint8_t* data, size_t nbytes);
    
private:
    
};

class FtiConfigEntryObserverRtsock : public FtiConfigEntryObserver,
				     public RoutingSocket,
				     public RoutingSocketObserver {
public:
    FtiConfigEntryObserverRtsock(FtiConfig& ftic);
    virtual ~FtiConfigEntryObserverRtsock();

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
     * @param data the buffer with the received data.
     * @param nbytes the number of bytes in the data buffer @ref data.
     */
    virtual void receive_data(const uint8_t* data, size_t nbytes);
    
    void rtsock_data(const uint8_t* data, size_t nbytes);
    
private:
    
};

class FtiConfigEntryObserverNetlink : public FtiConfigEntryObserver,
				      public NetlinkSocket,
				      public NetlinkSocketObserver {
public:
    FtiConfigEntryObserverNetlink(FtiConfig& ftic);
    virtual ~FtiConfigEntryObserverNetlink();

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
     * @param data the buffer with the received data.
     * @param nbytes the number of bytes in the data buffer @ref data.
     */
    virtual void receive_data(const uint8_t* data, size_t nbytes);
    
    void nlsock_data(const uint8_t* data, size_t nbytes);
    
private:
    
};

class FtiConfigEntryObserverIPHelper : public FtiConfigEntryObserver {
public:
    FtiConfigEntryObserverIPHelper(FtiConfig& ftic);
    virtual ~FtiConfigEntryObserverIPHelper();

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
     * @param data the buffer with the received data.
     * @param nbytes the number of bytes in the data buffer @ref data.
     */
    virtual void receive_data(const uint8_t* data, size_t nbytes);
};

class FtiConfigEntryObserverRtmV2 : public FtiConfigEntryObserver {
public:
    FtiConfigEntryObserverRtmV2(FtiConfig& ftic);
    virtual ~FtiConfigEntryObserverRtmV2();

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
    
#endif // __FEA_FTICONFIG_ENTRY_OBSERVER_HH__
