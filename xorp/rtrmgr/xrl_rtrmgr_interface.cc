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



#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/status_codes.h"

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "main_rtrmgr.hh"
#include "master_conf_tree.hh"
#include "module_manager.hh"
#include "randomness.hh"
#include "userdb.hh"
#include "xrl_rtrmgr_interface.hh"


XrlRtrmgrInterface::XrlRtrmgrInterface(XrlRouter& r, UserDB& userdb,
				       EventLoop& eventloop,
				       RandomGen& randgen,
				       Rtrmgr& rtrmgr) 
    : XrlRtrmgrTargetBase(&r),
      _xrl_router(r),
      _client_interface(&r),
      _finder_notifier_interface(&r),
      _userdb(userdb),
      _master_config_tree(NULL),
      _eventloop(eventloop), 
      _randgen(randgen),
      _rtrmgr(rtrmgr),
      _exclusive(false),
      _config_locked(false),
      _lock_holder_token(""),
      _verbose(rtrmgr.verbose())
{
}

XrlRtrmgrInterface::~XrlRtrmgrInterface()
{
    multimap<uid_t, UserInstance*>::iterator iter;
    for (iter = _users.begin(); iter != _users.end(); ++iter) {
	delete iter->second;
    }
}

XrlCmdError
XrlRtrmgrInterface::common_0_1_get_target_name(// Output values,
					       string& name)
{
    name = XrlRtrmgrTargetBase::name();
    return XrlCmdError::OKAY();
}


XrlCmdError
XrlRtrmgrInterface::common_0_1_get_version(// Output values, 
					   string& v)
{
    v = string(version());
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRtrmgrInterface::common_0_1_get_status(// Output values, 
					  uint32_t& status,
					  string& reason)
{
    // XXX placeholder only
    status = PROC_READY;
    reason = "Ready";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRtrmgrInterface::common_0_1_shutdown()
{
    string error_msg;

    // The rtrmgr does not accept XRL requests to shutdown via this interface.
    error_msg = "Command not supported";
    return XrlCmdError::COMMAND_FAILED(error_msg);
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_get_pid(// Output values, 
				      uint32_t& pid)
{
    pid = getpid();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_register_client(
	// Input values, 
	const uint32_t&	user_id, 
	const string&	clientname, 
	// Output values, 
	string&	filename,
	uint32_t& pid,
	uint32_t& clientid)
{
    pid = getpid();

    const User* user;
    UserInstance* newuser;
    string error_msg;

    //
    // Prevent a clientname passing characters that might cause us to
    // overwrite files later.  Prohibiting "/" makes allowing ".." safe.
    //
    if (strchr(clientname.c_str(), '/') != NULL) {
	error_msg = c_format("Illegal character in clientname: %s", 
			     clientname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Note that clientname is user supplied data, so this would be
    // dangerous if the user could pass "..".
    //
    filename = "/tmp/rtrmgr-" + clientname;
    mode_t oldmode = umask(S_IRWXG|S_IRWXO);
    debug_msg("newmode: %o oldmode: %o\n", S_IRWXG|S_IRWXO,
	      XORP_UINT_CAST(oldmode));

    FILE* file = fopen(filename.c_str(), "w+");
    if (file == NULL) {
	error_msg = c_format("Failed to create temporary file: %s",
			     filename.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    umask(oldmode);

    user = _userdb.find_user_by_user_id(user_id);
    if (user == NULL) {
	fclose(file);
	unlink(filename.c_str());
	error_msg = c_format("User ID %u not found",
			     XORP_UINT_CAST(user_id));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    newuser = new UserInstance(user->user_id(), user->username());
    newuser->set_clientname(clientname);
    newuser->set_authtoken(generate_auth_token(user_id, clientname));
    newuser->set_authenticated(false);
    clientid = allocate_clientid();
    newuser->set_clientid(clientid);
    _users.insert(pair<uid_t, UserInstance*>(user_id, newuser));

    //
    // Be careful here - if for any reason we fail to create, write to,
    // or close the temporary auth file, that's a very different error
    // from if the xorpsh process can't read it correctly.
    //
    if (fprintf(file, "%s", newuser->authtoken().c_str()) 
	< (int)(newuser->authtoken().size())) {
	fclose(file);
	unlink(filename.c_str());
	error_msg = c_format("Failed to write to temporary file: %s",
			     filename.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (fchown(fileno(file), user_id, (gid_t)-1) != 0) {
	fclose(file);
	unlink(filename.c_str());
	error_msg = c_format("Failed to chown temporary file %s to user_id %u",
			     filename.c_str(), XORP_UINT_CAST(user_id));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (fclose(file) != 0) {
	unlink(filename.c_str());
	error_msg = c_format("Failed to close temporary file: %s",
			     filename.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

#ifdef NO_XORPSH_AUTHENTICATION
    //
    // XXX: a hack to return back the key itself in case we run
    // xorpsh on a remote machine.
    //
    filename = newuser->authtoken();
#endif

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_unregister_client(const string& token)
{
    string error_msg;

    if (!verify_token(token)) {
	error_msg = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    multimap<uid_t, UserInstance*>::iterator iter;
    for (iter = _config_users.begin(); iter != _config_users.end(); ++iter) {
	if (iter->second->authtoken() == token) {
	    _config_users.erase(iter);
	    return XrlCmdError::OKAY();
	}
    }
    for (iter = _users.begin(); iter != _users.end(); ++iter) {
	if (iter->second->authtoken() == token) {
	    deallocate_clientid(iter->second->clientid());
	    delete iter->second;
	    _users.erase(iter);
	    return XrlCmdError::OKAY();
	}
    }
    // This cannot happen, because the token wouldn't have verified
    XLOG_UNREACHABLE();
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_authenticate_client(
    // Input values, 
    const uint32_t&	user_id, 
    const string&	clientname, 
    const string&	token)
{
    UserInstance *user;
    string error_msg;

    user = find_user_instance(user_id, clientname);
    if (user == NULL) {
	error_msg = "User instance not found (maybe login timed out)";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (token == user->authtoken()) {
	user->set_authenticated(true);
	initialize_client_state(user_id, user);
	return XrlCmdError::OKAY();
    } else {
	error_msg = "Bad authtoken";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
}

void
XrlRtrmgrInterface::initialize_client_state(uid_t user_id, 
					    UserInstance *user)
{
    // We need to register interest via the finder in the XRL target
    // associated with this user instance, so that when the user
    // instance goes away without deregistering, we can clean up the
    // state we associated with them.

    XrlFinderEventNotifierV0p1Client::RegisterInstanceEventInterestCB cb; 
    cb = callback(this, &XrlRtrmgrInterface::finder_register_done,
		  user->clientname());
    _finder_notifier_interface.
	send_register_class_event_interest("finder", 
					   _xrl_router.instance_name(),
					   user->clientname(), cb);
    XLOG_TRACE(_verbose, 
	       "registering interest in %s\n", user->clientname().c_str());


    //
    // We need to send the running config and module state to the
    // client, but we first need to return from the current XRL, so we
    // schedule this on a zero-second timer.
    //
    XorpTimer t;
    uint32_t delay = 0;
    if (!_rtrmgr.ready()) {
	delay = 2000;
    }
    t = _eventloop.new_oneoff_after_ms(delay,
             callback(this, &XrlRtrmgrInterface::send_client_state, 
		      user_id, user->clientname()));
    _background_tasks.push_front(t);
}

void
XrlRtrmgrInterface::finder_register_done(const XrlError& e, string clientname) 
{
    UNUSED(clientname);
    if (e != XrlError::OKAY()) {
	XLOG_ERROR("Failed to register with finder about XRL %s (err: %s)\n",
		   clientname.c_str(), e.error_msg());
    }
}

void
XrlRtrmgrInterface::send_client_state(uid_t user_id, string client)
{
    UserInstance *user = find_user_instance(user_id, client);
    if (user == NULL)
	return;

    debug_msg("send_client_state %s\n", client.c_str());
    if (!_rtrmgr.ready()) {
	//
	// We're still in the process of reconfiguring, so delay this
	// until later.
	//
	initialize_client_state(user_id, user);
	return;
    }

    GENERIC_CALLBACK cb2;
    cb2 = callback(this, &XrlRtrmgrInterface::client_updated, user_id, user);

    //
    // Send the module status
    //
    debug_msg("Sending mod status changed to %s\n", client.c_str());
    list<string> module_names;
    ModuleManager &mmgr(_master_config_tree->module_manager());
    module_names = mmgr.get_module_names();
    list<string>::iterator iter;
    for (iter = module_names.begin(); iter != module_names.end(); ++iter) {
	const string& module_name = *iter;
	debug_msg("module: %s\n", module_name.c_str());
	Module::ModuleStatus status = mmgr.module_status(module_name);
	if (status != Module::NO_SUCH_MODULE) {
	    debug_msg("status %d\n", status);
	    _client_interface.send_module_status(client.c_str(), module_name,
						 static_cast<uint32_t>(status),
						 cb2);
	}
    } 

    //
    // Send the configuration
    //
    debug_msg("Sending config changed to %s\n", client.c_str());
    string config = _master_config_tree->show_tree(/*numbered*/ true);
    _client_interface.send_config_changed(client.c_str(), 0, config, "", cb2);
}

XrlCmdError 
XrlRtrmgrInterface::rtrmgr_0_1_enter_config_mode(
    // Input values, 
    const string&	token, 
    const bool&		exclusive)
{
    string error_msg;
    string reason;

    if (! verify_token(token)) {
	error_msg = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    uid_t user_id = get_user_id_from_token(token);
    if (_userdb.has_capability(user_id, "config", reason) == false) {
	error_msg = "You do not have permission for this operation, token: " + token + " reason: " + reason;
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    XLOG_TRACE(_verbose, "user %u entering config mode\n",
	       XORP_UINT_CAST(user_id));
    if (exclusive)
	XLOG_TRACE(_verbose, "user requested exclusive config\n");
    else
	XLOG_TRACE(_verbose, "user requested non-exclusive config\n");

    //
    // If he's asking for exclusive, and we've already got config users,
    // or there's already an exclusive user, deny the request.
    //
    multimap<uid_t, UserInstance*>::iterator iter;
    if (exclusive && !_config_users.empty()) {
	string user_names;
	for (iter = _config_users.begin();
	     iter != _config_users.end();
	     ++iter) {
	    UserInstance* user = iter->second;
	    if (! user_names.empty())
		user_names += ", ";
	    user_names += user->username();
	}
	error_msg = c_format("Exclusive config mode requested, but there are "
			     "already other configuration mode users: %s",
			     user_names.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (_exclusive && !_config_users.empty()) {
	error_msg = c_format("User %s is in exclusive configuration mode",
			     _exclusive_username.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    for (iter = _users.begin(); iter != _users.end(); ++iter) {
	if (iter->second->authtoken() == token) {
	    UserInstance* this_user = iter->second;
	    _config_users.insert(make_pair(user_id, this_user));
	    if (exclusive) {
		_exclusive = true;
		_exclusive_username = this_user->username();
	    } else {
		_exclusive = false;
		_exclusive_username = "";
	    }
	    return XrlCmdError::OKAY();
	}
    }
    XLOG_UNREACHABLE();
}

XrlCmdError 
XrlRtrmgrInterface::rtrmgr_0_1_leave_config_mode(
    // Input values, 
    const string&	token)
{
    string error_msg;

    if (!verify_token(token)) {
	error_msg = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    XLOG_TRACE(_verbose, "user %u leaving config mode\n",
	       XORP_UINT_CAST(get_user_id_from_token(token)));
    multimap<uid_t, UserInstance*>::iterator iter;
    for (iter = _config_users.begin(); iter != _config_users.end(); ++iter) {
	if (iter->second->authtoken() == token) {
	    _config_users.erase(iter);
	    return XrlCmdError::OKAY();
	}
    }
    error_msg = "User was not in configuration mode";
    return XrlCmdError::COMMAND_FAILED(error_msg);
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_get_config_users(
    // Input values, 
    const string&	token, 
    // Output values, 
    XrlAtomList&	users)
{
    string error_msg;
    string reason;

    if (!verify_token(token)) {
	error_msg = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    uid_t user_id = get_user_id_from_token(token);
    if (_userdb.has_capability(user_id, "config", reason) == false) {
	error_msg = "You do not have permission to view the cnofiguration, token: " + token + " reason: " + reason;
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    multimap<uid_t, UserInstance*>::const_iterator iter;
    for (iter = _config_users.begin(); iter != _config_users.end(); ++iter) {
	UserInstance* user = iter->second;
	if (!user->is_a_zombie()) {
	    users.append(XrlAtom(static_cast<uint32_t>(user->user_id())));
	} else {
	    XLOG_WARNING("user %u is a zombie\n",
			 XORP_UINT_CAST(user->user_id()));
	}
    }
    return XrlCmdError::OKAY();
}

/**
 * This interface is deprecated as the main way for xorpsh to get the
 * running config from the router manager.  It is retained for
 * debugging purposes.  xorpsh now gets its config automatically
 * immediatedly after registering which removes a potential timing
 * hole where the client could be told about a change to the config
 * before it has asked what the current config is.
*/
XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_get_running_config(
    // Input values, 
    const string&	token, 
    // Output values, 
    bool&		ready,
    string&		config)
{
    string error_msg;

    if (!verify_token(token)) {
	error_msg = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (_rtrmgr.ready()) {
	ready = true;
	config = _master_config_tree->show_tree(/*numbered*/ true);
    } else {
	ready = false;
	config = "";
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_apply_config_change(
    // Input values, 
    const string&	token, 
    const string&	target, 
    const string&	deltas,
    const string&	deletions)
{
    string error_msg;
    string reason;

    if (!verify_token(token)) {
	error_msg = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (_config_locked == false) {
	error_msg = "Cannot commit config changes without locking the config";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    uid_t user_id = get_user_id_from_token(token);
    if (_userdb.has_capability(user_id, "config", reason) == false) {
	error_msg = "You do not have permission for this operation, token: " + token + " reason: " + reason;
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    // XXX: TBD
    XLOG_WARNING("\nXRL got config change: deltas: \n%s\nend deltas\ndeletions:\n%s\nend deletions\n",
	       deltas.c_str(), deletions.c_str());

    ConfigChangeCallBack cb;
    cb = callback(this, &XrlRtrmgrInterface::apply_config_change_done,
		  user_id, string(target));
    if (_master_config_tree->apply_config_change(user_id, error_msg, deltas,
						 deletions, cb) != true) {
	XLOG_WARNING("config change failed: %s", error_msg.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    return XrlCmdError::OKAY();
}

void
XrlRtrmgrInterface::apply_config_change_done(bool success,
					     string error_msg,
					     string deltas,
					     string deletions,
					     uid_t user_id,
					     string target)
{
    XLOG_WARNING("apply_config_change_done: status: %d response: %s target: %s",
		 success, error_msg.c_str(), target.c_str());

    if (success) {
	// Check everything really worked, and finalize the commit
	if (_master_config_tree->check_commit_status(error_msg) == false) {
	    XLOG_WARNING("check commit status indicates failure: >%s<, back out changes.\n",
			 error_msg.c_str());
	    success = false;
	}
    }  else {
	XLOG_WARNING("request failed: >%s<\n", error_msg.c_str());
    }

    GENERIC_CALLBACK cb1;
    cb1 = callback(this, &XrlRtrmgrInterface::apply_config_change_done_cb);

    if (success) {
	// Send the changes to all the clients, including the client
	// that originally sent them.
	multimap<uid_t, UserInstance*>::iterator iter;
	for (iter = _users.begin(); iter != _users.end(); ++iter) {
	    string client = iter->second->clientname();
	    GENERIC_CALLBACK cb2;
	    cb2 = callback(this, &XrlRtrmgrInterface::client_updated, 
			   iter->first, iter->second);
	    debug_msg("Sending config changed to %s\n", client.c_str());
	    _client_interface.send_config_changed(client.c_str(),
						  user_id,
						  deltas, deletions, cb2);
	}
	XLOG_WARNING("Sending config change done (success) to %s\n", target.c_str());
	_client_interface.send_config_change_done(target.c_str(),
						  true, "", cb1);
    } else {
	// Something went wrong
	_master_config_tree->discard_changes();
	XLOG_WARNING("Sending config change failed to %s, error_msg: %s\n",
		     target.c_str(), error_msg.c_str());
	_client_interface.send_config_change_done(target.c_str(),
						  false, error_msg, cb1);
    }
}

void
XrlRtrmgrInterface::config_saved_done_cb(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	XLOG_ERROR("Failed to notify client that config change was saved: %s",
		   e.error_msg());
    }
}

void
XrlRtrmgrInterface::apply_config_change_done_cb(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	XLOG_ERROR("Failed to notify client that config change was done: %s",
		   e.error_msg());
    }
}

void
XrlRtrmgrInterface::client_updated(const XrlError& e, uid_t user_id, 
				   UserInstance* user)
{
    if (e != XrlError::OKAY()) {
	XLOG_ERROR("Failed to notify client that config changed: %s\n",
		   e.error_msg());
	if (e != XrlError::COMMAND_FAILED()) {
	    // We failed to notify the user instance, so we note that
	    // they are probably dead.  We can't guarantee that the
	    // "user" pointer is still valid, so check before using it.
	    multimap<uid_t, UserInstance*>::iterator iter;
	    for (iter = _users.lower_bound(user_id);
		 iter != _users.end();
		 iter++) {
		if (iter->second == user) {
		    user->set_zombie(true);
		    return;
		}
	    }
	}
    }
}

void
XrlRtrmgrInterface::module_status_changed(const string& modname,
					  GenericModule::ModuleStatus status)
{
    multimap<uid_t, UserInstance*>::iterator iter;
    for (iter = _users.begin(); iter != _users.end(); ++iter) {
	string client = iter->second->clientname();
	GENERIC_CALLBACK cb2;
	cb2 = callback(this, &XrlRtrmgrInterface::client_updated, 
		       iter->first, iter->second);
	debug_msg("Sending mod statis changed to %s\n", client.c_str());
	_client_interface.send_module_status(client.c_str(), 
					     modname, (uint32_t)status, cb2);
    }
}


XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_lock_config(
    // Input values, 
    const string&	token, 
    const uint32_t&	timeout /* in milliseconds */, 
    // Output values, 
    bool&		success,
    uint32_t&		holder)
{
    string error_msg;
    string reason;

    if (!verify_token(token)) {
	error_msg = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    uid_t user_id = get_user_id_from_token(token);
    if (_userdb.has_capability(user_id, "config", reason) == false) {
	error_msg = "You do not have permission for this operation, token: " + token + " reason: " + reason;
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (_config_locked) {
	// Can't return COMMAND_FAILED and return the lock holder
	success = false;
	holder = get_user_id_from_token(_lock_holder_token);
	return XrlCmdError::OKAY();
    } else {
	success = true;
	_config_locked = true;
	_lock_holder_token = token;
	_lock_timer = _eventloop.new_oneoff_after_ms(
	    timeout, 
	    callback(this, &XrlRtrmgrInterface::lock_timeout));
	return XrlCmdError::OKAY();
    }
}

void
XrlRtrmgrInterface::lock_timeout()
{
    _config_locked = false;
    _lock_holder_token = "";
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_unlock_config(
    // Input values
    const string&	token)
{
    string error_msg;

    if (!verify_token(token)) {
	error_msg = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (_config_locked) {
	if (token != _lock_holder_token) {
	    error_msg = c_format("Configuration is not held by process %s", 
				 token.c_str());
	    return XrlCmdError::COMMAND_FAILED(error_msg);
	}
	_config_locked = false;
	_lock_timer.unschedule();
	_lock_holder_token = "";
	return XrlCmdError::OKAY();
    } else {
	error_msg = "Configuration is not locked";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_lock_node(
    // Input values, 
    const string&	token, 
    const string&	node, 
    const uint32_t&	timeout, 
    // Output values, 
    bool&		success, 
    uint32_t&		holder)
{
    string error_msg;
    string reason;

    if (!verify_token(token)) {
	error_msg = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    uid_t user_id = get_user_id_from_token(token);
    if (_userdb.has_capability(user_id, "config", reason) == false) {
	error_msg = "You do not have permission for this operation, token: " + token + " reason: " + reason;
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (user_id == (uid_t)-1) {
	// Shouldn't be possible as we already checked the token
	XLOG_UNREACHABLE();
    }
    success = _master_config_tree->lock_node(node, user_id, timeout, holder);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_unlock_node(
    // Input values, 
    const string&	token, 
    const string&	node)
{
    string error_msg;

    if (!verify_token(token)) {
	error_msg = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    uid_t user_id = get_user_id_from_token(token);
    if (user_id == (uid_t)-1) {
	// Shouldn't be possible as we already checked the token
	XLOG_UNREACHABLE();
    }
    bool success;
    success = _master_config_tree->unlock_node(node, user_id);
    if (success) {
	return XrlCmdError::OKAY();
    } else {
	error_msg = "Unlocking the configuration failed";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_save_config(// Input values: 
					   const string& token,
					   const string& target,
					   const string& filename)
{
    string error_msg;
    string reason;

    if (!verify_token(token)) {
	error_msg = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    uid_t user_id = get_user_id_from_token(token);
    if (_userdb.has_capability(user_id, "config", reason) == false) {
	error_msg = "You do not have permission for this operation, token: " + token + " reason: " + reason;
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (_config_locked && (token != _lock_holder_token)) {
	uid_t uid = get_user_id_from_token(_lock_holder_token);
	string holder = _userdb.find_user_by_user_id(uid)->username();
	error_msg = c_format("The configuration is currently locked by user %s",
			     holder.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    ConfigSaveCallBack cb;
    cb = callback(this, &XrlRtrmgrInterface::save_config_done,
		  filename, user_id, target);
    if (_master_config_tree->save_config(filename, user_id, error_msg, cb)
	!= true) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_load_config(// Input values:
                                           const string& token,
					   const string& target,
					   const string& filename)
{
    string error_msg;
    string reason;

    if (!verify_token(token)) {
	error_msg = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    uid_t user_id = get_user_id_from_token(token);
    if (_userdb.has_capability(user_id, "config", reason) == false) {
	error_msg = "You do not have permission for this operation, token: " + token + " reason: " + reason;
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (_config_locked && (token != _lock_holder_token)) {
	uid_t uid = get_user_id_from_token(_lock_holder_token);
	string holder = _userdb.find_user_by_user_id(uid)->username();
	error_msg = c_format("The configuration is currently locked by user %s",
			     holder.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    ConfigLoadCallBack cb;
    cb = callback(this, &XrlRtrmgrInterface::load_config_done,
		  filename, user_id, target);
    if (_master_config_tree->load_config(filename, user_id, error_msg, cb)
	!= true) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

void
XrlRtrmgrInterface::save_config_done(bool success,
				     string error_msg,
				     string filename,
				     uid_t user_id,
				     string target)
{
    GENERIC_CALLBACK cb1;

    cb1 = callback(this, &XrlRtrmgrInterface::config_saved_done_cb);
    _client_interface.send_config_saved_done(target.c_str(), success,
					     error_msg, cb1);

    UNUSED(filename);
    UNUSED(user_id);
}

void
XrlRtrmgrInterface::load_config_done(bool success,
				     string error_msg,
				     string deltas,
				     string deletions,
				     string filename,
				     uid_t user_id,
				     string target)
{
    // Propagate the changes to all clients
    apply_config_change_done(success, error_msg, deltas, deletions,
			     user_id, target);

    UNUSED(filename);
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_set_config_directory(
    // Input values,
    const string&	config_directory)
{
    _master_config_tree->set_config_directory(config_directory);

    return XrlCmdError::OKAY();
}

UserInstance *
XrlRtrmgrInterface::find_user_instance(uid_t user_id, const string& clientname)
{
    multimap<uid_t, UserInstance*>::iterator iter;

    iter = _users.lower_bound(user_id);
    while (iter != _users.end() && iter->second->user_id() == user_id) {
	if (iter->second->clientname() == clientname)
	    return iter->second;
	++iter;
    }
    return NULL;
}

bool
XrlRtrmgrInterface::verify_token(const string& token) const
{
    uid_t user_id = get_user_id_from_token(token);

    if (user_id == (uid_t)-1)
	return false;
    multimap<uid_t, UserInstance*>::const_iterator iter;
    iter = _users.lower_bound(user_id);
    while (iter != _users.end() && iter->second->user_id() == user_id) {
	if (iter->second->authtoken() == token)
	    return true;
	++iter;
    }
    return false;
}

uid_t
XrlRtrmgrInterface::get_user_id_from_token(const string& token) const
{
    uid_t user_id;
    unsigned int u;
    size_t tlen = token.size();

    if (token.size() < 10)
	return (uid_t)-1;
    string uidstr = token.substr(0, 10);
    string authstr = token.substr(10, tlen - 10);
    sscanf(uidstr.c_str(), "%u", &u);
    user_id = u;
    return user_id;
}

string
XrlRtrmgrInterface::generate_auth_token(uid_t user_id,
					const string& clientname)
{
    string token;
    token = c_format("%10u", XORP_UINT_CAST(user_id));
    string shortcname;
    size_t clen = clientname.size();

    if (clen > CNAMELEN) {
	shortcname = clientname.substr(clen - CNAMELEN, CNAMELEN);
    } else {
	shortcname = clientname;
	for (size_t i = 0; i < (CNAMELEN - clen); i++)
	    shortcname += '*';
    }
    token += shortcname;

    string randomness;
    uint8_t buf[16];
    _randgen.get_random_bytes(16, buf);
    for (size_t i = 0; i < 16; i++)
	randomness += c_format("%2x", buf[i]);
    token += randomness;

    return token;
}


XrlCmdError 
XrlRtrmgrInterface::finder_event_observer_0_1_xrl_target_birth(
    const string&	target_class,
    const string&	target_instance)
{
    XLOG_TRACE(_verbose, "XRL Birth: class %s instance %s\n",
	       target_class.c_str(), target_instance.c_str());
    UNUSED(target_class); // in case XLOG_TRACE is compiled out
    UNUSED(target_instance);
    return XrlCmdError::OKAY();
}


XrlCmdError 
XrlRtrmgrInterface::finder_event_observer_0_1_xrl_target_death(
    const string&	target_class,
    const string&	target_instance)
{
    XLOG_TRACE(_verbose, "XRL Death: class %s instance %s\n",
	       target_class.c_str(), target_instance.c_str());

    UNUSED(target_instance); // in case XLOG_TRACE is compiled out

    multimap<uid_t, UserInstance*>::iterator iter;
    for (iter = _config_users.begin(); iter != _config_users.end(); ++iter) {
	UserInstance* user = iter->second;
	if (user->clientname() == target_class) {
	    debug_msg("removing user %s from list of config users\n",
		      target_class.c_str());
	    _config_users.erase(iter);
	    break;
	}
    }
    for (iter = _users.begin(); iter != _users.end(); ++iter) {
	UserInstance* user = iter->second;
	if (user->clientname() == target_class) {
	    debug_msg("removing user %s from list of regular users\n",
		      target_class.c_str());
	    _users.erase(iter);
	    break;
	}
    }
    return XrlCmdError::OKAY();
}

uint32_t 
XrlRtrmgrInterface::allocate_clientid()
{
    /* find the first unused client ID number */
    uint32_t expected = 1;
    set<uint32_t>::iterator i;
    for (i = _clientids.begin();; i++) {
	if ((i == _clientids.end()) || (*i > expected)) {
	    _clientids.insert(expected);
	    return expected;
	}
	expected++;
    }
    XLOG_UNREACHABLE();
}

void
XrlRtrmgrInterface::deallocate_clientid(uint32_t clientid)
{
    set<uint32_t>::iterator i;
    i = _clientids.find(clientid);
    XLOG_ASSERT(i != _clientids.end());
    _clientids.erase(i);
}
