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

#ident "$XORP: xorp/libxorp/token.cc,v 1.5 2002/12/09 18:29:15 hodson Exp $"


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
    bool enclose_quotations_bool = false;
    string token;
    
    for (i = 0; i < token_org.size(); i++) {
	if (is_token_separator(token_org[i])) {
	    enclose_quotations_bool = true;
	    break;
	}
    }
    
    if (enclose_quotations_bool) {
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
    bool escape_begin_bool = false;	// True if reached quotation mark
    bool escape_end_bool = false;
    
    // Skip the spaces in front
    for (i = 0; i < token_line.length(); i++) {
	if (isspace(token_line[i])) {
	    continue;
	}
	break;
    }
    
    if (token_line.size() && (token_line[i] == '"')) {
	escape_begin_bool = true;
	i++;
    }
    // Get the substring until any other token separator
    for ( ; i < token_line.length(); i++) {
	if (escape_end_bool) {
	    if (! is_token_separator(token_line[i])) {
		// RETURN ERROR
	    }
	    break;
	}
	if (escape_begin_bool) {
	    if (token_line[i] == '"') {
		escape_end_bool = true;
		continue;
	    }
	}
	if (is_token_separator(token_line[i]) && !escape_begin_bool) {
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
    
    if (escape_begin_bool && !escape_end_bool) {
	// RETURN ERROR
    }
    
    return (token);
}

bool
is_token_separator(const char c)
{
    if (isspace(c) || c == '|')
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

//
// Create a copy of @char_line, but all tokens with a single space between.
//
string
char_line2token_line(const char *char_line)
{
    string token_line_org(char_line);
    string token, token_line_result;
    
    do {
	token = pop_token(token_line_org);
	if (token.empty())
	    break;
	if (token_line_result.size())
	    token_line_result += " ";
	token_line_result += token;
    } while (true);
    
    return (token_line_result);
}
