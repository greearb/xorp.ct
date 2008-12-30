// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/rtrmgr/test_templates.cc,v 1.21 2008/07/23 05:11:45 pavlin Exp $"


#include <signal.h>

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

//#include "libxipc/finder_server.hh"
#include "libxipc/xrl_std_router.hh"

#include "rtrmgr_error.hh"
#include "template_commands.hh"
#include "template_tree.hh"
#include "template_tree_node.hh"
//#include "userdb.hh"
//#include "xrl_rtrmgr_interface.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif // HAVE_GETOPT_H

// XXX: hack.
#ifdef HOST_OS_WINDOWS
#include "glob_win32.h"
#endif // HOST_OS_WINDOWS

//
// Defaults
//
static const char* c_srcdir = getenv("srcdir");
static const string srcdir = c_srcdir ? c_srcdir : ".";
static const string default_xorp_root_dir = srcdir + "/..";
static const string default_config_template_dir = srcdir + "/../etc/templates";
static const string default_xrl_targets_dir = srcdir + "/../xrl/targets";

void
usage(const char* name)
{
    fprintf(stderr,
	"usage: %s [-t cfg_dir] [-x xrl_targets_dir]\n",
	    name);
    fprintf(stderr, "options:\n");

    fprintf(stderr, 
	    "\t-t cfg_dir		specify config directory	[ %s ]\n",
	    default_config_template_dir.c_str());

    fprintf(stderr, 
	    "\t-x xrl_targets_dir	specify xrl directory		[ %s ]\n",
	    default_xrl_targets_dir.c_str());

    exit(1);
}

// the following two functions are an ugly hack to cause the C code in
// the parser to call methods on the right version of the TemplateTree

void
add_cmd_adaptor(char *cmd, TemplateTree* tt) throw (ParseError)
{
    tt->add_cmd(cmd);
}


void
add_cmd_action_adaptor(const string& cmd, const list<string>& action,
		       TemplateTree* tt) throw (ParseError)
{
    tt->add_cmd_action(cmd, action);
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

    string config_template_dir = default_config_template_dir;
    string xrl_targets_dir = default_xrl_targets_dir;

    int c;

    while ((c = getopt (argc, argv, "t:x:")) != EOF) {
	switch(c) {  
	case 't':
	    config_template_dir = optarg;
	    break;
	case 'x':
	    xrl_targets_dir = optarg;
	    break;
	case '?':
	default:
	    usage(argv[0]);
	}
    }

    // Read the router config template files
    TemplateTree *tt = NULL;
    try {
	tt = new TemplateTree(default_xorp_root_dir,
			      true /* verbose */);
    } catch (const InitError& e) {
	fprintf(stderr, "test_templates: template tree init error: %s\n",
		e.why().c_str());
	fprintf(stderr, "test_templates: TEST FAILED\n");
	exit(1);
    } catch (...) {
	xorp_unexpected_handler();
	fprintf(stderr, "test_templates: unexpected error\n");
	fprintf(stderr, "test_templates: TEST FAILED\n");
	exit(1);
    }

    string errmsg;
    if (tt->load_template_tree(config_template_dir, errmsg) == false) {
	fprintf(stderr, "%s", errmsg.c_str());
	fprintf(stderr, "test_templates: TEST FAILED\n");
	exit(1);
    }

    XLOG_INFO("%s", tt->tree_str().c_str());

    delete tt;

    return (0);
}
