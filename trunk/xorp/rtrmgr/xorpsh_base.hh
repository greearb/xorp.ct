// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/rtrmgr/xorpsh_base.hh,v 1.7 2008/10/02 21:58:27 bms Exp $

#ifndef __RTRMGR_XORPSH_BASE_HH__
#define __RTRMGR_XORPSH_BASE_HH__


class EventLoop;
class OpCommandList;
class SlaveConfigTree;
class TemplateTree;

/**
 * @short XorpShellBase base class
 *
 * The XorpShellBase class contains pure virtual methods and is used to
 * define the interface a router manager client like xorpsh should implement.
 * It is used as a base class by other classes (e.g., @ref XorpShell)
 * which contain the real implementation.
 */
class XorpShellBase {
public:
    typedef XorpCallback1<void, const XrlError&>::RefPtr GENERIC_CALLBACK;
    typedef XorpCallback2<void, bool, string>::RefPtr CallBack;
    typedef XorpCallback2<void, const XrlError&,
			  const XrlAtomList*>::RefPtr GET_USERS_CALLBACK;
    typedef XorpCallback2<void, const XrlError&,
			  const uint32_t*>::RefPtr PID_CALLBACK;
    typedef XorpCallback3<void, const XrlError&, const bool*,
			  const uint32_t*>::RefPtr LOCK_CALLBACK;

    virtual ~XorpShellBase() {}


    enum Mode {
	MODE_AUTHENTICATING, 
	MODE_INITIALIZING, 
	MODE_IDLE, 
	MODE_COMMITTING, 
	MODE_LOADING,
	MODE_SAVING,
	MODE_SHUTDOWN
    };

    virtual EventLoop& eventloop() = 0;

    virtual OpCommandList* op_cmd_list() = 0;

    virtual SlaveConfigTree* config_tree() = 0;

    virtual TemplateTree* template_tree() = 0;

    virtual uint32_t clientid() const = 0;

    virtual uint32_t rtrmgr_pid() const = 0;

    virtual bool commit_changes(const string& deltas, const string& deletions,
				GENERIC_CALLBACK cb, CallBack final_cb) = 0;
    
    virtual bool enter_config_mode(bool exclusive, GENERIC_CALLBACK cb) = 0;

    virtual bool get_config_users(GET_USERS_CALLBACK cb) = 0;

    virtual bool get_rtrmgr_pid(PID_CALLBACK cb) = 0;

    virtual bool leave_config_mode(GENERIC_CALLBACK cb) = 0;

    virtual bool load_from_file(const string& filename, GENERIC_CALLBACK cb,
				CallBack final_cb) = 0;

    virtual bool lock_config(LOCK_CALLBACK cb) = 0;

    virtual bool save_to_file(const string& filename, GENERIC_CALLBACK cb,
			      CallBack final_cb) = 0;

    virtual void set_mode(Mode mode) = 0;

    virtual bool unlock_config(GENERIC_CALLBACK cb) = 0;
};

#endif // __RTRMGR_XORPSH_BASE_HH__
