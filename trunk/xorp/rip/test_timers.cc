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

#ident "$XORP: xorp/rip/test_timers.cc,v 1.2 2003/04/10 02:41:50 pavlin Exp $"

#include "rip_module.h"

#include "libxorp/xlog.h"

#include "libxorp/c_format.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"

#include "port.hh"
#include "peer.hh"
#include "route_db.hh"
#include "system.hh"

///////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static const char *program_name         = "test_timers";
static const char *program_description  = "Test RIP timers and route scan";
static const char *program_version_id   = "0.1";
static const char *program_date         = "March, 2003";
static const char *program_copyright    = "See file LICENSE.XORP";
static const char *program_return_value = "0 on success, 1 if test error, 2 if internal error";

static const uint32_t N_TEST_ROUTES = 32000;

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

static bool
print_twirl(void)
{
    static const char t[] = { '\\', '|', '/', '-' };
    static const size_t nt = sizeof(t) / sizeof(t[0]);
    static size_t n = 0;
    static char erase = '\0';

    printf("%c%c", erase, t[n % nt]); fflush(stdout);
    n++;
    erase = '\b';
    return true;
}


// ----------------------------------------------------------------------------
// Spoof Port that supports just a single Peer
//

class SpoofPort4 : public Port<IPv4> {
public:
    SpoofPort4(PortManagerBase<IPv4>& pm, IPv4 addr) : Port<IPv4>(pm)
    {
	_peers.push_back(new Peer<IPv4>(*this, addr));
	verbose_log("Constructing SpoofPort4 instance\n");
    }
    ~SpoofPort4()
    {
	verbose_log("Destructing SpoofPort4 instance\n");
    }
};

// ----------------------------------------------------------------------------
// Spoof Port Manager instance support a single Spoof Port which in turn
// contains a single Peer.
//

class SpoofPortManager4 : public PortManagerBase<IPv4> {
public:
    SpoofPortManager4(System<IPv4>& s) : PortManagerBase<IPv4>(s) {
	_ports.push_back(new SpoofPort4(*this, IPv4("10.0.0.1")));
    }

    Port<IPv4>* the_port()
    {
	XLOG_ASSERT(_ports.size() == 1);
	return _ports.front();
    }

    Peer<IPv4>* the_peer()
    {
	XLOG_ASSERT(_ports.size() == 1);
	XLOG_ASSERT(_ports.front()->peers().size() == 1);
	return _ports.front()->peers().front();
    }
};


//----------------------------------------------------------------------------
// The test

static int
test_main()
{
    EventLoop		e;
    System<IPv4>	rip_system(e);
    SpoofPortManager4	spm(rip_system);

    RouteDB<IPv4>& rdb = rip_system.route_db();
    Peer<IPv4>* peer = spm.the_peer();

    // Fix up time constants for the impatient.
    spm.the_port()->constants().set_expiry_secs(3);
    spm.the_port()->constants().set_deletion_secs(2);
    
    const uint32_t n_routes = N_TEST_ROUTES;
    for (uint32_t i = 0; i < n_routes; i++) {
	IPv4 addr(htonl((128 << 24) + (i << 8) + (random() % 255)));
	size_t mlen  = size_t(24 + random() % 9);
	IPv4Net net(addr, mlen);
	verbose_log("Adding route to %s\n", net.str().c_str());
	if (rdb.update_route(net, IPv4("10.0.10.1"), 1, 0, peer) == false) {
	    verbose_log("Route caused no update\n");
	    return 1;
	}
    }

    if (peer->route_count() != n_routes) {
	verbose_log("Routes lost (count %u != %u)\n",
		    (uint32_t)peer->route_count(), n_routes);
	return 1;
    }

    {
	// Quick route dump test
	TimeVal t1, t2;
	e.current_time(t1);
	vector<RouteDB<IPv4>::ConstDBRouteEntry> l;
	rdb.dump_routes(l);
	e.current_time(t2);
	t2 -= t1;
	fprintf(stderr, "route db route dump took %d.%06d seconds\n",
		t2.sec(), t2.usec());
    }
    {
	// Quick route dump test
	TimeVal t1, t2;
	e.current_time(t1);
	vector<const RouteEntryOrigin<IPv4>::Route*> l;
	peer->dump_routes(l);
	e.current_time(t2);
	t2 -= t1;
	fprintf(stderr, "peer route dump took %d.%06d seconds\n",
		t2.sec(), t2.usec());
    }
    {
	// Quick route dump test
	TimeVal t1, t2;
	e.current_time(t1);
	vector<RouteDB<IPv4>::ConstDBRouteEntry> l;
	rdb.dump_routes(l);
	e.current_time(t2);
	t2 -= t1;
	fprintf(stderr, "route db route dump took %d.%06d seconds\n",
		t2.sec(), t2.usec());
    }
    
    const PortTimerConstants& ptc = peer->port().constants();
    uint32_t timeout_secs = ptc.expiry_secs() + ptc.deletion_secs() + 5;

    XorpTimer twirl;
    if (verbose())
	twirl = e.new_periodic(250, callback(print_twirl));

    bool timeout = false;
    XorpTimer t = e.set_flag_after_ms(1000 * timeout_secs, &timeout);

    while (timeout == false) {
	size_t route_count = peer->route_count();
	verbose_log("Route count = %u\n", (uint32_t)route_count);
	if (route_count == 0)
	    break;
	e.run();
    }

    if (peer->route_count()) {
	verbose_log("Routes did not clean up\n");
	return 1;
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
