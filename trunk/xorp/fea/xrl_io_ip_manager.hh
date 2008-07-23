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

// $XORP: xorp/fea/xrl_io_ip_manager.hh,v 1.4 2008/01/04 03:15:52 pavlin Exp $

#ifndef __FEA_XRL_IO_IP_MANAGER_HH__
#define __FEA_XRL_IO_IP_MANAGER_HH__

#include "io_ip_manager.hh"

class XrlRouter;

/**
 * @short A class that is the bridge between the raw IP I/O communications
 * and the XORP XRL interface.
 */
class XrlIoIpManager : public IoIpManagerReceiver {
public:
    /**
     * Constructor.
     */
    XrlIoIpManager(IoIpManager& io_ip_manager, XrlRouter& xrl_router);

    /**
     * Destructor.
     */
    virtual ~XrlIoIpManager();

    /**
     * Data received event.
     *
     * @param receiver_name the name of the receiver to send the
     * IP packet to.
     * @param header the IP header information.
     * @param payload the payload, everything after the IP header
     * and options.
     */
    void recv_event(const string&			receiver_name,
		    const struct IPvXHeaderInfo&	header,
		    const vector<uint8_t>&		payload);

private:
    XrlRouter&		xrl_router() { return _xrl_router; }

    /**
     * Method to be called by XRL sending filter invoker
     */
    void xrl_send_recv_cb(const XrlError& xrl_error, int family,
			  string receiver_name);

    IoIpManager&	_io_ip_manager;
    XrlRouter&		_xrl_router;
};

#endif // __FEA_XRL_IO_IP_MANAGER_HH__
