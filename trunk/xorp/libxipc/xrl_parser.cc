// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/libxipc/xrl_parser.cc,v 1.5 2003/03/10 23:20:27 hodson Exp $"

#include <stdio.h>

#include <string>

#include "xrl_module.h"
#include "libxorp/xorp.h"
#include "libxorp/c_format.hh"
#include "xrl_atom_encoding.hh"
#include "xrl_parser.hh"

// ----------------------------------------------------------------------------
// Miscellaneous utilities

#define xmin(a, b) (a < b) ? (a) : (b)
#define xmax(a, b) (a < b) ? (b) : (a)

// ----------------------------------------------------------------------------
// Xrl Parse Error class

void
XrlParseError::get_coordinates(size_t& lineno, size_t& charno) const
{
    lineno = 1;
    charno = 0;
    for (size_t i = 0; i < _offset; i++) {
	charno++;
	if (_input[i] == '\n') {
	    lineno++;
	    charno = 0;
	}
    }
}

string
XrlParseError::pretty_print(const size_t termwidth) const
{
    // pretty print == pretty ugly code
    if (_input == "")
	return string("No Error", 0, termwidth - 1);

    size_t wwidth = termwidth - 7; // ...XXX...\n

    if (wwidth < 20) {
	wwidth = 20; // just fix if termwidth is wacko
    }

    ssize_t start = xmax(0, (ssize_t)_offset - (ssize_t)wwidth / 2);
    ssize_t end = xmin(_input.size() , start + wwidth);

    ssize_t offset = _offset - start;
    string snapshot;	// Window of _input to display
    string indicator;	// indicator string to appear below snapshot

    if (start != 0) {
	snapshot = "...";
	indicator = string(3, ' ');
    }
    snapshot += string(_input, start, end - start);

    if (offset > 0)
	indicator += string(offset, ' ');
    indicator += string("^");

    if (end < (ssize_t)_input.size()) {
	snapshot += "...";
    }

    // Filter chars that will give us problems for display
    for (string::iterator si = snapshot.begin(); si != snapshot.end(); si++) {
	if (xorp_iscntrl(*si) || !isascii(*si)) {
	    *si = ' ';
	}
    }

    // Determine line and character on line of error
    size_t lno, cno;
    get_coordinates(lno, cno);

    // Gloop it up to go.
    return c_format("XrlParseError at line %u char %u: ", (uint32_t)lno,
		    (uint32_t)cno)
	+ _reason + string("\n") + snapshot + string("\n") + indicator;
}

// ----------------------------------------------------------------------------
// Utility routines for Xrl parsing

static inline void
advance_to_either(const string& input, string::const_iterator& sci,
		  const char* choices) {
    while (sci != input.end()) {
	if (strchr(choices,*sci)) break;
	sci++;
    }
}

static inline void
advance_to_char(const string& input, string::const_iterator& sci, char c)
{
    while (sci != input.end()) {
	if (*sci == c) break;
	sci++;
    }
}

static inline bool
isxrlplain(int c)
{
    return (xorp_isalnum(c) || c == '_' || c == '-');
}

static inline size_t
skip_xrl_plain_chars(const string& input, string::const_iterator& sci)
{
    string::const_iterator start = sci;
    for (; sci != input.end() && isxrlplain(*sci); sci++);
    return sci - start;
}

static inline char
c_escape_to_char(const string& input,
		 string::const_iterator sci) {

    if (sci == input.end())
	throw XrlParseError(input, sci,
			    "Unterminated escape sequence");

    switch (*sci) {
	// --- C character escape sequences ----
	// Unfortunately escapes 'abfnrtv' do not map to
	// <char value> - <offset>
    case 'a':
	sci++;
	return '\a';
    case 'b':
	sci++;
	return '\b';
    case 'f':
	sci++;
	return '\f';
    case 'n':
	sci++;
	return '\n';
    case 'r':
	sci++;
	return '\r';
    case 'v':
	sci++;
	return '\v';
	// ---- Hex value to be interpreted ----
    case 'x': {
	sci++;
	if (sci == input.end())
	    throw XrlParseError(input, sci, "Unexpected end of input.");
	char v = 0;
	for (int n = 0; sci != input.end() && n < 2 && xorp_isxdigit(*sci);
	     n++, sci++) {
	    char c = xorp_tolower(*sci);
	    v *= 16;
	    v += (c < '9') ? (c - '0') : (c - 'a' + 10);
	}
	return v;
    }
	// ---- Octal value to be interpreted ----
    case '9': // Invalid values
    case '8':
	throw XrlParseError(input, sci,
			    c_format("%c is not an octal character.", *sci));
    case '7': // Can have upto 3 octal chars...
    case '6':
    case '5':
    case '4':
    case '3':
    case '2':
    case '1':
    case '0':
	char v = *sci - '0';
	sci++;
	for (int n = 0;
	     n < 2 && sci != input.end() && *sci >= '0' && *sci <= '7';
	     n++, sci++) {
	    v *= 8;
	    v += *sci - '0';
	}
	return v;
    }
    // Everything else
    return *sci++;
}

static inline bool
iscrlf(int c)
{
    return (c == '\n') || (c == '\r');
}

static inline bool
skip_to_next_line(const string& s, string::const_iterator& sci)
{
    while ( sci != s.end() && !iscrlf(*sci) )
	sci++;
    while ( sci != s.end() && iscrlf(*sci) )
	sci++;
    return sci != s.end();
}

static inline void
skip_past_blanks(const string& s, string::const_iterator& sci)
{
    while (sci != s.end() && ( xorp_isspace(*sci) || xorp_iscntrl(*sci) )) {
	sci++;
    }
}

static inline void
skip_one_char(const string&, string::const_iterator* sci)
{
    sci++;
}

static inline void
skip_cplusplus_comments(const string& s, string::const_iterator& sci)
{
    assert(*sci == '/');
    sci++;
    if (sci == s.end()) {
	// the last character was a '/' - this isn't a comment, so push
	// it back on and return
	sci--;
	return;
    } else if (*sci == '*') {
	// it's a C-style comment
	string::const_iterator sci_start = sci;
	char prev = '\0';
	while (sci != s.end()) {
	    if (*sci == '/' && prev == '*') {
		sci++;
		return;
	    }
	    prev = *sci;
	    sci++;
	}
	throw XrlParseError(s, sci_start, "Unterminated comment.");
    } else if (*sci == '/') {
	// it's a C++ style comment
	skip_to_next_line(s, sci);
    } else {
	// it's not a comment - push the '/' back on, and return;
	sci--;
	return;
    }
}

static inline void
skip_comments_and_blanks(const string& s, string::const_iterator& sci)
{
    for (;;) {
	skip_past_blanks(s, sci);
	if (sci == s.end() || (*sci != '#' && *sci != '/')) {
	    break;
	}
	if (*sci == '/') {
	    skip_cplusplus_comments(s, sci);
	    continue;
	}
	skip_to_next_line(s, sci);
    }
}

static inline void
get_single_quoted_value(const string& s, string::const_iterator& sci,
			string& token)
{
    assert(*sci == '\'');

    sci++;
    token.erase();

    string::const_iterator sci_start = sci;
    advance_to_char(s, sci, '\'');
    if (sci == s.end()) {
	throw XrlParseError(s, sci_start, "Unterminated single quote.");
    }
    token = string(sci_start, sci);
    sci++;
}

static inline void
get_double_quoted_value(const string& input,
			string::const_iterator& sci,
			string& token)
{
    assert(*sci == '\"');

    token.erase();

    sci++;

    for (;;) {
	string::const_iterator sci_start = sci;
	while (sci != input.end() && *sci != '\"' && *sci != '\'')
	    sci++;
	token.append(sci_start, sci);
	if (*sci == '\\') {
	    sci++;
	    if (sci == input.end()) {
		throw XrlParseError(input, sci,
				    "Unterminated double quote");
	    }
	    char c = c_escape_to_char(input, sci);
	    token.append(1, c);
	}
	if (*sci == '\"')
	    break;
	if (sci == input.end())
	    throw XrlParseError(input, sci,
				"Unterminated double quote");

    }
    sci++;
}

static void
get_unquoted_value(const string& input,
		   string::const_iterator& sci,
		   string& token)
{
    string::const_iterator sci_start = sci;
    char prev = '\0';
    while (sci != input.end()) {
	if (xorp_isspace(*sci) || iscrlf(*sci) ||
	    *sci == '&' || *sci == ';' || *sci == '>')
	   break;
	prev = *sci;
	sci++;
    }
    // we need to backup if we hit "->" because "-" is allowed in
    // values, but "->" can terminate a value
    if ((sci != input.end()) && (*sci == '>') && (prev == '-')) {
	--sci;
    }
    token = string(sci_start, sci);
}

static inline string::const_iterator
uninterrupted_token_end(const string& input,
			string::const_iterator& sci)
{
    string::const_iterator end = sci;

    while (end != input.end() &&
	  ( !xorp_isspace(*end) && isascii(*end) && !xorp_iscntrl(*end) )) {
	end++;
    }

    return end;
}

static inline void
get_protocol_target_and_command(const string& input,
				string::const_iterator& sci,
				string& protocol,
				string& target,
				string& command)
{
    string::const_iterator start = sci;

    // Get target i.e. the bit like finder:// entity bit of xrl
    while (sci != input.end() && isxrlplain(*sci))
	sci++;
    if (string(sci, sci + 3) != string("://"))
	throw XrlParseError(input, sci, "Expected to find a ://");
    protocol = string(start, sci);
    sci += 3;
    start = sci;
    while (sci != input.end() && !(xorp_isspace(*sci) && iscrlf(*sci)) &&
	   *sci != '/')
	sci++;
    if (*sci != '/')
	throw XrlParseError(input, sci, "Expected to find a /");
    target = string(start, sci);

    // Get method - everything from end of target to beginning of arguments
    start = ++sci;
    char prev = '\0';
    while (sci != input.end() &&
	   !(xorp_isspace(*sci) || iscrlf(*sci) ||
	     !(isxrlplain(*sci) || *sci == '/' || *sci == '.'))) {
	prev = *sci;
	sci++;
    }

    // we need to backup if we hit "->" because "-" is allowed in the
    // method, but "->" can terminate the method
    if ((sci != input.end()) && (*sci == '>') && (prev == '-'))
	sci--;

    command = string(start, sci);

    return;
}

static void
push_atoms_and_spells(XrlArgs* args,
		      list<XrlAtomSpell>* spells,
		      const string& input,
		      const string::const_iterator& atom_start,
		      const string::const_iterator& value_start,
		      const string& atom_name,
		      const string& atom_type,
		      const string& atom_value) {
    try {
	XrlAtomType t = XrlAtom::lookup_type(atom_type);
	if (atom_value.empty()) {
	    if (args)
		args->add(XrlAtom(atom_name, t));
	    if (spells != 0)
		spells->push_back(XrlAtomSpell(atom_name, t, ""));
	} else if (atom_value[0] == '$') {
	    if (args)
		args->add(XrlAtom(atom_name, t));

	    if (spells == 0)
		throw XrlParseError(input, value_start,
				    "Found a spell character without a spell"
				    "list to store information.");

	    // This v ugly to have here - want to check for duplicate
	    // atom name or variable name.
	    for (list<XrlAtomSpell>::const_iterator i = spells->begin();
		 i != spells->end(); i++) {
		if (i->atom_name() == atom_name) {
		    string e = c_format("Duplicate atom name - \"%s\".",
						  atom_name.c_str());
		    throw XrlParseError(input, atom_start, e);
		}
		if (i->spell() == atom_value) {
		    string e = c_format("Duplicate variable name - \"%s\".",
						  atom_value.c_str());
		    throw XrlParseError(input, value_start, e);
		}
	    }
	    spells->push_back(XrlAtomSpell(atom_name, t, atom_value));
	} else {
	    if (args == 0)
		throw XrlParseError(input, value_start,
				    "Atom cannot be specified here");
	    args->add(XrlAtom(atom_name, t, atom_value));
	}
    } catch (const XrlArgs::XrlAtomFound& xaf) {
	string e = c_format("Duplicate atom name - \"%s\".",
				      atom_name.c_str());
	throw XrlParseError(input, atom_start, e);
    }
}

// ----------------------------------------------------------------------------
// XrlParser

bool
XrlParser::start_next() throw (XrlParserInputException)
{
    _input.erase();

    while (_xpi.getline(_input) == true) {
	// Ignore blank lines and CPP directives (at least for time being).
	if (_input.size() > 0 && _input[0] != '#') break;
    }
    _pos = _input.begin();
    return (_input.size() > 0);
}

bool
XrlParser::parse_atoms_and_spells(XrlArgs* args,
				  list<XrlAtomSpell>* spells)
{
    assert(_pos <= _input.end());
    skip_comments_and_blanks(_input, _pos);
    while (_pos != _input.end() && !iscrlf(*_pos) && !xorp_isspace(*_pos)) {
	assert(_pos < _input.end());
	skip_comments_and_blanks(_input, _pos);

	string atom_name;
	string::const_iterator atom_start, token_start;

	atom_start = token_start = _pos;
	while (_pos != _input.end() && isxrlplain(*_pos))
	    _pos++;
	if (token_start == _pos)
	    throw XrlParseError(_input, _pos, "Expected a name");
	assert(_pos < _input.end());
	atom_name = string(token_start, _pos);
	if (!XrlAtom::valid_name(atom_name))
	    throw XrlParseError(_input, token_start,
				c_format("%s is not a valid name",
					 atom_name.c_str()));

	skip_comments_and_blanks(_input, _pos);
	string atom_type;
	if (_pos != _input.end() && *_pos == ':') {
	    _pos++;
	    skip_comments_and_blanks(_input, _pos);
	    token_start = _pos;
	    char prev = '\0';
	    while (_pos != _input.end() && isxrlplain(*_pos)) {
		prev = *_pos;
		_pos++;
	    }

	    // We need to backup if we hit "->" because "-" by itself
	    // is allowed in a type name.
	    if ((_pos != _input.end()) && (*_pos == '>') && (prev == '-')) {
		_pos--;
	    }

	    if (_pos == token_start)
		throw XrlParseError(_input, _pos, "Expected a type");
	    atom_type = string(token_start, _pos);
	    if (!XrlAtom::valid_type(atom_type))
		throw XrlParseError(_input, token_start,
				    c_format("%s is not a valid type",
					     atom_type.c_str()));
	    assert(_pos <= _input.end());
	} else {
	    throw XrlParseError(_input, _pos, "Expected :<type> argument");
	}

	skip_comments_and_blanks(_input, _pos);
	assert(_pos <= _input.end());
	string atom_value;
	if (_pos != _input.end() && *_pos == '=') {
	    _pos++;
	    skip_comments_and_blanks(_input, _pos);
	    token_start = _pos;
	    assert(_pos <= _input.end());
	    if (*_pos == '\'') {
		get_single_quoted_value(_input, _pos, atom_value);
		atom_value = xrlatom_encode_value(atom_value);
	    } else if (*_pos == '\"') {
		get_double_quoted_value(_input, _pos, atom_value);
		atom_value = xrlatom_encode_value(atom_value);
	    } else {
		get_unquoted_value(_input, _pos, atom_value);
	    }
	    if (atom_value.empty())
		throw XrlParseError(_input, _pos, "Expected a value");
	    skip_comments_and_blanks(_input, _pos);
	}
	assert(_pos <= _input.end());
	push_atoms_and_spells(args, spells,
			      _input, atom_start, token_start,
			      atom_name, atom_type, atom_value);
	if (_pos != _input.end() && *_pos == '&')
	    _pos++;
	else
	    break;
	assert(_pos <= _input.end());
    }
    return true;
}

bool
XrlParser::get(string& protocol, string& target, string& command,
	       XrlArgs* args,
	       list<XrlAtomSpell>* spells) throw (XrlParseError) {

    skip_comments_and_blanks(_input, _pos);
    if (finished()) {
	return false;
    }

    target.erase();
    command.erase();
    if (args) args->clear();
    if (spells) spells->clear();

    assert(spells == 0 || spells->empty());

    get_protocol_target_and_command(_input, _pos, protocol, target, command);
    skip_comments_and_blanks(_input, _pos);

    if (*_pos == '?') {
	_pos++;
	parse_atoms_and_spells(args, spells);
	return true;
    }

    return true;
}

bool
XrlParser::get(string& r) throw (XrlParseError)
{
    string protocol, target, command;
    XrlArgs args;

    if (get(protocol, target, command, args)) {
	Xrl x(target, command, args);
	r = x.str();
	return true;
    } else {
	return false;
    }
}

bool
XrlParser::get(string&  protocol,
	       string&  target,
	       string&  command,
	       XrlArgs& args)
    throw (XrlParseError)
{
    return get(protocol, target, command, &args, 0);
}

bool
XrlParser::get(string&		   protocol,
	       string&		   target,
	       string&		   command,
	       XrlArgs&		   args,
	       list<XrlAtomSpell>& spells)
    throw (XrlParseError)
{
    return get(protocol, target, command, &args, &spells);
}

bool
XrlParser::get_return_specs(list<XrlAtomSpell>& spells)
{
    spells.clear();

    skip_comments_and_blanks(_input, _pos);
    if (_pos == _input.end()) {
	// There is no return spec
	return false;
    }

    if (string(_pos, _pos + 2) != string("->"))
	return false;
    _pos += 2;
    skip_comments_and_blanks(_input, _pos);

    parse_atoms_and_spells(0, &spells);

    return (!spells.empty());
}

bool
XrlParser::resync()
{

    while (start_next() == true) {
	if (_input.find("://") != string::npos)
	    return true;
    }
    return false;
}

const XrlParserInput&
XrlParser::parser_input() const
{
    return _xpi;
}

