// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/rtrmgr/slave_module_manager.hh,v 1.23 2008/01/04 03:17:43 pavlin Exp $

#ifndef __RTRMGR_SLAVE_MODULE_MANAGER_HH__
#define __RTRMGR_SLAVE_MODULE_MANAGER_HH__


#include "libxorp/eventloop.hh"
#include "generic_module_manager.hh"

class SlaveModuleManager : public GenericModuleManager {
public:
    SlaveModuleManager(EventLoop& eventloop);
    GenericModule* new_module(const string& module_name, string& error_msg);
    bool module_is_active(const string& module_name) const;
private:
};

#endif // __RTRMGR_SLAVE_MODULE_MANAGER_HH__
