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

#ident "$XORP: xorp/fea/test_ifmanager_transaction.cc,v 1.1.1.1 2002/12/11 23:56:03 hodson Exp $"

#include <assert.h>
#include <deque>
#include <map>

#include "config.h"

#include "fea_module.h"
#include "libxorp/xlog.h"

#include "libxorp/transaction.hh"
#include "ifmanager_transaction.hh"

// ----------------------------------------------------------------------------
// Verbose output

static bool s_verbose = false;

inline bool verbose() 		{ return s_verbose; }
inline void set_verbose(bool v)	{ s_verbose = v; }

#define verbose_log(x...) 						      \
do {									      \
    if (verbose()) {							      \
	printf("From %s:%d: ", __FILE__, __LINE__);			      \
	printf(x);							      \
    }									      \
} while(0)

// ----------------------------------------------------------------------------
// Test class

class TestInterfaceManagerOperations : public TransactionManager
{
public:
    TestInterfaceManagerOperations(EventLoop& e)
	: TransactionManager(e) {}

    /**
     * Add an operation to transaction and specify whether the
     * operation is expected to succeed.  If when the operation is
     * dispatched it doesn't meet the expected result, an error
     * is noted.
     *
     * @param tid transaction id.
     *
     * @param op operation to be added.
     *
     * @param expect_success whether operation dispatch is expected to
     *		succeed.
     */
    bool add_checked(uint32_t tid, const Operation& op, bool expect_success)
    {
	// Add the operation as per normal for a transaction manager
	if (add(tid, op) == false)
	    return false;
	// Add expected result to local storage
	_trm[tid].push_back(expect_success);
	return true;
    }

    uint32_t test_failures() const { return _test_failures; }
protected:
    typedef deque<bool> ResultDeque;
    typedef map<uint32_t, ResultDeque> TidResultMap;

    TidResultMap _trm;
    uint32_t	 _test_failures;
    uint32_t	 _commit_tid;

protected:
    void add_result(uint32_t tid, bool result) {
	_trm[tid].push_back(result);
    }

    bool results_empty(uint32_t tid) {
	return _trm[tid].empty();
    }

    bool get_result(uint32_t tid) {
	assert(results_empty(tid) == false);
	bool r = _trm[tid].front();
	_trm[tid].pop_front();
	return r;
    }

    void pre_commit(uint32_t tid) {
	_test_failures = 0;
	_commit_tid = tid;
    }

    void operation_result(bool success, const TransactionOperation& op)
    {
	if (success != get_result(_commit_tid)) {
	    verbose_log("Unexpected %s from operation\n\t%s\n",
			success ? "success" : "failure",
			op.str().c_str());
	    _test_failures++;
	}
	assert (pending() != 0 || results_empty(_commit_tid) == true);
    }
};

bool
run_test()
{
    EventLoop e;
    TestInterfaceManagerOperations tm(e);
    IfTree it;

    uint32_t tid;

    //
    // Start a first transaction
    //

    if (tm.start(tid) == false) {
	return false;
    }

    tm.add_checked(tid, new RemoveInterface(it, "ed0"), false);
    tm.add_checked(tid, new AddInterface(it, "ed0"), true);
    tm.add_checked(tid, new RemoveInterface(it, "ed0"), true);
    tm.add_checked(tid, new AddInterface(it, "ed0"), true);
    tm.add_checked(tid, new SetInterfaceEnabled(it, "dc0", true), false);
    tm.add_checked(tid, new SetInterfaceEnabled(it, "ed0", true), true);
    tm.add_checked(tid, new SetInterfaceMTU(it, "dc0", 1000), false);
    tm.add_checked(tid, new SetInterfaceMTU(it, "ed0", 1500), true);
    tm.add_checked(tid, new SetInterfaceMAC(it, "ed0",
					    Mac("00:b0:d0:f7:dd:1c")), true);

    tm.add_checked(tid, new RemoveInterfaceVif(it, "ed0", "ed0"), false);
    tm.add_checked(tid, new AddInterfaceVif(it, "ed0", "ed0"), true);

    tm.add_checked(tid, new AddAddr4(it, "ed0", "ed0",
				     IPv4("192.168.187.73")), true);

    tm.add_checked(tid,
		   new SetAddr4Broadcast(it, "ed0", "ed0",
					    IPv4("192.168.187.73"),
					    IPv4("192.168.187.147")), true);

    tm.add_checked(tid,
		   new SetAddr4Broadcast(it, "ed0", "ed0", IPv4("0.0.0.0"),
					    IPv4("192.168.187.147")), false);

    tm.add_checked(tid, new AddAddr4(it, "ed0", "ed0",
				     IPv4("192.168.10.10")), true);

    tm.add_checked(tid,
		   new SetAddr4Endpoint(it, "ed0", "ed0",
					   IPv4("192.168.10.10"),
					   IPv4("10.0.0.1")), true);


    tm.commit(tid);

    verbose_log("After Commit 1\n");
    verbose_log("\n%s\n", it.str().c_str());

    it.finalize_state();

    verbose_log("After Finalizing 1\n");
    verbose_log("\n%s\n", it.str().c_str());

    //
    // Start a second transaction
    //
    if (tm.start(tid) == false) {
	return false;
    }

    tm.add_checked(tid, new SetAddr4Endpoint(it, "ed0", "ed0",
						IPv4("192.168.10.10"),
						IPv4("0.0.0.0")), true);

    tm.add_checked(tid, new SetVifEnabled(it, "ed0", "ed0", true), true);

    tm.add_checked(tid,
		   new SetAddr4Enabled(it, "ed0", "ed0",
					  IPv4("192.168.10.10"), true), true);


    tm.commit(tid);

    verbose_log("After Commit 2\n");
    verbose_log("\n%s\n", it.str().c_str());

    it.finalize_state();

    verbose_log("After Finalizing 2\n");
    verbose_log("\n%s\n", it.str().c_str());

    //
    // Start a third transaction
    //
    if (tm.start(tid) == false) {
	return false;
    }

    tm.add_checked(tid, new SetAddr4Endpoint(it, "ed0", "ed0",
						IPv4("192.168.187.73"),
						IPv4("1.1.1.1")), true);

    tm.add_checked(tid, new SetVifEnabled(it, "ed0", "ed0", true), true);

    tm.add_checked(tid,
		   new SetAddr4Enabled(it, "ed0", "ed0",
					  IPv4("192.168.187.73"), true), true);


    tm.commit(tid);

    verbose_log("After Commit 3\n");
    verbose_log("\n%s\n", it.str().c_str());

    it.finalize_state();

    verbose_log("After Finalizing 3\n");
    verbose_log("\n%s\n", it.str().c_str());

    return tm.test_failures() == 0;
}

static void
usage(const char* argv0)
{
    fprintf(stderr, "usage: %s [-hv]\n", argv0);
    fprintf(stderr, "-h\t\t give this help");
    fprintf(stderr, "-v\t\t verbose mode");
    exit(EXIT_FAILURE);
}

int
main(int argc, char * const argv[])
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
	}
    }
    argc -= optind;
    argv += optind;
    if (argc != 0) usage(argv[0]);

    int r = 0;
    if (run_test() == false) {
	r = -1;
    } else {
	verbose_log("Test good\n");
    }

    xlog_stop();
    xlog_exit();
    return r;
}
