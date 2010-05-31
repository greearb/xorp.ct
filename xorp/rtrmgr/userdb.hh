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

// $XORP: xorp/rtrmgr/userdb.hh,v 1.16 2008/10/02 21:58:26 bms Exp $

#ifndef __RTRMGR_USERDB_HH__
#define __RTRMGR_USERDB_HH__


#include <map>
#include <set>


class User {
public:
    User(uid_t user_id, const string& username);

    const string& username() const { return _username; }
    uid_t user_id() const { return _user_id; }
    bool has_acl_capability(const string& capname, string& err_msg) const;
    void add_acl_capability(const string& capname);

private:
    uid_t	_user_id;
    string	_username;
    set<string> _capabilities;
};

//
// The same user may be logged in multiple times, so logged in users
// get a UserInstance.
//
class UserInstance : public User {
public:
    UserInstance(uid_t user_id, const string& username);

    const string& clientname() const { return _clientname; }
    void set_clientname(const string& clientname) { _clientname = clientname; }

    uint32_t clientid() const { return _clientid; }
    void set_clientid(uint32_t clientid) { _clientid = clientid; }

    const string& authtoken() const { return _authtoken; }
    void set_authtoken(const string& authtoken) { _authtoken = authtoken; }

    bool is_authenticated() const { return _authenticated; }
    void set_authenticated(bool authenticated) {
	_authenticated = authenticated;
    }

    bool is_in_config_mode() const { return _config_mode; }
    void set_config_mode(bool is_in_config_mode) {
	_config_mode = is_in_config_mode;
    }

    bool is_a_zombie() const { return _is_a_zombie; }
    void set_zombie(bool state) { _is_a_zombie = state; }

private:
    string _clientname;
    uint32_t	_clientid;  /* client ID is a unique number for every
			       connected client at any moment in time,
			       but not guaranteed to be unique over
			       time */
    string _authtoken;
    bool _authenticated;
    bool _config_mode;
    bool _is_a_zombie;	// A user instance is a zombie if we suspect
			// the client process no longer exists
};

class UserDB {
public:
    UserDB(bool verbose);
    ~UserDB();

    User* add_user(uid_t user_id, const string& username, gid_t gid);
    void load_password_file();
    const User* find_user_by_user_id(uid_t user_id);
    void remove_user(uid_t user_id);
    bool has_capability(uid_t user_id, const string& capability, string& err_msg);

private:
    map<uid_t, User*> _users;
    bool _verbose;	// Set to true if output is verbose
};

#endif // __RTRMGR_USERDB_HH__
