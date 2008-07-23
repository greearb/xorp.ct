// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/ospf/test_args.hh,v 1.4 2008/01/04 03:16:57 pavlin Exp $

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
