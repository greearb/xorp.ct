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

#ident "$XORP: xorp/libxipc/test_finder_ipc2.cc,v 1.4 2003/03/16 08:20:30 pavlin Exp $"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>

#include "finder_module.h"
#include "config.h"
#include "libxorp/xlog.h"
#include "libxorp/eventloop.hh"

#include "finder_ipc.hh"

// ----------------------------------------------------------------------------
// Global variables and hooks

static bool can_read = false;

static void
select_hook(int fd, SelectorMask m, FinderTCPIPCService* ipc)
{
    assert(ipc->descriptor() == fd);
    assert(m == SEL_RD);
    can_read = true;
}

static void
connect_hook(FinderTCPIPCService* created, void* thunk)
{
    FinderIPCService** pps = reinterpret_cast<FinderIPCService**>(thunk);
    *pps = created;
}

static bool
periodic_dot_printing()
{
    printf(".");
    fflush(stdout);
    return true;
}

// ----------------------------------------------------------------------------
// Tests

typedef bool (*test_iter_f)(EventLoop *, FinderIPCService* /* reader */,
			    FinderIPCService* /* writer */);

static bool
test_hello_iter(EventLoop *e,
		FinderIPCService* reader,
		FinderIPCService* writer) {
    bool timed_out = false;
    XorpTimer timeout_timer = e->set_flag_after_ms(500, &timed_out);

    FinderMessage m, n;

    writer->prepare_message(m, HELLO);
    writer->write_message(m);

    for (can_read = false; (can_read | timed_out) == false; e->run());
    if (can_read)
        reader->read_message(n);

    return (m == n);
}


static bool
test_bye_iter(EventLoop *e,
	      FinderIPCService* reader, FinderIPCService *writer) {
    bool timed_out = false;
    XorpTimer timeout_timer = e->set_flag_after_ms(500, &timed_out);

    FinderMessage m, n;

    writer->prepare_message(m, BYE);
    writer->write_message(m);

    for (can_read = false; (can_read|timed_out) == false; e->run());
    if (can_read)
        reader->read_message(n);

    return (m == n);
}

static bool
test_register_iter(EventLoop*		e,
		   FinderIPCService*	reader,
		   FinderIPCService*	writer) {
    bool timed_out = false;
    XorpTimer timeout_timer = e->set_flag_after_ms(500, &timed_out);

    const char key[] = "porridge", value[] = "goldilocks";
    FinderMessage m, n;

    writer->prepare_message(m, REGISTER, key, value);
    writer->write_message(m);

    for (can_read = false; (can_read|timed_out) == false; e->run());
    if (can_read)
        reader->read_message(n);

    return (m == n);
}

static bool
test_register_iter2(EventLoop*		e,
		    FinderIPCService*	reader,
		    FinderIPCService*	writer) {
    bool timed_out = false;
    XorpTimer timeout_timer = e->set_flag_after_ms(500, &timed_out);

    const char *key[] = {
	"porridge",     "posh",         "kernighan",
	"davis",        "adderley",     "coltrane",
	"giro",         "armstrong",    "gagarin",
	"charlie chan"
    };
    const char *value[] = {
	"goldilocks",   "peterborough", "ritchie",
	"trumpet",      "alto sax",     "tenor sax",
	"helmet",       "moon",         "space",
	"benson fong"
    };
    const size_t key_count = sizeof(key) / sizeof(key[0]);
    vector<FinderMessage> m(key_count), n(key_count);

    int i,  nd;
    nd = random() % (sizeof(key) / sizeof(key[0]));

    for (i = 0; i < nd; i++) {
	writer->prepare_message(m[i], REGISTER, key[i], value[i]);
	writer->write_message(m[i]);
    }

    for (i = 0; i < nd; i++) {
	for (can_read = false; (can_read|timed_out) == false; e->run());
	if (can_read) {
	    reader->read_message(n[i]);
	    if (n[i] != m[i])
		i = nd + 1;
	}
    }

    return (i == nd);
}

static bool
test_notify_iter(EventLoop*		e,
		 FinderIPCService*	reader,
		 FinderIPCService*	writer) {
    bool timed_out = false;
    XorpTimer timeout_timer = e->set_flag_after_ms(500, &timed_out);

    const char key[] = "porridge", value[] = "goldilocks";
    FinderMessage m, n;

    writer->prepare_message(m, NOTIFY, key, value);
    writer->write_message(m);

    for (can_read = false; (can_read|timed_out) == false; e->run());
    if (can_read) {
        reader->read_message(n);
    }

    return (m == n);
}

static bool
test_locate_iter(EventLoop*		e,
		 FinderIPCService*	reader,
		 FinderIPCService*	writer) {
    bool timed_out = false;
    XorpTimer timeout_timer = e->set_flag_after_ms(500, &timed_out);

    const char key[] = "porridge";
    FinderMessage m, n;

    writer->prepare_message(m, LOCATE, key);
    writer->write_message(m);

    for (can_read = false; (can_read|timed_out) == false; e->run());
    if (can_read) {
        reader->read_message(n);
    }

    return (m == n);
}

static bool
run_test(EventLoop*		event_loop,
	 FinderIPCService*	server,
	 FinderIPCService*	client,
	 test_iter_f		iter_func,
	 const char* 		name) {

    printf("Testing %s...", name);

    XorpTimer dot_printer =
	event_loop->new_periodic(100, callback(periodic_dot_printing));

    bool failed = false;
    for (int i = 0; i < 20 && failed == false; i++) {
	failed = !iter_func(event_loop, server, client);
    }

    printf("%s\n", (failed) ? "fail" : "okay");
    return failed;
}

int
main(int /* argc */, char *argv[])
{
    EventLoop event_loop;
    FinderTCPIPCService *client, *server;

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    FinderTCPServerIPCFactory f(event_loop.selector_list(), connect_hook,
				reinterpret_cast<void*>(&server));

    client = FinderTCPClientIPCFactory::create();
    for (server = NULL; server == NULL; event_loop.run());

    event_loop.add_selector(server->descriptor(), SEL_RD,
			    callback(&select_hook, server));

    bool failed = false;
    failed |= run_test(&event_loop, server, client, test_hello_iter, "HELLO");
    failed |= run_test(&event_loop, server, client, test_register_iter,
		       "REGISTER");
    failed |= run_test(&event_loop, server, client, test_register_iter2,
		       "REGISTER multi");
    failed |= run_test(&event_loop, server, client, test_notify_iter,
		       "NOTIFY");
    failed |= run_test(&event_loop, server, client, test_locate_iter,
		       "LOCATE");
    failed |= run_test(&event_loop, server, client, test_bye_iter, "BYE");
    if (failed)
	exit(-1);

    delete client;
    delete server;

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}





