// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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



#include "finder_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/xorpfd.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "libcomm/comm_api.h"

#include "sockutil.hh"
#include "finder_tcp.hh"
#include "finder_constants.hh"
#include "permits.hh"


///////////////////////////////////////////////////////////////////////////////
// Constants

static const char *program_name         = "test_finder_tcp";
static const char *program_description  = "Test Finder TCP transport";
static const char *program_version_id   = "0.1";
static const char *program_date         = "January, 2003";
static const char *program_copyright    = "See file LICENSE";
static const char *program_return_value = "0 on success, 1 if test error, 2 if internal error";

///////////////////////////////////////////////////////////////////////////////
// Verbosity level control

static bool s_verbose = false;
bool verbose()                  { return s_verbose; }
void set_verbose(bool v)        { s_verbose = v; }

#define verbose_log(x...) _verbose_log(__FILE__,__LINE__, x)

#define _verbose_log(file, line, x...)					\
do {									\
    if (verbose()) {							\
	printf("From %s:%d: ", file, line);				\
	printf(x);							\
    }									\
} while(0)

///////////////////////////////////////////////////////////////////////////////
// Tcp class for testing message read/writes

class DummyFinderTcp : public FinderTcpBase {
public:
    DummyFinderTcp(EventLoop& e, XorpFd fd, const char* name)
	: FinderTcpBase(e, fd), _name(name), _reads(0), _writes(0)
    {}
    
    bool read_event(int		   errval,
		    const uint8_t* data,
		    uint32_t	   data_bytes)
    {
	verbose_log("- %s %d %p %u\n", _name, errval, data,
		    XORP_UINT_CAST(data_bytes));
	_reads++;
	return true;
    }

    void write_event(int	    errval,
		     const uint8_t* data,
		     uint32_t	    data_bytes)
    {
	verbose_log("+ %s %d %p %u\n", _name, errval, data,
		    XORP_UINT_CAST(data_bytes));
	_writes++;
    }

    void
    write_msg()
    {
	string s = c_format("%p Write event %u", this,
			    XORP_UINT_CAST(_writes));
	strncpy(_buf, s.c_str(), sizeof(_buf) - 1);
	_buf[sizeof(_buf) - 1] = '\0';
	write_data(reinterpret_cast<const uint8_t*>(_buf), strlen(_buf));
    }
    
    uint32_t read_events() const { return _reads; }
    uint32_t write_events() const { return _writes; }

    bool can_write() const { return !_writer.running(); }

protected:
    const char*	_name;
    char	_buf[255];
    uint32_t	_reads;
    uint32_t	_writes;
};

class DummyFinder : public FinderTcpListenerBase {
public:
    DummyFinder(EventLoop&  e,
		IPv4	    interface,
		uint16_t    port = FinderConstants::FINDER_DEFAULT_PORT())
	throw (InvalidPort)
	: FinderTcpListenerBase(e, interface, port), _connection(0)
    {
	add_permitted_host(interface);
    }

    ~DummyFinder()
    {
	delete _connection;
    }
    
    bool connection_event(XorpFd fd)
    {
	assert(0 == _connection);
	_connection = new DummyFinderTcp(_e, fd, "server");
	verbose_log("Server accepted client connection\n");
	return true;
    }

    DummyFinderTcp* connection() { return _connection; }

protected:
    DummyFinderTcp* _connection;
};

static DummyFinderTcp* client_end;

static void
connect_client(EventLoop* e, bool* client_connect_failed)
{
    struct in_addr ia;
    ia.s_addr = FinderConstants::FINDER_DEFAULT_HOST().addr();

    int in_progress = 0;
    XorpFd fd = comm_connect_tcp4(&ia,
			       htons(FinderConstants::FINDER_DEFAULT_PORT()),
			       COMM_SOCK_NONBLOCKING, &in_progress, NULL);
    if (!fd.is_valid()) {
	fprintf(stderr, "Client failed to connect\n");
	*client_connect_failed = true;
	return;
    }
    verbose_log("Client connected to server\n");
    client_end = new DummyFinderTcp(*e, fd, "client");
}

static int
test_main()
{
    EventLoop   e;

    DummyFinder df(e, FinderConstants::FINDER_DEFAULT_HOST());

    bool client_connect_failed = false;
    XorpTimer	connect_timer = e.new_oneoff_after_ms(
				   200, callback(connect_client,
						 &e,
						 &client_connect_failed));

    while (client_connect_failed == false) {
	e.run();
	if (0 == client_end || 0 == df.connection())
	    continue;
	if (client_end->can_write() &&
	    client_end->write_events() <= df.connection()->write_events()) {
	    client_end->write_msg();
	}
	if (df.connection()->can_write() &&
	    df.connection()->write_events() < client_end->write_events()) {
	    df.connection()->write_msg();
	}
	if (df.connection()->write_events() == 500) {
	    break;
	}
    }

    client_end->close();
    delete client_end;	// allocated by connect_client

    return (client_connect_failed) ? 1 : 0;
}

/**
 * Print program info to output stream.
 *
 * @param stream the output stream the print the program info to.
 */
static void
print_program_info(FILE *stream)
{
    fprintf(stream, "Name:          %s\n", program_name);
    fprintf(stream, "Description:   %s\n", program_description);
    fprintf(stream, "Version:       %s\n", program_version_id);
    fprintf(stream, "Date:          %s\n", program_date);
    fprintf(stream, "Copyright:     %s\n", program_copyright);
    fprintf(stream, "Return:        %s\n", program_return_value);
}
/**
 * Print program usage information to the stderr.
 *
 * @param progname the name of the program.
 */
static void
usage(const char* progname)
{
    print_program_info(stderr);
    fprintf(stderr, "usage: %s [-v] [-h]\n", progname);
    fprintf(stderr, "       -h          : usage (this message)\n");
    fprintf(stderr, "       -v          : verbose output\n");
}

int
main(int argc, char * const argv[])
{
   int ret_value = 0;

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
            xlog_stop();
            xlog_exit();
            if (ch == 'h')
                return (0);
            else
                return (1);
        }
    }
    argc -= optind;
    argv += optind;
    
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    try {
	ret_value = test_main();
    } catch (...) {
        // Internal error
        xorp_print_standard_exceptions();
        ret_value = 2;
    }
   
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    
    return (ret_value);
}
