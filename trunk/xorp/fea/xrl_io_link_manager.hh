// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2008 International Computer Science Institute
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

// $XORP: xorp/fea/xrl_io_link_manager.hh,v 1.2 2007/08/09 00:46:58 pavlin Exp $

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
