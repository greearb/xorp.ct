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

// $XORP: xorp/libxipc/xrl.hh,v 1.10 2004/09/24 03:47:48 pavlin Exp $

#ifndef __LIBXIPC_XRL_HH__
#define __LIBXIPC_XRL_HH__

#include <string>

#include "libxorp/exceptions.hh"
#include "xrl_atom.hh"
#include "xrl_args.hh"
#include "xrl_tokens.hh"

/**
 * XORP IPC request.
 */
class Xrl {
public:
    /**
     * Construct an Xrl.
     */
    inline Xrl(const string&	protocol,
	       const string&	protocol_target,
	       const string&	command,
	       const XrlArgs&	args)
	: _protocol(protocol), _target(protocol_target), _command(command),
	  _args(args)
    {}

    /**
     * Construct an Xrl (with implicit finder protocol).
     */
    inline Xrl(const string&	target,
	       const string&	command,
	       const XrlArgs&	args)
	: _protocol(_finder_protocol), _target(target), _command(command),
	  _args(args)
    {}

    /**
     * Construct an Xrl that does not have an argument list.
     */
    inline Xrl(const string& protocol,
	       const string& protocol_target,
	       const string& command)
	: _protocol(protocol), _target(protocol_target), _command(command)
    {}

    /**
     * Construct an Xrl that does not have an argument list.
     */
    inline Xrl(const string& target,
	       const string& command)
	: _protocol(_finder_protocol), _target(target), _command(command)
    {}

    /**
     * Construct an Xrl object from the string representation of Xrl.
     */
    Xrl(const char* xrl_c_str) throw (InvalidString);

    inline Xrl() {}
    ~Xrl();

    /**
     * Render Xrl as a string
     */
    string str() const;

    /**
     * @return the protocol associated with XRL.
     */
    inline const string& protocol() const		{ return _protocol; }

    /**
     * @return the name of the XRL target entity.
     */
    inline const string& target() const			{ return _target; }

    /**
     * @return string representation of Xrl without arguments.
     */
    inline string string_no_args() const;

    /**
     * @return the name of the command
     */
    inline const string& command() const		{ return _command; }

    /**
     * Retrieve list of arguments associated with the XRL.
     */
    inline XrlArgs& args()				{ return _args; }

    /**
     * Retrieve list of arguments associated with the XRL.
     */
    inline const XrlArgs& args() const			{ return _args; }

    /**
     * Test the equivalence of two XRL's.
     *
     * @return true if the XRL's are equivalent.
     */
    bool operator==(const Xrl& x) const;

    /**
     * @return true if Xrl is resolved, ie protocol == finder.
     */
    bool is_resolved() const;

    /**
     * Get number of bytes needed to pack XRL into a serialized byte form.
     */
    size_t packed_bytes() const;

    /**
     * Pack XRL into a byte array.  The size of the byte
     * array should be larger or equal to the value returned by
     * packed_bytes().
     *
     * @param buffer buffer to receive data.
     * @param buffer_bytes size of buffer.
     * @return size of packed data on success, 0 on failure.
     */
    size_t pack(uint8_t* buffer, size_t buffer_bytes) const;

    /**
     * Unpack XRL from serialized byte array.
     *
     * @param buffer to read data from.
     * @param buffer_bytes size of buffer.  The size should exactly match
     *        number of bytes of packed data, ie packed_bytes() value
     *        used when packing.
     * @return number of bytes turned into atoms on success, 0 on failure.
     */
    size_t unpack(const uint8_t* buffer, size_t buffer_bytes);

private:
    const char* parse_xrl_path(const char* xrl_path);

private:
    string	_protocol;
    string	_target;   // if protocol != finder, target = protocol params
    string	_command;
    mutable XrlArgs	_args; // XXX only for packed_bytes() and pack()

    static const string _finder_protocol;
};

typedef Xrl XrlTemplate;

// ----------------------------------------------------------------------------
// Inline methods

#include "libxorp/c_format.hh"

inline string
Xrl::string_no_args() const
{
    //    return _protocol + string(XrlToken::PROTO_TGT_SEP) + _target +
    //	string(XrlToken::TGT_CMD_SEP) + _command;
    return c_format("%s%s%s%s%s", _protocol.c_str(), XrlToken::PROTO_TGT_SEP,
		    _target.c_str(), XrlToken::TGT_CMD_SEP, _command.c_str());
}

#endif // __LIBXIPC_XRL_HH__
