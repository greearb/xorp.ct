// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/policy/policy_target.cc,v 1.22 2008/08/06 08:28:01 abittau Exp $"

#include "policy_module.h"
#include "libxorp/xorp.h"
#include "policy/common/policy_utils.hh"
#include "policy/common/varrw.hh"
#include "policy_target.hh"

// static members
string PolicyTarget::policy_target_name = "policy";

PolicyTarget::PolicyTarget(XrlStdRouter& rtr) :
    _running(true), _commit_delay(2000), 
    _process_watch(rtr, _pmap),
    _conf(_process_watch),
    _filter_manager(_conf.import_filters(),
		    _conf.sourcematch_filters(),
		    _conf.export_filters(),
		    _conf.sets(),
		    _conf.tagmap(),
		    rtr,
		    _process_watch,
		    _pmap)

{
    _conf.set_filter_manager(_filter_manager);
    _process_watch.set_notifier(_filter_manager);
}

bool
PolicyTarget::running()
{
    return _running;
}

void
PolicyTarget::shutdown()
{
    _running = false;
}


void
PolicyTarget::create_term(const string& policy, const ConfigNodeId& order,
			  const string& term)
{
    _conf.create_term(policy, order, term);
}

void
PolicyTarget::delete_term(const string& policy, const string& term)
{
    _conf.delete_term(policy,term);
}

void
PolicyTarget::update_term_block(const string& policy,
				const string& term,
				const uint32_t& block,
				const ConfigNodeId& order,
				const string& statement)
{
    _conf.update_term_block(policy, term, block, order, statement);
}

void
PolicyTarget::create_policy(const string& policy)
{
    _conf.create_policy(policy);
}

void
PolicyTarget::delete_policy(const string& policy)
{
    _conf.delete_policy(policy);
}

void
PolicyTarget::create_set(const string& name)
{
    _conf.create_set(name);
}

void
PolicyTarget::update_set(const string& type, const string& name, 
			 const string& element)
{
    _conf.update_set(type, name, element);
}

void
PolicyTarget::delete_set(const string& name)
{
    _conf.delete_set(name);
}

void
PolicyTarget::add_to_set(const string& type, const string& name, 
			 const string& element)
{
    _conf.add_to_set(type, name, element);
}

void
PolicyTarget::delete_from_set(const string& type, const string& name, 
			      const string& element)
{
    _conf.delete_from_set(type, name, element);
}

void
PolicyTarget::update_import(const string& protocol,
			    const string& policies,
			    const string& mod)
{
    POLICIES p;

    policy_utils::str_to_list(policies, p);

    _conf.update_imports(protocol, p, mod);

    // commit after a bit, as we may get conf changes... especially on "global
    // conf change" or at startup
    _conf.commit(_commit_delay);
}

void
PolicyTarget::update_export(const string& protocol,
			    const string& policies,
			    const string& mod)
{
    POLICIES p;

    policy_utils::str_to_list(policies, p);

    _conf.update_exports(protocol, p, mod);

    // try to aggregate commits by delaying them
    _conf.commit(_commit_delay);
}

void
PolicyTarget::add_varmap(const string& protocol, const string& variable,
			 const string& type, const string& access,
			 const VarRW::Id& id)
{
    _conf.add_varmap(protocol, variable, type, access, id);
}

void
PolicyTarget::commit(uint32_t msec)
{
    _conf.commit(msec);
}

string
PolicyTarget::dump_state(uint32_t id)
{
    return _conf.dump_state(id);
}

void
PolicyTarget::birth(const string& tclass, const string& /* tinstance */)
{
    _process_watch.birth(tclass);
}

void
PolicyTarget::death(const string& tclass, const string& /* tinstance */)
{
    // Remove the "import" and "export" dependencies for the protocol
    string protocol = _pmap.protocol(tclass);

    _conf.clear_imports(protocol);
    _conf.clear_exports(protocol);

    _process_watch.death(tclass);
}

void
PolicyTarget::set_proto_target(const string& protocol, const string& target)
{
    _pmap.set_xrl_target(protocol, target);
}

string
PolicyTarget::test_policy(const string& args)
{
    string policy;
    string prefix;
    string attributes;

    // We receive the following string:
    // policyname prefix [route attributes]

    // parse policy
    string::size_type i = args.find(' ', 0);
    if (i == string::npos)
	xorp_throw(PolicyException, "No policy specified");

    policy = args.substr(0, i);

    // parse prefix
    i++;
    string::size_type j = args.find(' ', i);
    if (j == string::npos)
	prefix = args.substr(i);
    else {
	prefix = args.substr(i, j - i);

	j += 1;

	// strip quotes if present
	if (args.find('"') == j) {

	    string::size_type k = args.find_last_of('"');
	    if (j == k || k != (args.length() - 1))
	        xorp_throw(PolicyException, "Missing last quote");

	    j++;
	    attributes = args.substr(j, k - j);
	} else
	    attributes = args.substr(j);
    }

    string route;
    bool accepted = test_policy(policy, prefix, attributes, route);

    ostringstream oss;

    oss << "Policy decision: " << (accepted ? "accepted" : "rejected") << endl;
    if (!route.empty())
	oss << "Route modifications:" << endl
	    << route;

    return oss.str();
}

bool
PolicyTarget::test_policy(const string& policy, const string& prefix,
			  const string& attributes, string& mods)
{
    RATTR attrs;
    RATTR mod;

    // XXX lame IPv6 detection
    if (prefix.find(':') != string::npos)
	attrs["network6"] = prefix;
    else
	attrs["network4"] = prefix;

    parse_attributes(attributes, attrs);

    bool res = test_policy(policy, attrs, mod);

    for (RATTR::iterator i = mod.begin(); i != mod.end(); ++i) {
	mods += i->first;
	mods += "\t";
	mods += i->second;
	mods += "\n";
    }

    return res;
}

bool
PolicyTarget::test_policy(const string& policy, const RATTR& attrs, RATTR& mods)
{
    return _conf.test_policy(policy, attrs, mods);
}

void
PolicyTarget::parse_attributes(const string& attr, RATTR& out)
{
    // format: --attributename=value
    string::size_type i = 0;
    string::size_type j = 0;

    while ((j = attr.find("--", i)) != string::npos) {
	j += 2;

	// name
	i = attr.find('=', j);
	if (i == string::npos)
	    xorp_throw(PolicyException, "Need a value in attribute list");

	string name = attr.substr(j, i - j);

	// value
	string value;
	i++;
	j = attr.find(" --", i);
	if (j == string::npos)
	    value = attr.substr(i);
	else
	    value = attr.substr(i, j - i);

	out[name] = value;
    }
}

string
PolicyTarget::cli_command(const string& cmd)
{
    string command;
    string arg;
    
    string::size_type i = cmd.find(' ');
    if (i == string::npos)
	command = cmd;
    else {
	command = cmd.substr(0, i);
	arg = cmd.substr(i + 1);
    }

    if (command.compare("test") == 0)
	return test_policy(arg);
    else if (command.compare("show") == 0)
	return show(arg);
    else
	xorp_throw(PolicyException, "Unknown command");
}

string
PolicyTarget::show(const string& arg)
{
    string type;
    string name;

    string::size_type i = arg.find(' ');
    if (i == string::npos)
	type = arg;
    else {
	type = arg.substr(0, i);
	name = arg.substr(i + 1);
    }

    RESOURCES res;

    show(type, name, res);

    ostringstream oss;

    for (RESOURCES::iterator i = res.begin(); i != res.end(); ++i) {
	if (name.empty())
	    oss << i->first << "\t";

	oss << i->second << endl;
    }

    return oss.str();
}

void
PolicyTarget::show(const string& type, const string& name, RESOURCES& res)
{
    _conf.show(type, name, res);
}
