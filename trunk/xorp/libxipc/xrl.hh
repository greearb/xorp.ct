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

// $XORP: xorp/libxipc/xrl.hh,v 1.8 2003/09/18 19:06:45 hodson Exp $

#ifndef __LIBXIPC_XRL_HH__
#define __LIBXIPC_XRL_HH__

#include <string>

#include "libxorp/exceptions.hh"
#include "xrl_atom.hh"
#include "xrl_args.hh"

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
    string string_no_args() const;

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

private:
    string	_protocol;
    string	_target;   // if protocol != finder, target = protocol params
    string	_command;
    XrlArgs	_args;

    static const string _finder_protocol;
};

typedef Xrl XrlTemplate;

#endif // __LIBXIPC_XRL_HH__
