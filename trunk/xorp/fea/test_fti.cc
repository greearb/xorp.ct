// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the Xorp LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the Xorp LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/fea/test_fti.cc,v 1.1.1.1 2002/12/11 23:56:02 hodson Exp $"

#include <list>

#include "fea_module.h"
#include "config.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "fti.hh"

bool debug = false;

/**
 * Pretty print a forwarding table entry.
 */
void
pp_fte(Fte4& fte)
{
 	printf("dst = %-16s", fte.net().str().c_str());
 	printf("gateway = %-16s ", fte.gateway().str().c_str());
	printf("if = %s", fte.vifname().c_str());
	printf("\n");
}

void
lookup_route(Fti *fti, const char *addr)
{
    Fte4 fte;
    struct in_addr in;

    printf("Lookup route %s\n", addr);

    in.s_addr = inet_addr(addr);
    if (fti->lookup_route4(in.s_addr, fte))
	pp_fte(fte);
    else
	printf("lookup failed\n");
}

int
print_routing_table(Fti *fti)
{
    if (!fti->start_transaction()) {
	XLOG_ERROR("failed to start a transaction");
	return -1;
    }

    if (!fti->start_reading4()) {
	XLOG_ERROR("failed to start reading");
	return -1;
    }

    Fte4 fte;

    printf("Routing table *********\n");
    while (fti->read_entry4(fte)) {
	pp_fte(fte);
    }
    printf("Routing table *********\n");


    if (!fti->abort_transaction()) {
	XLOG_ERROR("failed abort transaction");
	return -1;
    }

    return 0;
}

int
add(Fti* fti, char *network, char *gateway, char *ifname)
{
    IPv4Net n;
    try {
	n = IPv4Net(network);
    } catch (const InvalidString& ) {
	XLOG_ERROR("Bad network: %s, expected <xx.xx.xx.xx>/<nn>", network);
	return -1;
    }

    IPv4 g;
    try {
	g = IPv4(gateway);
    } catch (const InvalidString& ) {
	XLOG_ERROR("Bad gateway: %s, expected xx.xx.xx.xx", gateway);
	return -1;
    }


    if (!fti->start_transaction()) {
	XLOG_ERROR("failed to start a transaction");
	return -1;
    }

    Fte4 fte(n, g, ifname);

    if (false == fti->add_entry4(fte)) {
	XLOG_ERROR("add_entry failed");
	return -1;
    }
    if (!fti->commit_transaction()) {
	XLOG_ERROR("commit transaction failed");
	return -1;
    }

    return 0;
}

int
del(Fti* fti, char *network)
{
    if (0 == fti)
	return -1;

    IPv4Net n;
    try {
	n = IPv4Net(network);
    } catch (const InvalidString&) {
	XLOG_ERROR("Invalid network: %s\n", network);
	return -1;
    }

    if (!fti->start_transaction()) {
	XLOG_ERROR("failed to start a transaction");
	return -1;
    }

    Fte4 fte(n);

    if (false == fti->delete_entry4(fte)) {
	XLOG_ERROR("del_entry failed");
    }

    if (!fti->commit_transaction()) {
	XLOG_ERROR("commit transaction failed");
	return -1;
    }

    return 0;
}

void
save(Fti* fti, list<Fte4>& rt)
{
    if (!fti->start_reading4()) {
	XLOG_ERROR("failed to start reading");
	return;
    }

    Fte4 fte;

    while (fti->read_entry4(fte)) {
	rt.push_back(fte);
    }
}

void
restore(Fti* fti, list<Fte4>& rt)
{
    /* XXX
    ** This could loop forever
    */
    while (!fti->start_transaction()) {
	XLOG_ERROR("failed to start a transaction");

	/*
	** Abort any current transaction.
	*/
	if (!fti->abort_transaction()) {
	    XLOG_ERROR("failed to abort transaction");
	    return;
	}
    }

    /*
    ** Empty the routing table and then add back the saved routes.
    */
    if (!fti->delete_all_entries4()) {
	XLOG_ERROR("failed to delete all entries");
	/* Carry on anyway */
    }

    for (list<Fte4>::iterator e = rt.begin(); e != rt.end(); e++) {
	if (false == fti->add_entry4(*e)) {
	    XLOG_ERROR("add_entry failed");
	}
    }

    if (!fti->commit_transaction()) {
	XLOG_ERROR("commit transaction failed");
	return;
    }
}

/**
 * test1
 * Save and restore the routing table. The restore is implemented by
 * deleting all entries in the table and then putting them back so this
 * is a good test for additions and deletions.
 */
void
test1(Fti* fti)
{
    list<Fte4> rt;

    save(fti, rt);
    restore(fti, rt);
}

/**
 * test2
 * Test the delete all method.
 */
void
test2(Fti* fti)
{
    if (0 == fti)
	return;

    /* XXX
    ** This could loop forever
    */
    while (!fti->start_transaction()) {
	XLOG_ERROR("failed to start a transaction");

	/*
	** Abort any current transaction.
	*/
	if (!fti->abort_transaction()) {
	    XLOG_ERROR("failed to abort transaction");
	    return;
	}
    }

    list<Fte4> rt;
    save(fti, rt);

    /*
    ** Empty the routing table and then add back the saved routes.
    */
    if (!fti->delete_all_entries4()) {
	XLOG_ERROR("failed to delete all entries");
	/* Carry on anyway */
    }

    if (!fti->commit_transaction()) {
	XLOG_ERROR("commit transaction failed");
    }

    restore(fti, rt);
}

void
usage(char *name)
{
    fprintf (stderr,
"usage: %s [\n"
"(routing table)\t -r\n"
"(add)\t\t -a <network> <gateway> <interface>\n"
"(delete)\t -d <network> \n"
"(zap)\t\t -z\n"
"(lookup)\t -l <dst>\n"
"(test) \t\t -T <test number>\n"
"(debug)\t\t -D\n"
"]\n",
	     name);

    exit(-1);
}

int
main(int argc, char *argv[])
{
    int c;
    bool prt = false;
    char *lookup = 0;

    /*
    ** Initialize and start xlog
    */
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    /* BSD specific contortions to create an Fti */
    EventLoop e;
    RoutingSocket rs(e);
    Iflist iflist(rs);
    FreeBSDFti fti(iflist);

    while ((c = getopt (argc, argv, "ra:d:zl:T:D")) != EOF) {
	switch (c) {
	case 'r':
	    prt = true;
	    break;
	case 'a':
	    /*
	    ** Check we have enough arguments.
	    */
	    if (optind + 2 > argc)
		usage(argv[0]);
	    add(&fti, optarg, argv[optind], argv[optind + 1]);
	    optind += 2;
	    break;
	case 'd':
	    del(&fti, optarg);
	    optind += 1;
	    break;
	case 'z':	/* zap */
	    {
	    list<Fte4> rt;
	    restore(&fti, rt);
	    break;
	    }
	case 'l':
	    lookup = optarg;
	    break;
	case 'T':
	    switch (atoi(optarg)) {
	    case 1:
		test1(&fti);
		break;
	    case 2:
		test2(&fti);
		break;
	    default:
		XLOG_ERROR("No test %s", optarg);
	    }
	    break;
	case 'D':
	    debug = true;
	    break;
	case '?':
	    usage(argv[0]);
        }
    }

    if (prt)
	print_routing_table(&fti);

    /*
    ** Lookup some routes
    */
    if (lookup)
	lookup_route(&fti, lookup);

#if	0
    lookup_route(fti, "192.150.187.11");
    lookup_route(fti, "128.16.64.16");
    lookup_route(fti, "10.0.3.2");
#endif

    /*
    ** Gracefully stop and exit xlog
    */
    xlog_stop();
    xlog_exit();

    return 0;
}
