// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include <iostream>

#include "xrl/interfaces/common_xif.hh"
#include "xrl/interfaces/test_xif.hh"

#include "test_xifs.hh"

/* ------------------------------------------------------------------------- */

/**
 * Xrl dispatch failure check method.  Exits if e indicates a failure.
 */
static void 
fail_xrl_error(const XrlError& e, const char* file, int line)
{
    if (e != XrlError::OKAY()) {
	cerr << "Error from " << file << " line " << line << endl;
	cerr << "Xrl failed: \n" << e.str().c_str() << endl;
	exit(-1);
    }
}

/**
 * Time out callback
 */
static void
timed_out(const char* task)
{
    cerr << "Timed out on " << task <<endl;
    exit(-1);
}

/* ------------------------------------------------------------------------- */

/**
 * Callback for common xif interface methods returning a string
 */
static void
common_xif_string_cb(const XrlError& e, const string *val,
		     const char* eventname, bool* done_flag)
{
    fail_xrl_error(e, __FILE__, __LINE__);
    cout << eventname << "\t" << val->c_str() << endl;
    *done_flag = true;
}

void
try_common_xif_methods(EventLoop* e, XrlRouter* r, const char* target_name)
{
    XrlCommonV0p1Client client(r);
    bool done_name, done_version;

    // Timer will cause program to exit if it expires
    XorpTimer t = e->new_oneoff_after_ms(3000, callback(&timed_out, 
							"testing common_xif"));

    done_name = false;
    client.send_get_target_name(target_name,
				callback(&common_xif_string_cb, 
					 "Target Name", &done_name));

    done_version = false;
    client.send_get_version(target_name,
			    callback(&common_xif_string_cb, 
				     "Target Version", &done_version));

    while (false == done_name || false == done_version) {
	e->run();
    }
    t.unschedule();
}

/* ------------------------------------------------------------------------- */

void
test_hw_cb(const XrlError& e, const char* cb_reason, bool* done_flag)
{
    fail_xrl_error(e, __FILE__, __LINE__);
    cout << "Callback for " << cb_reason << endl;
    *done_flag = true;
}

void
test_greeting_count_cb(const XrlError& e, const int32_t* rval, int32_t* lval)
{
    fail_xrl_error(e, __FILE__, __LINE__);
    cout << "Greeting count " << *rval << endl;
    assert(*rval >= 0);
    *lval = *rval;
}

void
test_greeting_print_cb(const XrlError&	e, 
		       const string*	greeting, 
		       const int32_t	greeting_no,
		       int32_t*		remain)
{
    fail_xrl_error(e, __FILE__, __LINE__);
    cout << "Greeting number " << greeting_no << " " << *greeting << endl;
    *remain = *remain - 1;
}

void
test_sf_cb(const XrlError& e,
	   bool*	   done)
{
    cout << "Shoot foot yields: " << e.str() << endl;
    if (e != XrlError::COMMAND_FAILED()) {
	cout << "Failed" << endl;
	exit(-1);
    }
    cout << "Good!" << endl;
    *done = true;
}

void
try_test_xif_methods(EventLoop* e, XrlRouter* r, const char* target_name)
{
    XrlTestV1p0Client client(r);
    bool done_hw, done_hwm, done_sf;
    int32_t count;

    // Timer will cause program to exit if it expires
    XorpTimer t = e->new_oneoff_after_ms(5000, callback(&timed_out,
							"testing test_xif"));

    done_hw = false;
    client.send_print_hello_world(target_name, callback(&test_hw_cb, 
							"hello world", 
							&done_hw));

    done_hwm = false;
    client.send_print_hello_world_and_message(target_name,
					      "Message from the other side",
					      callback(&test_hw_cb,
						       "hello world and " \
						       "message",
						       &done_hwm));

    count = -1;
    client.send_get_greeting_count(target_name, 
				   callback(test_greeting_count_cb, &count));


    done_sf = false;
    client.send_shoot_foot(target_name, callback(test_sf_cb, &done_sf));

    while (false == done_hw || false == done_hwm || -1 == count || 	
	   false == done_sf ) {
	e->run();
    }

    // Get greetings
    for (int32_t i = 0; i < count; i++) 
	client.send_get_greeting(target_name, i,
				 callback(&test_greeting_print_cb, i, &count));
    // count is decremented in test_greeting_print_cb.
    while (count > 0) {
	e->run();
    }
}    

