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

#ident "$XORP: xorp/libxipc/test_finder_tcp.cc,v 1.9 2003/04/21 22:40:11 hodson Exp $"

#include "finder_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
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
static const char *program_copyright    = "See file LICENSE.XORP";
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
    DummyFinderTcp(EventLoop& e, int fd, const char* name)
	: FinderTcpBase(e, fd), _name(name), _reads(0), _writes(0)
    {}
    
    void read_event(int		   errval,
		    const uint8_t* data,
		    uint32_t	   data_bytes)
    {
	verbose_log("- %s %d %p %u\n", _name, errval, data, data_bytes);
	_reads++;
    }

    void write_event(int	    errval,
		     const uint8_t* data,
		     uint32_t	    data_bytes)
    {
	verbose_log("+ %s %d %p %u\n", _name, errval, data, data_bytes);
	_writes++;
    }

    void
    write_msg()
    {
	string s = c_format("%p Write event %d", this, _writes);
	strncpy(_buf, s.c_str(), sizeof(_buf) - 1);
	_buf[sizeof(_buf) - 1] = '\0';
	write_data(reinterpret_cast<const uint8_t*>(_buf), strlen(_buf));
    }
    
    inline uint32_t read_events() const { return _reads; }
    inline uint32_t write_events() const { return _writes; }

    inline bool can_write() const { return !_writer.running(); }

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
		uint16_t    port = FINDER_DEFAULT_PORT)
	throw (InvalidPort)
	: FinderTcpListenerBase(e, interface, port), _connection(0)
    {
	add_permitted_host(interface);
    }

    ~DummyFinder()
    {
	delete _connection;
    }
    
    bool connection_event(int fd)
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
    ia.s_addr = FINDER_DEFAULT_HOST.addr();

    int fd = comm_connect_tcp4(&ia, htons(FINDER_DEFAULT_PORT));
    if (fd < 0) {
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

    DummyFinder df(e, FINDER_DEFAULT_HOST);

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
