// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/rtrmgr/main_rtrmgr.cc,v 1.2 2002/12/14 23:43:10 hodson Exp $"

#include <signal.h>

#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "rtrmgr_module.h"
#include "libxorp/xlog.h"

#include "libxipc/finder_server.hh"
#include "libxipc/xrl_std_router.hh"

#include "template_tree_node.hh"
#include "template_commands.hh"
#include "template_tree.hh"
#include "userdb.hh"
#include "xrl_rtrmgr_interface.hh"
#include "main_rtrmgr.hh"

//
// Defaults
//
static const char* default_config_template_dir = "../etc/templates";
static const char* default_xrl_dir 	       = "../xrl/targets";

void
usage(const char *name)
{
    fprintf(stderr,
	"usage: %s [-t cfg_dir] [-x xrl_dir]\n",
	    name);
    fprintf(stderr, "options:\n");

    fprintf(stderr, 
	    "\t-t cfg_dir	specify config directory	[ %s ]\n",
	    default_config_template_dir);

    fprintf(stderr, 
	    "\t-x xrl_dir	specify xrl directory		[ %s ]\n",
	    default_xrl_dir);

    exit(-1);
}

int
main(int argc, char* const argv[])
{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    const char*	config_template_dir = default_config_template_dir;
    const char*	xrl_dir 	    = default_xrl_dir;

    int c;

    while ((c = getopt (argc, argv, "t:x:")) != EOF) {
	switch(c) {  
	case 't':
	    config_template_dir = optarg;
	    break;
	case 'x':
	    xrl_dir = optarg;
	    break;
	case '?':
	default:
	    usage(argv[0]);
	}
    }

    //read the router config template files
    TemplateTree *tt;
    try {
	tt = new TemplateTree(config_template_dir, xrl_dir);
    } catch (const XorpException&) {
	printf("caught exception\n");
	xorp_unexpected_handler();
    }
    tt->display_tree();
    return (0);
}








