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

// $XORP: xorp/rtrmgr/xrl_rtrmgr_interface.hh,v 1.6 2003/05/07 23:15:17 mjh Exp $

#ifndef __RTRMGR_XRL_RTRMGR_INTERFACE_HH__
#define __RTRMGR_XRL_RTRMGR_INTERFACE_HH__

#include "xrl/targets/rtrmgr_base.hh"
#include "xrl/interfaces/rtrmgr_client_xif.hh"
#include "libxorp/timer.hh"

#define CNAMELEN 40

class User;
class UserDB;
class UserInstance;
class MasterConfigTree;
class RandomGen;

class XrlRtrmgrInterface : public XrlRtrmgrTargetBase {
    typedef XorpCallback2<void, bool, string>::RefPtr CallBack;
public:
    XrlRtrmgrInterface(XrlRouter& r, UserDB& db, MasterConfigTree& ct,
		       EventLoop& eventloop, RandomGen& randgen);
    ~XrlRtrmgrInterface();
    XrlCmdError common_0_1_get_target_name(// Output values,
					   string& name);

    XrlCmdError common_0_1_get_version(// Output values, 
				       string& version);

    /**
     *  Get status from Xrl Target
     */
    XrlCmdError common_0_1_get_status(// Output values,
				      uint32_t& status,
				      string& reason);

    /**
     * Request clean shutdown.
     */
    XrlCmdError common_0_1_shutdown();

    XrlCmdError rtrmgr_0_1_get_pid(
        // Output values, 
	uint32_t& pid);

    XrlCmdError rtrmgr_0_1_register_client(
	// Input values, 
	const uint32_t&	user_id, 
	const string& clientname, 
	// Output values, 
	string&	filename,
	uint32_t& pid);

    XrlCmdError rtrmgr_0_1_unregister_client(
        // Input values
	const string& token);

    XrlCmdError rtrmgr_0_1_authenticate_client(
	// Input values, 
	const uint32_t&	user_id, 
	const string& clientname, 
	const string& token);

    XrlCmdError rtrmgr_0_1_enter_config_mode(
	// Input values, 
	const string&	token, 
	const bool&	exclusive);

    XrlCmdError rtrmgr_0_1_leave_config_mode(
	// Input values, 
	const string&	token);

    XrlCmdError rtrmgr_0_1_get_config_users(
	// Input values, 
	const string& token, 
	// Output values, 
	XrlAtomList& users);

    XrlCmdError rtrmgr_0_1_get_running_config(
	// Input values, 
	const string& token, 
	// Output values, 
	string&	config);

    XrlCmdError rtrmgr_0_1_apply_config_change(
	// Input values, 
	const string& token, 
	const string& target, 
	const string& deltas,
	const string& deletions);

    void apply_config_change_done(bool success, string errmsg,
				  uid_t user_id,
				  string target,
				  string deltas,
				  string deletions);

    void apply_config_change_done_cb(const XrlError&);
    void client_updated(const XrlError& e, uid_t user_id, UserInstance* user);

    XrlCmdError rtrmgr_0_1_lock_config(
	// Input values, 
	const string& token, 
	const uint32_t& timeout, 
	// Output values, 
	bool& success, 
	uint32_t& holder);

    XrlCmdError rtrmgr_0_1_unlock_config(
	// Input values, 
	const string& token);

    XrlCmdError rtrmgr_0_1_lock_node(
	// Input values, 
	const string& token, 
	const string& node, 
	const uint32_t& timeout, 
	// Output values, 
	bool& success, 
	uint32_t& holder);

    XrlCmdError rtrmgr_0_1_unlock_node(
	// Input values, 
	const string& token, 
	const string& node);

    XrlCmdError rtrmgr_0_1_save_config(
	// Input values, 
	const string& token, 
	const string& filename);

    XrlCmdError rtrmgr_0_1_load_config(
	// Input values, 
	const string& token, 
	const string& target, 
	const string& filename);

    typedef XorpCallback1<void, const XrlError&>::RefPtr GENERIC_CALLBACK;
private:
    UserInstance *find_user_instance(uint32_t user_id, const string& clientname);

    string generate_auth_token(const uint32_t& user_id, 
			       const string& clientname);

    bool verify_token(const string& token) const;
    uint32_t get_user_id_from_token(const string& token) const;
    void lock_timeout();

    XrlRtrmgrClientV0p1Client _client_interface;
    multimap <uint32_t, UserInstance*> _users;
    multimap <uint32_t, UserInstance*> _config_users;
    UserDB& _userdb;
    MasterConfigTree& _conf_tree;
    EventLoop& _eventloop;
    RandomGen& _randgen;

    bool _exclusive; //indicates only one user allowed in config mode

    //variables to implement global lock on config changes
    bool _config_locked;
    uint32_t _lock_holder;
    XorpTimer _lock_timer;
};

#endif // __RTRMGR_XRL_RTRMGR_INTERFACE_HH__
