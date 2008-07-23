// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/rtrmgr/userdb.cc,v 1.22 2008/01/04 03:17:45 pavlin Exp $"

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
User::has_acl_capability(const string& capname) const
{
    set<string>::const_iterator iter;

    iter = _capabilities.find(capname);
    if (iter == _capabilities.end()) {
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
		debug_msg("user's default groyp is xorp\n");
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
UserDB::has_capability(uid_t user_id, const string& capability)
{
    const User* user = find_user_by_user_id(user_id);

    if (user == NULL) 
	return false;
    return (user->has_acl_capability(capability));
}
