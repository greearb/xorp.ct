// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/libxipc/header.cc,v 1.2 2002/12/19 01:29:09 hodson Exp $"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xrl_module.h"
#include "libxorp/debug.h"
#include "header.hh"

static const string HEADER_SEP(":\t");
static const string HEADER_EOL("\r\n");

bool
HeaderWriter::name_valid(const string &name)
{
    return (name.find(HEADER_SEP) == ~0U);
}

HeaderWriter&
HeaderWriter::add(const string& name,
		  const string& value) throw (InvalidName) {
    if (name_valid(name) == false) throw InvalidName();

    _list.push_back(Node(name, value));

    return *this;
}

HeaderWriter&
HeaderWriter::add(const string& name, int32_t value) throw (InvalidName)
{
    if (name_valid(name) == false) throw InvalidName();

    char buffer[32];
    snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%d", value);
    _list.push_back(Node(name, buffer));

    return *this;
}

HeaderWriter&
HeaderWriter::add(const string& name, uint32_t value) throw (InvalidName)
{
    if (name_valid(name) == false) throw InvalidName();

    char buffer[32];
    snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%u", value);
    _list.push_back(Node(name, buffer));

    return *this;
}

HeaderWriter&
HeaderWriter::add(const string& name,
		  const double& value) throw (InvalidName) {
    if (name_valid(name) == false) throw InvalidName();

    char buffer[32];
    snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%f", value);
    _list.push_back(Node(name, buffer));

    return *this;
}

string
HeaderWriter::str() const
{
    list<Node>::const_iterator ci;

    string r;
    for (ci = _list.begin(); ci != _list.end(); ci++) {
	r += ci->key + HEADER_SEP + ci->value + HEADER_EOL;
    }
    r += HEADER_EOL;
    return r;
}

HeaderReader::HeaderReader(const string& serialized) throw (InvalidString)
{
    debug_msg("HeaderReader:\n%s\n", serialized.c_str());

    if (serialized.find(HEADER_EOL + HEADER_EOL) == ~0U)
	throw InvalidString();

    bool found_terminator = false;

    const char *start, *sep, *end;

    start = serialized.c_str();
    size_t remain = serialized.size();

    while (remain > 0 && *start != '\0') {
	sep = strstr(start, HEADER_SEP.c_str());
	if (sep == NULL) break;

	end = strstr(sep, HEADER_EOL.c_str());
	if (end == NULL) break;

	string key(start, sep);
	string value(sep + HEADER_SEP.size(), end);
	debug_msg("#%s# #%s#\n", key.c_str(), value.c_str());

	_map[key] = value;

	remain -= end + HEADER_EOL.size() - start;
	start = end + HEADER_EOL.size();
	if (*start && !strncmp(start, HEADER_EOL.c_str(), HEADER_EOL.size())) {
	    found_terminator = true;
	    _bytes_consumed = start - serialized.c_str() + HEADER_EOL.size();
	    break;
	}
    }

    if (found_terminator == false)
	throw InvalidString();
}

HeaderReader&
HeaderReader::get(const string& name, string& value) throw (NotFound)
{
    CMI m = _map.find(name);
    if (m == _map.end())
	throw NotFound();
    value = m->second;
    return *this;
}

HeaderReader&
HeaderReader::get(const string& name, int32_t& value) throw (NotFound)
{
    string tmp;
    get(name, tmp);
    value = strtol(tmp.c_str(), 0, 10);
    return *this;
}

HeaderReader&
HeaderReader::get(const string& name, uint32_t& value) throw (NotFound)
{
    string tmp;
    get(name, tmp);
    value = strtoul(tmp.c_str(), 0, 10);
    return *this;
}

HeaderReader&
HeaderReader::get(const string& name, double& value) throw (NotFound)
{
    string tmp;
    get(name, tmp);
    value = strtod(tmp.c_str(), 0);
    return *this;
}

