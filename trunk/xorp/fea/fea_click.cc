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

#ident "$XORP: xorp/fea/fea_click.cc,v 1.8 2004/05/16 00:26:24 pavlin Exp $"

#include "config.h"
#include "fea_module.h"

#include "libxorp/xorp.h"

#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"
#include "libxorp/mac.hh"
#include "libxorp/ipvx.hh"
#include "libxipc/xrl_std_router.hh"
#include "fticonfig.hh"
#include "ifmanager.hh"
#include "ifconfig.hh"
#include "ifconfig_click.hh"
#include "ifconfig_freebsd.hh"
#include "click_glue.hh"
#include "fti_click.hh"
#include "click.hh"
#include "xrl_target.hh"

#include "engine.hh"
#include "engine_click.hh"

static const char *server = "fea";/* This servers name */

void
usage(char *name)
{
    fprintf(stderr,
	    "usage: %s [-h finder_host] [-m click_module] [-f fea_module] "
	    "[-c click_config] \n",
	    name);
    exit(-1);
}

int
main(int argc, char *argv[])
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    int c;
    string finder_host = FINDER_DEFAULT_HOST.str();
    const char *click_config_file = "/click/config";
    const char *click_module = 	"/usr/local/click/lib/click.ko";
    const char *fea_module = "/usr/local/click/lib/xorp.bo";

    /*
    ** Initialize and start xlog
    */
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    while((c = getopt (argc, argv, "h:m:f:c:")) != EOF) {
	switch (c) {
	case 'h':
	    finder_host = optarg;
	    break;
	case 'm':
	    click_module = optarg;
	    break;
	case 'f':
	    fea_module = optarg;
	    break;
	case 'c':
	    click_config_file = optarg;
	    break;
	case '?':
	    usage(argv[0]);
	}
    }


    /* XXX
    ** Do all that daemon stuff take off into the background disable signals
    ** and all that other good stuff.
    */

    EventLoop eventloop;
    ForwardingEngine* fe = 0;

    /*
    ** Setup and load click.
    */
    try {
	Click click(click_config_file, click_module, fea_module);
	if (!click.load()) {
	    XLOG_ERROR("Couldn't load click");
	    return 0;
	}
	fe = new ClickForwardingEngine(eventloop);
    } catch(FtiConfigError& xe) {
	XLOG_ERROR(c_format("%s from %s -> %s",
			    xe.what().c_str(),
			    xe.where().c_str(),
			    xe.why().c_str()).c_str());
	return 0;
    } catch(...) {
 	xorp_catch_standard_exceptions();
	return 0;
    }

    Ifconfig& ifc = fe->ifconfig();
    FtiConfig& ftic = fe->ftic();

    /*
    ** Attach to our ifmgr.
    */
    InterfaceManager ifmgr(&ifc);

    /*
    ** Configure fea.
    */
    XrlStdRouter fea(eventloop, server, finder_host.c_str());

    /*
    ** Add commands.
    */
    XrlFeaTarget xft(eventloop, &fea, ftic, ifmgr);

    {
	// Wait until the XrlRouter becomes ready
	bool timed_out = false;
	
	XorpTimer t = eventloop.set_flag_after_ms(10000, &timed_out);
	while (fea.ready() == false && timed_out == false) {
	    eventloop.run();
	}
	
	if (fea.ready() == false) {
	    XLOG_FATAL("XrlRouter did not become ready.  No Finder?");
	}
    }

    try {
	for (;;)
	    eventloop.run();
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    /*
    ** Gracefully stop and exit xlog
    */
    xlog_stop();
    xlog_exit();
    return 0;
}
