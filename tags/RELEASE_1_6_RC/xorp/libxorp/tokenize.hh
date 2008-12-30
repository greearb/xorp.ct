// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/libxorp/tokenize.hh,v 1.6 2008/07/23 05:10:57 pavlin Exp $

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
