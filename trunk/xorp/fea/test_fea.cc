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

#ident "$XORP: xorp/fea/test_fea.cc,v 1.7 2004/05/16 00:26:24 pavlin Exp $"

/* TODO: XXX: THIS CODE NEEDS UPDATING AS XRL INTERFACE HAS CHANGED */

#include "config.h"
#include "fea_module.h"
#include "libxorp/debug.h"
#include "libxorp/xorp.h"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxipc/xrl_std_router.hh"
#include "fte.hh"

typedef void (*func)(const XrlError&, XrlRouter&, const Xrl&,
		     XrlArgs*, void*);

static const char *server = "fea";
static const char *client = "test fea";

class routing_cookie {
public:

    routing_cookie(func ns = 0) {
	_next_send = ns;
	_completed = false;
	_error = false;
	_crank = 0;
    };

    void set_next_send(func ns) {
	_next_send = ns;
    };


    func next_send() {
	return _next_send;
    };

    void xrl(Xrl xrl) {
	_xrl = xrl;
    };

    Xrl& xrl() {
	return _xrl;
    };

    void completed(bool b) {
	_completed = b;
    };

    bool completed() {
	return _completed;
    };

    void error(bool b) {
	_error = b;
    };

    bool error() {
	return _error;
    };

    void crank(int c) {
	_crank = c;
    };

    int crank() {
	return _crank;
    };

    void transactionid(const string& tid) {
	_tid.set(tid);
    };

    const string transactionid() {
	return string(_tid);
    };

    void thunk(void * t) {
	_thunk = t;
    };

    bool thunk() {
	return _thunk;
    };

private:
    func _next_send;	/* The next response function in the chain */
    Xrl  _xrl;		/* The prebuilt xrl */
    bool _completed;
    bool _error;
    int _crank;		/* How many times have we been through the
			 * transaction complete.
			 */
    TransactionID _tid;
    void *_thunk;	/* just in case we didn't think of something. */
};

/**
 * response function for reading a routing table entry.
 */
static
void
read_entry(const XrlError& e, XrlRouter& xrlrouter,
	   const Xrl&  debug_use(request),
	   XrlArgs* response, void* cookie)
{
    assert(e == XrlError::OKAY());
    debug_msg("%s %s\n", request.command(), string(*response).c_str());

    routing_cookie *rc = reinterpret_cast<routing_cookie *>(cookie);

    try {
	if (false == response->get_int32("result")) {
	    rc->completed(true);
	    return;
	}
    } catch(XrlArgs::XrlAtomNotFound) {
	XLOG_ERROR("Failed to extract argument result");
	rc->completed(true);
	rc->error(true);
	return;
    }

    Fte4 fte;

    const char *current;
    try {
#define getter(ret, name) current = name;fte.ret = response->get_ipv4(name)
	getter(dst, "dst");
	getter(gateway, "gateway");
	getter(netmask, "netmask");
#undef getter
	current = "ifname"; fte.ifname = response->get_string(current);
    } catch(XrlArgs::XrlAtomNotFound) {
	XLOG_ERROR("Failed to extract argument %s", current);
	rc->completed(true);
	rc->error(true);
	return;
    }

    printf("%s\n", fte.str().c_str());

    Xrl x(server, "read_entry");
    x.args().add_string("TransactionID", rc->transactionid());
    debug_msg("request %s response read_entry\n", string(x).c_str());
    xrlrouter.send(x, read_entry, rc);
}

/**
 * response function for starting to read a routing table entry.
 */
static
void
start_reading(const XrlError& e, XrlRouter& xrlrouter,
	      const Xrl&  debug_use(request),
	      XrlArgs* response, void* cookie)
{
    assert(e == XrlError::OKAY());
    debug_msg("%s %s\n", request.command(), string(*response).c_str());

    routing_cookie *rc = reinterpret_cast<routing_cookie *>(cookie);

    const char *current;
    try {
	current = "result";
	if (false == response->get_int32(current)) {
	    rc->completed(true);
	    rc->error(true);
	    return;
	}
	current = "TransactionID";
	rc->transactionid(response->get_string(current));

    } catch(XrlArgs::XrlAtomNotFound) {
	XLOG_ERROR("Failed to extract argument %s", current);
	rc->completed(true);
	rc->error(true);
	return;
    }

    Xrl x(server, "read_entry");
    x.args().add_string("TransactionID", rc->transactionid());
    debug_msg("request %s response read_entry\n", string(x).c_str());
    xrlrouter.send(x, read_entry, rc);
}

/**
 * response function for starting a transaction.
 */
static
void
start_transaction(const XrlError& e, XrlRouter& xrlrouter,
		  const Xrl& /* request */,  XrlArgs* response,
		  void* cookie)
{
    assert(e == XrlError::OKAY());

    routing_cookie *rc = reinterpret_cast<routing_cookie *>(cookie);

    const char *current;
    try {
	current = "result";
	if (false == response->get_int32("result")) {
	    XLOG_ERROR("false == result");
	    rc->completed(true);
	    rc->error(true);
	    return;
	}
	current = "TransactionID";
	rc->transactionid(response->get_string(current));
	rc->xrl().args().add_string("TransactionID", rc->transactionid());
    } catch(XrlArgs::XrlAtomNotFound) {
	XLOG_ERROR("Failed to extract argument %s", current);
	rc->completed(true);
	rc->error(true);
	return;
    }

    /*
    ** Start transaction can be called from many contexts. So in the
    ** cookie we stash the precomputed xrl and the next response
    ** function. This way we don't have to write a response function
    ** for add, delete and delete_all.
    **
    */
    debug_msg("request %s\n", string(rc->xrl()).c_str());
    xrlrouter.send(rc->xrl(), rc->next_send(), rc);
}

/**
 * response function after adding/deleting an entry/entries
 */
static
void
response_complete(const XrlError& e, XrlRouter& xrlrouter,
		  const Xrl& debug_use(request),
		  XrlArgs* response,  void* cookie)
{
    debug_msg("%s %s\n", request.command(), string(*response).c_str());
    assert(e == XrlError::OKAY());

    routing_cookie *rc = reinterpret_cast<routing_cookie *>(cookie);

    try {
	if (false == response->get_int32("result")) {
	    XLOG_ERROR("false == result");
	    /*
	    ** Abort the transaction here
	    */
	    rc->error(true);
	    rc->crank(1);
	    Xrl x(server, "abort_transaction");
	    x.args().add_string("TransactionID", rc->transactionid());
	    debug_msg("request %s\n", string(x).c_str());
	    xrlrouter.send(x, response_complete, rc);
	    return;
	}
    } catch(XrlArgs::XrlAtomNotFound) {
	XLOG_ERROR("Failed to extract argument result");
	rc->completed(true);
	rc->error(true);
	return;
    }
    debug_msg("crank %d\n", rc->crank());
    switch (rc->crank()) {
    case 0: {
	rc->crank(1);
	Xrl x(server, "commit_transaction");
  	x.args().add_string("TransactionID", rc->transactionid());
	debug_msg("request %s response response_complete\n",
		  string(x).c_str());
	xrlrouter.send(x, response_complete, rc);
	return;
	break;
    }
    case 1:
	rc->completed(true);
	break;
    default:
	XLOG_ERROR("unexpected crank %d", rc->crank());
	rc->completed(true);
	break;
    }
}

void
print_routing_table(const char *host)
{
    EventLoop eventloop;
    string hostname = host;

    if (0 == host)
	hostname = FINDER_DEFAULT_HOST.str();

    XrlStdRouter test_fea(eventloop, client, hostname.c_str());

    routing_cookie rc;

    Xrl x(server, "start_reading");

    debug_msg("request %s response start_reading\n", string(x).c_str());
    test_fea.send(x, start_reading, (void *)&rc);

    while(rc.completed() == false)
	eventloop.run();
}

/**
 * Example of how to add a routing table entry using XRL's
 *
 * @param host where the finder is
 * @param tt transaction type Fti::SYNC or Fti::ASYNC
 * @param dst destination address
 * @param gateway gateway to which packets should be sent
 * @param netmask netmask associated with the destination address
 * @param ifname interface through which the packets should leave
 *
 * @return true on a success
 */
bool
add(const char *host, Fti::TransactionType tt, char *dst, char *gateway,
    char *netmask, char *ifname)
{
    EventLoop eventloop;

    if (0 == host)
	host = FINDER_DEFAULT_HOST.str().c_str();

    XrlStdRouter test_fea(eventloop, client, host);

    /*
    ** The response function to the send is common for all transaction
    ** starts (transaction_start). Therefore the cookie contains a
    ** prebuilt XRL for the next send command to be executed by the
    ** transaction_start. As well as a common transaction_complete
    ** response function.
    */
    routing_cookie rc;

    Xrl xadd(server, "add_entry_old");
    xadd.args().
	add_ipv4("dst", dst).
	add_ipv4("gateway", gateway).
	add_ipv4("netmask", netmask).
 	add_string("ifname", ifname);
    rc.xrl(xadd);

    rc.set_next_send(response_complete);

    /*
    ** Load up the XRL arguments for this call.
    */
    Xrl x(server, "start_transaction");
    x.args().add_int32("TransactionType", tt);

    debug_msg("request %s response start_transaction\n", string(x).c_str());
    test_fea.send(x, start_transaction, &rc);

    while(rc.completed() == false)
	eventloop.run();

    return rc.error();
}

/**
 * Example of how to delete a routing table entry using XRL's
 *
 * @param host where the finder is
 * @param tt transaction type Fti::SYNC or Fti::ASYNC
 * @param dst destination address
 * @param netmask netmask associated with the destination address
 *
 * @return true on a success
 */
bool
del(const char *host, Fti::TransactionType tt, char *dst, char *netmask)
{
    EventLoop eventloop;

    if (0 == host)
	host = FINDER_DEFAULT_HOST.str().c_str();

    XrlStdRouter test_fea(eventloop, client, host);

    /*
    ** begin - set up the routing cookie with the next call.
    */
    routing_cookie rc;

    Xrl xadd(server, "delete_entry_old");
    xadd.args().
	add_ipv4("dst", dst).
	add_ipv4("netmask", netmask);

    rc.xrl(xadd);
    rc.set_next_send(response_complete);
    /*
    ** end
    */

    Xrl x(server, "start_transaction");
    x.args().add_int32("TransactionType", tt);

    debug_msg("request %s response start_transaction\n", string(x).c_str());
    test_fea.send(x, start_transaction, &rc);

    while(rc.completed() == false)
	eventloop.run();

    return rc.error();
}

class DeviceCallback {
public:
    DeviceCallback() : _completed(false),
		       _error(false)
    {}

    /*
    ** Create interface response method.
    */
    void
    create_interface(const XrlError& e, XrlRouter& /*xrlrouter*/,
		  const Xrl& /* request */,  XrlArgs* response)
    {
	assert(e == XrlError::OKAY());

	const char *current;
	try {
	    current = "result";
	    if (false == response->get_int32("result")) {
		XLOG_ERROR("false == result %s",
			   response->get_string("error").c_str());
		_completed = true;
		_error = true;
		return;
	    }
	} catch(XrlArgs::XrlAtomNotFound) {
	    XLOG_ERROR("Failed to extract argument %s", current);
	    _completed = true;
	    _error = true;
	    return;
	}

	_completed = true;
    }

    bool completed() {
	return _completed;
    }
private:
    bool _completed;
    bool _error;
};

bool
create_interface(const char *host, char *interface)
{
    EventLoop eventloop;

    if (0 == host)
	host = FINDER_DEFAULT_HOST.str().c_str();

    XrlStdRouter test_fea(eventloop, client, host);

    Xrl x(server, "create_interface");
    x.args().add_string("ifname", interface);

    debug_msg("request %s response create_interface\n", string(x).c_str());

    DeviceCallback dcb;
    test_fea.send(x, callback(dcb, &DeviceCallback::create_interface));

    while(dcb.completed() == false)
	eventloop.run();

    return true;
}

void
usage(char *name)
{
    fprintf (stderr,
"usage: %s [\n"
"(routing table)\t -r\n"
"(remote host)\t -h\n"
"(trans type)\t -t sync|async\n"
"(add)\t\t -a <dst> <gateway> <netmask> <interface>\n"
"(delete)\t -d <dst> <netmask> \n"
"(interface)\t -i create_interface le0 | -i delete_interface le0"
#if	LATER
"(zap)\t\t -z\n"
"(lookup)\t -l <dst>\n"
"(test) \t\t -T <test number>\n"
"(debug)\t\t -D\n"
#endif
"]\n",
	     name);

    exit(-1);
}

int
main(int argc, char *argv[])
{
    int c;
    bool prt = false;
    Fti::TransactionType tt = Fti::SYNC;
    char *host = 0;

    /*
    ** Initialize and start xlog
    */
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    while((c = getopt (argc, argv, "rh:t:a:d:i:")) != EOF) {
	switch (c) {
	case 'r':
	    prt = true;
	    break;
	case 'h':
	    host = optarg;
	    break;
	case 't':
	    if (0 == strcmp(optarg, "sync")) {
		tt = Fti::SYNC;
	    } else if (0 == strcmp(optarg, "async")) {
		tt = Fti::ASYNC;
	    } else
		usage(argv[0]);
	    break;
	case 'a':
	    /*
	    ** Check we have enough arguments.
	    */
	    if (optind + 3 > argc)
		usage(argv[0]);
 	    add(host, tt, optarg, argv[optind], argv[optind + 1],
		argv[optind + 2]);
	    optind += 3;
	    break;
	case 'd':
	    /*
	    ** Check we have enough arguments.
	    */
	    if (optind + 1 > argc)
		usage(argv[0]);
	    del(host, tt, optarg, argv[optind]);
	    optind += 1;
	    break;
	case 'i':
	    if (0 == strcmp(optarg, "create_interface")) {
		create_interface(host, argv[optind]);
	    } else if (0 == strcmp(optarg, "delete_interface")) {
// 		delete_interface(host, optarg);
	    } else
		usage(argv[0]);
	    break;
	case '?':
	    usage(argv[0]);
	}
    }

    if (prt)
	print_routing_table(host);

    /*
    ** Gracefully stop and exit xlog
    */
    xlog_stop();
    xlog_exit();

    exit (0);
}
