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

// $XORP: xorp/rtrmgr/xorpsh_main.hh,v 1.3 2003/03/10 23:21:03 hodson Exp $

#ifndef __RTRMGR_XORPSH_MAIN_HH__
#define __RTRMGR_XORPSH_MAIN_HH__

#include "libxorp/eventloop.hh"
#include "libxipc/xrl_std_router.hh"
#include "xrl/interfaces/rtrmgr_xif.hh"
#include "xorp_client.hh"
#include "cli/cli_node.hh"
#include "xrl_xorpsh_interface.hh"

class SlaveConfigTree;
class OpCommandList;
class RouterCLI;
class TemplateTree;


class XorpShell {
public:
    XorpShell(const string& IPCname, 
	      const string& config_template_dir, 
	      const string& xrl_dir);
    ~XorpShell();
    void run();
    enum Mode {
	MODE_AUTHENTICATING, 
	MODE_INITIALIZING, 
	MODE_IDLE, 
	MODE_COMMITTING, 
	MODE_LOADING,
	MODE_SHUTDOWN
    };
    void set_mode(Mode mode) {_mode = mode;}
    
    void register_done(const XrlError& e, const string* token,
		       const uint32_t* pid);
    void generic_done(const XrlError& e);
    void receive_config(const XrlError& e, const string* config);

    typedef XorpCallback1<void, const XrlError&>::RefPtr GENERIC_CALLBACK;
    void enter_config_mode(bool exclusive, GENERIC_CALLBACK cb);

    void leave_config_mode(GENERIC_CALLBACK cb);

    typedef XorpCallback3<void, const XrlError&, 
	const bool*, 
	const uint32_t*>::RefPtr LOCK_CALLBACK;
    void lock_config(LOCK_CALLBACK cb);

    void commit_changes(const string& deltas, const string& deletions,
			GENERIC_CALLBACK cb,
			XorpBatch::CommitCallback final_cb);
    void commit_done(bool success, const string& errmsg);

    void unlock_config(GENERIC_CALLBACK cb);

    typedef XorpCallback2<void, const XrlError&, 
	const XrlAtomList*>::RefPtr GET_USERS_CALLBACK;
    void get_config_users(GET_USERS_CALLBACK cb);

    void new_config_user(uid_t user_id);

    void save_to_file(const string& filename, GENERIC_CALLBACK cb);

    void load_from_file(const string& filename, 
			GENERIC_CALLBACK cb,
			XorpBatch::CommitCallback final_cb);

    void config_changed(uid_t user_id, const string& deltas, 
			const string& deletions);

    typedef XorpCallback2<void, const XrlError&, 
	const uint32_t*>::RefPtr PID_CALLBACK;
    void get_rtrmgr_pid(PID_CALLBACK cb);

    EventLoop& eventloop() {return _eventloop;}
    SlaveConfigTree *config_tree() {return _ct;}
    OpCommandList *op_cmd_list() {return _ocl;}
    XorpClient& xorp_client() {return _xclient;}
    uint32_t rtrmgr_pid() const {return _rtrmgr_pid;}
private:
    EventLoop _eventloop; 
    XrlStdRouter _xrlrouter;
    XorpClient _xclient;
    XrlRtrmgrV0p1Client _rtrmgr_client;
    XrlXorpshInterface _xorpsh_interface;

    TemplateTree *_tt;
    SlaveConfigTree *_ct;
    OpCommandList *_ocl;
    CliNode _cli_node;
    RouterCLI *_router_cli;
    string _ipc_name;
    string _authfile;
    string _authtoken;
    string _configuration;

    bool _done;  //used to move through the startup process
    Mode _mode;

    //used to store the callback during a commit until we get called
    //with the response
    XorpBatch::CommitCallback _commit_callback;
    string _commit_status; /*used for transient storage of error
			     messages from commit */

    uint32_t _rtrmgr_pid;
};

#endif // __RTRMGR_XORPSH_MAIN_HH__
