// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2010 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/libxipc/xrl_sender.hh,v 1.10 2008/10/02 21:57:26 bms Exp $

#ifndef __LIBXIPC_XRL_SENDER_HH__
#define __LIBXIPC_XRL_SENDER_HH__

class Xrl;
class XrlArgs;
class XrlError;

/**
 * Base for classes able to transport Xrls.
 * See xrl_router.hh for implementor of this base class.
 */
class XrlSender {
public:
    virtual ~XrlSender() {}

    typedef XorpCallback2<void, const XrlError&, XrlArgs*>::RefPtr Callback;

    /**
     * @param xrl Xrl to be sent.
     * @param scb callback to be invoked with result from Xrl
     *
     * @return true if Xrl is accepted for sending, false otherwise.
     */
    virtual bool send(const Xrl& xrl, const Callback& scb) = 0;

    /**
     * Return true if sender has send requests pending.
     */
    virtual bool pending() const = 0;
};

#endif // __LIBXIPC_XRL_SENDER_HH__
