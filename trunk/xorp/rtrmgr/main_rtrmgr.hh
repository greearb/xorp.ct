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

// $XORP: xorp/devnotes/template.hh,v 1.2 2003/01/16 19:08:48 mjh Exp $

#ifndef __MAIN_RTRMGR_HH__
#define __MAIN_RTRMGR_HH__

class Rtrmgr {
public:
    Rtrmgr(const string& template_dir,
	   const string& xrl_dir,
	   const string& boot_file,
	   const list<IPv4>& bind_addrs,
	   uint16_t bind_port,
	   bool	do_exec,
	   int32_t quit_time);
    int run();
private:
    string _template_dir;
    string _xrl_dir;
    string _boot_file;
    list<IPv4> _bind_addrs;
    uint16_t _bind_port;
    bool _do_exec;
    int32_t _quit_time;
};

#endif // __MAIN_RTRMGR_HH__

