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

// $XORP: xorp/rtrmgr/main_rtrmgr.hh,v 1.19 2008/10/30 23:50:16 pavlin Exp $

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
    ~Rtrmgr();

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
