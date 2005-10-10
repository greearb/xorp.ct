// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP$"

// Get LSAs from OSPF and print them.

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <set>
#include <list>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "ospf/ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"
#include "libxipc/xrl_std_router.hh"

#include "ospf/ospf.hh"

#include "xrl/interfaces/ospfv2_xif.hh"

class Fetch {
 public:
    Fetch(XrlStdRouter& xrl_router, OspfTypes::Version version, IPv4 area)
	: _xrl_router(xrl_router), _lsa_decoder(version),
	  _done(false), _area(area), _index(0)
    {
	initialise_lsa_decoder(version, _lsa_decoder);
    }

    void start() {
	XrlOspfv2V0p1Client ospfv2(&_xrl_router);
	ospfv2.send_get_lsa("ospfv2", _area, _index,
			    callback(this, &Fetch::response));
    }

    bool busy() {
	return !_done;
    }

    list<Lsa::LsaRef>& get_lsas() {
	return _lsas;
    }

 private:
    void response(const XrlError& error,
		  const bool *valid,
		  const bool *toohigh,
		  const vector<uint8_t>* lsa) {
	if (XrlError::OKAY() != error) {
	    XLOG_WARNING("Attempt to get lsa failed");
	    _done = true;
	    return;
	}
	if (*valid) {
	    size_t len = lsa->size();
	    Lsa::LsaRef lsar = _lsa_decoder.
		decode(const_cast<uint8_t *>(&(*lsa)[0]), len);
	    _lsas.push_back(lsar);
	}
	if (*toohigh) {
	    _done = true;
	    return;
	}
	_index++;
	start();
    }
 private:
    XrlStdRouter &_xrl_router;
    LsaDecoder _lsa_decoder;
    bool _done;
    const IPv4 _area;
    uint32_t _index;

    list<Lsa::LsaRef> _lsas;
};

int
usage(const char *myname)
{
    fprintf(stderr, "usage: %s -a area\n",
	    myname);
    return -1;
}
int 
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    string area;

    int c;
    while ((c = getopt(argc, argv, "a:")) != -1) {
	switch (c) {
	case 'a':
	    area = optarg;
	    break;
	default:
	    return usage(argv[0]);
	}
    }

    if (area.empty())
	return usage(argv[0]);

    try {
	//
	// Init stuff
	//
	EventLoop eventloop;
	XrlStdRouter xrl_router(eventloop, "print_lsas");

	debug_msg("Waiting for router");
	xrl_router.finalize();
	wait_until_xrl_router_is_ready(eventloop, xrl_router);
	debug_msg("\n");

	Fetch fetch(xrl_router, OspfTypes::V2, IPv4(area.c_str()));

	fetch.start();
	while(fetch.busy())
	    eventloop.run();

	const list<Lsa::LsaRef>& lsas = fetch.get_lsas();
	list<Lsa::LsaRef>::const_iterator i;
	for (i = lsas.begin(); i != lsas.end(); i++)
	    printf("%s\n", (*i)->str().c_str());
	       
    } catch (...) {
	xorp_catch_standard_exceptions();
    }

    // 
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}

