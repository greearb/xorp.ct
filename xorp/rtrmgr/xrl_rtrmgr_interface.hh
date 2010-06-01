// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2010 XORP, Inc and Others
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

// $XORP: xorp/rtrmgr/xrl_rtrmgr_interface.hh,v 1.33 2008/10/02 21:58:27 bms Exp $

#ifndef __RTRMGR_XRL_RTRMGR_INTERFACE_HH__
#define __RTRMGR_XRL_RTRMGR_INTERFACE_HH__


#include "libxorp/timer.hh"

#include "xrl/targets/rtrmgr_base.hh"
#include "xrl/interfaces/rtrmgr_client_xif.hh"
#include "xrl/interfaces/finder_event_notifier_xif.hh"

#include "generic_module_manager.hh"


#define CNAMELEN 40


class MasterConfigTree;
class RandomGen;
class User;
class UserDB;
class UserInstance;
class Rtrmgr;

class XrlRtrmgrInterface : public XrlRtrmgrTargetBase {
    typedef XorpCallback2<void, bool, string>::RefPtr CallBack;
    typedef XorpCallback4<void, bool, string, string, string>::RefPtr ConfigChangeCallBack;
    typedef XorpCallback2<void, bool, string>::RefPtr ConfigSaveCallBack;
    typedef XorpCallback4<void, bool, string, string, string>::RefPtr ConfigLoadCallBack;

public:
    XrlRtrmgrInterface(XrlRouter& r, UserDB& db, EventLoop& eventloop,
		       RandomGen& randgen, Rtrmgr& rtrmgr);
    ~XrlRtrmgrInterface();

    void set_master_config_tree(MasterConfigTree* v) { _master_config_tree = v; }

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

    XrlCmdError common_0_1_startup() { return XrlCmdError::OKAY(); }

    XrlCmdError rtrmgr_0_1_get_pid(
        // Output values, 
	uint32_t& pid);

    XrlCmdError rtrmgr_0_1_register_client(
	// Input values, 
	const uint32_t&	user_id, 
	const string& clientname, 
	// Output values, 
	string&	filename,
	uint32_t& pid,
	uint32_t& clientid);

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
	bool& ready,
	string&	config);

    XrlCmdError rtrmgr_0_1_apply_config_change(
	// Input values, 
	const string& token, 
	const string& target, 
	const string& deltas,
	const string& deletions);

    void apply_config_change_done(bool success, string error_msg,
				  string deltas, string deletions,
				  uid_t user_id, string target);

    void config_saved_done_cb(const XrlError&);
    void apply_config_change_done_cb(const XrlError&);
    void client_updated(const XrlError& e, uid_t user_id, UserInstance* user);

    void module_status_changed(const string& modname,
			       GenericModule::ModuleStatus status);

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
	const string& target, 
	const string& filename);

    XrlCmdError rtrmgr_0_1_load_config(
	// Input values, 
	const string& token, 
	const string& target, 
	const string& filename);

    /**
     *  Set the name of the directory with the configuration files.
     *
     *  @param config_directory the name of the directory with the
     *  configuration files.
     */
    XrlCmdError rtrmgr_0_1_set_config_directory(
	// Input values,
	const string&	config_directory);

    XrlCmdError finder_event_observer_0_1_xrl_target_birth(
	// Input values,
	const string&	target_class,
	const string&	target_instance);

    XrlCmdError finder_event_observer_0_1_xrl_target_death(
	// Input values,
	const string&	target_class,
	const string&	target_instance);

    void finder_register_done(const XrlError& e, string clientname);

private:
    typedef XorpCallback1<void, const XrlError&>::RefPtr GENERIC_CALLBACK;

    void save_config_done(bool success, string error_msg, string filename,
			  uid_t user_id, string target);
    void load_config_done(bool success, string error_msg, string deltas,
			  string deletions, string filename, uid_t user_id,
			  string target);
    UserInstance* find_user_instance(uid_t user_id, const string& clientname);
    string generate_auth_token(uid_t user_id, const string& clientname);
    bool verify_token(const string& token) const;
    uid_t get_user_id_from_token(const string& token) const;
    void lock_timeout();
    void initialize_client_state(uid_t user_id, UserInstance *user);
    void send_client_state(uid_t user_id, string clientname);
    uint32_t allocate_clientid();
    void deallocate_clientid(uint32_t clientid);

    XrlRouter& _xrl_router;
    XrlRtrmgrClientV0p2Client		_client_interface;
    XrlFinderEventNotifierV0p1Client	_finder_notifier_interface;
    multimap<uid_t, UserInstance*>	_users;
    multimap<uid_t, UserInstance*>	_config_users;
    set<uint32_t>	_clientids;
    UserDB&		_userdb;
    MasterConfigTree*	_master_config_tree;
    EventLoop&		_eventloop;
    RandomGen&		_randgen;
    Rtrmgr&             _rtrmgr;

    bool	_exclusive;  // Indicates only one user allowed in config mode
    string	_exclusive_username;	// The current exclusive user name

    // Variables to implement global lock on config changes
    bool	_config_locked;
    string	_lock_holder_token;
    XorpTimer	_lock_timer;

    // generic place to store background task callback timers
    list <XorpTimer> _background_tasks;

    bool	_verbose;	//  Set to true if output is verbose
};

#endif // __RTRMGR_XRL_RTRMGR_INTERFACE_HH__
