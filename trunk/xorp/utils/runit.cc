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

#ident "$XORP: xorp/utils/runit.cc,v 1.3 2003/03/05 02:11:48 atanu Exp $"

#include "config.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include <iostream>
#include <string>
#include <vector>

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
 * "/dev/null" the "-v" flag stops this redirection. The "-q" sends
 * all output to "/dev/null".
 */

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

    while(string::npos != begin || string::npos != end) {
        tokens.push_back(str.substr(begin, end - begin));
        begin = str.find_first_not_of(delimiters, end);
        end = str.find_first_of(delimiters, begin);
    }
}

/**
 * Start a new process in the background.
 * @param process Absolute pathname and arguments.
 * @param output Optional file to redirect file descriptors 0,1 and 2.
 * @return Return the process id of the new process.
 */
pid_t
spawn(const string& process, const char *output = "")
{
    pid_t pid;
    switch(pid = fork()) {
    case 0:
	{
	    if(output != "") {
		close(0);
		close(1);
		close(2);
		open(output, O_RDONLY);
		open(output, O_WRONLY);
		open(output, O_WRONLY);
	    }

	    vector<string> tokens;
	    tokenize(process, tokens);
	    char *argv[tokens.size() + 1];
	    for(unsigned int i = 0; i < tokens.size(); i++) {
		argv[i] = const_cast<char *>(tokens[i].c_str());
	    }
	    argv[tokens.size()] = 0;

	    execv(argv[0], argv);
	    cerr << "Failed to exec: " << process << endl;
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
    static const pid_t EMPTY = 0;
    Command(string command, string wait_command) : 
	_command(command), 
	_wait_command(wait_command),
	_pid(EMPTY)
    {}
    string _command;	// The actual command that we wish to run.
    string _wait_command;// The command to run that exits when the
			 // command is ready.
    pid_t _pid;
};

/**
 * vector of all commands.
 */
vector<Command> commands;
/**
 * Process ID of main script/program.
 */
pid_t cpid;

/**
 * Process ID of wait script.
 */
pid_t wait_command_pid;
string wait_command;

bool core_dump = false;

/**
 * Signal handler that reaps dead children.
 */
void
sigchld(int)
{
    int status;
    pid_t pid = wait(&status);

    if(wait_command_pid == pid) {
	wait_command_pid = Command::EMPTY;
	if(WIFEXITED(status) && 0 != WEXITSTATUS(status)) {
	   cerr << "Wait command: " << wait_command 
	   << " exited with not zero status: " << WEXITSTATUS(status) << endl;
	   exit(-1);
	}
	return;
    }

    if(cpid == pid) {
	if(core_dump)
	    exit(-1);
	if(WIFEXITED(status))
	   exit(WEXITSTATUS(status));
	else
	    cerr << "Unexpected status";
	exit(-1);
    }

    cout << "\n******************* ";
    vector<Command>::iterator i;
    for(i = commands.begin(); i != commands.end(); i++) {
	if(pid == i->_pid) {
	    if(WIFSIGNALED(status) && WCOREDUMP(status)) {
		cout << "Command: " << i->_command <<  
		    " core dumped " << pid << endl;
		core_dump = true;
	    } else {
		cout << "Command: " << i->_command <<
		    " exited status: " << WEXITSTATUS(status) << " " << 
		    pid << endl;
	    }
	    commands.erase(i);
	    return;
	}
    }
    cerr << "Unknown pid: " << pid << endl;
}

void
die(int)
{
    for(unsigned int i = 0; i < commands.size(); i++)
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
    if(commands.begin() != i) {
	do {
	    i--;
	    if(Command::EMPTY != i->_pid) {
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

    for(;;) {
	if(commands.empty())
	    return;
	
	int status;
	pid_t pid = wait(&status);

	vector<Command>::iterator i;
	for(i = commands.begin(); i != commands.end(); i++) {
	    if(pid == i->_pid) {
// 		cout << "Command: " << i->_command << " killed\n";
		commands.erase(i);
		if(commands.empty())
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
    const char *silent = "";
    const char *output = "/dev/null";
    const char *command = 0;

    int ch;
    while((ch = getopt(argc, argv, "qvc:")) != -1)
	switch(ch) {
	case 'q':	// Absolutely no output (quiet).
	    silent = "/dev/null";
	    break;
	case 'v':	// All the output from the sub processes.
	    output = "";
	    break;
	case 'c':	// The main script.
	    command = optarg;
	    break;
	case '?':
	default:
	    usage(argv[0]);
	}

    if(0 == command)
	usage(argv[0]);

    /*
    ** Read in the commands that we need to start from the standard
    ** input. If there is an '=' on the line then it is assumed that
    ** everything after the '=' is a command to run that will exit
    ** when the real command is ready.
    */
    while(cin) {
	string line;
	getline(cin, line);
	if("" == line)
	    break;
	vector<string> tokens;
	tokenize(line, tokens, "=");
	switch(tokens.size()) {
	case 1:
	    {
		Command c(tokens[0], "/bin/sleep 2");
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
    signal(SIGCHLD, sigchld);

    /*
    ** Register the function to called on exit.
    */
    atexit(tidy);

    /*
    ** Start all the background processes.
    */
    for(unsigned int i = 0; i < commands.size(); i++) {
	commands[i]._pid = spawn(commands[i]._command, output);
	sleep(1);
	if("" != commands[i]._wait_command) {
	    wait_command = commands[i]._wait_command;
	    wait_command_pid = spawn(commands[i]._wait_command.c_str(),
				     output);
	    while(Command::EMPTY != wait_command_pid)
		pause();
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
    cpid = spawn(command, silent);
    for(;;)
	pause();

    return 0;
}
