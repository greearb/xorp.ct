// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2005 International Computer Science Institute
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

// $XORP$


#ifndef __LIBPROTO_CONFIG_NODE_ID_HH__
#define __LIBPROTO_CONFIG_NODE_ID_HH__


#include "libxorp/xorp.h"
#include "libxorp/exceptions.hh"


//
// Configuration Node ID support
//
/**
 * @short Class for encoding and decoding configuration-related node IDs.
 */
class ConfigNodeId {
public:
    typedef uint64_t Position;
    typedef uint64_t UniqueNodeId;

    /**
     * Default constructor
     *
     */
    ConfigNodeId() : _position(0), _unique_node_id(0) { generate_str(); }

    /**
     * Constructor from a string.
     *
     * The string format is two uint64_t integers separated by space.
     *
     * @param s the initialization string.
     */
    ConfigNodeId(const string& s) throw (InvalidString);

    /**
     * Constructor for a given position and unique node ID.
     *
     * @param position the position of the node.
     * @param unique_node_id the unique node ID.
     */
    ConfigNodeId(const Position& position, const UniqueNodeId& unique_node_id)
	: _position(position),
	  _unique_node_id(unique_node_id)
    {
	generate_str();
    }

    /**
     * Destructor
     */
    virtual ~ConfigNodeId() {}

    const Position& position() const { return (_position); }
    const UniqueNodeId& unique_node_id() const { return (_unique_node_id); }

    const string& str() const { return (_str); }

private:
    void		generate_str() {
	_str = c_format("%llu %llu", _position, _unique_node_id);
    }

    Position		_position;
    UniqueNodeId	_unique_node_id;
    string		_str;
};

ConfigNodeId::ConfigNodeId(const string& s) throw (InvalidString)
{
    string::size_type space, ix;

    space = s.find(' ');
    if ((space == string::npos) || (space == 0) || (space >= s.size() - 1)) {
	xorp_throw(InvalidString,
		   c_format("Bad ConfigNodeId \"%s\"", s.c_str()));
    }

    //
    // Check that everything from the beginning to "space" is digits,
    // and that everything after "space" to the end is also digits.
    //
    for (ix = 0; ix < space; ix++) {
	if (! xorp_isdigit(s[ix])) {
	    xorp_throw(InvalidString,
		       c_format("Bad ConfigNodeId \"%s\"", s.c_str()));
	}
    }
    for (ix = space + 1; ix < s.size(); ix++) {
	if (! xorp_isdigit(s[ix])) {
	    xorp_throw(InvalidString,
		       c_format("Bad ConfigNodeId \"%s\"", s.c_str()));
	}
    }

    //
    // Extract the position and the unique node ID
    //
    string tmp_str = s.substr(0, space);
    _position = strtoll(tmp_str.c_str(), (char **)NULL, 10);
    tmp_str = s.substr(space + 1);
    _unique_node_id = strtoll(tmp_str.c_str(), (char **)NULL, 10);

    generate_str();
}

#endif // __LIBPROTO_CONFIG_NODE_ID_HH__
