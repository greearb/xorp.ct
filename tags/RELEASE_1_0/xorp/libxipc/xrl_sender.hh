// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/libxipc/xrl_sender.hh,v 1.3 2003/03/10 23:20:29 hodson Exp $

#ifndef __LIBXIPC_XRL_SENDER_HH__
#define __LIBXIPC_XRL_SENDER_HH__

class Xrl;
class XrlArgs;
class XrlError;

/**
 * Base for classes able to transport Xrls.
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
