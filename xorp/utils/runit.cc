// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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



#include "libxorp/xorp.h"
#include "libxorp/utils.hh"




#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#ifdef HAVE_PROCESS_H
#include <process.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <signal.h>

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

typedef pid_t PID_T;
#define INVALID_PID (0)

/**
 * For a lot of our testing from shell scripts it is necessary to have
 * a number of subsidiary programs running before starting the main
 * script. For example most of out programs require the finder to be
 * running.
 *
 * The absolute path names of the subsidiary programs that need to be
 * started are passed on the stardard input.
 * 
 * The absolute pathname of the test script is then passed in as a
 * command line argument ("-c").
 *
 * Once the test script exits all the subsidiary programs are sent a
 * SIGTERM. If any of the subsidiary programs exits before the test
 * script terminates the test script is sent a SIGTERM.
 *
 * The exit status from the test script is the exit status returned by
 * program.
 *
 * By default the output of the subsidiary programs is sent to
 * DEVNULL the "-v" flag stops this redirection. The "-q" sends
 * all output to DEVNULL.
 */

#define DEVNULL "/dev/null"
#define SLEEP_CMD "/bin/sleep 2"

/**
 * Split a line into multple tokens.
 * @param str line.
 * @param tokens tokens.
 * @param delimiters optional delimiter.
 */
void 
tokenize(const string& str,
	 vector<string>& tokens,
	 const string& delimiters = " ")
{
    string::size_type begin = str.find_first_not_of(delimiters, 0);
    string::size_type end = str.find_first_of(delimiters, begin);

    while (string::npos != begin || string::npos != end) {
        tokens.push_back(str.substr(begin, end - begin));
        begin = str.find_first_not_of(delimiters, end);
        end = str.find_first_of(delimiters, begin);
    }
}

/**
 * Start a new process in the background.
 * @param process Absolute pathname and arguments.
 * @param output Optional file to redirect file descriptors 0 and 1.
 * @return Return the process id of the new process.
 */
PID_T
xorp_spawn(const string& process, const char *output = NULL)
{
    PID_T pid;
    switch (pid = fork()) {
    case 0:
	{
	    if (output != NULL) {
		close(0);
		close(1);
// 		close(2);
		open(output, O_RDONLY);
		open(output, O_WRONLY);
//		open(output, O_WRONLY);
	    }

	    vector<string> tokens;
	    tokenize(process, tokens);
	    char *argv[tokens.size() + 1];
	    for (unsigned int i = 0; i < tokens.size(); i++) {
		argv[i] = const_cast<char *>(tokens[i].c_str());
	    }
	    argv[tokens.size()] = 0;

	    /*
	    ** Unblock any blocked signals.
	    */
	    sigset_t set;
	    if (0 != sigfillset(&set)) {
		cerr << "sigfillset failed: " << strerror(errno) << endl;
		exit(-1);
	    }
	    
	    if (0 != sigprocmask(SIG_UNBLOCK, &set, 0)) {
		cerr << "sigprockmask failed: " << strerror(errno) << endl;
		exit(-1);
	    }

	    errno = 0;
	    execvp(argv[0], argv);
	    cerr << "\nFailed to exec: " << process << " errno: "
		 << strerror(errno) << " argv[0]: " << argv[0]
		 << " argv: ";

	    for (unsigned int i = 0; i<tokens.size(); i++) {
		cerr << " arg [" << i << "] = -:" << argv[i] << ":-" << endl;
	    }
	    // Can't call the regular exit as it will cause tidy() to
	    // be run.
	    _exit(-1);
	}
	break;
    case -1:
	break;
    default:
// 	    cout << "command: " << process << " pid: " << pid << endl;
	    return pid;
    }

    return -1;
}

/**
 * The command and its associated process ID.
 */
struct Command {
    Command(string command, string wait_command) : 
	_command(command), 
	_wait_command(wait_command),
	_pid(INVALID_PID)
    {}
#ifdef XORP_USE_USTL
    Command() { _pid = -1;}
#endif

    string _command;	// The actual command that we wish to run.
    string _wait_command;// The command to run that exits when the
			 // command is ready.
    PID_T _pid;
};

/**
 * vector of all commands.
 */
vector<Command> commands;
/**
 * Process ID of main script/program.
 */
PID_T cpid;

/**
 * Process ID of wait script.
 */
PID_T wait_command_pid;
string wait_command;

bool core_dump = false;

/**
 * Signal handler that reaps dead children.
 */
void
sigchld(int)
{
    int status;
    PID_T pid = wait(&status);

    if (wait_command_pid == pid) {
	wait_command_pid = INVALID_PID;
	if (status == 0) {
	    return;
	}

	cerr << "Error running command: " << wait_command << endl;
	if (status == -1) {
	    cerr << " (execv of /bin/sh failed), possible error: "
		 << strerror(errno) << endl;
	}
	else {
	    if (WIFEXITED(status)) {
		int child_rv = WEXITSTATUS(status);
		cerr << " exited with not zero status: "
		     << child_rv << " status: "
		     << status << "  possible error: " << strerror(errno)
		     << endl;
	    }
	    else {
		if (WIFSIGNALED(status)) {
		    int csig = WTERMSIG(status);
		    if (errno == 0) {
			cerr << " terminated with signal: " << csig << endl;
		    }
		    else {
			cerr << " terminated with signal: " << csig << " possible error: "
			     << strerror(errno) << endl;
		    }
		}
		else {
		    cerr << " terminated without exiting properly" << endl;
		}
	    }
	}
	exit(1);
    }

    if (cpid == pid) {
	if (core_dump)
	    exit(-1);
	if (WIFEXITED(status))
	   exit(WEXITSTATUS(status));
	else
	    cerr << "Unexpected status";
	exit(-1);
    }

    cout << "\n******************* ";
    vector<Command>::iterator i;
    for (i = commands.begin(); i != commands.end(); i++) {
	if (pid == i->_pid) {
	    if (WIFSIGNALED(status) && WCOREDUMP(status)) {
		cout << "Command: " << i->_command <<  
		    " core dumped " << pid << endl;
		core_dump = true;
	    } else {
		cout << "Command: " << i->_command <<
		    " exited status: " << WEXITSTATUS(status) << " " << 
		    pid << endl;
	    }
	    i->_pid = INVALID_PID;
// 	    commands.erase(i);
	    return;
	}
    }
    cerr << "Unknown pid: " << pid << endl;
}

void
die(int)
{
    for (unsigned int i = 0; i < commands.size(); i++)
	cout << "Command: " << commands[i]._command  << " " << 
	    commands[i]._pid << " did not die\n";
    _exit(-1);
}

/**
 * When this process exits kill all the background processes.
 */
void
tidy()
{
    signal(SIGCHLD, SIG_DFL);
    vector<Command>::iterator i;

    /*
    ** Traverse the list backwards so as to kill the first program
    ** started last.
    */
 restart:
    i = commands.end();
    if (commands.begin() != i) {
	do {
	    i--;
	    if (INVALID_PID != i->_pid) {
		kill(i->_pid, SIGTERM);
	    } else {
		commands.erase(i);
		goto restart;
	    }
	} while(i != commands.begin());
    }

    /*
    ** Wait for ten seconds for the processes that we have sent kills
    ** to to die then exit anyway.
    */
    signal(SIGALRM, die);
    alarm(10);

    for (;;) {
	PID_T pid;
	vector<Command>::iterator i;

	if (commands.empty())
	    return;

	int status;
	pid = wait(&status);

	for (i = commands.begin(); i != commands.end(); i++) {
	    if (pid == i->_pid) {
		commands.erase(i);
		if (commands.empty())
		    return;
		break;
	    }
	}
    }
}

void
usage(const char *myname)
{
    cerr << "usage: " << myname << " [-q] [-v] -c command" << endl;
    exit(-1);
}

int
main(int argc, char *argv[])
{
    const char *silent = NULL;
    const char *output = DEVNULL;
    const char *command = 0;

    int ch;
    while ((ch = getopt(argc, argv, "qvc:")) != -1)
	switch (ch) {
	case 'q':	// Absolutely no output (quiet).
	    silent = DEVNULL;
	    break;
	case 'v':	// All the output from the sub processes.
	    output = NULL;
	    break;
	case 'c':	// The main script.
	    command = optarg;
	    break;
	case '?':
	default:
	    usage(argv[0]);
	}

    if (0 == command)
	usage(argv[0]);

    /*
    ** Read in the commands that we need to start from the standard
    ** input. If there is an '=' on the line then it is assumed that
    ** everything after the '=' is a command to run that will exit
    ** when the real command is ready.
    */
    while (cin) {
	string line;
	getline(cin, line);
	if ("" == line)
	    break;
	vector<string> tokens;
	tokenize(line, tokens, "=");
	switch (tokens.size()) {
	case 1:
	    {
		Command c(tokens[0], SLEEP_CMD);
		commands.push_back(c);
	    }
	    break;
	case 2:
	    {
		Command c(tokens[0], tokens[1]);
		commands.push_back(c);
	    }
	    break;
	default:
	    cerr << "Too many '=''s " << tokens.size() << endl;
	    exit(-1);
	}
    }
    
    /*
    ** Before we spawn the background processes install a signal
    ** handler. If any of the background processes die while we are
    ** executing the main script we can immediately flag an error.
    */
#ifdef SIGCHLD
    signal(SIGCHLD, sigchld);
#endif

    /*
    ** Register the function to called on exit.
    */
    atexit(tidy);

    /*
    ** Start all the background processes.
    */
    for (unsigned int i = 0; i < commands.size(); i++) {
	sigset_t set;
	if (0 != sigemptyset(&set)) {
	    cerr << "sigemptyset failed: " << strerror(errno) << endl;
	    exit(-1);
	}
	if (0 != sigaddset(&set, SIGCHLD)) {
	    cerr << "sigaddset failed: " << strerror(errno) << endl;
	    exit(-1);
	}
	if (0 != sigprocmask(SIG_BLOCK, &set, 0)) {
	    cerr << "sigprocmask failed: " << strerror(errno) << endl;
	    exit(-1);
	}

	commands[i]._pid = xorp_spawn(commands[i]._command, output);

	if (0 != sigprocmask(SIG_UNBLOCK, &set, 0)) {
	    cerr << "sigprockmask failed: " << strerror(errno) << endl;
	    exit(-1);
	}
 	sleep(1);
	if ("" != commands[i]._wait_command) {
	    wait_command = commands[i]._wait_command;

	    /*
	    ** Block SIGCHLD delivery until the xorp_spawn command has completed.
	    ** It seems on Linux the child can run to completion.
	    */
	    if (0 != sigprocmask(SIG_BLOCK, &set, 0)) {
		cerr << "sigprockmask failed: " << strerror(errno) << endl;
		exit(-1);
	    }

	    wait_command_pid = xorp_spawn(commands[i]._wait_command.c_str(),
					  output);

	    if (0 != sigprocmask(SIG_UNBLOCK, &set, 0)) {
		cerr << "sigprockmask failed: " << strerror(errno) << endl;
		exit(-1);
	    }

	    while (INVALID_PID != wait_command_pid) {
		pause();
	    }
	}
    }

    /*
    ** Wait five seconds for the commands that we have started to
    ** settle.
    */
//     sleep(5);

    /*
    ** Start the main script.
    */
    cpid = xorp_spawn(command, silent);
    for (;;)
	pause();

    return 0;
}
