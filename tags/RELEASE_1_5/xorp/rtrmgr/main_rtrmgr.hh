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

// $XORP: xorp/rtrmgr/main_rtrmgr.hh,v 1.16 2008/06/17 03:37:08 atanu Exp $

#ifndef __RTRMGR_MAIN_RTRMGR_HH__
#define __RTRMGR_MAIN_RTRMGR_HH__


#include <list>

#include "libxorp/ipv4.hh"
#include "generic_module_manager.hh"


class MasterConfigTree;
class XrlRtrmgrInterface;

class Rtrmgr {
public:
    Rtrmgr(const string& template_dir,
	   const string& xrl_targets_dir,
	   const string& boot_file,
	   const list<IPv4>& bind_addrs,
	   uint16_t bind_port,
	   bool	do_exec,
	   bool	do_restart,
	   bool verbose,
	   int32_t quit_time,
	   bool daemon_mode);
    int run();
    bool ready() const;
    void module_status_changed(const string& module_name,
			       GenericModule::ModuleStatus status);
    void daemonize();
    bool verbose() const { return _verbose; }

private:
    string	_template_dir;
    string	_xrl_targets_dir;
    string	_boot_file;
    list<IPv4>	_bind_addrs;
    uint16_t	_bind_port;

    bool	_do_exec;
    bool	_do_restart;
    bool	_verbose;		// Set to true if output is verbose
    int32_t	_quit_time;
    bool 	_daemon_mode;

    bool	_ready;
    MasterConfigTree* _mct;
    XrlRtrmgrInterface* _xrt;
};

#endif // __RTRMGR_MAIN_RTRMGR_HH__
