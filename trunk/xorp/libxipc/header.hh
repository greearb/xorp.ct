// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/libxipc/header.hh,v 1.16 2008/10/02 21:57:21 bms Exp $

#ifndef __LIBXIPC_HEADER_HH__
#define __LIBXIPC_HEADER_HH__

#include "libxorp/xorp.h"

#include <list>
#include <map>


class HeaderWriter {
public:
    class InvalidName {};
    HeaderWriter& add(const string& name, const string& value)
	throw (InvalidName);
    HeaderWriter& add(const string& name, int32_t value)
	throw (InvalidName);
    HeaderWriter& add(const string& name, uint32_t value)
	throw (InvalidName);
    HeaderWriter& add(const string& name, const double& value)
	throw (InvalidName);
    string str() const;
private:
    static bool name_valid(const string &s);
    struct Node {
	string key;
	string value;
	Node(const string& k, const string& v) : key(k), value(v) {}
    };
    list<Node> _list;
};

class HeaderReader {
public:
    class InvalidString {};
    HeaderReader(const string& serialized) throw (InvalidString);

    class NotFound {};
    HeaderReader& get(const string& name, string& val) throw (NotFound);
    HeaderReader& get(const string& name, int32_t& val) throw (NotFound);
    HeaderReader& get(const string& name, uint32_t& val) throw (NotFound);
    HeaderReader& get(const string& name, double& val) throw (NotFound);
    size_t bytes_consumed() const { return _bytes_consumed; }
private:
    size_t _bytes_consumed;
    map<string, string> _map;
    typedef map<string, string>::iterator CMI;
};

#endif // __LIBXIPC_HEADER_HH__
