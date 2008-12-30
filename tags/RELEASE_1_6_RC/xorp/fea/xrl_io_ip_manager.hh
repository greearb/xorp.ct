// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/fea/xrl_io_ip_manager.hh,v 1.5 2008/07/23 05:10:12 pavlin Exp $

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
