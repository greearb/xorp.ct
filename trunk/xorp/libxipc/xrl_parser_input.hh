// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/libxipc/xrl_parser_input.hh,v 1.2 2002/12/19 01:29:13 hodson Exp $

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
	inline istream*	input() const { return _input; }
	inline const char* 	filename() const { return _fname; }
	inline int 		line() const { return _line; }
	inline void 		incr_line() { _line++; }
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
