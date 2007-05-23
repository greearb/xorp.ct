// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/fea/forwarding_plane/control_socket/windows_rtm_pipe.hh,v 1.1 2007/05/01 08:21:56 pavlin Exp $

#ifndef __FEA_FORWARDING_PLANE_CONTROL_SOCKET_WINDOWS_RTM_PIPE_HH__
#define __FEA_FORWARDING_PLANE_CONTROL_SOCKET_WINDOWS_RTM_PIPE_HH__

#include <list>

#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"

class WinRtmPipeObserver;
struct WinRtmPipePlumber;

/**
 * WinRtmPipe class opens a routing socket and forwards data arriving
 * on the socket to WinRtmPipeObservers.  The WinRtmPipe hooks itself
 * into the EventLoop and activity usually happens asynchronously.
 */
class WinRtmPipe {
public:
    WinRtmPipe(EventLoop& eventloop);
    ~WinRtmPipe();

    /**
     * Start the routing socket operation for a given address family.
     * 
     * @param af the address family.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start(int af, string& error_msg);

    /**
     * Stop the routing socket operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int stop(string& error_msg);

    /**
     * Test if the routing socket is open.
     * 
     * This method is needed because WinRtmPipe may fail to open
     * routing socket during startup.
     * 
     * @return true if the routing socket is open, otherwise false.
     */
    bool is_open() const { return _fd.is_valid(); }

    /**
     * Write data to routing socket.
     * 
     * This method also updates the sequence number associated with
     * this routing socket.
     * 
     * @return the number of bytes which were written, or -1 if error.
     */
    ssize_t write(const void* data, size_t nbytes);

    /**
     * Get the sequence number for next message written into the kernel.
     * 
     * The sequence number is derived from the instance number of this routing
     * socket and a 16-bit counter.
     * 
     * @return the sequence number for the next message written into the
     * kernel.
     */
    uint32_t seqno() const { return (_instance_no << 16 | _seqno); }

    /**
     * Get the cached process identifier value.
     * 
     * @return the cached process identifier value.
     */
    pid_t pid() const { return _pid; }

    /**
     * Force socket to read data.
     * 
     * This usually is performed after writing a request that the
     * kernel will answer (e.g., after writing a route lookup).
     * Use sparingly, with caution, and at your own risk.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int force_read(string& error_msg);

private:
    typedef list<WinRtmPipeObserver*> ObserverList;

    /**
     * Read data available for WinRtmPipe and invoke
     * WinRtmPipeObserver::rtsock_data() on all observers of routing
     * socket.
     */
    void io_event(XorpFd fd, IoEventType type);

    WinRtmPipe& operator=(const WinRtmPipe&);	// Not implemented
    WinRtmPipe(const WinRtmPipe&);		// Not implemented

private:
    static const size_t RTSOCK_BYTES = 8*1024; // Initial guess at msg size

private:
    EventLoop&	 _eventloop;
    XorpFd	 _fd;
    ObserverList _ol;

    uint16_t 	 _seqno;	// Seqno of next write()
    uint16_t	 _instance_no;  // Instance number of this routing socket

    static uint16_t _instance_cnt;
    static pid_t    _pid;

    friend class WinRtmPipePlumber; // class that hooks observers in and out
};

class WinRtmPipeObserver {
public:
    WinRtmPipeObserver(WinRtmPipe& rs);

    virtual ~WinRtmPipeObserver();

    /**
     * Receive data from the routing socket.
     *
     * Note that this method is called asynchronously when the routing socket
     * has data to receive, therefore it should never be called directly by
     * anything else except the routing socket facility itself.
     *
     * @param buffer the buffer with the received data.
     */
    virtual void rtsock_data(const vector<uint8_t>& buffer) = 0;

    /**
     * Get WinRtmPipe associated with Observer.
     */
    WinRtmPipe& routing_socket();

private:
    WinRtmPipe& _rs;
};

#endif // __FEA_FORWARDING_PLANE_CONTROL_SOCKET_WINDOWS_RTM_PIPE_HH__
