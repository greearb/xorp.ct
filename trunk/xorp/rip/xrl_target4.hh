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

#ifndef __RIP_XRL_TARGET4_HH__
#define __RIP_XRL_TARGET4_HH__

#include "libxorp/status_codes.h"
#include "xrl/targets/rip4_base.hh"

class XrlRouter;
class XrlProcessSpy;

class XrlRip4Target : public XrlRip4TargetBase {
public:
    XrlRip4Target(XrlRouter& rtr, XrlProcessSpy& pspy, bool& should_exit);
    ~XrlRip4Target();

    void set_status(ProcessStatus ps, const string& annotation = "");

    XrlCmdError common_0_1_get_target_name(string& name);
    XrlCmdError common_0_1_get_version(string& version);
    XrlCmdError common_0_1_get_status(uint32_t& status, string& reason);
    XrlCmdError common_0_1_shutdown();

    XrlCmdError
    finder_event_observer_0_1_xrl_target_birth(const string& class_name,
					       const string& instance_name);

    XrlCmdError
    finder_event_observer_0_1_xrl_target_death(const string& class_name,
					       const string& instance_name);

    XrlCmdError rip4_0_1_add_rip_address(const string&	ifname,
					 const string&	vifname,
					 const IPv4&	addr);

    XrlCmdError rip4_0_1_remove_rip_address(const string&	ifname,
					    const string&	vifname,
					    const IPv4&		addr);

    XrlCmdError rip4_0_1_set_rip_address_enabled(const string&	ifname,
						 const string&	vifname,
						 const IPv4&	addr,
						 const bool&	enabled);

    XrlCmdError rip4_0_1_get_rip_address_enabled(const string&	ifname,
						 const string&	vifname,
						 const IPv4&	addr,
						 bool&		enabled);

    XrlCmdError rip4_0_1_get_rip_address_status(const string&	ifname,
						const string&	vifname,
						const IPv4&	addr,
						string&		status);

protected:
    XrlProcessSpy&	_pspy;
    bool&		_should_exit;

    ProcessStatus	_status;
    string		_status_note;
};

#endif // __RIP_XRL_TARGET4_HH__
