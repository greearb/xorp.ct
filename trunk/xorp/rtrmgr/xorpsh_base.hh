// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/rtrmgr/xorpsh_base.hh,v 1.4 2007/02/16 22:47:27 pavlin Exp $

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
