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

// $XORP: xorp/libxorp/token.hh,v 1.10 2008/07/23 05:10:57 pavlin Exp $


#ifndef __LIBXORP_TOKEN_HH__
#define __LIBXORP_TOKEN_HH__


//
// Token related definitions.
//


#include <list>
#include <vector>
#include <string>

#include "xorp.h"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

//
// Global variables
//

//
// Global functions prototypes
//

/**
 * Copy a token.
 * 
 * If the token contains token separators, enclose it within quotations.
 * 
 * @param token_org the token to copy.
 * @return the copy of the token with token separators enclosed within
 * quotations.
 */
string	copy_token(const string& token_org);

/**
 * Pop a token from a token line.
 * 
 * @param token_line the token line to pop a token from.
 * @return the first token from the front of the line. Also,
 * @ref token_line is modified to exlude that token.
 */
string	pop_token(string& token_line);

/**
 * Test if a character is a token separator.
 * 
 * Currently, the is_space(3) characters and '|' are considered as
 * token separators.
 * 
 * @param c the character to test whether it is token separator.
 * @return true if @ref c is a token separator, otherwise false.
 */
bool	is_token_separator(const char c);

/**
 * Test if a token line contains more tokens.
 * 
 * @param token_line the token line to test.
 * @return true if @ref token_line contains more tokens, otherwise false.
 */
bool	has_more_tokens(const string& token_line);

/**
 * Split a token line into a vector with the tokens.
 * 
 * @param token_line the token line to split.
 * @return a vector with all tokens.
 */
vector<string> token_line2vector(const string& token_line);

/**
 * Split a token line into a list with the tokens.
 * 
 * @param token_line the token line to split.
 * @return a list with all tokens.
 */
list<string> token_line2list(const string& token_line);

/**
 * Combine a vector with the tokens into a single line with spaces as
 * separators.
 * 
 * @param token_vector the vector with the tokens.
 * @return a line with the tokens separated by spaces.
 */
string token_vector2line(const vector<string>& token_vector);

/**
 * Combine a list with the tokens into a single line with spaces as
 * separators.
 * 
 * @param token_list the list with the tokens.
 * @return a line with the tokens separated by spaces.
 */
string token_list2line(const list<string>& token_list);

#endif // __LIBXORP_TOKEN_HH__
