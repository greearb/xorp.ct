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

// $XORP: xorp/libxorp/token.hh,v 1.1.1.1 2002/12/11 23:56:05 hodson Exp $


#ifndef __LIBXORP_TOKEN_HH__
#define __LIBXORP_TOKEN_HH__


//
// Token related definitions.
//


#include <list>
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
 * Create a copy of a token line.
 * 
 * Create a copy of a token line, but all tokens with a single space between.
 * 
 * @param char_line C-style token line to copy.
 * @return C++ string copy of @ref char_line, but with all tokens inside
 * with a single space between.
 */
string	char_line2token_line(const char *char_line);

#endif // __LIBXORP_TOKEN_HH__
