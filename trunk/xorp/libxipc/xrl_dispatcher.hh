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

// $XORP: xorp/libxipc/xrl_dispatcher.hh,v 1.1 2003/05/09 19:36:16 hodson Exp $

#ifndef __LIBXIPC_XRL_DISPATCHER_HH__
#define __LIBXIPC_XRL_DISPATCHER_HH__

#include "xrl_cmd_map.hh"

class XrlDispatcher : public XrlCmdMap {
public:
    XrlDispatcher(const char* class_name)
	: XrlCmdMap(class_name)
    {}
    virtual ~XrlDispatcher() {}
    virtual XrlError dispatch_xrl(const Xrl& xrl, XrlArgs& ret_vals) const;
};

#endif // __LIBXIPC_XRL_DISPATCHER_HH__
