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

// $XORP: xorp/rtrmgr/main_rtrmgr.hh,v 1.8 2004/05/28 22:27:56 pavlin Exp $

#ifndef __RTRMGR_MAIN_RTRMGR_HH__
#define __RTRMGR_MAIN_RTRMGR_HH__


#include <list>

#include "libxorp/ipv4.hh"


class MasterConfigTree;

class Rtrmgr {
public:
    Rtrmgr(const string& template_dir,
	   const string& xrl_targets_dir,
	   const string& boot_file,
	   const list<IPv4>& bind_addrs,
	   uint16_t bind_port,
	   const string& save_hook,
	   bool	do_exec,
	   bool	do_restart,
	   bool verbose,
	   int32_t quit_time);
    int run();
    bool ready() const;
    const string& save_hook() const { return _save_hook; }
    bool verbose() const { return _verbose; }

private:
    int validate_save_hook();
    string	_template_dir;
    string	_xrl_targets_dir;
    string	_boot_file;
    list<IPv4>	_bind_addrs;
    uint16_t	_bind_port;

    /**
     * _save_hook should either be an empty string, or should contain
     * the path to an executable.  When the config file is saved from
     * rtrmgr, if _save_hook is non-empty, the executable will be run
     * with the userid of the user that executed the save.  The only
     * parameter to the executable will be the full path of the file
     * that has just been saved.  The purpose of the save hook is to
     * allow the preservation of saved config files when XORP is
     * running out of a memory filesystem,, such as when run from a
     * live CD. 
     */
    string	_save_hook;

    bool	_do_exec;
    bool	_do_restart;
    bool	_verbose;		// Set to true if output is verbose
    int32_t	_quit_time;

    bool	_ready;
    MasterConfigTree* _mct;
};

#endif // __RTRMGR_MAIN_RTRMGR_HH__
