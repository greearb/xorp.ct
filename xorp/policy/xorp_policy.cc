// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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



#include "policy_module.h"
#include "libxorp/xorp.h"

#include "policy/common/policy_exception.hh"
#include "policy/common/policy_utils.hh"

#include "xrl_target.hh"


void go() {

    setup_dflt_sighandlers();

    EventLoop e;

    XrlStdRouter rtr(e,PolicyTarget::policy_target_name.c_str(),
		     FinderConstants::FINDER_DEFAULT_HOST().str().c_str());

    PolicyTarget policy_target(rtr);
    XrlPolicyTarget xrl_policy_target(&rtr,policy_target);

    while (xorp_do_run && !rtr.ready())
	e.run();

    while (xorp_do_run && policy_target.running())
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
