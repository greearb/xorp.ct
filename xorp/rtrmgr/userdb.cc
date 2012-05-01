// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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

#ifdef HAVE_GRP_H
#include <grp.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include "userdb.hh"


User::User(uid_t user_id, const string& username)
    : _user_id(user_id),
      _username(username)
{

}

bool 
User::has_acl_capability(const string& capname, string& err_msg) const
{
    set<string>::const_iterator iter;

    iter = _capabilities.find(capname);
    if (iter == _capabilities.end()) {
	char msg[128];
	snprintf(msg, 128, "Cannot find capability: %s in user: %s (%i)",
		 capname.c_str(), _username.c_str(), (int)(_user_id));
	err_msg = msg;
	return false;
    } else {
	return true;
    }
}

void 
User::add_acl_capability(const string& capname)
{
    set<string>::iterator iter;

    iter = _capabilities.find(capname);
    if (iter == _capabilities.end()) {
	_capabilities.insert(capname);
    }
}

UserInstance::UserInstance(uid_t user_id, const string& username) 
    : User(user_id, username), _authenticated(false), _config_mode(false),
      _is_a_zombie(false)
{

}


UserDB::UserDB(bool verbose)
    : _verbose(verbose)
{

}

UserDB::~UserDB()
{
    map<uid_t, User*>::iterator iter;

    for (iter = _users.begin(); iter != _users.end(); ++iter) {
	delete iter->second;
    }
}

void 
UserDB::load_password_file()
{
#ifdef HOST_OS_WINDOWS
    string username("Administrator");
    uid_t userid = 0;
    add_user(userid, username, userid);
#else // ! HOST_OS_WINDOWS
    struct passwd* pwent;

    setpwent();			// XXX: Rewind the database
    pwent = getpwent();
    while (pwent != NULL) {
	debug_msg("User: %s UID: %u\n", pwent->pw_name,
		  XORP_UINT_CAST(pwent->pw_uid));
	add_user(pwent->pw_uid, pwent->pw_name, pwent->pw_gid);
	pwent = getpwent();
    }
    endpwent();
#endif // ! HOST_OS_WINDOWS
}

User* 
UserDB::add_user(uid_t user_id, const string& username, gid_t pw_gid)
{
    if (_users.find(user_id) == _users.end()) {
	User* newuser = new User(user_id, username);
#ifndef HOST_OS_WINDOWS
	struct group* grp = getgrnam("xorp");
	if (grp != NULL) {
	    debug_msg("group xorp exists, id=%u\n",
		      XORP_UINT_CAST(grp->gr_gid));
	    if (pw_gid == (gid_t)grp->gr_gid) {
		/* is the user's default group is the "xorp" group */
		debug_msg("user's default group is xorp\n");
		newuser->add_acl_capability("config");
	    } else {
		/* if not, then check if they're listed in /etc/group as
		   being in the xorp group */
		char **gr_mem = grp->gr_mem;
		while (*gr_mem != NULL) {
		    debug_msg("found user %s in group xorp\n", *gr_mem);
		    if (*gr_mem == username) {
			newuser->add_acl_capability("config");
			break;
		    }
		    gr_mem++;
		}
	    }
	} else {
	    XLOG_ERROR("Group \"xorp\" does not exist on this system.");
	}
#else // ! HOST_OS_WINDOWS
	UNUSED(pw_gid);
	newuser->add_acl_capability("config");
#endif // ! HOST_OS_WINDOWS
	_users[user_id] = newuser;
	return newuser;
    } else {
	// This user_id already exists
	return NULL;
    }
}

const User* 
UserDB::find_user_by_user_id(uid_t user_id)
{
    //
    // XXX: we always reloads the cache on each access to the
    // find_user_by_user_id(). This is not optimal, but guarantees that
    // the user accounts in the system and the rtrmgr are in-sync.
    //
    load_password_file();

    map<uid_t, User*>::const_iterator iter = _users.find(user_id);

    if (iter == _users.end())
	return NULL;
    else
	return iter->second;
}

void 
UserDB::remove_user(uid_t user_id)
{
    map<uid_t, User*>::iterator iter = _users.find(user_id);
    User* user = iter->second;

    _users.erase(iter);
    delete user;
}

bool
UserDB::has_capability(uid_t user_id, const string& capability, string& err_msg)
{
    const User* user = find_user_by_user_id(user_id);

    if (user == NULL) {
	char msg[128];
	snprintf(msg, 128, "Cannot find user: %i\n", (int)(user_id));
	err_msg = msg;
	return false;
    }
    return (user->has_acl_capability(capability, err_msg));
}
