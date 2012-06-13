// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
//
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include <stdexcept>

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/c_format.hh"

#include "parser.hh"


// ----------------------------------------------------------------------------
// Argument Parsing methods

Datum*
Uint32ArgumentParser::parse(const string& str) const
{
    try {
	return new Uint32Datum(str);
    } catch (const InvalidString&) {
	return NULL;
    }
}

Datum*
StringArgumentParser::parse(const string& str) const
{
    try {
	return new StringDatum(str);
    } catch (const InvalidString&) {
	return NULL;
    }
}

Datum*
IPv4ArgumentParser::parse(const string& str) const
{
    try {
	return new IPv4Datum(str);
    } catch (const InvalidString&) {
	return NULL;
    }
}

Datum*
IPv4NetArgumentParser::parse(const string& str) const
{
    try {
	return new IPv4NetDatum(str);
    } catch (const InvalidString&) {
	return NULL;
    }
}

// ----------------------------------------------------------------------------
// Command methods

Command::~Command()
{
    for (size_t i = 0; i < _bindings.size(); i++) {
	DatumVariableBinding* d = find_binding(i);
	delete d;
	_bindings[i] = NULL;
    }
}

void
Command::check_syntax()
{
    size_t nargs = count(_syntax.begin(), _syntax.end(), '~');
    XLOG_ASSERT((int)nargs == _nargs);
}

DatumVariableBinding*
Command::find_binding(int n)
{
    map<int, DatumVariableBinding* >::iterator mi = _bindings.find(n);
    if (mi == _bindings.end())
	return NULL;
    return mi->second;
}

void
Command::bind(int n, DatumVariableBinding* d)
{
    // Test if binding already exists for argument
    XLOG_ASSERT(find_binding(n) == NULL);
    // Test if non-contiguous positional binding
    XLOG_ASSERT((size_t)n == _bindings.size());

    map<int, DatumVariableBinding* >::value_type b(n, d);
    _bindings.insert(_bindings.end(), b);
}

void
Command::bind_uint32(int n, uint32_t& i)
{
    bind(n, new DatumUint32Binding(i));
}

void
Command::bind_string(int n, string& s)
{
    bind(n, new DatumStringBinding(s));
}

void
Command::bind_ipv4(int n, IPv4& ipv4)
{
    bind(n, new DatumIPv4Binding(ipv4));
}

void
Command::bind_ipv4net(int n, IPv4Net& ipv4net)
{
    bind(n, new DatumIPv4NetBinding(ipv4net));
}

void
Command::set_arg(int n, Datum* d) throw (Parse_error)
{
    DatumVariableBinding* b = find_binding(n);
    if (b == NULL)
	throw Parse_error(c_format("Argument %d has no variable associated "
				   "with it", n));
    b->transfer(d);
}

// ----------------------------------------------------------------------------
// Parser methods

Parser::Parser()
    : _separator(' ')
{
    add_argtype(new Uint32ArgumentParser());
    add_argtype(new StringArgumentParser());
    add_argtype(new IPv4ArgumentParser());
    add_argtype(new IPv4NetArgumentParser());
}

Parser::~Parser()
{
    while (_templates.size() != 0) {
	delete _templates.begin()->second;
	_templates.erase(_templates.begin());
    }
    while (_argtypes.size() != 0) {
	delete _argtypes.begin()->second;
	_argtypes.erase(_argtypes.begin());
    }
}

bool
Parser::add_command(Command* command)
{
    const string& s = command->syntax();
    debug_msg("Parser::add_command %s\n", s.c_str());

    return (_templates.insert(pair<string, Command* >(s, command)).second);
}

ArgumentParser*
Parser::get_argument_parser(const string& name) const
{
    map<string, ArgumentParser* >::const_iterator mi = _argtypes.find(name);
    if (mi == _argtypes.end())
	return NULL;
    return mi->second;
}

// ----------------------------------------------------------------------------
// Helper functions

static string
ltrim(const string& s)
{
    string::const_iterator i;
    for (i = s.begin(); i != s.end() && xorp_isspace(*i); i++)
	;
    return string(i, s.end());
}

static string
rtrim(const string& s)
{
    string::const_iterator i, lns = s.begin();
    for (i = s.begin(); i != s.end(); i++) {
	if (xorp_isspace(*i) == false)
	    lns = i + 1;
    }
    return string(s.begin(), lns);
}

static string
single_space(const string& s)
{
    if (s.size() == 0)
	return s;

    string r;
    string::const_iterator i;
    string::const_iterator lt = s.begin();
    bool inspace = (*lt == ' ');
    for (i = s.begin(); i != s.end(); i++) {
	if (*i == ' ' && inspace == false) {
	    r += string(lt, i + 1);
	    lt = i + 1;
	    inspace = true;
	}
	if (*i != ' ' && inspace == true) {
	    lt = i;
	    inspace = false;
	}
    }
    if (inspace == false) {
	r += string(lt, s.end());
    }

    return r;
}

static string
blat_control(const string& s)
{
    string tmp(s);
    for (size_t i = 0; i < s.size(); i++) {
	if (isascii(s[i]) == false || xorp_iscntrl(s[i]))
	    tmp[i] = ' ';
    }
    return tmp;
}

static string
prepare_line(const string& s)
{
    return ltrim(rtrim(single_space(blat_control(s))));
}

// ----------------------------------------------------------------------------

int
Parser::parse(const string& s) const
{
    debug_msg("-------------------------------------------------------\n");
    debug_msg("Parser::parse: %s\n", s.c_str());

    string str = prepare_line(s);
    if (s.empty() || str[0] == '#')
	return XORP_OK;

    typedef map<string, Command* >::const_iterator CI;
    CI rpair = _templates.lower_bound(str);
    debug_msg("Best match: %s\n", rpair->first.c_str());
    Command* cmd = rpair->second;

    vector<string> words(10);
    int wcount = split_into_words(str, words);

    // Accept empty lines
    if (wcount == 0)
	return XORP_OK;

    vector<string> template_words(10);
    int twcount = split_into_words(rpair->first, template_words);

    try {
	if (cmd == NULL)
	    throw Parse_error("Command invalid");
	if (wcount != twcount) {
	    throw Parse_error(c_format("word count bad (got %d expected %d)",
				       wcount, twcount));
	}
	int args_done = 0;
	for (int i = 0; i < twcount; i++) {
	    debug_msg("word %d\n", i);
	    if (template_words[i][0] != '~') {
		if (template_words[i] != words[i]) {
		    throw Parse_error(words[i]);
		}
	    } else {
		const ArgumentParser* arg;
		arg = get_argument_parser(template_words[i]);
		if (arg == NULL)
		    throw Parse_error(c_format("No parser for type %s",
					       template_words[i].c_str()));
		Datum* d = arg->parse(words[i]);
		if (d == NULL)
		    throw Parse_error(words[i]);

		debug_msg("%s = %s\n",
			  template_words[i].c_str(), words[i].c_str());
		cmd->set_arg(args_done, d);
		args_done++;
		delete d;
	    }
	}
	if (args_done != cmd->num_args()) {
	    throw Parse_error(c_format("Got %d args, expected %d\n",
				       args_done, cmd->num_args()));
	}
	if (cmd->execute() != XORP_OK) {
	    throw Parse_error("Command failed\n");
	}
    }
    catch (Parse_error &pe) {
	cerr << "Parse Error: " << str << "\n";
	cerr << pe.str() << "\n";
	exit(1);
    }
    return XORP_OK;
}

int
Parser::split_into_words(const string& str, vector<string>& words) const
{
    int word = 0;
    int len;
    string tmpstr(str);

    try {
	while (true) {
	    // Remove leading whitespace
	    while (tmpstr.at(0) == ' ' && tmpstr.length() > 0) {
		tmpstr.erase(0, 1);
	    }
	    if (tmpstr.length() == 0)
		break;
	    len = tmpstr.find_first_of(' ');
	    words[word] = tmpstr.substr(0, len);
	    tmpstr.erase(0, len);
	    word++;
	    if (tmpstr.length() == 0)
		break;
	}
    } catch (out_of_range) {
    }
    return word;
}

bool
Parser::add_argtype(ArgumentParser* arg)
{
    debug_msg("Parser::add_argtype %s\n", arg->name().c_str());
    if (arg->name()[0] != '~') {
	XLOG_FATAL("ArgumentParser Type names must begin with ~");
    }

    return (_argtypes.insert(pair<string, ArgumentParser* >(arg->name(), arg)).second);
}
