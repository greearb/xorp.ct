// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "rib/rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/status_codes.h"

#ifndef XORP_USE_USTL
#include <iomanip>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "libxorp/c_format.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/service.hh"

#include "libxipc/xrl_std_router.hh"

#include "xrl/interfaces/rib_xif.hh"
#include "xrl/interfaces/finder_event_notifier_xif.hh"
#include "xrl/targets/show_distances_base.hh"

// ----------------------------------------------------------------------------
// Structure for holding command line options

struct ShowDistancesOptions {
    bool ribin;			// ribin (true), ribout (false)
    bool ipv4;			// IPv4 (true), IPv6(false)
    bool unicast;		// unicast (true), multicast (false)
    string	xrl_target;
    string	finder_host;
    uint16_t	finder_port;

    ShowDistancesOptions()
	: ribin(false),
	  ipv4(true),
	  unicast(true)
    {}
};

// ----------------------------------------------------------------------------
// Class for Querying RIB administrative distances

class ShowDistancesProcessor
    : public XrlShowDistancesTargetBase, public ServiceBase {
public:
    ShowDistancesProcessor(EventLoop&		    e,
			   ShowDistancesOptions&    opts);
    ~ShowDistancesProcessor();

    int startup();
    int shutdown();

public:
    XrlCmdError common_0_1_get_target_name(string& name);
    XrlCmdError common_0_1_get_version(string& version);
    XrlCmdError common_0_1_get_status(uint32_t& status, string& reason);
    XrlCmdError common_0_1_shutdown();
    XrlCmdError common_0_1_startup() { return XrlCmdError::OKAY(); }

    XrlCmdError finder_event_observer_0_1_xrl_target_birth(const string& cls,
							   const string& ins);
    XrlCmdError finder_event_observer_0_1_xrl_target_death(const string& cls,
							   const string& ins);

    bool poll_ready_failed();

    /**
     * Register with Finder to watch RIB birth and death events.  If
     * the RIB crashes we don't want to hang waiting for messages from
     * the RIB that will never arrive.
     */
    void step_100_watch_rib();
    void watch_rib_cb(const XrlError& xe);

    /**
     * Get all registered admin distances for the selected RIB.
     */
    void step_200_get_admin_distances();
    void get_distances_cb(const XrlError& xe,
			  const XrlAtomList* protocols,
			  const XrlAtomList* distances);

protected:
    EventLoop&				_e;
    const ShowDistancesOptions&		_opts;
    XrlRouter*				_rtr;
    XorpTimer				_t;
    XorpTask				_task;
    multimap<uint32_t, string>		_admin_distances;
};

ShowDistancesProcessor::ShowDistancesProcessor(EventLoop&		e,
					       ShowDistancesOptions&	o)
    : _e(e),
      _opts(o),
      _rtr(NULL)
{
}

ShowDistancesProcessor::~ShowDistancesProcessor()
{
    if (_rtr != NULL) {
	delete _rtr;
	_rtr = NULL;
    }
}

int
ShowDistancesProcessor::startup()
{
    if (status() != SERVICE_READY) {
	return (XORP_ERROR);
    }

    // Create XrlRouter
    string process = c_format("show_distances<%d>", XORP_INT_CAST(getpid()));
    _rtr = new XrlStdRouter(_e, process.c_str(),
			    _opts.finder_host.c_str(), _opts.finder_port);

    // Glue the router to the Xrl methods class exports
    this->set_command_map(_rtr);

    // Register methods with Finder
    _rtr->finalize();

    // Start timer to poll whether XrlRouter becomes ready or fails so
    // we can continue processing.
    _t = _e.new_periodic_ms(250, callback(this,
			    &ShowDistancesProcessor::poll_ready_failed));
    set_status(SERVICE_STARTING);
    return (XORP_OK);
}

int
ShowDistancesProcessor::shutdown()
{
    this->set_command_map(NULL);

    ServiceStatus st = this->status();
    if (st == SERVICE_FAILED
	|| st == SERVICE_SHUTTING_DOWN
	|| st == SERVICE_SHUTDOWN)
	return (XORP_ERROR);

    set_status(SERVICE_SHUTTING_DOWN);
    return (XORP_OK);
}

bool
ShowDistancesProcessor::poll_ready_failed()
{
    if (_rtr == 0) {
	return false;
    } else if (_rtr->ready()) {
	set_status(SERVICE_RUNNING);
	step_100_watch_rib();
	return false;
    } else if (_rtr->failed()) {
	set_status(SERVICE_FAILED, "Failed: No Finder?");
	return false;
    }
    return true;
}

void
ShowDistancesProcessor::step_100_watch_rib()
{
    XrlFinderEventNotifierV0p1Client fen(_rtr);

    if (fen.send_register_class_event_interest(
	"finder", _rtr->instance_name(), _opts.xrl_target,
	callback(this, &ShowDistancesProcessor::watch_rib_cb)) == false) {
	set_status(SERVICE_FAILED,
		   c_format("Failed to register interest in %s",
			    _opts.xrl_target.c_str()));
	return;
    }
}

void
ShowDistancesProcessor::watch_rib_cb(const XrlError& xe)
{

    if (xe == XrlError::OKAY()) {
	step_200_get_admin_distances();
	return;
    }
    set_status(SERVICE_FAILED,
	       c_format("Failed to register interest in %s",
			_opts.xrl_target.c_str()));
    return;
}

void
ShowDistancesProcessor::step_200_get_admin_distances()
{
    XrlRibV0p1Client rib(_rtr);
    XrlRibV0p1Client::GetProtocolAdminDistancesCB cb =
	    callback(this, &ShowDistancesProcessor::get_distances_cb);

    bool sent = rib.send_get_protocol_admin_distances(
		    _opts.xrl_target.c_str(), _opts.ipv4, _opts.unicast, cb);
    if (sent == false) {
	set_status(SERVICE_FAILED,
		   "Failed to request admin distances from RIB.");
	return;
    }

    return;
}

void
ShowDistancesProcessor::get_distances_cb(const XrlError& xe,
					 const XrlAtomList* protocols,
					 const XrlAtomList* distances)
{
    const uint32_t __UNSET_ADMIN_DISTANCE = 256;    // XXX

    if (xe != XrlError::OKAY()) {
	set_status(SERVICE_FAILED,
		   "Request to obtain admin distances from RIB failed.");
	shutdown();
	return;
    }

    try {
	XLOG_ASSERT(protocols->get(0).type() == XrlAtomType(xrlatom_text));
	XLOG_ASSERT(protocols->size() >= 1);
	XLOG_ASSERT(distances->get(0).type() == XrlAtomType(xrlatom_uint32));
	XLOG_ASSERT(distances->size() >= 1);
	XLOG_ASSERT(protocols->size() == distances->size());

	for (size_t i = 0; i < protocols->size(); i++) {
	    (void)_admin_distances.insert(
		pair<uint32_t, string>(
		    distances->get(i).uint32(),
		    protocols->get(i).text()));
	}

    } catch (XrlAtomList::InvalidIndex& ie) {
	fprintf(stdout, "Invalid data was returned by the RIB.");
	set_status(SERVICE_FAILED, "Invalid data was returned by the RIB.");
	shutdown();
	return;
    }

    if (_admin_distances.size() == 0) {
	fprintf(stdout,
		"No administrative distances registered for this RIB.\n\n");
    } else {
	fprintf(stdout, "Protocol                 Administrative distance\n");
	for (multimap<uint32_t, string>::iterator ii =
	     _admin_distances.begin(); ii != _admin_distances.end(); ++ii) {
	    fprintf(stdout, "%-24s ", ii->second.c_str());
	    if (ii->first == __UNSET_ADMIN_DISTANCE) {
		fprintf(stdout, "(unset)\n");
	    } else {
		fprintf(stdout, "%d\n",  ii->first);
	    }
	}
	fprintf(stdout, "\n");
    }

    set_status(SERVICE_SHUTDOWN);
    shutdown();
    return;
}

XrlCmdError
ShowDistancesProcessor::common_0_1_get_target_name(string& name)
{
    name = this->get_name();
    return XrlCmdError::OKAY();
}

XrlCmdError
ShowDistancesProcessor::common_0_1_get_version(string& version)
{
    version = this->version();
    return XrlCmdError::OKAY();
}

XrlCmdError
ShowDistancesProcessor::common_0_1_get_status(uint32_t&	status,
					   string&	/* reason */)
{
    switch (this->status()) {
    case SERVICE_READY:		status = PROC_NULL;		break;
    case SERVICE_STARTING:	status = PROC_STARTUP;		break;
    case SERVICE_RUNNING:	status = PROC_READY;		break;
    case SERVICE_PAUSED:					/* FALLTHRU */
    case SERVICE_PAUSING:					/* FALLTHRU */
    case SERVICE_RESUMING:	status = PROC_NOT_READY;	break;
    case SERVICE_SHUTTING_DOWN:	status = PROC_SHUTDOWN;		break;
    case SERVICE_SHUTDOWN:	status = PROC_DONE;		break;
    case SERVICE_FAILED:	status = PROC_FAILED;		break;
    case SERVICE_ALL:						break;
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
ShowDistancesProcessor::common_0_1_shutdown()
{
    this->shutdown();
    return XrlCmdError::OKAY();
}

XrlCmdError
ShowDistancesProcessor::finder_event_observer_0_1_xrl_target_birth(
						const string& /* cls */,
						const string& /* ins */
						)
{
    return XrlCmdError::OKAY();
}

XrlCmdError
ShowDistancesProcessor::finder_event_observer_0_1_xrl_target_death(
						const string& cls,
						const string& /* ins */)
{
    if (cls == _opts.xrl_target)
	this->shutdown();
    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// Utility methods

static void
usage()
{
    fprintf(stderr,
	    "Usage: %s [options] (ribin|ribout) (ipv4|ipv6) (unicast|multicast) <protocol>\n",
	    xlog_process_name());
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t -F <finder_host>:<finder_port> "
	    "Specify Finder host and port to use.\n");
    fprintf(stderr, "\t -T <targetname>                "
	    "Specify XrlTarget to query.\n\n");
    exit(1);
}

static bool
parse_finder_args(const string& host_colon_port, string& host, uint16_t& port)
{
    string::size_type sp = host_colon_port.find(":");
    if (sp == string::npos) {
        host = host_colon_port;
        // Do not set port, by design it has default finder port value.
    } else {
        host = string(host_colon_port, 0, sp);
        string s_port = string(host_colon_port, sp + 1, 14);
        uint32_t t_port = atoi(s_port.c_str());
        if (t_port == 0 || t_port > 65535) {
            XLOG_ERROR("Finder port %u is not in range 1--65535.\n",
		       XORP_UINT_CAST(t_port));
            return false;
        }
        port = (uint16_t)t_port;
    }
    return true;
}

static int
find_option(const char* s, const char* opts[], size_t n_opts)
{
    for (size_t i = 0; i < n_opts; i++) {
	if (strcmp(s, opts[i]) == 0) {
	    return (int)i;
	}
    }
    fprintf(stderr, "Invalid option \"%s\", expected one of:", s);
    for (size_t i = 0; i < n_opts; i++) {
	fprintf(stderr, "%s\"%s\"", (i == 0) ? " " : ", ", opts[i]);
    }
    fprintf(stderr, "\n");
    return -1;
}

static inline bool
match_binary_option(const char* input,
		    const char* option1,
		    const char* option2,
		    bool&	matches_first)
{
    const char* argv[2] = { option1, option2 };
    int i = find_option(input, argv, 2);
    if (i < 0)
	return false;
    matches_first = (i == 0) ? true : false;
    return true;
}

// ----------------------------------------------------------------------------
// Main

int
main(int argc, char* const argv[])
{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);         // Least verbose messages
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    try {
	ShowDistancesOptions sad_opts;

	sad_opts.finder_host = FinderConstants::FINDER_DEFAULT_HOST().str();
        sad_opts.finder_port = FinderConstants::FINDER_DEFAULT_PORT();
	sad_opts.xrl_target  = "rib";

	int ch;
	while ((ch = getopt(argc, argv, "F:T:")) != -1) {
	    switch (ch) {
	    case 'F':
		if (!parse_finder_args(optarg,
				       sad_opts.finder_host,
				       sad_opts.finder_port))
		    usage();
		break;
	    case 'T':
		sad_opts.xrl_target = optarg;
		break;
	    default:
		usage();
	    }
	}
	argc -= optind;
	argv += optind;

	if (argc != 3) {
	    usage();
	}

	if (match_binary_option(argv[0], "ribin", "ribout", sad_opts.ribin)
	    == false) {
	    usage();
	}

	if (match_binary_option(argv[1], "ipv4", "ipv6", sad_opts.ipv4)
	    == false) {
	    usage();
	}

	if (match_binary_option(argv[2], "unicast", "multicast",
				sad_opts.unicast) == false) {
	    usage();
	}

	EventLoop e;
	ShowDistancesProcessor sad(e, sad_opts);

	sad.startup();

	while ((sad.status() != SERVICE_FAILED)
	       && (sad.status() != SERVICE_SHUTDOWN)) {
	    e.run();
	}

	if (sad.status() == SERVICE_FAILED) {
	    sad.shutdown();
	    if (sad.status_note().empty() == false) {
		cout << sad.status_note() << endl;
	    } else {
		cout << "Failed" << endl;
	    }
	    cout << endl;
	}

    } catch (...) {
	xorp_print_standard_exceptions();
    }
    // if shutdown() is not called we may get free warnings
    // when sad goes out of scope.

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}

