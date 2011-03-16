// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008-2011 XORP, Inc and Others
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



#include "libxorp/xorp.h"
#include "cli/cli_module.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxipc/xrl_std_router.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

namespace {

class CGException : public XorpReasonedException {
public:
    CGException(const char* file, size_t line, const string& why = "")
        : XorpReasonedException("CGException", file, line, why) {}
};

class CliGeneric {
public:
    CliGeneric(XrlRouter& rtr);

    string str() const;
    void   set_xrl(const string& xrl);
    void   set_fmt(const string& fmt);
    void   add_arg(const string& arg);
    void   add_iterator(const string& iter);
    void   set_separator(const string& sep);
    bool   running();
    void   own();
    string unescape(const string& in);
    string replace(const string& in, char old, char n);

private:
    typedef map<string, string*>        ITERATORS; // varname -> XRL
    typedef set<string>			VARS;
    typedef map<string, const XrlAtom*> SYMBOLS; // varname -> value

    void          send_xrl(const string& xrl);
    void          xrl_cb(const XrlError& error, XrlArgs* args);
    void	  print(const XrlArgs& args);
    void	  iterate(const VARS& vars);
    void	  wait_xrl();
    bool	  iterator_resolved(const string& var);
    void	  grab_keys(XrlAtomList& keys, const string& var);
    void	  do_grab_keys(const XrlArgs& args);
    string	  set_variables(const string& xrl);
    string	  atom_val(const XrlAtom& atom);
    void	  get_vars(VARS& vars, const string& in);

    bool	    _first;
    string	    _separator;
    string*	    _last;
    string	    _xrl;
    string	    _fmt;
    XrlRouter&	    _rtr;
    bool	    _running;
    ITERATORS	    _iterators;
    bool	    _xrl_pending;
    SYMBOLS	    _symbols;
    XrlAtomList*    _keys; // XXX
};

CliGeneric::CliGeneric(XrlRouter& rtr) 
    : _first(true),
      _last(NULL),
      _rtr(rtr),
      _running(false),
      _xrl_pending(false),
      _keys(NULL)
{
}

bool
CliGeneric::running()
{
    return _running || _xrl_pending;
}

void
CliGeneric::set_separator(const string& sep)
{
    _separator = sep;
}

void
CliGeneric::add_iterator(const string& iter)
{
    string::size_type i = iter.find('=');
    if (i == string::npos)
	xorp_throw(CGException, "bad iterator spec");

    string var = iter.substr(0, i);
    string xrl = iter.substr(i + 1);

    if (_iterators.find(var) != _iterators.end())
	xorp_throw(CGException, "iterator already present");

    string* x = new string(xrl);
    _iterators[var] = x;
    _last = x;
}

void
CliGeneric::set_xrl(const string& xrl)
{
    _xrl  = xrl;
    _last = &_xrl;
}

void
CliGeneric::set_fmt(const string& fmt)
{
    _fmt  = unescape(fmt);
    _fmt  = replace(_fmt, '_', ' ');

    _last = &_fmt;
}

void
CliGeneric::wait_xrl()
{
    while (_xrl_pending)
	_rtr.eventloop().run();
}

string
CliGeneric::replace(const string& in, char old, char n)
{
    ostringstream oss;

    for (string::const_iterator i = in.begin(); i != in.end(); ++i) {
	char x = *i;

	if (x == old)
	    oss << n;
	else
	    oss << x;
    }

    return oss.str();
}

string
CliGeneric::unescape(const string& in)
{
    ostringstream oss;

    string x = in;
    while (!x.empty()) {
	string::size_type i = x.find('\\');
	if (i == string::npos) {
	    oss << x;
	    break;
	}

	oss << x.substr(0, i);

	switch (x.at(i + 1)) {
	case '\\':
	    oss << "\\";
	    break;

	case 't':
	    oss << "\t";
	    break;
	
	case 'n':
	    oss << "\n";
	    break;

	default:
	    xorp_throw(CGException, "Unknown escape sequence");
	}

	x = x.substr(i + 2);
    }

    return oss.str();
}

void
CliGeneric::add_arg(const string& arg)
{
    if (!_last)
	xorp_throw(CGException, "Specify XRL first");

    string::size_type i = _last->find('$');
    if (i == string::npos)
	xorp_throw(CGException, "XRL has no unresolved parameters");

    ostringstream result;

    result << _last->substr(0, i);
    result << arg;
    result << _last->substr(i + 2); // XXX assume one digit only

    _last->assign(result.str());
}

string
CliGeneric::str() const
{
    ostringstream oss;

    oss << "XRL [" << _xrl << "]" << endl
	<< "FMT [" << _fmt << "]" << endl
	;

    return oss.str();
}

void
CliGeneric::own()
{
    _running = true;

    VARS v;
    get_vars(v, _xrl);

    // XXX I gotta think too much if I gotta return to the caller so lets do it
    // the old school way
    iterate(v);

    _running = false;
}

void
CliGeneric::iterate(const VARS& vars)
{
    VARS v = vars;

    // all variables resolved - pwn it.
    if (v.empty()) {
	send_xrl(_xrl);

	return;
    }

    // choose an iterator.
    bool   found = false;
    string var;

    for (VARS::iterator i = v.begin(); i != v.end(); ++i) {
	var = *i;

	// see if we can use this iterator
	if (iterator_resolved(var)) {
	    v.erase(i);
	    found = true;
	    break;
	}
    }

    if (!found)
	xorp_throw(CGException, "Cannot find an iterator");

    // grab keys
    XrlAtomList keys;
    grab_keys(keys, var);

    // iterate through keys
    for (unsigned i = 0; i < keys.size(); ++i) {
	const XrlAtom& atom = keys.get(i);

	_symbols[var] = &atom;

	iterate(v);
    }

    _symbols.erase(var);
}

void
CliGeneric::grab_keys(XrlAtomList& keys, const string& var)
{
    ITERATORS::iterator i = _iterators.find(var);
    XLOG_ASSERT(i != _iterators.end());

    XLOG_ASSERT(!_keys);

    _keys = &keys;

    send_xrl(*(i->second));

    _keys = NULL;
}

bool
CliGeneric::iterator_resolved(const string& var)
{
    ITERATORS::iterator i = _iterators.find(var);

    if (i == _iterators.end())
	xorp_throw(CGException,
		   c_format("cannot find iterator for var %s", var.c_str()));

    VARS v;
    get_vars(v, *(i->second));

    // all vars must be resolved
    for (VARS::iterator j = v.begin(); j != v.end(); ++j) {
	const string& var = *j;

	if (_symbols.find(var) == _symbols.end())
	    return false;
    }

    return true;
}

void
CliGeneric::get_vars(VARS& vars, const string& in)
{
    string::size_type i = 0;

    while ((i = in.find('%', i)) != string::npos) {
	i++;

	char x = in.at(i);

	string var = string("%") + x;
	vars.insert(var);
    }
}

string
CliGeneric::set_variables(const string& xrl)
{
    ostringstream oss;

    string x = xrl;

    while (!x.empty()) {
	string::size_type i = x.find('%');

	if (i == string::npos) {
	    oss << x;
	    break;
	}

	oss << x.substr(0, i);

	string var = x.substr(i, 2);

	if (xorp_isdigit(var.at(1)))
	    oss << var;
	else {
	    SYMBOLS::iterator i = _symbols.find(var);
	    XLOG_ASSERT(i != _symbols.end());

	    const XrlAtom* atom = i->second;
	    oss << atom_val(*atom);
	} 

	x = x.substr(i + 2);
    }

    return oss.str();
}

void
CliGeneric::send_xrl(const string& xrl)
{
    string xrl_fixed = set_variables(xrl);

    Xrl x(xrl_fixed.c_str());

    XLOG_ASSERT(!_xrl_pending);

    if (!_rtr.send(x, callback(this, &CliGeneric::xrl_cb)))
	xorp_throw(CGException, "can't send XRL");

    _xrl_pending = true;

    wait_xrl();
}

void
CliGeneric::xrl_cb(const XrlError& error, XrlArgs* args)
{
    if (error != XrlError::OKAY())
	XLOG_FATAL("XRL error: %s", error.str().c_str());

    _xrl_pending = false;

    if (!_keys)
	print(*args);
    else {
	do_grab_keys(*args);
    }
}

void
CliGeneric::do_grab_keys(const XrlArgs& args)
{
    if (args.size() != 1)
	xorp_throw(CGException, "bad size for keys XRL");

    const XrlAtom& atom = args.item(0);

    if (atom.type() != xrlatom_list)
	xorp_throw(CGException, "not a list");

    const XrlAtomList& list = atom.list();

    XLOG_ASSERT(_keys);

    *_keys = list;
}

void
CliGeneric::print(const XrlArgs& args)
{
    string fmt = set_variables(_fmt);

    if (!_first && !_separator.empty())
	cout << _separator;

    while (!fmt.empty()) {
	string::size_type i = fmt.find('%');
	if (i == string::npos) {
	    cout << fmt;
	    break;
	}

	// print everything upto the argument
	string bit = fmt.substr(0, i);
	cout << bit;

	int numlen = 1; // XXX
	string num = fmt.substr(i + 1, numlen);
	unsigned idx = (unsigned) atoi(num.c_str());

	fmt = fmt.substr(i + 1 + numlen);

	// print the argument
	if (idx >= args.size())
	    xorp_throw(CGException, "invalid index");

	const XrlAtom& a = args.item(idx);
	cout << atom_val(a);
    }

    _first = false;
}

string
CliGeneric::atom_val(const XrlAtom& a)
{
    string s = a.str().c_str();

    string::size_type i = s.find('=');
    if (i == string::npos)
        xorp_throw(CGException, "bad result");

    s = s.substr(i + 1);

    return s;
}

void
usage(const char* progname)
{
    cout << "Usage: " << progname << " <opts>"	<< endl
         << "-h\t\thelp"			<< endl
	 << "-x\t\t<XRL to call>"		<< endl
	 << "-a\t\t<add arg to previous XRL>"	<< endl
	 << "-f\t\t<output format>"		<< endl
	 << "-i\t\t<iterator>"			<< endl
	 << "-s\t\t<separator>"			<< endl
	 ;

    exit(1);
}

void
own(int argc, char* argv[])
{
    EventLoop e;
    XrlStdRouter router(e, "cli_generic",
		        FinderConstants::FINDER_DEFAULT_HOST().str().c_str());

    CliGeneric cg(router);
    int c;

    while ((c = getopt(argc, argv, "x:hf:a:i:s:")) != -1) {
	switch (c) {
	case 'i':
	    cg.add_iterator(optarg);
	    break;

	case 'x':
	    cg.set_xrl(optarg);
	    break;

	case 'f':
	    cg.set_fmt(optarg);
	    break;

	case 'a':
	    cg.add_arg(optarg);
	    break;

	case 's':
	    cg.set_separator(optarg);
	    break;

	default:
	case 'h':
	    usage(argv[0]);
	    break;
	}
    }

    router.finalize();

    while (false == router.failed() && false == router.ready())
	e.run();

    if (true == router.failed())
	xorp_throw(CGException, "Router failed to communicate with finder.");

    if (false == router.ready())
	xorp_throw(CGException, 
		   "Connected to finder, but did not become ready.");

    cg.own();

    while (cg.running())
	e.run();
}

} // anon namespace

int
main(int argc, char* argv[])
{
    xlog_init(argv[0], 0);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    try {
	own(argc, argv);
    } catch (const CGException& e) {
	UNUSED(e);
	XLOG_FATAL("CGException: %s", e.str().c_str());
    }

    xlog_stop();
    xlog_exit();

    exit(0);
}
