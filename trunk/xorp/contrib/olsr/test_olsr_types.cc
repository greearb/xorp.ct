// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 International Computer Science Institute
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

#include "olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/test_main.hh"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/timeval.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/tokenize.hh"

#include "olsr_types.hh"

//
// Test EightBitTime::to_timeval() against values specified in RFC.
//
bool
eightbittime_totv_test(TestInfo& info)
{
    struct dtuple {
	uint8_t htime;
	double dtime;
    };
    struct dtuple dtuples[] = {
	{ 0x05, 2.0 }, { 0x86, 6.0 }, { 0xe7, 15.0 }, { 0xe8, 30.0 }
    };

    for (int i = 0; i < 4; i++) {
	DOUT_LEVEL(info, 2) <<
	    c_format("htime is %02x", (int)dtuples[i].htime) << endl;

	TimeVal tv1 = EightBitTime::to_timeval(dtuples[i].htime);
	DOUT_LEVEL(info, 2) <<
	    "tv1 is " << tv1.str() << endl;

	XLOG_ASSERT(tv1.get_double() == dtuples[i].dtime);
    }

    return true;
}

//
// Test EightBitTime::from_timeval() against values specified in RFC.
//
bool
eightbittime_fromtv_test(TestInfo& info)
{
    struct dtuple {
	uint8_t htime;
	double dtime;
    };
    struct dtuple dtuples[] = {
	{ 0x05, 2.0 }, { 0x86, 6.0 }, { 0xe7, 15.0 }, { 0xe8, 30.0 }
    };

    for (int i = 0; i < 4; i++) {
	DOUT_LEVEL(info, 2) <<
	    c_format("dtime is %.4f", dtuples[i].dtime) << endl;

	TimeVal tv1(dtuples[i].dtime);

	uint8_t et1 = EightBitTime::from_timeval(tv1);
	DOUT_LEVEL(info, 2) <<
	    c_format("htime is %02x", et1) << endl;

	XLOG_ASSERT(et1 == dtuples[i].htime);
    }

    return true;
}

//
// Test EightBitTime::from_timeval() and EightBitTime::to_timeval()
// together with an expected range of values.
//
bool
eightbittime_roundtrip_test(TestInfo& info)
{
    //
    // Test entire range of values which EightBitTime can represent;
    // that is, 4 bits of mantissa, and 4 bits of exponent.
    //
    const double min_time = 0.0625;
    const double max_time = 3840.0;
    const double increment = 0.0625f;	// 1/16th of a second

    for (double dtime = 0.0f; dtime < max_time; dtime += increment) {
	TimeVal tv1(dtime);
	DOUT_LEVEL(info, 2) << c_format("dtime is %.4f", dtime) << endl;

	uint8_t et1 = EightBitTime::from_timeval(tv1);
	DOUT_LEVEL(info, 2) << c_format("et1 is %02x", (int)et1) << endl;

	TimeVal tv2 = EightBitTime::to_timeval(et1);
	DOUT_LEVEL(info, 2) << "tv2 is " << tv2.str() << endl;

	double finaldtime = tv2.get_double();
	DOUT_LEVEL(info, 2) <<
	    c_format("finaldtime is %.4f", finaldtime) << endl;

	XLOG_ASSERT(finaldtime >= min_time);
	XLOG_ASSERT(finaldtime <= max_time);

	// integer parts must match up to 32, then they diverge
	if (dtime <= 32.0)
	    XLOG_ASSERT((int)finaldtime == (int)dtime);
    }

    return true;
}

bool
seqno_test(TestInfo& info)
{
    struct seqno_tuple {
	uint16_t s1;
	uint16_t s2;
	bool expected_result;
    } sv[] = {
	{ 65535, 2,	false },
	{ 2,	 65535,	true },
	{ 32767, 65535, true },
	{ 65535, 32767, false },
	{ 33791, 1024,	true },
	{ 33791, 1023,	false }
    };

    for (size_t i = 0; i < (sizeof(sv)/sizeof(struct seqno_tuple)); i++) {
	bool result = is_seq_newer(sv[i].s1, sv[i].s2);
	DOUT_LEVEL(info, 2) <<
	    c_format("is_seq_newer(%u, %u) is %s\n", sv[i].s1, sv[i].s2,
		     bool_c_str(result));
	XLOG_ASSERT(result == sv[i].expected_result);
    }

    return true;
    UNUSED(info);
}

int
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    TestMain t(argc, argv);

    string test =
	t.get_optional_args("-t", "--test", "run only the specified test");
    t.complete_args_parsing();

    struct test {
	string test_name;
	XorpCallback1<bool, TestInfo&>::RefPtr cb;
    } tests[] = {
	{"eightbittime_totv", callback(eightbittime_totv_test)},
	{"eightbittime_fromtv", callback(eightbittime_fromtv_test)},
	{"eightbittime_roundtrip", callback(eightbittime_roundtrip_test)},
	{"seqno", callback(seqno_test)}
    };

    try {
	if (test.empty()) {
	    for (size_t i = 0; i < sizeof(tests) / sizeof(struct test); i++)
		t.run(tests[i].test_name, tests[i].cb);
	} else {
	    for (size_t i = 0; i < sizeof(tests) / sizeof(struct test); i++)
		if (test == tests[i].test_name) {
		    t.run(tests[i].test_name, tests[i].cb);
		    return t.exit();
		}
	    t.failed("No test with name " + test + " found\n");
	}
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    xlog_stop();
    xlog_exit();

    return t.exit();
}
