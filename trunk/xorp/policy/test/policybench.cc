// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/policy/test/policybench.cc,v 1.8 2008/08/06 08:30:58 abittau Exp $"

#include "policy/policy_module.h"
#include "policy/common/policy_utils.hh"
#include "policy/common/filter.hh"
#include "policy/backend/policy_filter.hh"
#include "libxorp/xorp.h"
#include "libxorp/timer.hh"
#include "file_varrw.hh"
#include "bgp/bgp_varrw.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

using namespace policy_utils;

namespace {

enum {
    BTYPE_FILE = 0,
    BTYPE_BGP,
};

template<class A>
struct bgp_routes {
    bgp_routes();

    InternalMessage<A>*	    bgp_message;
    SubnetRoute<A>*	    bgp_route;
    PathAttributeList<A>*   bgp_attributes;
};

template<class A>
bgp_routes<A>::bgp_routes()
{
    IPNet<A> net("192.168.0.0/24");

    A nexthop("192.168.0.1");
    ASPath path("7865");
    OriginType origin = EGP;

    bgp_attributes = new PathAttributeList<A>(nexthop, path, origin);
    bgp_attributes->rehash();

    bgp_route   = new SubnetRoute<A>(net, bgp_attributes, NULL);
    bgp_message = new InternalMessage<A>(bgp_route, NULL, 1);
}

#define AF IPv4
typedef BGPVarRW<AF>	BGP_VARRW;
typedef bgp_routes<AF>	BGP_ROUTES;

struct conf {
    conf() : c_bgp_varrw(NULL), c_bgp_routes(NULL)
    {
	memset(&c_exec_total, 0, sizeof(c_exec_total));
    }

    // conf
    string	c_policy_file;
    string	c_varrw_file;
    unsigned	c_iterations;
    int		c_type;
    int		c_profiler;

    // stats
    TimeVal	    c_start;
    TimeVal	    c_end;
    PolicyProfiler  c_exec;
    double	    c_exec_total[PolicyProfiler::MAX_SAMPLES];

    // BGP stuff
    BGP_VARRW*	    c_bgp_varrw;
    BGP_ROUTES**    c_bgp_routes;
} _conf;

void
usage(const string& progname)
{
    cout << "Usage: " << progname << " <opts>" << endl
	 << "-p\t<policy file>"	    << endl
	 << "-v\t<varrw file>"	    << endl
	 << "-i\t<iterations>"	    << endl
	 << "-t\t<benchmark type>"  << endl
	 << "-n\tdisable profiler"  << endl
         << "-h\thelp"		    << endl
	 << endl
	 << "Supported benchmark types:" << endl
	 << "0\tFileVarRW (test policy filter)" << endl
	 << "1\tBGPVarRW" << endl
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
do_iter_bgp(PolicyFilter& filter, VarRW& varrw, int i)
{
    BGP_VARRW& v = static_cast<BGP_VARRW&>(varrw);

    v.attach_route(*_conf.c_bgp_routes[i]->bgp_message, false);

    filter.acceptRoute(v);

    if (v.modified()) {
	v.filtered_message(); // XXX leak
    }
}

void
do_iter(PolicyFilter& filter, VarRW& varrw, int i)
{
    _conf.c_exec.clear();

    switch (_conf.c_type) {
    case BTYPE_BGP:
	do_iter_bgp(filter, varrw, i);
	break;

    case BTYPE_FILE:
	filter.acceptRoute(varrw);
	break;
    }

    for (unsigned i = 0; i < _conf.c_exec.count(); i++)
	_conf.c_exec_total[i] += _conf.c_exec.sample(i);
}

VarRW*
create_varrw_file()
{
    FileVarRW* f = new FileVarRW();

    f->set_verbose(false);

    if (!_conf.c_varrw_file.empty())
	f->load(_conf.c_varrw_file);

    return f;
}

VarRW*
create_varrw_bgp(int num)
{
    if (!_conf.c_bgp_varrw)
	_conf.c_bgp_varrw = new BGP_VARRW(filter::filter2str(filter::IMPORT));

    if (!_conf.c_bgp_routes)
	_conf.c_bgp_routes = new BGP_ROUTES* [_conf.c_iterations];

    _conf.c_bgp_routes[num] = new BGP_ROUTES;

    return _conf.c_bgp_varrw;
}

VarRW*
create_varrw(int i)
{
    switch (_conf.c_type) {
    case BTYPE_FILE:
	return create_varrw_file();

    case BTYPE_BGP:
	return create_varrw_bgp(i);
    }

    XLOG_ASSERT(false);

    return NULL;
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
    for (unsigned i = 0; i < iters; i++)
	varrws[i] = create_varrw(i);

    cout << "Benchmarking.  Iterations: " << iters << endl;
    get_time(_conf.c_start);

    for (unsigned i = 0; i < iters; i++)
	do_iter(filter, *varrws[i], i);

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

    _conf.c_iterations = 100000;
    _conf.c_type       = 0;
    _conf.c_profiler   = 1;

    while ((opt = getopt(argc, argv, "hp:v:i:t:n")) != -1) {
	switch (opt) {
	    case 'n':
		_conf.c_profiler = 0;
		break;

	    case 't':
		_conf.c_type = atoi(optarg);
		break;

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
