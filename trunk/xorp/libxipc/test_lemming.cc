// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/libxipc/test_lemming.cc,v 1.2 2002/12/14 23:42:56 hodson Exp $"

#define XORP_MODULE_NAME "lemming"

#include "libxorp/xorp.h"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/xlog.h"

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
	_rtimer = e.new_periodic(200, callback(this, &Lemming::login));
    }
    ~Lemming() {
	vout << "...a lemming dies." << endl;
    }

    const XrlCmdError
    ping(const Xrl&, XrlArgs*)
    {
	return XrlCmdError::OKAY();
    }

    const XrlCmdError
    jump(const Xrl&, XrlArgs*)
    {
	_live = false;	// trigger death
	return XrlCmdError::OKAY();
    }

private:

    void login_cb(const XrlError& e, XrlRouter&, const Xrl&, XrlArgs*) {
	if (e == XrlError::OKAY()) {
	    vout << "...lemming registered with pinger..." << endl;
	    _registered = true;
	} else {
	    vout << e.str() << endl;
	}
    }

    bool login() {
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
    Pinger(EventLoop& e) : _r(e, "pinger") {
	_r.add_handler("register", callback(this, &Pinger::set_who));
	_ptimer = e.new_periodic(20, callback(this, &Pinger::send_ping));
    }

    const XrlCmdError
    set_who(const Xrl& x, XrlArgs*) {
	x.const_args().get("who", _who);
	vout << "Pinger set target to " << _who << endl;
	return XrlCmdError::OKAY();
    }

    void
    ping_cb(const XrlError& e, XrlRouter&, const Xrl&, XrlArgs*) {
	vout << "Ping callback: " << e.str() << endl;
    }

    bool
    send_ping() {
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
	*ppfs = new FinderServer(*e);
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

    FinderServer* pfs = 0;		/* Finder */

    toggle_finder(&e, &pfs);

    Pinger p(e);

    XorpTimer lemming_timer = e.new_oneoff_after_ms(1 * 60 * 1000,
						    callback(&snuff_flag,
							     &run));

    XorpTimer frt = e.new_periodic(18 * 1000,
				   callback(&toggle_finder, &e, &pfs));

    while (run && pfs) {
	bool life = true;
	XorpTimer snuffer = e.new_oneoff_after_ms((random() % 7777) + 1000,
						  callback(&snuff_flag,
							   &life));

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

