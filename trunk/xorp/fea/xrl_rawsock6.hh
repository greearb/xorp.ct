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

// $XORP: xorp/fea/xrl_rawsock6.hh,v 1.11 2007/05/22 22:57:20 pavlin Exp $

#ifndef __FEA_XRL_RAWSOCK6_HH__
#define __FEA_XRL_RAWSOCK6_HH__

#include <map>

#include "rawsock6.hh"

class XrlRouter;

/**
 * @short A class that manages raw sockets as used by the XORP Xrl Interface.
 */
class XrlRawSocket6Manager : public RawSocket6Manager {
public:
    /**
     * Constructor for XrlRawSocket6Manager instances.
     */
    XrlRawSocket6Manager(EventLoop& eventloop, const IfTree& iftree,
			 XrlRouter& xrl_router);

    /**
     * Received an IPv6 packet on a raw socket and forward it to the
     * recipient.
     *
     * @param xrl_target_name the XRL target name to send the IP packet to.
     * @param header the IPv6 header information.
     * @param payload the payload, everything after the IP header and options.
     */
    void recv_and_forward(const string&			xrl_target_name,
			  const struct IPv6HeaderInfo&	header,
			  const vector<uint8_t>&	payload);

    XrlRouter&		xrl_router() { return _xrl_router; }

    /**
     * Method to be called by Xrl sending filter invoker
     */
    void xrl_send_recv_cb(const XrlError& xrl_error, string xrl_target_name);

private:
    XrlRouter&		_xrl_router;
};

#endif // __FEA_XRL_RAWSOCK6_HH__
