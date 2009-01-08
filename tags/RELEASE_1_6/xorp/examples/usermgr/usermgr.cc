// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008-2009 XORP, Inc.
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

#ident "$XORP: xorp/examples/usermgr/usermgr.cc,v 1.4 2008/11/18 22:57:08 atanu Exp $"

#include "libxorp/xorp.h"
#include "usermgr_module.h"
#include "usermgr.hh"

int
Users::add_user(const string& username, uint32_t userid)
{
    User * tmp = new User(username, userid);
    _users.push_front(tmp);
    return 0;
}

int
Users::del_user(const string& username) 
{
    User * tmp = NULL;
    list<User*>::iterator pos = _users.begin();
    for (; pos != _users.end(); ++pos) {
	if (((User*)*pos)->name() == username) {
	    tmp = *pos;
	    break;
	}
    }
    if (tmp != NULL) {
	_users.remove(tmp);
       	delete tmp;
       	return 0;
    } else {
	return -1;
    }
}

string
Users::str(void) const
{
    string result;

    list<User*>::const_iterator pos = _users.begin();
    result = "\nUsers\n";
    result += "Name	        ID\n";
    result += "-----------     -------\n";
    for (; pos != _users.end(); ++pos) {
	result += (*pos)->str();
    }

    return result;
}

vector<string>
Users::list_users(void) const
{
    list<User*>::const_iterator pos = _users.begin();
    vector<string> result;
    for (; pos != _users.end(); ++pos) {
	result.push_back((*pos)->name());
    }
    return result;
}

int
Groups::add_group(const string& groupname, uint32_t groupid)
{
    Group * tmp = new Group(groupname, groupid);
    _groups.push_front(tmp);
    return 0;
}

int
Groups::del_group(const string& groupname )
{
    Group * tmp = NULL;
    list<Group*>::iterator pos = _groups.begin();
    for (; pos != _groups.end(); ++pos) {
	if (((Group*)*pos)->name() == groupname) {
	    tmp = *pos;
	    break;
	}
    }
    if (tmp != NULL) {
	_groups.remove(tmp);
       	delete tmp;
       	return 0;
    } else {
	return -1;
    }
}

string
Groups::str() const
{
    string result;

    list<Group*>::const_iterator pos = _groups.begin();
    result = "\nGroups\n";
    result += "Name	        ID\n";
    result += "-----------     -------\n";
    for (; pos != _groups.end(); ++pos) {
	result += (*pos)->str();
    }

    return result;
}

vector<string>
Groups::list_groups(void)
{
    list<Group*>::iterator pos = _groups.begin();
    vector<string> result;
    for (; pos != _groups.end(); ++pos) {
	result.push_back(((Group*)*pos)->name());
    }
    return result;
}

