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

// $XORP: xorp/fea/fibconfig_table_observer.hh,v 1.6 2007/05/01 08:21:55 pavlin Exp $

#ifndef __FEA_FIBCONFIG_TABLE_OBSERVER_HH__
#define __FEA_FIBCONFIG_TABLE_OBSERVER_HH__

#include "fea/data_plane/control_socket/netlink_socket.hh"
#include "fea/data_plane/control_socket/routing_socket.hh"
#include "fea/data_plane/control_socket/windows_rtm_pipe.hh"


class FibConfig;

class FibConfigTableObserver {
public:
    FibConfigTableObserver(FibConfig& fibconfig)
	: _is_running(false),
	  _fibconfig(fibconfig),
	  _is_primary(true)
    {}
    virtual ~FibConfigTableObserver() {}
    
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

class FibConfigTableObserverDummy : public FibConfigTableObserver {
public:
    FibConfigTableObserverDummy(FibConfig& fibconfig);
    virtual ~FibConfigTableObserverDummy();

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

class FibConfigTableObserverRtsock : public FibConfigTableObserver,
				     public RoutingSocket,
				     public RoutingSocketObserver {
public:
    FibConfigTableObserverRtsock(FibConfig& fibconfig);
    virtual ~FibConfigTableObserverRtsock();

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


class FibConfigTableObserverRtmV2 : public FibConfigTableObserver {
public:
    FibConfigTableObserverRtmV2(FibConfig& fibconfig);
    virtual ~FibConfigTableObserverRtmV2();

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
		      FibConfigTableObserverRtmV2& rtmo)
	    : WinRtmPipeObserver(rs), _af(af), _rtmo(rtmo) {}
    	virtual ~RtmV2Observer() {}
	void rtsock_data(const vector<uint8_t>& buffer) {
	    _rtmo.receive_data(buffer);
	}
    private:
	int _af;
    	FibConfigTableObserverRtmV2& _rtmo;
    };
private:
    WinRtmPipe*		_rs4;
    RtmV2Observer*	_rso4;
    WinRtmPipe*		_rs6;
    RtmV2Observer*	_rso6;
};

class FibConfigTableObserverNetlink : public FibConfigTableObserver,
				      public NetlinkSocket,
				      public NetlinkSocketObserver {
public:
    FibConfigTableObserverNetlink(FibConfig& fibconfig);
    virtual ~FibConfigTableObserverNetlink();

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

class FibConfigTableObserverIPHelper : public FibConfigTableObserver {
public:
    FibConfigTableObserverIPHelper(FibConfig& fibconfig);
    virtual ~FibConfigTableObserverIPHelper();

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

#endif // __FEA_FIBCONFIG_TABLE_OBSERVER_HH__
