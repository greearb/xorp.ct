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

// $XORP: xorp/libxipc/xrl_parser_input.hh,v 1.12 2008/10/02 21:57:24 bms Exp $

#ifndef __LIBXIPC_XRL_PARSER_INPUT_HH__
#define __LIBXIPC_XRL_PARSER_INPUT_HH__

#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <string>

#include "libxorp/xorp.h"
#include "libxorp/exceptions.hh"

/**
 * @short Base class for XrlParserInput's.
 *
 * XrlParserInput's are used by the @ref XrlParser class to manage
 * input.  The output of XrlParserInput should only contain
 * material likely to XRL's and C-preprocessor hash directives,
 * ie # <num>  "file" <line> directives.
 */

class XrlParserInput {
public:
    /** Retrieves 1 line of input from source.
     *
     *  @param lineout string that is set if line of text is available.
     *  @return true if line was read into lineout, false otherwise.
     */
    virtual bool getline(string& lineout) = 0;

    /**
     * @return true if no more data is available for reading.
     */
    virtual bool eof() const = 0;

    /**
     * @return string containing stack trace to be used for tracking
     * errors in the input.
     */
    virtual string stack_trace() const = 0;

    virtual ~XrlParserInput() {};
};

/**
 * @short Exception class used by @ref XrlParserInput difficulties.
 */

struct XrlParserInputException : public XorpReasonedException {
    XrlParserInputException(const char* file, int line, const string& reason)
	: XorpReasonedException("XrlParserInputException", file, line, reason) {}
};

/** XrlParserFileInput class reads lines from a data source, strips out
 *  comments and handles continuation characters.  It is similar to the
 *  C-preprocessor in that it strips out C and C++ comments and handles
 *  #include directives.
 */

class XrlParserFileInput : public XrlParserInput {
public:
    /** Constructor
     *
     * @param input input file stream.
     * @param fname filename.
     * @throws XrlParserInputException if input file stream is not good().
     */
    XrlParserFileInput(istream* input, const char* fname = "")
	throw (XrlParserInputException);

    XrlParserFileInput(const char* filename)
	throw (XrlParserInputException);

    ~XrlParserFileInput();

    bool eof() const;
    bool getline(string& line) throw (XrlParserInputException);
    string stack_trace() const;

    /** @return include path preprocessor looks for files in. */
    list<string>& path() { return _path; }

protected:
    bool slurp_line(string& line) throw (XrlParserInputException);

    struct FileState {
	FileState(istream* input, const char* fname) :
	    _input(input), _fname(fname), _line(0) {}
	// Accessors
	istream*	input() const { return _input; }
	const char* 	filename() const { return _fname; }
	int 		line() const { return _line; }
	void 		incr_line() { _line++; }
    private:
	istream*   _input;
	const char* _fname;
	int	    _line;
    };
    /** Push FileState onto stack
     * @throws XrlParserInputException if input file stream is not good();
     */
    void push_stack(const FileState& fs)
	throw (XrlParserInputException);

    void pop_stack();
    FileState& stack_top();
    size_t stack_depth() const;

    ifstream* path_open_input(const char* filename)
	throw (XrlParserInputException);
    void close_input(istream* pif);

    string try_include(string::const_iterator& begin,
		       const string::const_iterator& end)
	throw (XrlParserInputException);
    void initialize_path();

    vector<FileState>	_stack;
    list<string>	_path;
    bool		_own_bottom;	 // We alloced stack bottom istream
    list<string>	_inserted_lines;

    bool filter_line(string& output, const string& input);
    enum Mode {
	NORMAL		= 0x00,
	IN_SQUOTE	= 0x01,
	IN_DQUOTE	= 0x02,
	IN_C_COMMENT	= 0x04
    } _current_mode;
};

#endif // __LIBXIPC_XRL_PARSER_INPUT_HH__
