// -*- C-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/libxipc/finder_ng_xrl_target.cc,v 1.1 2003/01/28 00:42:24 hodson Exp $"

#include "finder_ng_xrl_target.hh"
#include "finder_ng.hh"
#include "xuid.hh"

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

FinderNGXrlTarget::FinderNGXrlTarget(FinderNG& finder)
    : XrlFinderTargetBase(&(finder.commands())), _finder(finder)
{
}

XrlCmdError
FinderNGXrlTarget::common_0_1_get_target_name(string& name)
{
    name = XrlFinderTargetBase::name();
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderNGXrlTarget::common_0_1_get_version(string& name)
{
    name = XrlFinderTargetBase::version();
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderNGXrlTarget::finder_0_1_register_finder_client(const string& tgt_name,
						     const string& class_name,
						     const string& in_cookie,
						     string&	   out_cookie)
{
    if (in_cookie.empty() == false) {
	out_cookie = in_cookie;
	_finder.remove_target_with_cookie(out_cookie);
    } else {
	out_cookie = make_cookie();
    }
    
    if (_finder.add_target(tgt_name, class_name, out_cookie) == false) {
	return XrlCmdError::COMMAND_FAILED(c_format("%s already registered.",
						    tgt_name.c_str()));
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderNGXrlTarget::finder_0_1_unregister_finder_client(const string& tgt_name)
{
    if (_finder.active_messenger_represents_target(tgt_name)) {
	_finder.remove_target(tgt_name);
	return XrlCmdError::OKAY();
    }

    return XrlCmdError::COMMAND_FAILED(bad_target_message(tgt_name));
}

XrlCmdError
FinderNGXrlTarget::finder_0_1_add_xrl(const string& xrl,
				      const string& protocol_name,
				      const string& protocol_args,
				      string&	    resolved_xrl_method_name)
{
    Xrl u;

    // Construct unresolved Xrl
    try {
	u = Xrl(xrl.c_str());
    } catch (InvalidString&) {
	return XrlCmdError::COMMAND_FAILED("Invalid xrl string");
    }

    // Check active messenger is responsible for target described in
    // unresolved Xrl
    if (false == _finder.active_messenger_represents_target(u.target())) {
	return XrlCmdError::COMMAND_FAILED(bad_target_message(u.target()));
    }

    // Construct resolved Xrl, appended string should very hard to guess :-)
    resolved_xrl_method_name = u.command() + "-" + make_cookie();
    Xrl r(protocol_name, protocol_args, resolved_xrl_method_name);

    // Register Xrl
    if (false == _finder.add_resolution(u.target(), u.str(), r.str())) {
	return XrlCmdError::COMMAND_FAILED("Xrl already registered");
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderNGXrlTarget::finder_0_1_remove_xrl(const string&	xrl)
{
    Xrl u;

    // Construct Xrl
    try {
	u = Xrl(xrl.c_str());
    } catch (InvalidString&) {
	return XrlCmdError::COMMAND_FAILED("Invalid xrl string");
    }

    // Check active messenger is responsible for target described in Xrl
    if (false == _finder.active_messenger_represents_target(u.target())) {
	return XrlCmdError::COMMAND_FAILED(bad_target_message(u.target()));
    }

    // Unregister Xrl
    if (false == _finder.remove_resolutions(u.target(), u.str())) {
	return XrlCmdError::COMMAND_FAILED("Xrl does not exist");
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderNGXrlTarget::finder_0_1_resolve_xrl(const string&	xrl,
					  XrlAtomList&	resolved_xrls)
{
    Xrl u;

    // Construct Xrl
    try {
	u = Xrl(xrl.c_str());
    } catch (InvalidString&) {
	return XrlCmdError::COMMAND_FAILED("Invalid xrl string");
    }

    const FinderNG::Resolveables* resolutions = _finder.resolve(u.target(),
								u.str());
    if (0 == resolutions)
	return XrlCmdError::COMMAND_FAILED("Xrl does not resolve");

    FinderNG::Resolveables::const_iterator ci = resolutions->begin();
    while (resolutions->end() != ci) {
	string s;
	try {
	    s = Xrl(ci->c_str()).str();
	} catch (const InvalidString& ) {
	    XLOG_ERROR("Resolved something that did not look an xrl: \"%s\"\n",
		       ci->c_str());
	}
	resolved_xrls.append(XrlAtom(s));
	++ci;
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
FinderNGXrlTarget::finder_0_1_get_xrl_targets(XrlAtomList&)
{
    return XrlCmdError::COMMAND_FAILED("Unimplemented");
}

XrlCmdError
FinderNGXrlTarget::finder_0_1_get_xrls_registered_by(const string&,
						     XrlAtomList&)
{
    return XrlCmdError::COMMAND_FAILED("Unimplemented");
}
