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
#include "xrl_port_manager.hh"

XrlRip4Target::XrlRip4Target(XrlRouter&			xr,
			     XrlProcessSpy&		xps,
			     XrlPortManager<IPv4>& 	xpm,
			     bool&			should_exit)
    : XrlRip4TargetBase(&xr), _xps(xps), _xpm(xpm), _should_exit(should_exit),
      _status(PROC_NULL), _status_note("")
{
}

XrlRip4Target::~XrlRip4Target()
{
}

void
XrlRip4Target::set_status(ProcessStatus status, const string& note)
{
    debug_msg("Status Update %d -> %d: %s\n", _status, status, note.c_str());
    _status 	 = status;
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
    debug_msg("Shutdown requested.\n");
    _should_exit = true;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::finder_event_observer_0_1_xrl_target_birth(const string& cname,
							  const string& iname)
{
    _xps.birth_event(cname, iname);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::finder_event_observer_0_1_xrl_target_death(const string& cname,
							  const string& iname)
{
    _xps.death_event(cname, iname);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::rip4_0_1_add_rip_address(const string& ifname,
					const string& vifname,
					const IPv4&   addr)
{
    debug_msg("rip4_0_1_add_rip_address %s/%s/%s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str());
    if (_xpm.add_rip_address(ifname, vifname, addr) == false) {
	return XrlCmdError::COMMAND_FAILED();
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::rip4_0_1_remove_rip_address(const string& ifname,
					   const string& vifname,
					   const IPv4&   addr)
{
    debug_msg("rip4_0_1_remove_rip_address %s/%s/%s\n",
	      ifname.c_str(), vifname.c_str(), addr.str().c_str());
    if (_xpm.remove_rip_address(ifname, vifname, addr) == false) {
	return XrlCmdError::COMMAND_FAILED();
    }
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

XrlCmdError
XrlRip4Target::socket4_user_0_1_recv_event(const string&	sockid,
					   const IPv4&		src_host,
					   const uint32_t&	src_port,
					   const vector<uint8_t>& pdata)
{
    debug_msg("socket4_user_0_1_recv_event %s %s/%u %u bytes\n",
	      sockid.c_str(), src_host.str().c_str(), src_port,
	      static_cast<uint32_t>(pdata.size()));
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::socket4_user_0_1_connect_event(const string&	sockid,
					      const IPv4&	src_host,
					      const uint32_t&	src_port,
					      const string&	new_sockid,
					      bool&		accept)
{
    debug_msg("socket4_user_0_1_connect_event %s %s/%u %s\n",
	      sockid.c_str(), src_host.str().c_str(), src_port,
	      new_sockid.c_str());
    accept = false;
    return XrlCmdError::COMMAND_FAILED("Connect not requested.");
}

XrlCmdError
XrlRip4Target::socket4_user_0_1_error_event(const string&	sockid,
					    const string& 	reason,
					    const bool&		fatal)
{
    debug_msg("socket4_user_0_1_error_event %s %s %s \n",
	      sockid.c_str(), reason.c_str(),
	      fatal ? "fatal" : "non-fatal");
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRip4Target::socket4_user_0_1_close_event(const string&	sockid,
					    const string&	reason)
{
    debug_msg("socket4_user_0_1_close_event %s %s\n",
	      sockid.c_str(), reason.c_str());
    return XrlCmdError::OKAY();
}

