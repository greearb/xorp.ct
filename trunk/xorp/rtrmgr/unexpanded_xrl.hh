// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/rtrmgr/unexpanded_xrl.hh,v 1.15 2008/10/02 21:58:26 bms Exp $

#ifndef __RTRMGR_UNEXPANDED_XRL_HH__
#define __RTRMGR_UNEXPANDED_XRL_HH__


#include "libxipc/xrl_router.hh"
#include "libxipc/xrl_pf_sudp.hh"


class MasterConfigTreeNode;
class XrlAction;

//
// We want to build a queue of XRLs, but the variables needed for those
// XRLs may depend on the results from previous XRLs.  So we need to
// delay expanding them until we're actually going to send the XRL
// request.
//

class UnexpandedXrl {
public:
    UnexpandedXrl(const MasterConfigTreeNode& node, const XrlAction& action);
    ~UnexpandedXrl();

    /**
     * Expand the variables in the unexpanded XRL, and create an
     * XRL that we can actually send.
     * 
     * @param errmsg the error message (if error).
     * 
     * @return the created XRL that we can send, or NULL if error.
     */
    Xrl* expand(string& errmsg) const;

    /**
     * Return the XRL return specification as a string.
     * 
     * @return the XRL return specification.
     */
    string return_spec() const;

    string str() const;

private:
    const MasterConfigTreeNode& _node;
    const XrlAction& _action;

    // _xrl is mutable because expanding the XRL doesn't conceptually
    // change the UnexpandedXrl - storing the result here is purely an
    // optimization.
    mutable Xrl* _xrl;
};

#endif // __RTRMGR_UNEXPANDED_XRL_HH__
