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

#include "config.h"
#include "libxorp/xorp.h"

#define DEBUG_LOGGING

#include "libxorp/debug.h"
#include "libxorp/status_codes.h"
#include "libxipc/xrl_router.hh"

#include "xrl_target4.hh"
#include "xrl_process_spy.hh"

XrlRip4Target::XrlRip4Target(XrlRouter&		r,
			     XrlProcessSpy&	pspy,
			     bool&		should_exit)
    : XrlRip4TargetBase(&r), _pspy(pspy), _should_exit(should_exit),
      _status(PROC_NULL), _status_note("")
{
}

XrlRip4Target::~XrlRip4Target()
{
}

void
XrlRip4Target::set_status(ProcessStatus status, const string& note)
{
    _status = status;
    _status_note = note;
}

XrlCmdError
XrlRip4Target::common_0_1_get_target_name(string& n)
{
    n = name();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::common_0_1_get_version(string& v)
{
    v = string(version());
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::common_0_1_get_status(uint32_t& status,
				     string&   reason)
{
    status = _status;
    reason = _status_note;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::common_0_1_shutdown()
{
    _should_exit = true;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::finder_event_observer_0_1_xrl_target_birth(const string& cname,
							  const string& iname)
{
    _pspy.birth_event(cname, iname);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::finder_event_observer_0_1_xrl_target_death(const string& cname,
							  const string& iname)
{
    _pspy.death_event(cname, iname);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::rip4_0_1_add_rip_address(const string& ifname,
					const string& vifname,
					const IPv4&   addr)
{
    debug_msg("rip4_0_1_add_rip_address %s/%s/%s",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str());
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::rip4_0_1_remove_rip_address(const string& ifname,
					   const string& vifname,
					   const IPv4&   addr)
{
    debug_msg("rip4_0_1_remove_rip_address %s/%s/%s",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str());
    return XrlCmdError::OKAY();
}

static bool s_enabled = false;

XrlCmdError
XrlRip4Target::rip4_0_1_set_rip_address_enabled(const string& ifname,
						const string& vifname,
						const IPv4&   addr,
						const bool&   enabled)
{
    debug_msg("rip4_0_1_set_rip_address_enabled %s/%s/%s %s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str(),
	      enabled ? "true" : "false");
    s_enabled = enabled;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::rip4_0_1_get_rip_address_enabled(const string& ifname,
						const string& vifname,
						const IPv4&   addr,
						bool&	      enabled)
{
    enabled = s_enabled;

    debug_msg("rip4_0_1_set_rip_address_enabled %s/%s/%s -> %s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str(),
	      enabled ? "true" : "false");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::rip4_0_1_get_rip_address_status(const string& ifname,
					       const string& vifname,
					       const IPv4&   addr,
					       string&	     status)
{
    status = (s_enabled) ? "running" : "not running";

    debug_msg("rip4_0_1_get_rip_address_status %s/%s/%s -> %s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str(),
	      status.c_str());

    return XrlCmdError::OKAY();
}
