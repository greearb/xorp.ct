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

#ident "$XORP$"

#include <string>
#include <list>

#include "config.h"
#include "xorp.h"
#include "xlog.h"
#include "callback.hh"

/**
 * This class is passed as the first argument to each test function/method.
 */
class TestInfo {
public:
    TestInfo(string myname, bool verbose, int verbose_level, ostream& o) :
	_myname(myname), _verbose(verbose), _verbose_level(verbose_level),
	_ostream(o)
    {
    }

    /*
     * @return The name of the current test.
     */
    string test_name()
    {
	return _myname;
    }

    /*
     * @return True if the verbose flag has been enabled.
     */
    bool
    verbose()
    {
	return _verbose;
    }
    
    /*
     * @return The verbose level.
     */
    int
    verbose_level()
    {
	return _verbose_level;
    }

    /*
     * @return The stream to which output should be sent.
     */
    ostream& out()
    {
	return _ostream;
    }
private:
    string _myname;
    bool _verbose;
    int _verbose_level;
    ostream& _ostream;
    
};

/**
 * A helper class for test programs.
 *
 * This class is used to parse the command line arguments, enable the
 * xlog_ framework and return the exit status from the test
 * functions/methods. An example of how to use this class can be found
 * in test_test_main.cc.
 *
 */
class TestMain {
public:
    /**
     * A test function/method should return one of these values.
     */
    enum { SUCCESS = 0, FAILURE = 1};

    /**
     * Start the parsing of command line arguments and enable xlog_*.
     */
    TestMain(int argc, char **argv) :
	_verbose(false), _verbose_level(0), _exit_status(SUCCESS)
    {
	_progname = argv[0];

	xlog_begin(argv[0]);

	for(int i = 1; i < argc; i++) {
	    string argname;
	    string argvalue = "";
	    Arg a;
	    // Argument flag
	    if(argv[i][0] == '-') {
		// Long form argument.
		if(argv[i][1] == '-') {
		    argname = argv[i];
		} else {
		    argname = argv[i][1];
		    argname = "-" + argname;
		    if('\0' != argv[i][2]) {
			argvalue = &argv[i][2];
		    }
		}
		// Try and get the argument value if we don't already
		// have it.
		if("" == argvalue && (i + 1) < argc) {
		    if(argv[i + 1][0] != '-') {
			i++;
			argvalue = argv[i];
		    }
		}
		if("" == argvalue)
		    a = Arg(Arg::FLAG, argname);
		else
		    a = Arg(Arg::VALUE, argname, argvalue);
	    } else {
		a = Arg(Arg::REST, argv[i]);
	    }
	    _args.push_back(a);
	}
    }

    /**
     * Get an optional argument from the command line.
     *
     * @param short_form The short form of the argument e.g. "-t".
     * @param long_form The long form of the argument
     * e.g. "--testing".
     * @param description The description of this argument that will
     * be used in the usage message.
     * @return the argument or "" if not found or an error occured in
     * previous parsing of the arguments.
     */
    string
    get_optional_args(const string &short_form, const string &long_form,
		      const string &description)
    {
	_usage += short_form + "|" + long_form + " arg\t" + description + "\n";

	if(SUCCESS != _exit_status)
	    return "";
	list<Arg>::iterator i;
	for(i = _args.begin(); i != _args.end(); i++) {
	    if(short_form == i->name() || long_form == i->name()) {
		bool has_value;
		string value;
		value = i->value(has_value);
		if(!has_value) {
		    _exit_status = FAILURE;
		    return "";
		}
		_args.erase(i);
		return value;
	    }
	}
	return "";
    }

    /**
     * Get an optional flag from the command line.
     *
     * @param short_form The short form of the argument e.g. "-t".
     * @param long_form The long form of the argument
     * e.g. "--testing".
     * @param description The description of this argument that will
     * be used in the usage message.
     * @return true if the flag is present or false found or an error
     * occured in previous parsing of the arguments.
     */
    bool
    get_optional_flag(const string &short_form, const string &long_form,
		      const string &description)
    {
	_usage += short_form + "|" + long_form + " arg\t" + description + "\n";

	if(SUCCESS != _exit_status)
	    return false;
	list<Arg>::iterator i;
	for(i = _args.begin(); i != _args.end(); i++) {
	    if(short_form == i->name() || long_form == i->name()) {
		_args.erase(i);
		return true;
	    }
	}
	return false;
    }

    /**
     * Complete parsing the arguments.
     *
     * Process generic arguments and verify that there are no
     * arguments left unprocessed.
     */
    void
    complete_args_parsing()
    {
	_verbose = get_optional_flag("-v", "--verbose", "Verbose");

	string level = get_optional_args("-l",
					 "--verbose-level","Verbose level");
	if("" != level)
	    _verbose_level = atoi(level.c_str());

	bool h = get_optional_flag("-h", "--help","Print help information");
	bool q = get_optional_flag("-?", "--help","Print help information");

	if(h || q) {
	    cerr << usage();
	    xlog_end();
	    ::exit(0);
	}

	if(!_args.empty()) {
	    list<Arg>::iterator i;
	    for(i = _args.begin(); i != _args.end(); i++) {
		cerr << "Unused argument: " << i->name() << endl;
	    }
	    cerr << usage();
	    _exit_status = FAILURE;
	}
    }

    /**
     *
     * Run a test function/method. The test function/method is passed
     * a TestInfo. The test function/method should return
     * "TestMain::SUCCESS" for sucess and "TestMain::FAILURE" for
     * failure.
     *
     * To run a function call "test":
     * run("test", callback(test));
     *
     * @param test_name The name of the test.
     * @param cb Callback object.
     */
    void
    run(string test_name, XorpCallback1<int, TestInfo&>::RefPtr cb)
    {
	if(SUCCESS != _exit_status)
	    return;
	if(_verbose)
	    cout << "Running: " << test_name << endl;
	TestInfo info(test_name, _verbose, _verbose_level, cout);
	if(SUCCESS != cb->dispatch(info)) {
	   _exit_status = FAILURE;
	   cerr << "Failed: " << test_name << endl;
	}
    }

    /**
     * @return The usage string.
     */
    const string
    usage()
    {
	return "Usage " + _progname + ":\n" + _usage;
    }

    /**
     * Mark the tests as having failed. Used for setting an error
     * condition from outside a test.
     *
     * @param error Error string.
     */
    void
    failed(string error)
    {
	_error_string += error;
	_exit_status = FAILURE;
    }

    /**
     * Must be called at the end of the tests.
     *
     * @return The status of the tests. Should be passed to exit().
     */
    int
    exit()
    {
	if("" != _error_string)
	    cerr << _error_string;

	xlog_end();

	return _exit_status;
    }

private:
    class Arg;
    string _progname;
    list<Arg> _args;
    bool _verbose;
    int _verbose_level;
    int _exit_status;
    string _error_string;
    string _usage;

    void
    xlog_begin(const char *progname)
    {
	//
	// Initialize and start xlog
	//
	xlog_init(progname, NULL);
	xlog_set_verbose(XLOG_VERBOSE_LOW);	// Least verbose messages
	// XXX: verbosity of the error messages temporary increased
	xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
	xlog_level_set_verbose(XLOG_LEVEL_WARNING, XLOG_VERBOSE_HIGH);
	xlog_add_default_output();
	xlog_start();
    }

    void
    xlog_end()
    {
	//
	// Gracefully stop and exit xlog
	//
	xlog_stop();
	xlog_exit();
    }

    class Arg {
    public:
	typedef enum {FLAG, VALUE, REST} arg_type;
	Arg() {}

	Arg(arg_type a, string name, string value = "")
	    : _arg_type(a), _name(name), _value(value)
	{
	    // 	debug_msg("Argument type = %d flag name = %s value = %s\n",
	    // 		  a, name.c_str(), value.c_str());
	}

	Arg(const Arg& rhs)
	{
	    copy(rhs);
	}

	Arg operator=(const Arg& rhs)
	{
	    if(&rhs == this)
		return *this;
	    copy(rhs);

	    return *this;
	}
    
	void
	copy(const Arg& rhs)
	{
	    _arg_type = rhs._arg_type;
	    _name = rhs._name;
	    _value = rhs._value;
	}

	const string&
	name()
	{
	    return _name;
	}

	const string&
	value(bool& has_value)
	{
	    if(VALUE != _arg_type) {
		cerr << "Argument " << _name <<
		    " was not provided with a value\n";
		has_value = false;
	    } else
		has_value = true;

	    return _value;
	}

    private:
	arg_type _arg_type;
	string _name;
	string _value;
    };
};
