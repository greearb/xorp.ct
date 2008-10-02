// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2008 XORP, Inc.
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

// $XORP: xorp/fea/xrl_io_link_manager.hh,v 1.4 2008/07/23 05:10:13 pavlin Exp $

#ifndef __FEA_XRL_IO_LINK_MANAGER_HH__
#define __FEA_XRL_IO_LINK_MANAGER_HH__

#include "io_link_manager.hh"

class XrlRouter;

/**
 * @short A class that is the bridge between the raw link I/O communications
 * and the XORP XRL interface.
 */
class XrlIoLinkManager : public IoLinkManagerReceiver {
public:
    /**
     * Constructor.
     */
    XrlIoLinkManager(IoLinkManager& io_link_manager, XrlRouter& xrl_router);

    /**
     * Destructor.
     */
    virtual ~XrlIoLinkManager();

    /**
     * Data received event.
     *
     * @param receiver_name the name of the receiver to send the
     * link-level packet to.
     * @param header the link-level header information.
     * @param payload the payload, everything after the link-level header.
     */
    void recv_event(const string&		receiver_name,
		    const struct MacHeaderInfo&	header,
		    const vector<uint8_t>&	payload);

private:
    XrlRouter&		xrl_router() { return _xrl_router; }

    /**
     * Method to be called by XRL sending filter invoker
     */
    void xrl_send_recv_cb(const XrlError& xrl_error, string receiver_name);

    IoLinkManager&	_io_link_manager;
    XrlRouter&		_xrl_router;
};

#endif // __FEA_XRL_IO_LINK_MANAGER_HH__
