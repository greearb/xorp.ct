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

// $XORP: xorp/rib/parser.hh,v 1.13 2004/11/02 22:52:14 bms Exp $

#ifndef __RIB_PARSER_HH__
#define __RIB_PARSER_HH__

#include <iostream>
#include <map>
#include <vector>

#include "libxorp/xorp.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"

#include "rib.hh"


class Command;

/**
 * Base class for data types available to Parser.
 */
class Datum {
public:
    virtual ~Datum() {}
};

class IntDatum : public Datum {
public:
    IntDatum(const string& s) {
	_n = 0;
	for (size_t i = 0; i < s.size(); i++) {
	    if (!xorp_isdigit(s[i]))
		xorp_throw0(InvalidString);
	    _n *= 10;
	    _n += s[i] - '0';
	}
    }
    const int& get() const { return _n; }

protected:
    int _n;
};

class StringDatum : public Datum {
public:
    StringDatum(const string& s) : _s(s) {}
    const string& get() const { return _s; }

protected:
    const string _s;
};

class IPv4Datum : public Datum {
public:
    IPv4Datum(const string& s) : _ipv4(s.c_str()) {}
    const IPv4& get() const { return _ipv4; }

protected:
    const IPv4 _ipv4;
};

class IPv4NetDatum : public Datum {
public:
    IPv4NetDatum(const string& s) : _ipv4net(s.c_str()) {}
    const IPv4Net& get() const { return _ipv4net; }

protected:
    const IPv4Net _ipv4net;
};

/**
 * Base class for Argument Parsers.
 */
class ArgumentParser {
public:
    ArgumentParser(const string& parser_name) : _argname(parser_name) {}
    virtual ~ArgumentParser() {}
    virtual Datum* parse(const string& s) const = 0;
    const string& name() const { return _argname;}

private:
    const string _argname;
};

class IntArgumentParser : public ArgumentParser {
public:
    IntArgumentParser() : ArgumentParser("~Int") {}
    Datum* parse(const string& str) const; 
};

class StringArgumentParser : public ArgumentParser {
public:
    StringArgumentParser() : ArgumentParser("~String") {}
    Datum* parse(const string& str) const;
};

class IPv4ArgumentParser : public ArgumentParser {
public:
    IPv4ArgumentParser() : ArgumentParser("~IPv4") {}
    Datum* parse(const string& str) const;
};

class IPv4NetArgumentParser : public ArgumentParser {
public:
    IPv4NetArgumentParser() : ArgumentParser("~IPv4Net") {}
    Datum* parse(const string& str) const;
};

class Parser {
public:
    Parser();
    ~Parser();
    int parse(const string& str) const;
    bool add_command(Command* command);
    bool add_argtype(ArgumentParser* arg); 

private:
    ArgumentParser* get_argument_parser(const string& name) const;
    int split_into_words(const string& str, vector <string>& words) const;

    char _separator;
    map<string, Command* > _templates;
    map<string, ArgumentParser* > _argtypes;
};

class Parse_error {
public:
    Parse_error() : _str("generic error") {}
    Parse_error(const string& s) : _str(s) {}
    const string& str() const { return _str; }

private:
    string _str;
};

/**
 * Datum to variable binding.
 */
class DatumVariableBinding {
public:
    virtual void transfer(Datum* d) throw (Parse_error) = 0;
};

class DatumIntBinding : public DatumVariableBinding {
public:
    DatumIntBinding(int& i) : _i(i) {}
    void transfer(Datum* d) throw (Parse_error) {
	IntDatum* id = dynamic_cast<IntDatum *>(d);
	if (NULL == id)
	    throw Parse_error("Wrong type ? int decoding failed");
	_i = id->get();
    }

private:
    int& _i;
};

class DatumStringBinding : public DatumVariableBinding {
public:
    DatumStringBinding(string& s) : _s(s) {}
    void transfer(Datum* d) throw (Parse_error) {
	StringDatum* id = dynamic_cast<StringDatum *>(d);
	if (NULL == id)
	    throw Parse_error("Wrong type ? string decoding failed");
	_s = id->get();
    }

private:
    string& _s;
};

class DatumIPv4Binding : public DatumVariableBinding {
public:
    DatumIPv4Binding(IPv4& ipv4) : _ipv4(ipv4) {}
    void transfer(Datum* d) throw (Parse_error) {
	IPv4Datum* id = dynamic_cast<IPv4Datum *>(d);
	if (NULL == id)
	    throw Parse_error("Wrong type ? ipv4 decoding failed");
	_ipv4 = id->get();
    }

private:
    IPv4& _ipv4;
};

class DatumIPv4NetBinding : public DatumVariableBinding {
public:
    DatumIPv4NetBinding(IPv4Net& ipv4net) : _ipv4net(ipv4net) {}
    void transfer(Datum* d) throw (Parse_error) {
	IPv4NetDatum* id = dynamic_cast<IPv4NetDatum *>(d);
	if (NULL == id)
	    throw Parse_error("Wrong type ? ipv4 decoding failed");
	_ipv4net = id->get();
    }
private:
    IPv4Net& _ipv4net;
};

class Command {
public:
    Command(const string& cmd_syntax, int nargs) : 
	_syntax(cmd_syntax), _nargs(nargs), _last_arg(-1) { check_syntax(); }
    virtual ~Command();
    virtual int execute() = 0;
    const string& syntax() const { return _syntax; }

    void set_arg(int argnum, Datum* d) throw (Parse_error);
    int num_args() const { return _nargs; }

protected:
    //
    // Abort if number of args in syntax string does not matches
    //  number given. 
    //
    void check_syntax();

    void set_last_arg(int n) { _last_arg = n; }

    //
    // Bind positional argument to Datum type so when argument n arrives, it
    // can be decoded into a member variable.
    //
    void bind(int n, DatumVariableBinding* b);
    void bind_int(int n, int& i);
    void bind_string(int n, string& s);
    void bind_ipv4(int n, IPv4& addr);
    void bind_ipv4net(int n, IPv4Net& net);

    DatumVariableBinding* find_binding(int n);

protected:
    const string _syntax;
    const int	 _nargs;	// Number of args before execute can be called
    int		 _last_arg;	// Last argument added

    map<int, DatumVariableBinding *> _bindings;
};

class TableOriginCommand : public Command {
public:
    TableOriginCommand() : Command("table origin ~String ~Int", 2) {
	bind_string(0, _tablename);
	bind_int(1, _admin_distance);
    }
    virtual int execute() = 0;

protected:
    string	_tablename;
    int		_admin_distance;
};

class TableMergedCommand : public Command {
public:
    TableMergedCommand() : Command("table merged ~String ~String ~String", 3) {
	bind_string(0, _tablename);
	bind_string(1, _t1);
	bind_string(2, _t2);
    }
    virtual int execute() = 0;

protected:
    string _tablename;
    string _t1;
    string _t2;
};

class TableExtIntCommand : public Command {
public:
    TableExtIntCommand() : Command("table extint ~String ~String ~String", 3) {
	bind_string(0, _tablename);
	bind_string(1, _t1);
	bind_string(2, _t2);
    }
    virtual int execute() = 0;

protected:
    string _tablename;
    string _t1;
    string _t2;
};

class RouteAddCommand : public Command {
public:
    RouteAddCommand() : Command("route add ~String ~IPv4Net ~IPv4 ~Int", 4) {
	bind_string(0, _tablename);
	bind_ipv4net(1, _net);
	bind_ipv4(2, _nexthop);
	bind_int(3, _metric);
    }
    virtual int execute() = 0;

protected:
    string	_tablename;
    IPv4Net	_net;
    IPv4	_nexthop;
    int         _metric;
};

class RouteVifAddCommand : public Command {
public:
    RouteVifAddCommand() : Command("route vifadd ~String ~IPv4Net ~String ~IPv4 ~Int", 5) {
	bind_string(0, _tablename);
	bind_ipv4net(1, _net);
	bind_string(2, _vifname);
	bind_ipv4(3, _nexthop);
	bind_int(4, _metric);
    }
    virtual int execute() = 0;

protected:
    string	_tablename;
    IPv4Net	_net;
    string	_vifname;
    IPv4	_nexthop;
    int         _metric;
};

class RouteDeleteCommand : public Command {
public:
    RouteDeleteCommand() : Command("route delete ~String ~IPv4Net", 2) {
	bind_string(0, _tablename);
	bind_ipv4net(1, _net);
    }
    virtual int execute() = 0;

protected:
    string	_tablename;
    IPv4Net	_net;
};

class RouteVerifyCommand : public Command {
public:
    RouteVerifyCommand() : Command(
"route verify ~String ~IPv4 ~String ~IPv4 ~Int", 5)
    {
	bind_string(0, _type);
	bind_ipv4(1, _lookupaddr);
	bind_string(2, _ifname);
	bind_ipv4(3, _nexthop);
	bind_int(4, _metric);
    }
    virtual int execute() = 0;

protected:
    string	_type;			// Type of result expected.
    string	_ifname;
    IPv4	_lookupaddr;
    IPv4	_nexthop;
    int		_metric;
};

class DiscardVifCommand : public Command {
public:
    DiscardVifCommand() : Command("vif Discard ~String ~IPv4 ~Int", 3) {
	bind_string(0, _ifname);
	bind_ipv4(1, _addr);
	bind_int(2, _prefix_len);
    }
    virtual int execute() = 0;

protected:
    string	_ifname;
    IPv4	_addr;
    int		_prefix_len;
};

class EtherVifCommand : public Command {
public:
    EtherVifCommand() : Command("vif Ethernet ~String ~IPv4 ~Int", 3) {
	bind_string(0, _ifname);
	bind_ipv4(1, _addr);
	bind_int(2, _prefix_len);
    }
    virtual int execute() = 0;

protected:
    string	_ifname;
    IPv4	_addr;
    int		_prefix_len;
};

class LoopbackVifCommand : public Command {
public:
    LoopbackVifCommand() : Command("vif Loopback ~String ~IPv4 ~Int", 3) {
	bind_string(0, _ifname);
	bind_ipv4(1, _addr);
	bind_int(2, _prefix_len);
    }
    virtual int execute() = 0;

protected:
    string	_ifname;
    IPv4	_addr;
    int		_prefix_len;
};

class RedistEnableCommand : public Command {
public:
    RedistEnableCommand() : Command("redistribute enable ~String ~String", 2) {
	bind_string(0, _from_table);
	bind_string(1, _to_table);
    }
    virtual int execute() = 0;

protected:
    string _from_table;
    string _to_table;
};

class RedistDisableCommand : public Command {
public:
    RedistDisableCommand() : Command("redistribute disable ~String ~String", 2)
    {
	bind_string(0, _from_table);
	bind_string(1, _to_table);
    }
    virtual int execute() = 0;

protected:
    string _from_table;
    string _to_table;
};

class AddIGPTableCommand : public Command {
public:
    AddIGPTableCommand() : Command("add_igp_table ~String", 1) {
	bind_string(0, _tablename);
    }
    virtual int execute() = 0;

protected:
    string _tablename;
};

class DeleteIGPTableCommand : public Command {
public:
    DeleteIGPTableCommand() : Command("delete_igp_table ~String", 1) {
	bind_string(0, _tablename);
    }
    virtual int execute() = 0;

protected:
    string _tablename;
};

class AddEGPTableCommand : public Command {
public:
    AddEGPTableCommand() : Command("add_egp_table ~String", 1) {
	bind_string(0, _tablename);
    }
    virtual int execute() = 0;

protected:
    string _tablename;
};

class DeleteEGPTableCommand : public Command {
public:
    DeleteEGPTableCommand() : Command("delete_egp_table ~String", 1) {
	bind_string(0, _tablename);
    }
    virtual int execute() = 0;

protected:
    string _tablename;
};

#endif // __RIB_PARSER_HH__
