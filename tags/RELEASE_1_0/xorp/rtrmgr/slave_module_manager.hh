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

// $XORP: xorp/rtrmgr/slave_module_manager.hh,v 1.15 2004/05/11 16:50:58 mjh Exp $

#ifndef __RTRMGR_SLAVE_MODULE_MANAGER_HH__
#define __RTRMGR_SLAVE_MODULE_MANAGER_HH__


#include "libxorp/eventloop.hh"


class ModuleManager {
    typedef XorpCallback2<void, bool, string>::RefPtr CallBack;
public:
    ModuleManager(EventLoop& eventloop);

    bool new_module(const string& module_name, const string& path);
    int start_module(const string& module_name, bool do_exec, 
		   XorpCallback1<void, bool>::RefPtr cb);
    int kill_module(const string& module_name, XorpCallback0<void>::RefPtr cb);
    bool module_exists(const string& module_name) const;
    bool module_has_started(const string& module_name) const;
    int shell_execute(uid_t userid, const vector<string>& argv, 
		      ModuleManager::CallBack cb, bool do_exec);
private:
};

#endif // __RTRMGR_SLAVE_MODULE_MANAGER_HH__
