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

#ident "$XORP: xorp/rtrmgr/userdb.cc,v 1.6 2004/05/28 22:28:00 pavlin Exp $"


#include <sys/types.h>
#include <grp.h>
#include <pwd.h>

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "userdb.hh"


User::User(uint32_t user_id, const string& username)
    : _user_id(user_id),
      _username(username)
{

}

bool 
User::has_acl_capability(const string& capname) const
{
    set<string>::iterator iter;

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

UserInstance::UserInstance(uint32_t user_id, const string& username) 
    : User(user_id, username)
{

}


UserDB::UserDB(bool verbose)
    : _verbose(verbose)
{

}

UserDB::~UserDB()
{
    map<uint32_t, User*>::iterator iter;

    for (iter = _users.begin(); iter != _users.end(); ++iter) {
	delete iter->second;
    }
}

void 
UserDB::load_password_file()
{
    struct passwd* pwent;

    pwent = getpwent();
    while (pwent != NULL) {
	debug_msg("User: %s UID: %u\n", pwent->pw_name, pwent->pw_uid);
	add_user(pwent->pw_uid, pwent->pw_name);
	pwent = getpwent();
    }
    endpwent();
}

User* 
UserDB::add_user(uint32_t user_id, const string& username)
{
    if (_users.find(user_id) == _users.end()) {
	User* newuser = new User(user_id, username);
	struct group* grp = getgrnam("xorp");
	if (grp != NULL) {
	    debug_msg("group xorp exists, id=%d\n", grp->gr_gid);
	    char **gr_mem = grp->gr_mem;
	    while (*gr_mem != NULL) {
		debug_msg("found user %s in group xorp\n", *gr_mem);
		if (*gr_mem == username) {
		    newuser->add_acl_capability("config");
		    break;
		}
		gr_mem++;
	    }
	} else {
	    XLOG_ERROR("Group \"xorp\" does not exist on this system.");
	}
	_users[user_id] = newuser;
	return newuser;
    } else {
	// This user_id already exists
	return NULL;
    }
}

const User* 
UserDB::find_user_by_user_id(uint32_t user_id) const
{
    map<uint32_t,User*>::const_iterator iter = _users.find(user_id);

    if (iter == _users.end())
	return NULL;
    else
	return iter->second;
}

void 
UserDB::remove_user(uint32_t user_id)
{
    map<uint32_t,User*>::iterator iter = _users.find(user_id);
    User* user = iter->second;

    _users.erase(iter);
    delete user;
}

bool
UserDB::has_capability(uint32_t user_id, const string& capability) const
{
    const User* user = find_user_by_user_id(user_id);

    if (user == NULL) 
	return false;
    return (user->has_acl_capability(capability));
}
