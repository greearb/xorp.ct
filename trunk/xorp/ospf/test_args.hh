// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/ospf/test_args.hh,v 1.6 2008/10/02 21:57:49 bms Exp $

#ifndef __OSPF_TEST_ARGS_HH__
#define __OSPF_TEST_ARGS_HH__

#include "libxorp/tokenize.hh"

/**
 * Break a string into a sequence of space separated words.
 */
class Args {
 public:
    Args(string& line) : _line(line), _pos(0) {
	tokenize(line, _words);
    }

    /**
     * @param word if a word is available it is placed here.
     * @return true if a word has been returned.
     */
    bool get_next(string& word) {
	if (_pos >= _words.size())
	    return false;

	word = _words[_pos++];

	return true;
    }

    void push_back() {
	if (_pos > 1)
	    _pos--;
    }

    const string& original_line() const {
	return _line;
    }

 private:
    const string _line;
    vector<string> _words;
    size_t _pos;
};


/**
 * Get a number in base 8,10 or 16.
 */
inline
uint32_t
get_number(const string& word) throw(InvalidString)
{
    char *endptr;
    
    uint32_t number = strtoul(word.c_str(), &endptr, 0);
    if (0 != *endptr)
	xorp_throw(InvalidString,
		   c_format("<%s> is not a number", word.c_str()));

    return number;
}

/**
 * Get the next word throw an exception if its not present.
 */
inline
string
get_next_word(Args& args, const string& varname) throw(InvalidString)
{
    string var;
    if (!args.get_next(var))
	xorp_throw(InvalidString,
		   c_format("No argument to %s. [%s]",
			    varname.c_str(),
			    args.original_line().c_str()));

    return var;
}

inline
uint32_t
get_next_number(Args& args, const string& varname) throw(InvalidString)
{
    return get_number(get_next_word(args, varname));
}


#endif // __OSPF_TEST_ARGS_HH__
