// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#include "libxorp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/run_command.hh"


//
// XXX: MODIFY FOR YOUR TEST PROGRAM
//
static const char *program_name		= "test_run_command";
static const char *program_description	= "Test a class for running an external program";
static const char *program_version_id	= "0.1";
static const char *program_date		= "November 22, 2004";
static const char *program_copyright	= "See file LICENSE.XORP";
static const char *program_return_value	= "0 on success, 1 if test error, 2 if internal error";

static bool s_verbose = false;
bool verbose()			{ return s_verbose; }
void set_verbose(bool v)	{ s_verbose = v; }

static int s_failures = 0;
bool failures()			{ return s_failures; }
void incr_failures()		{ s_failures++; }



//
// printf(3)-like facility to conditionally print a message if verbosity
// is enabled.
//
#define verbose_log(x...) _verbose_log(__FILE__,__LINE__, x)

#define _verbose_log(file, line, x...)					\
do {									\
    if (verbose()) {							\
	printf("From %s:%d: ", file, line);				\
	printf(x);							\
    }									\
} while(0)


//
// Test and print a message whether two strings are lexicographically same.
// The strings can be either C or C++ style.
//
#define verbose_match(s1, s2)						\
    _verbose_match(__FILE__, __LINE__, s1, s2)

bool
_verbose_match(const char* file, int line, const string& s1, const string& s2)
{
    bool match = s1 == s2;

    _verbose_log(file, line, "Comparing %s == %s : %s\n",
		 s1.c_str(), s2.c_str(), match ? "OK" : "FAIL");
    if (match == false)
	incr_failures();
    return match;
}


//
// Test and print a message whether a condition is true.
//
// The first argument is the condition to test.
// The second argument is a string with a brief description of the tested
// condition.
//
#define verbose_assert(cond, desc) 					\
    _verbose_assert(__FILE__, __LINE__, cond, desc)

bool
_verbose_assert(const char* file, int line, bool cond, const string& desc)
{
    _verbose_log(file, line,
		 "Testing %s : %s\n", desc.c_str(), cond ? "OK" : "FAIL");
    if (cond == false)
	incr_failures();
    return cond;
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
    fprintf(stderr, "Return 0 on success, 1 if test error, 2 if internal error.\n");
}


//
// Local state
//
static bool is_interrupted = false;

static void
signal_handler(int sig)
{
    is_interrupted = true;
    UNUSED(sig);
}

/**
 * @short A class used for testing of @see RunCommand.
 */
class TestRunCommand {
public:
    TestRunCommand() {
	clear();
    }

    void clear() {
	_is_stdout_received = false;
	_is_stderr_received = false;
	_is_done_received = false;
	_is_done_failed = false;
	_stdout_msg.erase();
	_stderr_msg.erase();
	_done_error_msg.erase();
    }

    bool is_stdout_received() const { return _is_stdout_received; }
    bool is_stderr_received() const { return _is_stderr_received; }
    bool is_done_received() const { return _is_done_received; }
    bool is_done_failed() const { return _is_done_failed; }
    const string& stdout_msg() const { return _stdout_msg; }
    const string& stderr_msg() const { return _stderr_msg; }
    const string& done_error_msg() const { return _done_error_msg; }

    void command_stdout_cb(const string& output) {
	_is_stdout_received = true;
	_stdout_msg += output;
    }

    void command_stderr_cb(const string& output) {
	_is_stderr_received = true;
	_stderr_msg += output;
    }

    void command_done_cb(RunCommand* run_command, bool success,
			 const string& error_msg) {
	_is_done_received = true;
	if (! success) {
	    _is_done_failed = !success;
	    _done_error_msg += error_msg;
	}
	UNUSED(run_command);
    }

private:
    bool	_is_stdout_received;
    bool	_is_stderr_received;
    bool	_is_done_received;
    bool	_is_done_failed;
    string	_stdout_msg;
    string	_stderr_msg;
    string	_done_error_msg;
};

/**
 * Test RunCommand executing an invalid command.
 */
static void
test_execute_invalid_command()
{
    EventLoop eventloop;
    TestRunCommand test_run_command;
    bool done = false;

    //
    // Try to start an invalid command.
    //
    RunCommand run_command(eventloop,
			   "/no/such/command",
			   "--no-such-flags",
			   callback(test_run_command,
				    &TestRunCommand::command_stdout_cb),
			   callback(test_run_command,
				    &TestRunCommand::command_stderr_cb),
			   callback(test_run_command,
				    &TestRunCommand::command_done_cb));

    int ret_value = run_command.execute();
    if (ret_value != XORP_OK) {
	verbose_assert(true, "Executing an invalid command");
	return;
    }

    XorpTimer timeout_timer = eventloop.set_flag_after(TimeVal(10, 0),
						       &done, true);
    while (!done) {
	if (is_interrupted
	    || test_run_command.is_done_received()) {
	    break;
	}
	eventloop.run();
    }

    if (is_interrupted) {
	verbose_log("Command interrupted by user\n");
	incr_failures();
	run_command.terminate();
	return;
    }

    verbose_assert(test_run_command.is_done_received()
		   && test_run_command.is_done_failed(),
		   "Executing an invalid command");
    run_command.terminate();
}

/**
 * Test RunCommand executing a command with invalid arguments.
 */
static void
test_execute_invalid_arguments()
{
    EventLoop eventloop;
    TestRunCommand test_run_command;
    bool done = false;

    //
    // Try to start an invalid command.
    //
    RunCommand run_command(eventloop,
			   "/bin/sleep",
			   "--no-such-flags",
			   callback(test_run_command,
				    &TestRunCommand::command_stdout_cb),
			   callback(test_run_command,
				    &TestRunCommand::command_stderr_cb),
			   callback(test_run_command,
				    &TestRunCommand::command_done_cb));

    int ret_value = run_command.execute();
    if (ret_value != XORP_OK) {
	verbose_assert(true, "Executing a command with invalid arguments");
	return;
    }

    XorpTimer timeout_timer = eventloop.set_flag_after(TimeVal(10, 0),
						       &done, true);
    while (!done) {
	if (is_interrupted
	    || test_run_command.is_done_received()) {
	    break;
	}
	eventloop.run();
    }

    if (is_interrupted) {
	verbose_log("Command interrupted by user\n");
	incr_failures();
	run_command.terminate();
	return;
    }

    verbose_assert(test_run_command.is_done_received()
		   && test_run_command.is_done_failed(),
		   "Executing a command with invalid arguments");
    run_command.terminate();
}

/**
 * Test RunCommand executing and terminating a command.
 */
static void
test_execute_terminate_command()
{
    EventLoop eventloop;
    TestRunCommand test_run_command;
    bool done = false;

    //
    // Start a sleep(1) command that should not terminate anytime soon
    //
    RunCommand run_command(eventloop,
			   "/bin/sleep",
			   "10000",
			   callback(test_run_command,
				    &TestRunCommand::command_stdout_cb),
			   callback(test_run_command,
				    &TestRunCommand::command_stderr_cb),
			   callback(test_run_command,
				    &TestRunCommand::command_done_cb));

    if (run_command.execute() != XORP_OK) {
	verbose_log("Cannot execute command %s : FAIL\n",
		    run_command.command().c_str());
	incr_failures();
	return;
    }

    XorpTimer timeout_timer = eventloop.set_flag_after(TimeVal(10, 0),
						       &done, true);
    while (!done) {
	if (is_interrupted
	    || test_run_command.is_stdout_received()
	    || test_run_command.is_stderr_received()
	    || test_run_command.is_done_received()) {
	    break;
	}
	eventloop.run();
    }

    if (is_interrupted) {
	verbose_log("Command interrupted by user\n");
	incr_failures();
	run_command.terminate();
	return;
    }
    if (test_run_command.is_stdout_received()) {
	verbose_log("Unexpected stdout output (%s) : FAIL\n",
		    test_run_command.stdout_msg().c_str());
	incr_failures();
	run_command.terminate();
	return;
    }
    if (test_run_command.is_stderr_received()) {
	verbose_log("Unexpected stderr output (%s) : FAIL\n",
		    test_run_command.stderr_msg().c_str());
	incr_failures();
	run_command.terminate();
	return;
    }
    if (test_run_command.is_done_received()) {
	verbose_log("Unexpected command termination : FAIL\n");
	incr_failures();
	run_command.terminate();
	return;
    }

    verbose_assert(true, "Executing and terminating a command");
    run_command.terminate();
}

/**
 * Test RunCommand command stdout reading.
 */
static void
test_command_stdout_reading()
{
    EventLoop eventloop;
    TestRunCommand test_run_command;
    bool done = false;
    string stdout_msg_in = "Line1 on stdout";
    string stdout_msg_out = stdout_msg_in;

    //
    // Try to start an invalid command.
    //
    string args = c_format("'BEGIN { "
			   "printf(\"%s\") > \"/dev/stdout\"; "
			   "exit 0;}'",
			   stdout_msg_in.c_str());

    RunCommand run_command(eventloop,
			   "/usr/bin/awk",
			   args,
			   callback(test_run_command,
				    &TestRunCommand::command_stdout_cb),
			   callback(test_run_command,
				    &TestRunCommand::command_stderr_cb),
			   callback(test_run_command,
				    &TestRunCommand::command_done_cb));

    int ret_value = run_command.execute();
    if (ret_value != XORP_OK) {
	verbose_assert(false, "Command stdout reading");
	return;
    }

    XorpTimer timeout_timer = eventloop.set_flag_after(TimeVal(10, 0),
						       &done, true);
    while (!done) {
	if (is_interrupted
	    || test_run_command.is_done_received()) {
	    break;
	}
	eventloop.run();
    }

    if (is_interrupted) {
	verbose_log("Command interrupted by user\n");
	incr_failures();
	run_command.terminate();
	return;
    }

    bool success = false;
    do {
	if (! test_run_command.is_done_received())
	    break;
	if (test_run_command.is_done_failed())
	    break;
	if (test_run_command.is_stderr_received())
	    break;
	if (! test_run_command.is_stdout_received())
	    break;
	if (test_run_command.stdout_msg() != stdout_msg_out)
	    break;
	success = true;
	break;
    } while (false);

    verbose_assert(success, "Command stdout reading");
    run_command.terminate();
}

/**
 * Test RunCommand command stderr reading.
 */
static void
test_command_stderr_reading()
{
    EventLoop eventloop;
    TestRunCommand test_run_command;
    bool done = false;
    string stdout_msg_in = "Line1 on stdout";
    string stderr_msg_in = "Line1 on stderr";
    string stderr_msg_out = stderr_msg_in;

    //
    // Try to start an invalid command.
    //
    string args = c_format("'BEGIN { "
			   "printf(\"%s\") > \"/dev/stdout\"; "
			   "printf(\"%s\") > \"/dev/stderr\"; "
			   "exit 0;}'",
			   stdout_msg_in.c_str(),
			   stderr_msg_in.c_str());

    RunCommand run_command(eventloop,
			   "/usr/bin/awk",
			   args,
			   callback(test_run_command,
				    &TestRunCommand::command_stdout_cb),
			   callback(test_run_command,
				    &TestRunCommand::command_stderr_cb),
			   callback(test_run_command,
				    &TestRunCommand::command_done_cb));

    int ret_value = run_command.execute();
    if (ret_value != XORP_OK) {
	verbose_assert(false, "Command stderr reading");
	return;
    }

    XorpTimer timeout_timer = eventloop.set_flag_after(TimeVal(10, 0),
						       &done, true);
    while (!done) {
	if (is_interrupted
	    || test_run_command.is_done_received()) {
	    break;
	}
	eventloop.run();
    }

    if (is_interrupted) {
	verbose_log("Command interrupted by user\n");
	incr_failures();
	run_command.terminate();
	return;
    }

    bool success = false;
    do {
	if (! test_run_command.is_done_received())
	    break;
	if (! test_run_command.is_done_failed())
	    break;
#if 0
	//
	// XXX: if there is output on stderr, this is an error and
	// we have to check TestRunCommand::done_error_msg()
	//
	if (! test_run_command.is_stdout_received())
	    break;
	if (! test_run_command.is_stderr_received())
	    break;
	if (test_run_command.stderr_msg() != stderr_msg_out)
	    break;
#endif
	if (test_run_command.done_error_msg() != stderr_msg_out)
	    break;
	success = true;
	break;
    } while (false);

    verbose_assert(success, "Command stderr reading");
    run_command.terminate();
}

/**
 * Test RunCommand command termination failure.
 */
static void
test_command_termination_failure()
{
    EventLoop eventloop;
    TestRunCommand test_run_command;
    bool done = false;
    string stdout_msg_in = "Line1 on stdout";
    string stdout_msg_out = stdout_msg_in;

    //
    // Try to start an invalid command.
    //
    string args = c_format("'BEGIN { "
			   "printf(\"%s\") > \"/dev/stdout\"; "
			   "exit 1;}'",
			   stdout_msg_in.c_str());

    RunCommand run_command(eventloop,
			   "/usr/bin/awk",
			   args,
			   callback(test_run_command,
				    &TestRunCommand::command_stdout_cb),
			   callback(test_run_command,
				    &TestRunCommand::command_stderr_cb),
			   callback(test_run_command,
				    &TestRunCommand::command_done_cb));

    int ret_value = run_command.execute();
    if (ret_value != XORP_OK) {
	verbose_assert(false, "Command termination failure");
	return;
    }

    XorpTimer timeout_timer = eventloop.set_flag_after(TimeVal(10, 0),
						       &done, true);
    while (!done) {
	if (is_interrupted
	    || test_run_command.is_done_received()) {
	    break;
	}
	eventloop.run();
    }

    if (is_interrupted) {
	verbose_log("Command interrupted by user\n");
	incr_failures();
	run_command.terminate();
	return;
    }

    bool success = false;
    do {
	if (! test_run_command.is_done_received())
	    break;
	if (! test_run_command.is_done_failed())
	    break;
	if (test_run_command.is_stderr_received())
	    break;
	if (! test_run_command.is_stdout_received())
	    break;
	if (test_run_command.stdout_msg() != stdout_msg_out)
	    break;
	success = true;
	break;
    } while (false);

    verbose_assert(success, "Command termination failure");
    run_command.terminate();
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
	//
	// Intercept the typical signals that the user may use to stop
	// the program.
	//
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	test_execute_invalid_command();
	test_execute_invalid_arguments();
	test_execute_terminate_command();
	test_command_stdout_reading();
	test_command_stderr_reading();
	test_command_termination_failure();
	ret_value = failures() ? 1 : 0;
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
