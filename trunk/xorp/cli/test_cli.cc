// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/cli/test_cli.cc,v 1.44 2007/02/16 22:45:29 pavlin Exp $"


//
// CLI (Command-Line Interface) test program.
//


#include "cli_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/token.hh"
#include "libxorp/xlog.h"

#include "libxipc/finder_server.hh"

#include "cli_client.hh"
#include "xrl_cli_node.hh"

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

//
// Local variables
//
CliNode *global_cli_node = NULL;

//
// Local functions prototypes
//
static	bool wakeup_hook();
static	bool wakeup_hook2(int, int);
static	CliNode& cli_node();
static	void usage(const char *argv0, int exit_value);

/**
 * usage:
 * @argv0: Argument 0 when the program was called (the program name itself).
 * @exit_value: The exit value of the program.
 *
 * Print the program usage.
 * If @exit_value is 0, the usage will be printed to the standart output,
 * otherwise to the standart error.
 **/
static void
usage(const char *argv0, int exit_value)
{
    FILE *output;
    const char *progname = strrchr(argv0, '/');

    if (progname != NULL)
	progname++;		// Skip the last '/'
    if (progname == NULL)
	progname = argv0;

    //
    // If the usage is printed because of error, output to stderr, otherwise
    // output to stdout.
    //
    if (exit_value == 0)
	output = stdout;
    else
	output = stderr;

    fprintf(output, "Usage: %s [-F <finder_hostname>[:<finder_port>]]\n",
	    progname);
    fprintf(output, "           -F <finder_hostname>[:<finder_port>]  : finder hostname and port\n");
    fprintf(output, "           -h                                    : usage (this message)\n");
    fprintf(output, "\n");
    fprintf(output, "Program name:   %s\n", progname);
    fprintf(output, "Module name:    %s\n", XORP_MODULE_NAME);
    fprintf(output, "Module version: %s\n", XORP_MODULE_VERSION);

    exit (exit_value);

    // NOTREACHED
}

class Foo {
public:
    Foo() {}
    bool print() { printf("Hello %p\n", this); return (1); }
};

int
add_my_cli_commands(CliNode& cli_node);

static CliNode&
cli_node()
{
    return (*global_cli_node);
}

static void
cli_main(const string& finder_hostname, uint16_t finder_port,
	 bool start_finder)
{
    string error_msg;
    EventLoop eventloop;

    //
    // Init stuff
    //

    //
    // Start our own finder
    //
    FinderServer *finder = NULL;
    if (start_finder) {
	try {
	    finder = new FinderServer(eventloop, IPv4(finder_hostname.c_str()),
				      finder_port);
	} catch (const InvalidPort&) {
	    XLOG_FATAL("Could not start in-process Finder");
	}
    }

    //
    // The main body
    //
    Foo f;

    //
    // CLI
    //
    // XXX: we use a single CLI node to handle both IPv4 and IPv6
    CliNode cli_node(AF_INET, XORP_MODULE_CLI, eventloop);
    cli_node.set_cli_port(12000);

    //
    // CLI access
    //
    IPvXNet enable_ipvxnet1("127.0.0.1/32");
    // IPvXNet enable_ipvxnet2("192.150.187.0/25");
    IPvXNet disable_ipvxnet1("0.0.0.0/0");	// Disable everything else
    //
    cli_node.add_enable_cli_access_from_subnet(enable_ipvxnet1);
    // cli_node.add_enable_cli_access_from_subnet(enable_ipvxnet2);
    cli_node.add_disable_cli_access_from_subnet(disable_ipvxnet1);

    //
    // Create and configure the CLI XRL interface
    //
    XrlCliNode xrl_cli_node(eventloop,
			    cli_node.module_name(),
			    finder_hostname,
			    finder_port,
			    "finder",
			    cli_node);
    wait_until_xrl_router_is_ready(eventloop, xrl_cli_node.xrl_router());

#if 0
#ifdef HAVE_IPV6
    CliNode cli_node6(AF_INET6, XORP_MODULE_CLI, eventloop);
    cli_node6.set_cli_port(12000);
    XrlCliNode xrl_cli_node(eventloop,
			    cli_node6.module_name(),
			    finder_hostname,
			    finder_port,
			    "finder",
			    cli_node6);
    wait_until_xrl_router_is_ready(eventloop, xrl_cli_node.xrl_router());
#endif // HAVE_IPV6
#endif // 0


    //
    // XXX: CLI test-specific setup
    //
    global_cli_node = &cli_node;
    add_my_cli_commands(cli_node);
    // add_my_cli_commands(cli_node6);

    //
    // Start the nodes
    //
    cli_node.enable();
    cli_node.start();
    // cli_node6.enable();
    // cli_node6.start();

    // Test timer
    XorpTimer wakeywakey = eventloop.new_periodic_ms(
	1000,
	callback(wakeup_hook));
    XorpTimer wakeywakey2 = eventloop.new_periodic_ms(
	5000,
	callback(wakeup_hook2, 3, 5));
    XorpTimer wakeywakey3 = eventloop.new_periodic_ms(
	2000,
	callback(f, &Foo::print));

    //
    // Main loop
    //
    for (;;) {
	eventloop.run();
    }
}

static bool
wakeup_hook()
{
    fprintf(stdout, "%s\n", xlog_localtime2string());
    fflush(stdout);

    return (true);
}

static bool
wakeup_hook2(int a, int b)
{
    fprintf(stdout, "%s ARGS = %d %d\n", xlog_localtime2string(), a, b);
    fflush(stdout);

    return (true);
}

int
cli_myset_func(const string& ,		// server_name
	       const string& cli_term_name,
	       uint32_t ,		// cli_session_id
	       const vector<string>& command_global_name,
	       const vector<string>& argv)
{
    CliClient *cli_client = cli_node().find_cli_by_term_name(cli_term_name);
    if (cli_client == NULL)
	return (XORP_ERROR);

    string command_line = token_vector2line(command_global_name);
    if (command_global_name.size() > 0)
	cli_client->cli_print(c_format("MYSET_FUNC command_global_name = %s\n",
				       command_line.c_str()));
    for (size_t i = 0; i < argv.size(); i++)
	cli_client->cli_print(c_format("MYSET_FUNC arg = %s\n",
				       argv[i].c_str()));

    return (XORP_OK);
}

int
cli_print(const string& ,		// server_name
	  const string& cli_term_name,
	  uint32_t ,			// cli_session_id
	  const vector<string>& ,	// command_global_name,
	  const vector<string>& argv)
{
    CliClient *cli_client = cli_node().find_cli_by_term_name(cli_term_name);
    if (cli_client == NULL)
	return (XORP_ERROR);

    if (argv.size() > 0) {
	cli_client->cli_print("Error: unexpected arguments\n");
	return (XORP_ERROR);
    }

    //
    // Test to print a number of lines at once
    //
    string my_string = "";
    for (int i = 0; i < 100; i++)
	my_string += c_format("This is my multi-line number %u\n", i);
    cli_client->cli_print(my_string);

    //
    // Test to print a number of lines one-by-one
    //
    for (int i = 0; i < 100; i++)
	cli_client->cli_print(c_format("This is my line number %u\n", i));


    return (XORP_OK);
}

int
cli_print2(const string& ,		// server_name
	   const string& cli_term_name,
	   uint32_t ,			// cli_session_id
	   const vector<string>& ,	// command_global_name,
	   const vector<string>& argv)
{
    CliClient *cli_client = cli_node().find_cli_by_term_name(cli_term_name);
    if (cli_client == NULL)
	return (XORP_ERROR);

    if (argv.size() > 0) {
	cli_client->cli_print("Error: unexpected arguments\n");
	return (XORP_ERROR);
    }

    for (int i = 0; i < 10; i++)
	cli_client->cli_print(c_format("This is my line number %d\n", i));

    return (XORP_OK);
}

int
cli_print2_newline(const string& ,		// server_name
		   const string& cli_term_name,
		   uint32_t ,			// cli_session_id
		   const vector<string>& ,	// command_global_name,
		   const vector<string>& argv)
{
    CliClient *cli_client = cli_node().find_cli_by_term_name(cli_term_name);
    if (cli_client == NULL)
	return (XORP_ERROR);

    if (argv.size() > 0) {
	cli_client->cli_print("Error: unexpected arguments\n");
	return (XORP_ERROR);
    }

    for (int i = 0; i < 10; i++)
	cli_client->cli_print(c_format("This is my line number %d\n", i));
    cli_client->cli_print(c_format("\n"));

    return (XORP_OK);
}

int
cli_print_wide(const string& ,		// server_name
	       const string& cli_term_name,
	       uint32_t ,		// cli_session_id
	       const vector<string>& ,	// command_global_name,
	       const vector<string>& argv)
{
    CliClient *cli_client = cli_node().find_cli_by_term_name(cli_term_name);
    if (cli_client == NULL)
	return (XORP_ERROR);

    if (argv.size() > 0) {
	cli_client->cli_print("Error: unexpected arguments\n");
	return (XORP_ERROR);
    }

    string trail = "111111111122222222223333333333444444444455555555556666666666777777777788888888889999999999";
#if 1
    for (int i = 0; i < 200; i++)
	cli_client->cli_print(c_format("This is my line number %d %s %d\n",
				       i, trail.c_str(), i));
#endif

    return (XORP_OK);
}

int
cli_print_test(const string& ,		// server_name
	       const string& cli_term_name,
	       uint32_t ,		// cli_session_id
	       const vector<string>& ,	// command_global_name,
	       const vector<string>& argv)
{
    CliClient *cli_client = cli_node().find_cli_by_term_name(cli_term_name);
    if (cli_client == NULL)
	return (XORP_ERROR);

    if (argv.size() > 0) {
	cli_client->cli_print("Error: unexpected arguments\n");
	return (XORP_ERROR);
    }

    string s = "\
Group           Source          RP              Flags\n\
ff0e::8320:1    ::              ffff:fff:ffff:300:1:: WC   \n\
    Upstream interface (RP):   gif0\n\
    Upstream MRIB next hop (RP): fe80::ffff:19\n\
    Upstream RPF'(*,G):        fe80::ffff:19\n\
    Upstream state:            Joined \n\
    Join timer:                31\n\
    Local receiver include WC: .........O..\n\
    Joins RP:                  ............\n\
    Joins WC:                  ............\n\
    Join state:                ............\n\
    Prune state:               ............\n\
    Prune pending state:       ............\n\
    I am assert winner state:  .........O..\n\
    I am assert loser state:   ............\n\
    Assert winner WC:          .........O..\n\
    Assert lost WC:            ............\n\
    Assert tracking WC:        .....O...O..\n\
    Could assert WC:           .........O..\n\
    I am DR:                   .....O...O..\n\
    Immediate olist RP:        ............\n\
    Immediate olist WC:        .........O..\n\
    Inherited olist SG:        .........O..\n\
    Inherited olist SG_RPT:    .........O..\n\
    PIM include WC:            .........O..\n\
ff0e::8320:1    ffff:700:0:fff2::20 ffff:fff:ffff:300:1:: SG_RPT DirectlyConnectedS \n\
    Upstream interface (S):    rl0\n\
    Upstream interface (RP):   gif0\n\
    Upstream MRIB next hop (RP): fe80::ffff:19\n\
    Upstream RPF'(S,G,rpt):    fe80::ffff:19\n\
    Upstream state:            Pruned \n\
    Override timer:            -1\n\
    Local receiver include WC: .........O..\n\
    Joins RP:                  ............\n\
    Joins WC:                  ............\n\
    Prunes SG_RPT:             ............\n\
    Join state:                ............\n\
    Prune state:               ............\n\
    Prune pending state:       ............\n\
    Prune tmp state:           ............\n\
    Prune pending tmp state:   ............\n\
    Assert winner WC:          .........O..\n\
    Assert lost WC:            ............\n\
    Assert lost SG_RPT:        ............\n\
    Could assert WC:           .........O..\n\
    Could assert SG:           ...........O\n\
    I am DR:                   .....O...O..\n\
    Immediate olist RP:        ............\n\
    Immediate olist WC:        .........O..\n\
    Inherited olist SG:        .........O..\n\
    Inherited olist SG_RPT:    .........O..\n\
    PIM include WC:            .........O..\n\
ff0e::8320:1    ffff:700:0:fff2::20 ffff:fff:ffff:300:1:: SG SPT DirectlyConnectedS \n\
    Upstream interface (S):    rl0\n\
    Upstream interface (RP):   gif0\n\
    Upstream MRIB next hop (RP): fe80::ffff:19\n\
    Upstream MRIB next hop (S):  UNKNOWN\n\
    Upstream RPF'(S,G):        UNKNOWN\n\
    Upstream state:            Joined \n\
    Register state:            RegisterJoin RegisterCouldRegister \n\
    Join timer:                39\n\
    Local receiver include WC: .........O..\n\
    Local receiver include SG: ............\n\
    Local receiver exclude SG: ............\n\
    Joins RP:                  ............\n\
    Joins WC:                  ............\n\
    Joins SG:                  ...........O\n\
    Join state:                ...........O\n\
    Prune state:               ............\n\
    Prune pending state:       ............\n\
    I am assert winner state:  ............\n\
    I am assert loser state:   ............\n\
    Assert winner WC:          .........O..\n\
    Assert winner SG:          ............\n\
    Assert lost WC:            ............\n\
    Assert lost SG:            ............\n\
    Assert lost SG_RPT:        ............\n\
    Assert tracking SG:        .........O.O\n\
    Could assert WC:           .........O..\n\
    Could assert SG:           ...........O\n\
    I am DR:                   .....O...O..\n\
    Immediate olist RP:        ............\n\
    Immediate olist WC:        .........O..\n\
    Immediate olist SG:        ...........O\n\
    Inherited olist SG:        .........O.O\n\
    Inherited olist SG_RPT:    .........O..\n\
    PIM include WC:            .........O..\n\
    PIM include SG:            ............\n\
    PIM exclude SG:            ............\n\
";

    cli_client->cli_print(s);

    return (XORP_OK);
}

int
add_my_cli_commands(CliNode& cli_node)
{
    CliCommand *com0, *com1, *com2, *com3;
    string error_msg;

    com0 = cli_node.cli_command_root();
    com2 = com0->add_command("myset", "Set my variable", true, cli_myset_func,
			     error_msg);
    com1 = com0->add_command("myshow", "Show my information", "Myshow> ",
			     true, error_msg);
    com2 = com0->add_command("myshow version",
			     "Show my information about system", true,
			     error_msg);
    com3 = com0->add_command("myshow version pim",
			     "Show my information about PIM", true, error_msg);
    com3 = com0->add_command("myshow version igmp",
			     "Show my information about IGMP", true, error_msg);

    com1 = com0->add_command("myset2", "Set my variable2", true,
			     cli_myset_func, error_msg);
    com1 = com0->add_command("myshow2", "Show my information2", "Myshow2> ",
			     true, error_msg);

    com1 = com0->add_command("print", "Print numbers", true, cli_print,
			     error_msg);
    com1 = com0->add_command("print2", "Print few numbers", true, cli_print2,
			     error_msg);
    com1 = com0->add_command("print2 newline",
			     "Print few numbers with an extra newline",
			     true, cli_print2_newline, error_msg);
    com1 = com0->add_command("print_wide", "Print wide lines", true,
			     cli_print_wide, error_msg);
    com1 = com0->add_command("print_test", "Print test lines", true,
			     cli_print_test, error_msg);

    return (XORP_OK);
}

int
main(int argc, char *argv[])
{
    int ch;
    string::size_type idx;
    const char *argv0 = argv[0];
    string finder_hostname = FinderConstants::FINDER_DEFAULT_HOST().str();
    uint16_t finder_port = FinderConstants::FINDER_DEFAULT_PORT();

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);	// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    //
    // Get the program options
    //
    while ((ch = getopt(argc, argv, "F:h")) != -1) {
	switch (ch) {
	case 'F':
	    // Finder hostname and port
	    idx = finder_hostname.find(':');
	    if (idx != string::npos) {
		if (idx + 1 >= finder_hostname.length()) {
		    // No port number
		    usage(argv0, 1);
		    // NOTREACHED
		}
		char* p = &finder_hostname[idx + 1];
		finder_port = static_cast<uint16_t>(atoi(p));
		finder_hostname = finder_hostname.substr(0, idx);
	    }
	    break;
	case 'h':
	case '?':
	    // Help
	    usage(argv0, 0);
	    break;
	default:
	    usage(argv0, 1);
	    break;
	}
    }
    argc -= optind;
    argv += optind;
    if (argc != 0) {
	usage(argv0, 1);
	// NOTREACHED
    }

    //
    // Run everything
    //
    try {
	cli_main(finder_hostname, finder_port, true);
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    exit (0);
}
