// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/libxipc/xrl_parser.hh,v 1.12 2008/10/02 21:57:24 bms Exp $

#ifndef __LIBXIPC_XRL_PARSER_HH__
#define __LIBXIPC_XRL_PARSER_HH__

#include <stdarg.h>
#include <list>
#include "xrl.hh"
#include "xrl_parser_input.hh"

class XrlParseError {
public:

    XrlParseError(const string& input, ssize_t offset, const string& reason)
	: _input(input), _offset(offset), _reason(reason) {}

    XrlParseError(const string& input, string::const_iterator pos,
		  const string& reason)
	: _input(input), _offset(pos - input.begin()), _reason(reason) {}

    XrlParseError() : _input(""), _offset(0xffffffff), _reason("") {}

    virtual ~XrlParseError() {}

    const string& input() const		{ return _input; }
    ssize_t offset() const		{ return _offset; }
    const string& reason() const	{ return _reason; }

    string pretty_print(size_t termwidth = 80u) const;

protected:
    const string    _input;
    size_t	    _offset;
    string 	    _reason;

    void get_coordinates(size_t& lineno, size_t& charno) const;
};

// XrlLocator - locates sub-strings that look like spaceless and
// presentation Xrl's.  It does weak examination of syntax and leaves
// finer grained examination to the Xrl instantiation routines.

class XrlParser {
public:
    XrlParser(XrlParserInput& xpi) : _xpi(xpi) {}
    virtual ~XrlParser() {}

    /** Starts new parsing cycle.
     * @return true upon success, false if there is no more data
     */
    bool start_next() throw (XrlParserInputException);

    /** Check if input is exhausted.
     *  @return true if input is exhausted, false otherwise.
     */
    bool finished() const { return _xpi.eof(); }

    bool get(string&  protocol,
	     string&  target,
	     string&  command,
	     XrlArgs& args)
	throw (XrlParseError);

    bool get(string& protocol,
	     string& target,
	     string& command,
	     XrlArgs& args,
	     list<XrlAtomSpell>& spells)
	throw (XrlParseError);

    bool get(string& xrl_c_str) throw (XrlParseError);

    bool get_return_specs(list<XrlAtomSpell>& spells);

    const string& input() const { return _input; }

    /**
     * Attempt to find a new XRL starting point after an error has
     * occurred.
     *
     * @return true if text resembling an XRL start is found.
     */
    bool resync();

    const XrlParserInput& parser_input() const;

protected:

    bool get(string& 		 protocol,
	     string& 		 target,
	     string& 		 command,
	     XrlArgs*		 args,
	     list<XrlAtomSpell>* spells)
	throw (XrlParseError);

    bool parse_atoms_and_spells(XrlArgs*	    args,
				list<XrlAtomSpell>* spells);

    XrlParserInput&	   _xpi;
    string		   _input;
    string::const_iterator _pos;
};

#endif // __LIBXIPC_XRL_PARSER_HH__
