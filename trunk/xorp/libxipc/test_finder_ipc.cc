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

#ident "$XORP: xorp/devnotes/template.cc,v 1.2 2003/01/16 19:08:48 mjh Exp $"

#include <stdio.h>
#include <stdlib.h>

#include "finder_module.h"
#include "config.h"
#include "libxorp/xlog.h"
#include "finder_ipc.hh"

typedef bool (*test_iter_f)(FinderIPCService*);

static bool
test_hello_iter(FinderIPCService* s)
{
    FinderMessage m, n;

    s->prepare_message(m, HELLO);
    s->write_message(m);

    if (s->alive() && s->bytes_buffered()) {
        s->read_message(n);
    } else {
        return false;
    }
    return (m == n);
}

static bool
test_bye_iter(FinderIPCService* s)
{
    FinderMessage m, n;

    s->prepare_message(m, BYE);
    s->write_message(m);

    if (s->alive() && s->bytes_buffered()) {
        s->read_message(n);
    } else {
        return false;
    }
    return (m == n);
}

static bool
test_register_iter(FinderIPCService* s)
{
    const char key[] = "porridge", value[] = "goldilocks";
    FinderMessage m, n;

    s->prepare_message(m, REGISTER);
    m.add_arg(key);
    m.add_arg(value);
    s->write_message(m);

    if (s->alive() && s->bytes_buffered()) {
        s->read_message(n);
        if ((strcmp(n.get_arg(0), key) != 0) ||
            (strcmp(n.get_arg(1), value) != 0)) {
            return false;
        }
    }
    return (m == n);
}

static bool
test_register_iter2(FinderIPCService* s)
{

    FinderMessage m, n;
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

    int i,  nd;
    nd = random() % (sizeof(key) / sizeof(key[0]));

    for (i = 0; i < nd; i++) {
        s->prepare_message(m, REGISTER);
        m.add_arg(key[i]);
        m.add_arg(value[i]);
        s->write_message(m);
    }
    for (i = 0; i < nd; i++) {
        if (s->alive() && s->bytes_buffered()) {
            s->read_message(n);
            if ((strcmp(n.get_arg(0), key[i]) != 0) ||
                (strcmp(n.get_arg(1), value[i]) != 0)) {
                return false;
            }
        }
    }
    return (m == n);
}

static bool
test_notify_iter(FinderIPCService* s)
{
    const char key[] = "porridge", value[] = "goldilocks";
    FinderMessage m, n;

    s->prepare_message(m, NOTIFY);
    m.add_arg(key);
    m.add_arg(value);
    s->write_message(m);

    if (s->alive() && s->bytes_buffered()) {
        s->read_message(n);
        if ((strcmp(n.get_arg(0), key) != 0) ||
            (strcmp(n.get_arg(1), value) != 0)) {
            return false;
        }
    }
    return (m == n);
}

static bool
test_locate_iter(FinderIPCService* s)
{
    const char key[] = "porridge";
    FinderMessage m, n;

    s->prepare_message(m, LOCATE);
    m.add_arg(key);
    s->write_message(m);

    if (s->alive() && s->bytes_buffered()) {
        s->read_message(n);
        if ((strcmp(n.get_arg(0), key) != 0)) {
            return false;
        }
    }
    return (m == n);
}

static bool
run_test(FinderIPCService *s,
         test_iter_f iter_func, const char* name) {
    const int iters = 10000;
    bool failed = false;

    printf("Testing %s...", name);

    for (int i = 0; i < iters && failed == false; i++) {
        failed = !iter_func(s);
    }

    printf("%s\n", (failed) ? "fail" : "okay");
    return failed;
}

int main(int /* argc */, char *argv[])
{
    FinderTestIPCService s;
    bool failed;
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    failed = false;
    failed |= run_test(&s, test_hello_iter, "HELLO");
    failed |= run_test(&s, test_register_iter, "REGISTER");
    failed |= run_test(&s, test_register_iter2, "REGISTER multiple");
    failed |= run_test(&s, test_notify_iter, "NOTIFY");
    failed |= run_test(&s, test_locate_iter, "LOCATE");
    failed |= run_test(&s, test_bye_iter, "BYE");

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}





