// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2006 International Computer Science Institute
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

// $XORP: xorp/devnotes/template.hh,v 1.5 2005/03/25 02:52:59 pavlin Exp $

#ifndef __OSPF_TEST_BUILD_LSA_HH__
#define __OSPF_TEST_BUILD_LSA_HH__

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

    const string& original_line() const {
	return _line;
    }

 private:
    const string _line;
    vector<string> _words;
    size_t _pos;
};

class BuildLsa {
 public:
    BuildLsa(OspfTypes::Version version) : _version(version)
    {}

    /**
     * From a textual representation build an LSA that can be used for
     * testing.
     */
    Lsa *generate(Args& args);

 private:
    OspfTypes::Version _version;

    Options get_options(Lsa *lsa);

    void set_options(Lsa *lsa, Options& options);

    bool common_header(Lsa *lsa, const string& word, Args& args);

    Lsa *router_lsa(Args& args);

    Lsa *network_lsa(Args& args);

    Lsa *summary_network_lsa(Args& args);

    Lsa *summary_router_lsa(Args& args);

    Lsa *as_external_lsa(Args& args);

    Lsa *type_7_lsa(Args& args);
};

#endif // __OSPF_TEST_BUILD_LSA_HH__
