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

#ident "$XORP: xorp/libxipc/finder_xrl_target.cc,v 1.10 2003/04/23 20:50:48 hodson Exp $"

#include "libxorp/debug.h"
#include "libxorp/status_codes.h"

#include "finder_xrl_target.hh"
#include "finder.hh"
#include "permits.hh"
#include "xuid.hh"

static class TraceFinder
{
public:
    TraceFinder() {
	_do_trace = !(getenv("FINDERTRACE") == 0);
    }
    inline bool on() const { return _do_trace; }
    operator bool() { return _do_trace; }
    inline void set_context(const string& s) { _context = s; }
    inline const string& context() const { return _context; }
protected:
    bool _do_trace;
    string _context;
} finder_tracer;

#define finder_trace_init(x...) 					      \
do {									      \
    if (finder_tracer.on()) finder_tracer.set_context(c_format(x));	      \
} while (0)

#define finder_trace_result(x...)					      \
do {									      \
    if (finder_tracer.on()) {						      \
	string r = c_format(x);						      \
	XLOG_INFO(c_format("%s -> %s\n",				      \
		  finder_tracer.context().c_str(), r.c_str()).c_str());	      \
    }									      \
} while (0)

/**
 * Helper method to pass back consistent message when it is discovered that
 * a client is trying to manipulate state for a non-existent target or
 * a target it is not registered to administer.
 */
static inline string
bad_target_message(const string& tgt_name)
{
    return c_format("Target \"%s\" does not exist "
		    "or caller is not responsible "
		    "for it.\n", tgt_name.c_str());
}

static string
make_cookie()
{
    static uint32_t invoked = 0;
    static uint32_t hash_base;
    if (invoked == 0) {
	srandom(uint32_t(getpid()) ^ uint32_t(&hash_base));
	invoked = random() ^ random();
	hash_base = random();
    }
    uint32_t r = random();
    invoked++;
    return c_format("%08x%08x", invoked, r ^ hash_base);
}

FinderXrlTarget::FinderXrlTarget(Finder& finder)
    : XrlFinderTargetBase(&(finder.commands())), _finder(finder)
{
}

XrlCmdError
FinderXrlTarget::common_0_1_get_target_name(string& name)
{
    name = XrlFinderTargetBase::name();
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderXrlTarget::common_0_1_get_version(string& name)
{
    name = XrlFinderTargetBase::version();
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderXrlTarget::common_0_1_get_status(uint32_t& status, string& reason)
{
    //the finder is always ready if it can receive an XRL request.
    status = PROC_READY;
    reason = "Ready";
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderXrlTarget::finder_0_1_register_finder_client(const string& tgt_name,
						     const string& class_name,
						     const string& in_cookie,
						     string&	   out_cookie)
{
    finder_trace_init("register_finder_client(\"%s\", \"%s\", \"%s\")",
		      tgt_name.c_str(), class_name.c_str(),
		      in_cookie.c_str());

    if (in_cookie.empty() == false) {
	out_cookie = in_cookie;
	_finder.remove_target_with_cookie(out_cookie);
    } else {
	out_cookie = make_cookie();
    }

    if (_finder.add_target(tgt_name, class_name, out_cookie) == false) {
	finder_trace_result("failed (already registered)");
	return XrlCmdError::COMMAND_FAILED(c_format("%s already registered.",
						    tgt_name.c_str()));
    }

    finder_trace_result("\"%s\" okay",  out_cookie.c_str());
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderXrlTarget::finder_0_1_unregister_finder_client(const string& tgt_name)
{
    finder_trace_init("unregister_finder_client(\"%s\")", tgt_name.c_str());

    if (_finder.active_messenger_represents_target(tgt_name)) {
	_finder.remove_target(tgt_name);
	finder_trace_result("okay");
	return XrlCmdError::OKAY();
    }

    finder_trace_result("failed");

    return XrlCmdError::COMMAND_FAILED(bad_target_message(tgt_name));
}

XrlCmdError
FinderXrlTarget::finder_0_1_set_finder_client_enabled(const string& tgt_name,
							const bool&   en)
{
    finder_trace_init("set_finder_client_enabled(\"%s\", %s)",
		      tgt_name.c_str(), (en) ? "true" : "false");

    if (_finder.active_messenger_represents_target(tgt_name)) {
	_finder.set_target_enabled(tgt_name, en);
	finder_trace_result("okay");
	return XrlCmdError::OKAY();
    }
    finder_trace_result("failed (not originator)");
    return XrlCmdError::COMMAND_FAILED(bad_target_message(tgt_name));
}

XrlCmdError
FinderXrlTarget::finder_0_1_finder_client_enabled(const string& tgt_name,
						    bool&         en)
{
    finder_trace_init("finder_client_enabled(\"%s\")",
		      tgt_name.c_str());

    if (_finder.target_enabled(tgt_name, en) == false) {
	finder_trace_result("failed (invalid target name)");
	return XrlCmdError::COMMAND_FAILED(
		c_format("Invalid target name \"%s\"", tgt_name.c_str()));
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderXrlTarget::finder_0_1_add_xrl(const string& xrl,
				      const string& protocol_name,
				      const string& protocol_args,
				      string&	    resolved_xrl_method_name)
{
    Xrl u;

    finder_trace_init("add_xrl(\"%s\", \"%s\", \"%s\")",
		      xrl.c_str(), protocol_name.c_str(),
		      protocol_args.c_str());

    // Construct unresolved Xrl
    try {
	u = Xrl(xrl.c_str());
    } catch (InvalidString&) {
	finder_trace_result("fail (bad xrl).");
	return XrlCmdError::COMMAND_FAILED("Invalid xrl string");
    }

    // Check active messenger is responsible for target described in
    // unresolved Xrl
    if (false == _finder.active_messenger_represents_target(u.target())) {
	finder_trace_result("fail (inappropriate message source).");
	return XrlCmdError::COMMAND_FAILED(bad_target_message(u.target()));
    }

    // Construct resolved Xrl, appended string should very hard to guess :-)
    resolved_xrl_method_name = u.command() + "-" + make_cookie();
    Xrl r(protocol_name, protocol_args, resolved_xrl_method_name);

    // Register Xrl
    if (false == _finder.add_resolution(u.target(), u.str(), r.str())) {
	finder_trace_result("fail (already registered).");
	return XrlCmdError::COMMAND_FAILED("Xrl already registered");
    }
    finder_trace_result("okay");
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderXrlTarget::finder_0_1_remove_xrl(const string&	xrl)
{
    Xrl u;

    finder_trace_init("remove_xrl(\"%s\")", xrl.c_str());

    // Construct Xrl
    try {
	u = Xrl(xrl.c_str());
    } catch (InvalidString&) {
	finder_trace_result("fail (bad xrl).");
	return XrlCmdError::COMMAND_FAILED("Invalid xrl string");
    }

    // Check active messenger is responsible for target described in Xrl
    if (false == _finder.active_messenger_represents_target(u.target())) {
	finder_trace_result("fail (inappropriate message source).");
	return XrlCmdError::COMMAND_FAILED(bad_target_message(u.target()));
    }

    // Unregister Xrl
    if (false == _finder.remove_resolutions(u.target(), u.str())) {
	finder_trace_result("fail (xrl does not exist).");
	return XrlCmdError::COMMAND_FAILED("Xrl does not exist");
    }
    finder_trace_result("okay");
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderXrlTarget::finder_0_1_resolve_xrl(const string&	xrl,
					  XrlAtomList&	resolved_xrls)
{
    finder_trace_init("resolve_xrl(\"%s\")", xrl.c_str());

    Xrl u;

    // Construct Xrl
    try {
	u = Xrl(xrl.c_str());
    } catch (InvalidString&) {
	finder_trace_result("fail (bad xrl).");
	return XrlCmdError::COMMAND_FAILED("Invalid xrl string");
    }

    // Check target exists and is enabled
    bool en;
    if (_finder.target_enabled(u.target(), en) == false) {
	finder_trace_result("fail (target does not exist).");
	return XrlCmdError::COMMAND_FAILED("Xrl target does not exist.");
    } else if (en == false) {
	finder_trace_result("fail (xrl exists but is not enabled).");
	return XrlCmdError::COMMAND_FAILED("Xrl target is not enabled.");
    }

    const Finder::Resolveables* resolutions = _finder.resolve(u.target(),
								u.str());
    if (0 == resolutions) {
	finder_trace_result("fail (does not resolve).");
	return XrlCmdError::COMMAND_FAILED("Xrl does not resolve");
    }

    Finder::Resolveables::const_iterator ci = resolutions->begin();
    while (resolutions->end() != ci) {
	string s;
	try {
	    s = Xrl(ci->c_str()).str();
	} catch (const InvalidString& ) {
	    finder_trace_result("fail (does not resolve as an xrl).");
	    XLOG_ERROR("Resolved something that did not look an xrl: \"%s\"\n",
		       ci->c_str());
	}
	resolved_xrls.append(XrlAtom(s));
	++ci;
    }
    finder_trace_result("resolves okay.");
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderXrlTarget::finder_0_1_get_xrl_targets(XrlAtomList& xal)
{
    list<string> tgts;

    _finder.fill_target_list(tgts);

    // Special case, add finder itself to list
    tgts.push_back("finder");
    tgts.sort();

    for (list<string>::const_iterator i = tgts.begin(); i != tgts.end(); i++) {
	xal.append(XrlAtom(*i));
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
FinderXrlTarget::finder_0_1_get_xrls_registered_by(const string& tgt,
						     XrlAtomList&  xal)
{
    list<string> xrls;

    // Special case, finder request
    if (tgt == "finder") {
	list<string> cmds;
	_finder.commands().get_command_names(cmds);
	// Turn command names into Xrls
	for (list<string>::iterator i = cmds.begin(); i != cmds.end(); i++) {
	    xrls.push_back(Xrl("finder", *i).str());
	}
    } else if (_finder.fill_targets_xrl_list(tgt, xrls) == false) {
	return
	    XrlCmdError::COMMAND_FAILED
	    (c_format("No such target \"%s\"", tgt.c_str()));
    }
    for (list<string>::const_iterator i = xrls.begin(); i != xrls.end(); i++) {
	xal.append(XrlAtom(*i));
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderXrlTarget::finder_0_1_get_ipv4_permitted_hosts(XrlAtomList& ipv4hosts)
{
    const IPv4Hosts& hl = permitted_ipv4_hosts();
    for (IPv4Hosts::const_iterator ci = hl.begin(); ci != hl.end(); ++ci)
	ipv4hosts.append(XrlAtom(*ci));

    return XrlCmdError::OKAY();
}

XrlCmdError
FinderXrlTarget::finder_0_1_get_ipv4_permitted_nets(XrlAtomList& ipv4nets)
{
    const IPv4Nets& nl = permitted_ipv4_nets();
    for (IPv4Nets::const_iterator ci = nl.begin(); ci != nl.end(); ++ci)
	ipv4nets.append(XrlAtom(*ci));

    return XrlCmdError::OKAY();
}

XrlCmdError
FinderXrlTarget::finder_0_1_get_ipv6_permitted_hosts(XrlAtomList& ipv6hosts)
{
    const IPv6Hosts& hl = permitted_ipv6_hosts();
    for (IPv6Hosts::const_iterator ci = hl.begin(); ci != hl.end(); ++ci)
	ipv6hosts.append(XrlAtom(*ci));

    return XrlCmdError::OKAY();
}

XrlCmdError
FinderXrlTarget::finder_0_1_get_ipv6_permitted_nets(XrlAtomList& ipv6nets)
{
    const IPv6Nets& nl = permitted_ipv6_nets();
    for (IPv6Nets::const_iterator ci = nl.begin(); ci != nl.end(); ++ci)
	ipv6nets.append(XrlAtom(*ci));

    return XrlCmdError::OKAY();
}
