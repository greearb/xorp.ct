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

// $XORP: xorp/rip/xrl_target4.hh,v 1.2 2004/01/09 00:29:03 hodson Exp $

#ifndef __RIP_XRL_TARGET4_HH__
#define __RIP_XRL_TARGET4_HH__

#include "libxorp/status_codes.h"
#include "xrl/targets/rip4_base.hh"

class XrlRouter;
class XrlProcessSpy;
template<typename A> class XrlPortManager;

class XrlRip4Target : public XrlRip4TargetBase {
public:
    XrlRip4Target(XrlRouter& 		xr,
		  XrlProcessSpy& 	xps,
		  XrlPortManager<IPv4>& xpm,
		  bool& 		should_exit);
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

    XrlCmdError socket4_user_0_1_recv_event(const string&	sockid,
					    const IPv4&		src_host,
					    const uint32_t&	src_port,
					    const vector<uint8_t>& pdata);

    XrlCmdError socket4_user_0_1_connect_event(const string&	sockid,
					       const IPv4&	src_host,
					       const uint32_t&	src_port,
					       const string&	new_sockid,
					       bool&		accept);

    XrlCmdError socket4_user_0_1_error_event(const string&	sockid,
					     const string& 	reason,
					     const bool&	fatal);

    XrlCmdError socket4_user_0_1_close_event(const string&	sockid,
					     const string&	reason);

protected:
    XrlProcessSpy&		_xps;
    XrlPortManager<IPv4>&	_xpm;
    bool&			_should_exit;

    ProcessStatus		_status;
    string			_status_note;
};

#endif // __RIP_XRL_TARGET4_HH__
