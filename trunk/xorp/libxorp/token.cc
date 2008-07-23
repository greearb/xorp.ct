// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/libxorp/token.cc,v 1.13 2008/01/04 03:16:44 pavlin Exp $"


//
// Token processing functions.
//


#include "token.hh"
#include <ctype.h>


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//


//
// Local variables
//

//
// Local functions prototypes
//


// Copy a token from @token_org
// If it contains token separators, enclose it within quotations
string
copy_token(const string& token_org)
{
    size_t i;
    bool is_enclose_quotations = false;
    string token;
    
    for (i = 0; i < token_org.size(); i++) {
	if (is_token_separator(token_org[i])) {
	    is_enclose_quotations = true;
	    break;
	}
    }
    
    if (is_enclose_quotations) {
	token = "\"" + token_org + "\"";
    } else {
	token = token_org;
    }
    return (token);
}

// Pop a token from @token_line
// @return the token, and modify @token_line to exclude the return token.
string
pop_token(string& token_line)
{
    size_t i = 0;
    string token;
    bool is_escape_begin = false;	// True if reached quotation mark
    bool is_escape_end = false;

    // Check for empty token line
    if (token_line.empty())
	return (token);

    // Skip the spaces in front
    for (i = 0; i < token_line.length(); i++) {
	if (xorp_isspace(token_line[i])) {
	    continue;
	}
	break;
    }

    // Check if we reached the end of the token line
    if (i == token_line.length()) {
	token_line = token_line.erase(0, i);
	return (token);
    }
    
    if (token_line[i] == '"') {
	is_escape_begin = true;
	i++;
    }
    // Get the substring until any other token separator
    for ( ; i < token_line.length(); i++) {
	if (is_escape_end) {
	    if (! is_token_separator(token_line[i])) {
		// RETURN ERROR
	    }
	    break;
	}
	if (is_escape_begin) {
	    if (token_line[i] == '"') {
		is_escape_end = true;
		continue;
	    }
	}
	if (is_token_separator(token_line[i]) && !is_escape_begin) {
	    if ((token_line[i] == '|') && (token.empty())) {
		// XXX: "|" with or without a space around it is a token itself
		token += token_line[i];
		i++;
	    }
	    break;
	}
	token += token_line[i];
    }
    
    token_line = token_line.erase(0, i);
    
    if (is_escape_begin && !is_escape_end) {
	// RETURN ERROR
    }
    
    return (token);
}

bool
is_token_separator(const char c)
{
    if (xorp_isspace(c) || c == '|')
	return (true);
    return (false);
}

bool
has_more_tokens(const string& token_line)
{
    string tmp_token_line = token_line;
    string token = pop_token(tmp_token_line);
    
    return (token.size() > 0);
}

/**
 * Split a token line into a vector with the tokens.
 * 
 * @param token_line the token line to split.
 * @return a vector with all tokens.
 */
vector<string>
token_line2vector(const string& token_line)
{
    string token_line_org(token_line);
    string token;
    vector<string> token_vector_result;
    
    do {
	token = pop_token(token_line_org);
	if (token.empty())
	    break;
	token_vector_result.push_back(token);
    } while (true);
    
    return (token_vector_result);
}

/**
 * Split a token line into a list with the tokens.
 * 
 * @param token_line the token line to split.
 * @return a list with all tokens.
 */
list<string>
token_line2list(const string& token_line)
{
    string token_line_org(token_line);
    string token;
    list<string> token_list_result;
    
    do {
	token = pop_token(token_line_org);
	if (token.empty())
	    break;
	token_list_result.push_back(token);
    } while (true);
    
    return (token_list_result);
}

/**
 * Combine a vector with the tokens into a single line with spaces as
 * separators.
 * 
 * @param token_vector the vector with the tokens.
 * @return a line with the tokens separated by spaces.
 */
string
token_vector2line(const vector<string>& token_vector)
{
    string token_line_result;

    vector<string>::const_iterator iter;
    for (iter = token_vector.begin(); iter != token_vector.end(); ++iter) {
	const string& token = *iter;
	if (! token_line_result.empty())
	    token_line_result += " ";	// XXX: the token separator
	token_line_result += token;
    }

    return (token_line_result);
}

/**
 * Combine a list with the tokens into a single line with spaces as
 * separators.
 * 
 * @param token_list the list with the tokens.
 * @return a line with the tokens separated by spaces.
 */
string
token_list2line(const list<string>& token_list)
{
    string token_line_result;

    list<string>::const_iterator iter;
    for (iter = token_list.begin(); iter != token_list.end(); ++iter) {
	const string& token = *iter;
	if (! token_line_result.empty())
	    token_line_result += " ";	// XXX: the token separator
	token_line_result += token;
    }

    return (token_line_result);
}
