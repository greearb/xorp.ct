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

#ident "$XORP: xorp/fea/test_ifconfig_rtsock.cc,v 1.6 2002/12/09 18:29:00 hodson Exp $"

#include <sys/types.h>
#include <sys/uio.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#define DEBUG_LOGGING

#include <iostream>
#include <netdb.h>
#include <sysexits.h>

#include "config.h"
#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "ifconfig_rtsock.hh"

/* ------------------------------------------------------------------------- */
/* Verbose output control */

static bool s_verbose = false;

inline bool verbose()           { return s_verbose; }
inline void set_verbose(bool v) { s_verbose = v; }

#define verbose_log(x...)                                                     \
do {                                                                          \
    if (verbose()) {                                                          \
        printf("From %s:%d: ", __FILE__, __LINE__);                           \
        printf(x);                                                            \
    }                                                                         \
} while(0)

/* ------------------------------------------------------------------------- */

class VerboseIfConfigUpdateReporter : public IfConfigUpdateReporterBase {
public:
    typedef IfConfigUpdateReporterBase::Update Update;

    static const char* update(Update u) {
	switch (u) {
	case CREATED: return "created";
	case DELETED: return "deleted";
	case CHANGED: return "changed";
	}
	return "bork";
    }

    void interface_update(const string& ifname,
			  const Update& u) {
	verbose_log("%s %s", update(u), ifname.c_str());
    }

    void vif_update(const string& ifname,
		    const string& vifname,
		    const Update& u) {
	verbose_log("%s %s %s", update(u), ifname.c_str(), vifname.c_str());
    }

    void vifaddr4_update(const string& ifname,
			 const string& vifname,
			 const IPv4&   addr,
			 const Update& u) {
	verbose_log("%s %s %s %s",
		    update(u), ifname.c_str(), vifname.c_str(),
		    addr.str().c_str());
    }


    void vifaddr6_update(const string& ifname,
				 const string& vifname,
				 const IPv6&   addr,
				 const Update& u) {
	verbose_log("%s %s %s %s",
		    update(u), ifname.c_str(), vifname.c_str(),
		    addr.str().c_str());
    }
};

static void
test_main()
{
    EventLoop e;
    RoutingSocket rs(e);
    VerboseIfConfigUpdateReporter ur;
    SimpleIfConfigErrorReporter   er;

    IfConfigRoutingSocket ifc(rs, ur, er);

    IfTree original;
    original = ifc.pull_config(original);

    ifc.push_config(original);
}

/* ------------------------------------------------------------------------- */
static void
usage()
{
    fprintf(stderr, "usage: test_ifconfig_rtsock\n");
    fprintf(stderr,
	    "reads interface configuration information from routing socket "
	    "and attempts to set it again."
	);
    exit(EX_USAGE);
}

/* ------------------------------------------------------------------------- */

int
main(int argc, char* const* argv)
{
    //
    // Root test, can't run if not root.
    //
    if (geteuid() != 0) {
	fprintf(stderr, "%s\n*\n", string(79, '*').c_str());
	fprintf(stderr, "* This program needs root privelages.\n");
	fprintf(stderr, "* Returning success even though not run.\n");
	fprintf(stderr, "*\n%s\n", string(79, '*').c_str());
	// Return 0 because if part of 'make check' we don't want this
	// to count as a failure.
	return EXIT_SUCCESS;
    }

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);         // Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    if (argc > 1) {
	usage();
    }

    try {
	test_main();
    } catch (...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return EXIT_SUCCESS;
}
