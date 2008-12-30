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

// $XORP: xorp/examples/usermgr/usermgr.hh,v 1.3 2008/11/18 19:15:09 atanu Exp $

#ifndef __USERMGR_USERMGR_HH__
#define __USERMGR_USERMGR_HH__

/*
 * operational code to maintain an in core user database.
 */
#include "usermgr_module.h"
#include "libxorp/xorp.h"
#include "libxorp/c_format.hh"
#include <list>
#include <vector>

class User {
public:
    User(const string& username, uint32_t userid)
	: _name(username),
	  _id(userid)
    {}

//     ~User() { return; }

    void set_name(const string& name) { _name = name; }

    void set_description(const string& description) { 
	_description = description;
    }

    void set_password(const string& password) {
	_password = password;
    }

    void set_id(const uint32_t id) { _id = id; }

    string name() const { return _name; }

    string description() const { return _description; }

    string password() const { return _password; }

    uint32_t id() const { return _id; }

    void set_next(User * next) { _next = next; }

    User *next() { return _next; }

    string str() const {
	return c_format("%11s     %d\n", _name.c_str(), _id);
    }

private:
    string	_name;
    string	_description;
    string	_password;
    uint32_t	_id;
    User *	_next;
};

class Users {
public:
//     Users() { }
//     ~Users() { return; }

    int add_user(const string& username, uint32_t userid);

    int del_user(const string& username );

    vector<string> list_users() const;

    string str() const;

private:
    list<User*>	_users;
};

class Group {
public:
    Group(const string& groupname, uint32_t groupid)
	: _name(groupname), _id(groupid)
    {}

//     ~Group() { }

    void set_name(const string name) { _name = name; }

    void set_description(const string description) {
	_description = description;
    }

    void set_id(const uint32_t id) { _id = id; }

    void set_next(Group * next) { _next = next; }

    string name() const { return _name; }

    string description() const { return _description; }

    uint32_t id() const { return _id; }

    Group *next() const { return _next; }

    string str() const {
	return c_format("%11s     %d\n", _name.c_str(), _id);
    }

private:
    string	_name;
    string	_description;
    uint32_t	_id;
    Group *	_next;
};

class Groups {
public:
//     Groups() {}
//     ~Groups() { return; }

    int add_group(const string& groupname, uint32_t groupid);

    int del_group(const string& groupname);

    vector<string> list_groups();

    string str() const;

private:
    list<Group*>	_groups;
};

class UserDB {
public:
//     UserDB() { }
//     ~UserDB() { return; }

    int add_user(const string username, uint32_t userid) {
	return _users.add_user(username, userid);
    }

    int del_user(const string username)	{
	return _users.del_user(username);
    }
    
    int add_group(const string groupname, uint32_t groupid) {
	return _groups.add_group(groupname, groupid);
    }
    
    int del_group(const string groupname) {
	return _groups.del_group(groupname);
    }

    vector<string> list_groups() {
	return _groups.list_groups();
    }

    vector<string> list_users() {
	return _users.list_users();
    }

    string str() const {
	string result;
	
	result = _users.str();
	result += _groups.str();

	return result;
    }

private:
    Users _users;
    Groups _groups;
};

#endif // __USERMGR_USERMGR_HH__

