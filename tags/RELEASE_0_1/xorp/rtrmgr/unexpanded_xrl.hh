// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/rtrmgr/unexpanded_xrl.hh,v 1.5 2002/12/09 18:29:40 hodson Exp $

#ifndef __RTRMGR_UNEXPANDED_XRL_HH__
#define __RTRMGR_UNEXPANDED_XRL_HH__

#include <string>
#include "libxipc/xrlrouter.hh"
#include "libxipc/xrlpf-sudp.hh"

class ConfigTreeNode;
class XrlAction;

//We want to build a queue of XRLs, but the variables needed for those
//XRLs may depend on the results from previous XRLs.  So we need to
//delay expanding them until we're actually going to send the XRL
//request.

class UnexpandedXrl {
public:
    UnexpandedXrl(const ConfigTreeNode* node,
		  const XrlAction* action);
    ~UnexpandedXrl();
    Xrl* xrl();
    string xrl_return_spec() const;
    string str() const;
private:
    const ConfigTreeNode *_node;
    const XrlAction *_action;
    Xrl *_xrl;
};

#endif // __RTRMGR_UNEXPANDED_XRL_HH__
