// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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

// $XORP: xorp/ospf/test_args.hh,v 1.3 2007/02/16 22:46:42 pavlin Exp $

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
