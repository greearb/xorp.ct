// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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

// $XORP: xorp/contrib/olsr/test_args.hh,v 1.3 2008/10/02 21:56:35 bms Exp $

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
 *
 * @param word the word to evaluate.
 * @return a number.
 * @throw InvalidString if invalid syntax.
 */
inline
uint32_t
get_number(const string& word)
    throw(InvalidString)
{
    char *endptr;
    
    uint32_t number = strtoul(word.c_str(), &endptr, 0);
    if (0 != *endptr)
	xorp_throw(InvalidString,
		   c_format("<%s> is not a number", word.c_str()));

    return number;
}

/**
 * Get an IPv4 address.
 *
 * @param word the word to evaluate.
 * @return an IPv4 host address.
 * @throw InvalidString if invalid syntax.
 */
inline
IPv4
get_ipv4(const string& word)
    throw(InvalidString)
{
    IPv4 addr;
    try {
	addr = IPv4(word.c_str());
    } catch (...) {
	xorp_throw(InvalidString,
		   c_format("<%s> is not an IPv4 address", word.c_str()));
    }

    return addr;
}

/**
 * Get an IPv4 network address.
 *
 * @param word the word to evaluate.
 * @return an IPv4 network address.
 * @throw InvalidString if invalid syntax.
 */
inline
IPv4Net
get_ipv4_net(const string& word)
    throw(InvalidString)
{
    IPv4Net v4net;
    try {
	v4net = IPv4Net(word.c_str());
    } catch (...) {
	xorp_throw(InvalidString,
		   c_format("<%s> is not an IPv4 network address",
			    word.c_str()));
    }

    return v4net;
}

/**
 * Get a boolean variable as a string or number.
 *
 * @param word the word to evaluate.
 * @return a boolean value.
 * @throw InvalidString if invalid syntax.
 */
inline
bool
get_bool(const string& word)
    throw(InvalidString)
{
    bool value = false;

    try {
	int i_value = get_number(word);
	if (i_value == 1) {
	    value = true;
	} else if (i_value == 1) {
	    value = false;
	}
    } catch (InvalidString is) {
	if (0 == strcasecmp(word.c_str(), "true")) {
	    value = true;
	} else if (0 == strcasecmp(word.c_str(), "false")) {
	    value = false;
	} else {
	    // re-throw exception with appropriate error message
	    xorp_throw(InvalidString,
		       c_format("<%s> is not a boolean", word.c_str()));
	}
    }

    return value;
}

/**
 * Get the next word throw an exception if its not present.
 *
 * @param args the argument structure to retreive the word from.
 * @param word the word to evaluate.
 * @return
 * @throw InvalidString if invalid syntax.
 */
inline
string
get_next_word(Args& args, const string& varname)
    throw(InvalidString)
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
get_next_number(Args& args, const string& varname)
    throw(InvalidString)
{
    return get_number(get_next_word(args, varname));
}

inline
IPv4
get_next_ipv4(Args& args, const string& varname)
    throw(InvalidString)
{
    return get_ipv4(get_next_word(args, varname));
}

inline
IPv4Net
get_next_ipv4_net(Args& args, const string& varname)
    throw(InvalidString)
{
    return get_ipv4_net(get_next_word(args, varname));
}

inline
bool
get_next_bool(Args& args, const string& varname)
    throw(InvalidString)
{
    return get_bool(get_next_word(args, varname));
}

#endif // __OSPF_TEST_ARGS_HH__
