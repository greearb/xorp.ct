// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

// $XORP: xorp/devnotes/template.hh,v 1.2 2003/01/16 19:08:48 mjh Exp $

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
