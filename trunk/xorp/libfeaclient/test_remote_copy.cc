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

#ident "$XORP: xorp/libfeaclient/test_remote_copy.cc,v 1.1 2003/09/10 19:21:33 hodson Exp $"

#ident "$XORP: xorp/libfeaclient/test_remote_copy.cc,v 1.1 2003/09/10 19:21:33 hodson Exp $"

#include "libfeaclient_module.h"

#include "libxorp/xlog.h"

#include "libxorp/c_format.hh"
#include "libxorp/eventloop.hh"

#include "libxipc/finder_server.hh"
#include "libxipc/xrl_std_router.hh"

#include "xrl/targets/test_fea_ifmgr_mirror_base.hh"

#include "ifmgr_atoms.hh"
#include "ifmgr_cmds.hh"
#include "ifmgr_cmd_queue.hh"
#include "ifmgr_xrl_replicator.hh"
#include "ifmgr_xrl_mirror.hh"

///////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static const char *program_name         = "test_remote_copy";
static const char *program_description  = "Test local copying of IfMgr "
					  "configuration state";
static const char *program_version_id   = "0.1";
static const char *program_date         = "August, 2003";
static const char *program_copyright    = "See file LICENSE.XORP";
static const char *program_return_value = "0 on success, "
					  "1 if test error, "
					  "2 if internal error";

///////////////////////////////////////////////////////////////////////////////
//
// Verbosity level control
//

static bool s_verbose = false;
bool verbose()                  { return s_verbose; }
void set_verbose(bool v)        { s_verbose = v; }

#define verbose_log(x...) _verbose_log(__FILE__,__LINE__, x)

#define _verbose_log(file, line, x...)					\
do {									\
    if (verbose()) {							\
	printf("From %s:%d: ", file, line);				\
	printf(x);							\
    }									\
} while(0)


// ----------------------------------------------------------------------------
//

class XrlTestFeaIfmgrMirrorTarget : public XrlTestFeaIfmgrMirrorTargetBase
{
public:
    XrlTestFeaIfmgrMirrorTarget(XrlRouter& rtr,
				IfMgrXrlReplicationManager& rep)
	:     XrlTestFeaIfmgrMirrorTargetBase(&rtr), _rep(rep)
    {
    }

    XrlCmdError
    ifmgr_replicator_0_1_register_ifmgr_mirror(const string& n)
    {
	if (_rep.add_mirror(n) == true)
	    return XrlCmdError::OKAY();
	return XrlCmdError::COMMAND_FAILED();
    }

    XrlCmdError
    ifmgr_replicator_0_1_unregister_ifmgr_mirror(const string& n)
    {
	if (_rep.remove_mirror(n) == true)
	    return XrlCmdError::OKAY();
	return XrlCmdError::COMMAND_FAILED();
    }

protected:
    IfMgrXrlReplicationManager& _rep;
};


// ----------------------------------------------------------------------------
// The test

static int
populate_iftree(IfMgrIfTree& t)
{
    // Create an interface using command objects
    if (IfMgrIfAdd("if0").execute(t) == false) {
	verbose_log("Failed to create interface\n");
	return 1;
    }
    if (IfMgrIfSetEnabled("if0", true).execute(t) == false) {
	verbose_log("Failed to enable interface\n");
	return 1;
    }
    if (IfMgrIfSetMtu("if0", 9000).execute(t) == false) {
	verbose_log("Failed to set mtu\n");
	return 1;
    }
    if (IfMgrIfSetMac("if0", Mac("00:2e:dd:01:02:03")).execute(t) == false) {
	verbose_log("Failed to set mac address\n");
	return 1;
    }

    // Create a vif on a non-existant interface and check it fails
    if (IfMgrVifAdd("iiii0", "bad0").execute(t) == true) {
	verbose_log("Created vif on non-existent interface\n");
	return 1;
    }

    // Create and configure a vif
    if (IfMgrVifAdd("if0", "vif0").execute(t) == false) {
	verbose_log("Failed to create vif on interface\n");
	return 1;
    }
    if (IfMgrVifSetEnabled("if0", "vif0", true).execute(t) == false) {
	verbose_log("Failed to enable vif\n");
	return 1;
    }
    if (IfMgrVifSetMulticastCapable("if0", "vif0", true).execute (t)== false) {
	verbose_log("Failed to set multicast capable\n");
	return 1;
    }
    if (IfMgrVifSetBroadcastCapable("if0", "vif0", true).execute(t) == false) {
	verbose_log("Failed to set broadcast capable\n");
	return 1;
    }
    if (IfMgrVifSetP2PCapable("if0", "vif0", true).execute(t) == false) {
	verbose_log("Failed to set point-to-point capable\n");
	return 1;
    }
    if (IfMgrVifSetLoopbackCapable("if0", "vif0", true).execute(t) == false) {
	verbose_log("Failed to set loopback capable\n");
	return 1;
    }
    if (IfMgrVifSetPifIndex("if0", "vif0", 74).execute(t) == false) {
	verbose_log("Failed to set pif index\n");
	return 1;
    }

    // Create an address on a non-existant interface and check it fails
    if (IfMgrIPv4Add("if0", "bad0", "252.17.17.17").execute(t) == true) {
	verbose_log("Created address on non-existent interface\n");
	return 1;
    }

    // Create and configure an IPv4 address
    IPv4 v4a("10.0.0.1");
    if (IfMgrIPv4Add("if0", "vif0", v4a).execute(t) == false) {
	verbose_log("Failed to create an address\n");
	return 1;
    }
    if (IfMgrIPv4SetPrefix("if0", "vif0", v4a, 8).execute(t) == false) {
	verbose_log("Failed to set address prefix");
	return 1;
    }
    if (IfMgrIPv4SetEnabled("if0", "vif0", v4a, true).execute(t)
	== false) {
	verbose_log("Failed to set address enabled");
	return 1;
    }
    if (IfMgrIPv4SetMulticastCapable("if0", "vif0", v4a, true).execute(t)
	== false) {
	verbose_log("Failed to set multicast capable");
	return 1;
    }
    if (IfMgrIPv4SetLoopback("if0", "vif0", v4a, false).execute(t) == false) {
	verbose_log("Failed to set loopback");
	return 1;
    }
    if (IfMgrIPv4SetBroadcast("if0", "vif0", v4a, "10.255.255.255").execute(t)
	== false) {
	verbose_log("Failed to set broadcast");
	return 1;
    }
    if (IfMgrIPv4SetEndpoint("if0", "vif0", v4a, "192.168.0.1").execute(t)
	== false) {
	verbose_log("Failed to set endpoint");
	return 1;
    }

    // Create an address on a non-existant interface and check it fails
    IPv6 v6a("fe80::909:b3ff:fe10:b467");
    if (IfMgrIPv6Add("if0", "bad0", v6a).execute(t) == true) {
	verbose_log("Created address on non-existent interface\n");
	return 1;
    }

    // Create and configure an IPv6 address
    if (IfMgrIPv6Add("if0", "vif0", v6a).execute(t) == false) {
	verbose_log("Failed to create an address\n");
	return 1;
    }
    if (IfMgrIPv6SetPrefix("if0", "vif0", v6a, 48).execute(t) == false) {
	verbose_log("Failed to set address prefix");
	return 1;
    }
    if (IfMgrIPv6SetEnabled("if0", "vif0", v6a, true).execute(t)
	== false) {
	verbose_log("Failed to set address enabled");
	return 1;
    }
    if (IfMgrIPv6SetMulticastCapable("if0", "vif0", v6a, true).execute(t)
	== false) {
	verbose_log("Failed to set multicast capable");
	return 1;
    }
    if (IfMgrIPv6SetLoopback("if0", "vif0", v6a, false).execute(t) == false) {
	verbose_log("Failed to set loopback");
	return 1;
    }
    IPv6 oaddr("fe80::220:edff:fe5c:d0c7");
    if (IfMgrIPv6SetEndpoint("if0", "vif0", v6a, oaddr).execute(t)
	== false) {
	verbose_log("Failed to set endpoint");
	return 1;
    }

    return 0;
}

static int
test_main()
{
    //
    // Instantiate a Finder
    //
    EventLoop e;
    ref_ptr<FinderServer> fs = 0;
    for (uint16_t port = 32000; port < 32500; port++) {
	try {
	    fs = new FinderServer(e, port);
	    goto ___got_finder;
	} catch (const InvalidPort&) {
	    continue;
	}
    }
    verbose_log("Could not instantiate FinderServer");
    return -1;

 ___got_finder:

    bool expired = false;
    XorpTimer t = e.set_flag_after_ms(3000, &expired);

    //
    // Build distribution center
    //
    XrlStdRouter		mrtr(e, "ifmgr", fs->addr(), fs->port());
    IfMgrXrlReplicationManager	mgr(mrtr);
    XrlTestFeaIfmgrMirrorTarget	xtf(mrtr, mgr);

    while (mrtr.ready() == false) {
	e.run();
	if (expired) {
	    verbose_log("IfMgr XrlRouter did not become ready.");
	    return -1;
	}
    }
    verbose_log("Connected ifmgr to finder\n");

    //
    // Build configuration (not very efficiently)
    //
    IfMgrIfTree ift;
    int r = populate_iftree(ift);
    if (r != 0)
	return r;

    IfMgrIfTreeToCommands config_commands(ift);
    config_commands.convert(mgr);

    // Create mirror
    IfMgrXrlMirror m0(e, fs->addr(), fs->port(), mrtr.class_name().c_str());

    expired = false;
    t = e.set_flag_after_ms(3000, &expired);
    while (expired == false && m0.status() !=  IfMgrXrlMirror::READY) {
	e.run();
	if (expired) {
	    verbose_log("Did not get complete tree.\n");
	    return -1;
	}
    }

    if (m0.iftree() != mgr.iftree()) {
	verbose_log("Local and remote trees do not match up.\n");
	return -1;
    }

    return 0;
}


/**
 * Print program info to output stream.
 *
 * @param stream the output stream the print the program info to.
 */
static void
print_program_info(FILE *stream)
{
    fprintf(stream, "Name:          %s\n", program_name);
    fprintf(stream, "Description:   %s\n", program_description);
    fprintf(stream, "Version:       %s\n", program_version_id);
    fprintf(stream, "Date:          %s\n", program_date);
    fprintf(stream, "Copyright:     %s\n", program_copyright);
    fprintf(stream, "Return:        %s\n", program_return_value);
}

/**
 * Print program usage information to the stderr.
 *
 * @param progname the name of the program.
 */
static void
usage(const char* progname)
{
    print_program_info(stderr);
    fprintf(stderr, "usage: %s [-v] [-h]\n", progname);
    fprintf(stderr, "       -h          : usage (this message)\n");
    fprintf(stderr, "       -v          : verbose output\n");
}

int
main(int argc, char* const argv[])
{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);         // Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    int ch;
    while ((ch = getopt(argc, argv, "hv")) != -1) {
        switch (ch) {
        case 'v':
            set_verbose(true);
            break;
        case 'h':
        case '?':
        default:
            usage(argv[0]);
            xlog_stop();
            xlog_exit();
            if (ch == 'h')
                return (0);
            else
                return (1);
        }
    }
    argc -= optind;
    argv += optind;

    int rval = 0;
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    try {
	rval = test_main();
    } catch (...) {
        // Internal error
        xorp_print_standard_exceptions();
        rval = 2;
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return rval;
}
