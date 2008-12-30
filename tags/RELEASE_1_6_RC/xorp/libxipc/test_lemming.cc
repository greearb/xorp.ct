// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/libxipc/test_lemming.cc,v 1.22 2008/07/23 05:10:43 pavlin Exp $"

#include "ipc_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/random.h"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "libxipc/finder_server.hh"
#include "libxipc/xrl_std_router.hh"


class verbose_ostream {
public:
    verbose_ostream() : _verbose(false), _out(cout) {}
    void set_verbose(bool v) { _verbose = v; }
    bool verbose() const { return _verbose; }

    verbose_ostream& operator<<(const char* cc) {
	if (_verbose) _out << cc;
	return *this;
    }
    verbose_ostream& operator<<(const string& s) {
	if (_verbose) _out << s;
	return *this;
    }
    typedef ostream& (F) (ostream&);
    verbose_ostream& operator<<(F f) {
	if (_verbose) _out << f;
	return *this;
    }
protected:
    bool _verbose;
    ostream& _out;
} vout;

// ----------------------------------------------------------------------------
// Lemming
//

// A test to check the correct thing happens when a process registers itself
// by name with another process for notifications and subsequently dies.
//
// The Lemming is a an Xrl entity that periodically dies and is reborn.  When
// the Lemming is reborn it registers itself with the "pinger", that proceeds
// to ping the Xrl target named "lemming" repeatedly.  To make things
// interesting the finder dies periodically too.
//

class Lemming {
public:
    Lemming(EventLoop& e, bool& life) : _r(e, "lemming"), _live(life)
    {
	vout << "A lemming is born..." << endl;
	_r.add_handler("ping", callback(this, &Lemming::ping));
	_r.add_handler("jump", callback(this, &Lemming::jump));
	_registered = false;
	_rtimer = e.new_periodic_ms(200, callback(this, &Lemming::login));
    }

    ~Lemming()
    {
	vout << "...a lemming dies." << endl;
    }

    const XrlCmdError
    ping(const XrlArgs&, XrlArgs*)
    {
	return XrlCmdError::OKAY();
    }

    const XrlCmdError
    jump(const XrlArgs&, XrlArgs*)
    {
	_live = false;	// trigger death
	return XrlCmdError::OKAY();
    }

private:

    void login_cb(const XrlError& e, XrlArgs*)
    {
	if (e == XrlError::OKAY()) {
	    vout << "...lemming registered with pinger..." << endl;
	    _registered = true;
	} else {
	    vout << e.str() << endl;
	}
    }

    bool login()
    {
	if (false == _registered) {
	    Xrl x("pinger", "register",
		  XrlArgs().add(XrlAtom("who", _r.name())));
	    _r.send(x, callback(this, &Lemming::login_cb));
	    return true;
	}
	return false;
    }

private:
    XrlStdRouter _r;
    bool&	 _live;		// flag marking lemmings life force
    bool	 _registered;	// registered with prodder
    XorpTimer	 _rtimer;	// registration timer
};

// ----------------------------------------------------------------------------
// Pinger
//
// Listens for someone to register with it and then pings them periodically
//

class Pinger
{
public:
    Pinger(EventLoop& e) : _r(e, "pinger")
    {
	_r.add_handler("register", callback(this, &Pinger::set_who));
	_ptimer = e.new_periodic_ms(20, callback(this, &Pinger::send_ping));
    }

    const XrlCmdError
    set_who(const XrlArgs& inputs, XrlArgs*)
    {
	inputs.get("who", _who);
	vout << "Pinger set target to " << _who << endl;
	return XrlCmdError::OKAY();
    }

    void
    ping_cb(const XrlError& e, XrlArgs*)
    {
	vout << "Ping callback: " << e.str() << endl;
    }

    bool
    send_ping()
    {
	vout << "ping \"" << _who << "\"" << endl;
	if (_who.empty() == false) {
	    Xrl x(_who, "ping");
	    _r.send(x, callback(this, &Pinger::ping_cb));
	}
	return true;
    }

private:
    XrlStdRouter _r;
    string	 _who; 		// Name of entity being pinged
    XorpTimer	 _ptimer;	// ping timer
};

// ----------------------------------------------------------------------------
// Main body

static bool
toggle_finder(EventLoop* e, FinderServer** ppfs)
{
    if (*ppfs) {
	delete *ppfs;
	*ppfs = 0;
    } else {
	*ppfs = new FinderServer(*e,
				 FinderConstants::FINDER_DEFAULT_HOST(),
				 FinderConstants::FINDER_DEFAULT_PORT());
    }
    return true;
}

static void
snuff_flag(bool *flag)
{
    *flag = false;
}

static void
lemming_main()
{
    EventLoop e;
    bool run = true;

    FinderServer* pfs = 0;

    toggle_finder(&e, &pfs);

    Pinger p(e);

    XorpTimer lemming_timer = e.new_oneoff_after_ms(1 * 60 * 1000,
						    callback(&snuff_flag,
							     &run));

    XorpTimer frt = e.new_periodic_ms(18 * 1000,
				      callback(&toggle_finder, &e, &pfs));

    while (run && pfs) {
	bool life = true;
	XorpTimer snuffer = e.new_oneoff_after_ms(
	    (xorp_random() % 7777) + 1000, callback(&snuff_flag, &life));
	{
	    Lemming l(e, life);
	    while (life) e.run();
	}
    }
}

static void
usage(const char* proc)
{
    cerr << "usage: " << proc << "[-h] [-v]" << endl;
    cerr << "\t-h=t: usage (this message)" << endl;
    cerr << "\t-v=t: verbose output" << endl;
}

int
main(int argc, char* const argv[])
{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_add_default_output();
    xlog_start();

  int ch;
    while ((ch = getopt(argc, argv, "hv")) != -1) {
        switch (ch) {
        case 'v':
            vout.set_verbose(true);
            break;
        case 'h':
        case '?':
        default:
            usage(argv[0]);
            xlog_stop();
            xlog_exit();
            if (ch == 'h')
                return (0);
            else
                return (1);
        }
    }

    try {
	lemming_main();
    } catch (...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}

