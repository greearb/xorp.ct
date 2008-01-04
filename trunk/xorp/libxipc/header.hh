// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/libxipc/header.hh,v 1.13 2007/02/16 22:46:07 pavlin Exp $

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
