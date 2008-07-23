// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/policy/xorp_policy.cc,v 1.9 2008/01/04 03:17:13 pavlin Exp $"

#include "policy_module.h"
#include "libxorp/xorp.h"

#include "policy/common/policy_exception.hh"
#include "policy/common/policy_utils.hh"

#include "xrl_target.hh"


void go()
{
    EventLoop e;

    XrlStdRouter rtr(e,PolicyTarget::policy_target_name.c_str(),
		     FinderConstants::FINDER_DEFAULT_HOST().str().c_str());

    PolicyTarget policy_target(rtr);
    XrlPolicyTarget xrl_policy_target(&rtr,policy_target);

    while (!rtr.ready())
	e.run();

    while (policy_target.running())
	e.run();
}

int main(int /* argc */, char* argv[])
{
    xlog_init(argv[0], 0);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    try {
	go();
    } catch (const PolicyException& e) {
	XLOG_FATAL("PolicyException: %s",e.str().c_str());
    }
   
    xlog_stop();
    xlog_exit();
	
    exit(0);
}
