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

// $XORP$

#ifndef __LIBXORP_TOKENIZE_HH__
#define __LIBXORP_TOKENIZE_HH__

/*
** Tokenizer.
*/
inline
void 
tokenize(const string& str,
	      vector<string>& tokens,
	      const string& delimiters = " ")
{
    string::size_type begin = str.find_first_not_of(delimiters, 0);
    string::size_type end = str.find_first_of(delimiters, begin);

    while(string::npos != begin || string::npos != end) {
        tokens.push_back(str.substr(begin, end - begin));
        begin = str.find_first_not_of(delimiters, end);
        end = str.find_first_of(delimiters, begin);
    }
}

#endif // __LIBXORP_TOKENIZE_HH__
