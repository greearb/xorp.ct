// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/libfeaclient/ifmgr_cmd_base.hh,v 1.8 2008/10/02 21:57:15 bms Exp $

#ifndef __LIBFEACLIENT_IFMGR_CMD_BASE_HH__
#define __LIBFEACLIENT_IFMGR_CMD_BASE_HH__

#include "libxorp/callback.hh"

class XrlError;
class XrlArgs;
class XrlSender;
class IfMgrIfTree;

typedef XorpCallback1<void, const XrlError&>::RefPtr IfMgrXrlSendCB;

/**
 * @short Base class for Interface Manager Commands.
 *
 * Commands may be forwarded either on an existing interface
 * configuration tree (represented by IfMgrIfTree objects) or as Xrls.
 * When forwarded as Xrls the command is sent to a remote target.
 * The two methods of forward are intended to facilitate maintaining
 * local and remote copies of IfMgrIfTree objects.
 */
class IfMgrCommandBase {
public:
    virtual ~IfMgrCommandBase() = 0;
    /**
     * Execute Command to interface tree.
     *
     * @return true on success, false on failure.
     */
    virtual bool execute(IfMgrIfTree& tree) const = 0;

    /**
     * Forward Command as an Xrl call to a remote target.
     *
     * @param sender xrl router to use as the command sender.
     * @param xrl_target the target to direct the command to.
     * @param xscb callback to invoke with Xrl result.
     *
     * @return true on success, false on failure.
     */
    virtual bool forward(XrlSender&	 sender,
			 const string&	 xrl_target,
			 const IfMgrXrlSendCB& xscb) const = 0;

    /**
     * Render command as string.
     */
    virtual string str() const = 0;
};

#endif // __LIBFEACLIENT_IFMGR_CMD_BASE_HH__
