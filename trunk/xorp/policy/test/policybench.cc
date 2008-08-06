// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 International Computer Science Institute
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

#ident "$XORP: xorp/policy/test/policybench.cc,v 1.1 2008/08/06 08:04:52 abittau Exp $"

#include "policy/policy_module.h"
#include "policy/common/policy_utils.hh"
#include "policy/backend/policy_filter.hh"
#include "libxorp/xorp.h"
#include "libxorp/timer.hh"
#include "file_varrw.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

using namespace policy_utils;

#if defined(__i386__) && defined(__GNUC__)
static PolicyProfiler::TU get_time(void)
{
    uint64_t tsc;

    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (tsc));

    return tsc;
}
#define GET_TIME get_time

#else
#define GET_TIME NULL
#endif // i386 && GNUC

namespace {

struct conf {
    conf() {
	memset(&c_exec_total, 0, sizeof(c_exec_total));
    }

    // conf
    string	c_policy_file;
    string	c_varrw_file;
    unsigned	c_iterations;

    // stats
    TimeVal	    c_start;
    TimeVal	    c_end;
    PolicyProfiler  c_exec;
    double	    c_exec_total[PolicyProfiler::MAX_SAMPLES];
} _conf;

void
usage(const string& progname)
{
    cout << "Usage: " << progname << " <opts>" << endl
	 << "-p\t<policy file>"	    << endl
	 << "-v\t<varrw file>"	    << endl
	 << "-i\t<iterations>"	    << endl
         << "-h\thelp"		    << endl
	 ;

    exit(1);
}

void
die(const string& err)
{
    cout << "Death: " << err << endl;

    exit(1);
}

void
get_time(TimeVal& tv)
{
    TimerList::system_gettimeofday(&tv);
}

void
stats()
{
    double elapsed = (_conf.c_end - _conf.c_start).to_ms();
    double iter    = _conf.c_iterations;
    double speed   = iter / elapsed * 1000.0;

    printf("Total time %d (ms) iterations %d\n"
	   "Iterations/s %.3f\n"
	   , (int) elapsed, (int) iter, speed
	   );

    double total = 0.0;
    for (unsigned i = 0; i < _conf.c_exec.count(); i++) {
	_conf.c_exec_total[i] /= iter;
	total += _conf.c_exec_total[i];
    }

    if (total == 0.0)
	return;

    for (unsigned i = 0; i < _conf.c_exec.count(); i++) {
	double tm = _conf.c_exec_total[i];

	printf("Instr %2d Avg Time %10.3f\t(%5.2f%%)\n"
	       , i+1, tm, tm/total * 100.0);
    }
}

void
do_iter(PolicyFilter& filter, VarRW& varrw)
{
    _conf.c_exec.clear();
    filter.acceptRoute(varrw);

    for (unsigned i = 0; i < _conf.c_exec.count(); i++)
	_conf.c_exec_total[i] += _conf.c_exec.sample(i);
}

void
benchmark_run(void)
{
    string	    policy;
    VarRW**	    varrws;
    unsigned	    iters = _conf.c_iterations;
    PolicyFilter    filter;

    cout << "Loading..." << endl;

    read_file(_conf.c_policy_file, policy);
    filter.configure(policy);
    filter.set_profiler_exec(&_conf.c_exec);

    varrws = new VarRW* [iters];
    for (unsigned i = 0; i < iters; i++) {
	FileVarRW* f = new FileVarRW();
	
	f->set_verbose(false);

	if (!_conf.c_varrw_file.empty())
	    f->load(_conf.c_varrw_file);

	varrws[i] = f;
    }

    cout << "Benchmarking.  Iterations: " << iters << endl;
    get_time(_conf.c_start);

    for (unsigned i = 0; i < iters; i++)
	do_iter(filter, *varrws[i]);

    get_time(_conf.c_end);

    cout << "Stats:" << endl;
    stats();
}

void
own(void)
{
    benchmark_run();
}

} // namespace

int
main(int argc, char* argv[])
{
    int opt;

    _conf.c_iterations = 10000;

    while ((opt = getopt(argc, argv, "hp:v:i:")) != -1) {
	switch (opt) {
	    case 'p':
		_conf.c_policy_file = optarg;
		break;

	    case 'v':
		_conf.c_varrw_file = optarg;
		break;

	    case 'i':
		_conf.c_iterations = atoi(optarg);
		break;

	    case 'h': // fall-through
	    default:
		usage(argv[0]);
		break;
	}
    }

    if (_conf.c_policy_file.empty())
	usage(argv[0]);

    xlog_init(argv[0], 0);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    xlog_disable(XLOG_LEVEL_TRACE);
    xlog_add_default_output();
    xlog_start();

    PolicyProfiler::set_get_time(GET_TIME);

    try {
	own();
    } catch (const XorpReasonedException& e) {
	die(e.str());
    } catch (const exception& e) {
	die(e.what());
    }

    xlog_stop();
    xlog_exit();

    exit(0);
}
