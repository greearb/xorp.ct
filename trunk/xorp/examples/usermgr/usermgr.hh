// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 XORP, Inc.
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

// $XORP: xorp/devnotes/template_gpl.hh,v 1.1 2008/10/02 21:56:43 bms Exp $

#ifndef __USERMGR_USERMGR_HH__
#define __USERMGR_USERMGR_HH__


/*
 * operational code to maintain an in core user database.
 */
#include "usermgr_module.h"
#include "../libxorp/xorp.h"
#include <list>
#include <vector>

class User {
public:
    User(const string username, uint32_t userid)
	{
	_name=username;
	_id=userid;
	};

    ~User(void) { return; };

    void	set_name(const string name)
		 { _name = name; };
    void	set_description(const string description)
		 { _description = description; };
    void	set_password(const string password)
		 { _password =  password; };
    void	set_id(const uint32_t id)
		 { _id = id; };

    string	name(void)
		 { return _name; };
    string	description(void)
		 { return _description; };
    string	password(void)
		 { return _password; };
    uint32_t	id(void)
		 { return _id; };

    void	set_next(User * next)
		{ _next = next; };
    User * 	next(void)
		 { return _next; };

    void describe(void) {
	printf ("%11s     %d\n", _name.c_str(), _id);
    };
private:
    string	_name;
    string	_description;
    string	_password;
    uint32_t	_id;
    User *	_next;
};

class Users {
public:
    Users(void) { };
    ~Users(void) { return; };
    int AddUser(const string username, uint32_t userid) {
	User * tmp = new User(username, userid);
	_users.push_front(tmp);
	return 0;
    }
    int DelUser(const string username ) 
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
        };

    void describe(void) {
	list<User*>::iterator pos = _users.begin();
	printf("\nUsers\n");
	printf("Name	        ID\n");
	printf("-----------     -------\n");
	for (; pos != _users.end(); ++pos) {
	    ((User*)*pos)->describe();
	}
    };
    vector<string> list_users(void) {
	list<User*>::iterator pos = _users.begin();
	vector<string> result;
	for (; pos != _users.end(); ++pos) {
	    result.push_back(((User*)*pos)->name());
	}
	return result;
    }
private:
    list<User*>	_users;
};

class Group {
public:
    Group(const string groupname, uint32_t groupid)
	{
	_name=groupname;
	_id=groupid;
	};

    ~Group(void) { } ;

    void	set_name(const string name)
		{ _name = name; };
    void	set_description(const string description)
		{ _description = description; };
    void	set_id(const uint32_t id)
		{ _id = id; };
    void	set_next(Group * next)
		{ _next = next; };

    string	name(void)
		{ return _name; };
    string	description(void)
		{ return _description; };
    uint32_t	id(void)
		{ return _id; };
    Group *	next(void)
		{ return _next; };

    void describe(void) {
	printf ("%11s     %d\n", _name.c_str(), _id);
    };
private:
    string	_name;
    string	_description;
    uint32_t	_id;
    Group *	_next;
};

class Groups {
public:
    Groups(void) { };
    ~Groups(void) { return; };
    int AddGroup(const string groupname, uint32_t groupid) {
	Group * tmp = new Group(groupname, groupid);
	_groups.push_front(tmp);
	return 0;
    }
    int DelGroup(const string groupname )
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
        };
    void describe(void) {
	list<Group*>::iterator pos = _groups.begin();
	printf("\nGroups\n");
	printf("Name	        ID\n");
	printf("-----------     -------\n");
	for (; pos != _groups.end(); ++pos) {
	    ((Group*)*pos)->describe();
	}
    };
    vector<string> list_groups(void) {
	list<Group*>::iterator pos = _groups.begin();
	vector<string> result;
	for (; pos != _groups.end(); ++pos) {
	    result.push_back(((Group*)*pos)->name());
	}
	return result;
    }
private:
    list<Group*>	_groups;
};

class UserDB {
public:
    UserDB(void) { };
    ~UserDB(void) { return; } ;
    int AddUser(const string username, uint32_t userid)
	{ return _users.AddUser(username, userid); } ;
    int DelUser(const string username)
	{ return _users.DelUser(username); } ;
    int AddGroup(const string groupname, uint32_t groupid)
	{ return _groups.AddGroup(groupname, groupid); } ;
    int DelGroup(const string groupname)
	{ return _groups.DelGroup(groupname); } ;
    void describe(void) {
	_users.describe();
	_groups.describe();
    };
    vector<string> list_groups() {
	return _groups.list_groups();
    }
    vector<string> list_users() {
	return _users.list_users();
    }
private:
    Users _users;
    Groups _groups;
};

#endif // __USERMGR_USERMGR_HH__

