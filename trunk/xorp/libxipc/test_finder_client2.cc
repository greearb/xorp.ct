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

#ident "$XORP: xorp/libxipc/test_finder_client2.cc,v 1.4 2002/12/19 01:29:09 hodson Exp $"

#include <string>

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "finder_module.h"
#include "config.h"
#include "libxorp/xlog.h"
#include "finder_client.hh"
#include "finder_server.hh"

// Test description:
//
// Upto three clients connect to a server instance.  The first client
// registers the association of name and values below.  The second
// client queries all the values and is then destroyed, it's then
// recreated and repeats the queries, the third client the queries all
// the values.

// ----------------------------------------------------------------------------
// Global state

static struct association
{
    const char* name;
    const char* value;
} entries[] = {
    { "Ronald", "Beef Burger" },
    { "Heaven", "Hell" },
    { "Dennis the Menace", "Gnasher" },
};

static const size_t num_entries = sizeof(entries) / sizeof(entries[0]);

// ----------------------------------------------------------------------------
// Printing

void
announce_test(const char* name)
{
    printf("%s...", name);
    fflush(stdout);
}

void
announce_result(bool success)
{
    printf("%s\n", (success) ? "okay" : "fail");
    fflush(stdout);
}

// ----------------------------------------------------------------------------
// Shared timeout hook for client-server interactions

// ----------------------------------------------------------------------------
// Connecting to server

void
add_hook(FinderClient::Error e,
	 const char* /* name */,
	 const char* /* value */,
	 void *cookie) {
    int* n = reinterpret_cast<int*>(cookie);
    if (e == FinderClient::FC_OKAY) {
	*n = *n + 1;
    } else {
	*n = (int)num_entries + 1;
    }
}

static bool
connect_okay(EventLoop* e,
	     FinderClient* pfc) {
    announce_test("Connecting to server");

    bool timed_out = false;
    XorpTimer timeout = e->set_flag_after_ms(2000, &timed_out);

    while (pfc->connected() == false && timed_out == false) {
	e->run();
    }

    if (timed_out == false) {
	announce_result(true);
	return true;
    }
    announce_result(false);
    return false;
}

// ----------------------------------------------------------------------------
// Server registration

static bool
register_okay(EventLoop* e, FinderClient* fc)
{
    announce_test("Client registering values");

    int done = 0;
    for (int i = 0; i < (int)num_entries; i++) {
	fc->add(entries[i].name, entries[i].value, add_hook,
	       reinterpret_cast<int*>(&done));
    }

    bool timed_out = false;
    XorpTimer timeout = e->set_flag_after_ms(2000, &timed_out);

    while (done < (int)num_entries && !timed_out) {
	e->run();
    }

    if (done != (int)num_entries) {
	announce_result(false);
	return false;
    }

    announce_result(true);
    return true;
}

// ----------------------------------------------------------------------------
// Resolve test

void
resolve_callback(FinderClient::Error	e,
		 const char*		name,
		 const char*		value,
		 void*			cookie) {
    size_t* n = reinterpret_cast<size_t*>(cookie);

    printf("%d %d \"%s\" \"%s\"", (int) e, *n, name, value);
    if (*n < num_entries) {
	printf(" \"%s\" \"%s\"\n", entries[*n].name, entries[*n].value);
    }

    if (e == FinderClient::FC_OKAY &&
	*n < num_entries &&
	strcmp(name, entries[*n].name) == 0 &&
	strcmp(value, entries[*n].value) == 0) {
	*n = *n + 1;
    } else {
	*n = 10 * num_entries + 1;
    }
}

static bool
client_resolves_all_values(EventLoop *e, FinderClient *fc)
{
    announce_test("Another client resolving registered values");

    size_t num_resolved = 0;
    for (size_t i = 0; i < num_entries; i++) {
	fc->lookup(entries[i].name, resolve_callback,
		   reinterpret_cast<void*>(&num_resolved));
    }

    bool timed_out = false;
    XorpTimer timeout = e->set_flag_after_ms(5000, &timed_out);

    while (num_resolved < num_entries && timed_out == false) {
	e->run();
    }

    printf("%d\n", num_resolved);

    if (num_resolved != num_entries) {
	announce_result(false);
	return false;
    }

    announce_result(true);
    return true;
}

int main(int /* argc */, char *argv[])
{
    EventLoop e;
    FinderServer fs(e);
    FinderClient fc1(e), fc2(e);
    FinderClient *pfc = new FinderClient(e);

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    if (connect_okay(&e, &fc1) == false ||
	connect_okay(&e, &fc2) == false ||
	connect_okay(&e, pfc) == false ||
	register_okay(&e, &fc1) == false ||
	client_resolves_all_values(&e, pfc) == false) {
	delete pfc;
	return -1;
    }

    delete pfc;

    pfc = new FinderClient(e);
    if (connect_okay(&e, pfc) == false ||
	client_resolves_all_values(&e, pfc) == false) {
	delete pfc;
	return -1;
    }
    delete pfc;

    if (client_resolves_all_values(&e, &fc2) == false) {
	return -1;
    }

    if (client_resolves_all_values(&e, &fc1) == false) {
	return -1;
    }

    bool pause_for_hello;
    XorpTimer t = e.set_flag_after_ms(4000, &pause_for_hello);
    while (pause_for_hello == false)
	e.run();

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
