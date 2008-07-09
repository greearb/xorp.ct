// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/utils/runit.cc,v 1.20 2007/02/16 22:47:32 pavlin Exp $"

#include "libxorp/xorp.h"
#include "libxorp/utils.hh"

#include <iostream>
#include <vector>

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

#ifdef HOST_OS_WINDOWS
typedef HANDLE PID_T;
#define INVALID_PID (INVALID_HANDLE_VALUE)
#else
typedef pid_t PID_T;
#define INVALID_PID (0)
#endif


#ifndef XORP_WIN32_SH_PATH
#define XORP_WIN32_SH_PATH "C:\\MINGW\\BIN\\SH.EXE"
#endif

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

#ifdef HOST_OS_WINDOWS
static HANDLE hTimer = INVALID_HANDLE_VALUE;
#define DEVNULL "NUL:"
#define SLEEP_CMD "sleep 2"
#else
#define DEVNULL "/dev/null"
#define SLEEP_CMD "/bin/sleep 2"
#endif

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
#ifdef HOST_OS_WINDOWS
    HANDLE houtput;
    STARTUPINFOA si;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    PROCESS_INFORMATION pi;

    GetStartupInfoA(&si);

    if (output != NULL) {
	houtput = CreateFileA(output,
			     FILE_READ_DATA | FILE_WRITE_DATA,
			     FILE_SHARE_READ,
			     &sa,
			     OPEN_EXISTING,
			     FILE_ATTRIBUTE_NORMAL,
			     NULL);
	if (houtput == INVALID_HANDLE_VALUE)
		return (0);
	si.hStdInput = houtput;
	si.hStdOutput = houtput;
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    } else {
	si.hStdInput = NULL;	/* XXX: is this OK? */
	si.hStdOutput = NULL;
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    }

    //
    // XXX: Convert POSIX paths to NT ones, and insert shell if needed.
    //
    static const char *exe_suffix = ".exe";
    static const char *sh_suffix = ".sh";
    static const char *sh_interp = XORP_WIN32_SH_PATH;

    string::size_type n;
    string _process = process;

    //fprintf(stderr, "old process string is: '%s'\n", _process.c_str());

    // Strip any leading or trailing white space.
    n = _process.find_first_not_of("\t\n\v\f\r ");
    _process.erase(0, n);

    //fprintf(stderr, "process string after leading space is: '%s'\n", _process.c_str());

    string::size_type _cmd_end = _process.find(' ');
    string _cmd = _process.substr(0, _cmd_end);

    //fprintf(stderr, "old argv[0] is: '%s'\n", _cmd.c_str());

    // Convert slashes.
    _cmd = unix_path_to_native(_cmd);

    //fprintf(stderr, "argv[0] after slashify is: '%s'\n", _cmd.c_str());

    // Deal with shell scripts.
    bool is_shell_script = false;
    if (_cmd.rfind(sh_suffix) != string::npos)
	is_shell_script = true;

    if (!is_shell_script && _cmd.rfind(exe_suffix) == string::npos) {
	    _cmd.append(exe_suffix);
    }

    if (is_shell_script) {
	_cmd.insert(0, " ");
	_cmd.insert(0, sh_interp); 
    }

    //fprintf(stderr, "new argv[0] is: %s\n", _cmd.c_str());

    // Prepend the new argv[0].
    _process.erase(0, _cmd_end);
    _process.insert(0, _cmd);

    //fprintf(stderr, "XXX: about to launch: '%s'\n", _process.c_str());

    if (CreateProcessA(NULL,
		       const_cast<char *>(_process.c_str()),
		       NULL, NULL, TRUE,
		       CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED,
		       NULL, NULL, &si, &pi) == 0) {
	DWORD err = GetLastError();
	CloseHandle(houtput);
	fprintf(stderr, "Failed to exec: %s reason: %ld\n",
		_process.c_str(), err);
	return (0);
    }

    // XXX
    //fprintf(stderr, "XXX: process 0x%08lx launched!\n", pi.hProcess);

    ResumeThread(pi.hThread);
    return (pi.hProcess);

#else /* !HOST_OS_WINDOWS */

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
#endif /* !HOST_OS_WINDOWS */
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

#ifndef HOST_OS_WINDOWS
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
	if (WIFEXITED(status) && 0 != WEXITSTATUS(status)) {
	   cerr << "Wait command: " << wait_command 
	   << " exited with not zero status: " << WEXITSTATUS(status) << endl;
	   exit(-1);
	}
	return;
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
#endif /* !HOST_OS_WINDOWS */

void
die(int)
{
    for (unsigned int i = 0; i < commands.size(); i++)
	cout << "Command: " << commands[i]._command  << " " << 
	    commands[i]._pid << " did not die\n";
    _exit(-1);
}

#ifdef HOST_OS_WINDOWS
CALLBACK void
die_wrapper(LPVOID arg, DWORD timerLow, DWORD timerHigh)
{
    die(0);
    UNUSED(arg);
    UNUSED(timerLow);
    UNUSED(timerHigh);
}

void
cleanup_timer(void)
{
    if (hTimer != INVALID_HANDLE_VALUE) {
	CancelWaitableTimer(hTimer);
	CloseHandle(hTimer);
    }
}
#endif

/**
 * When this process exits kill all the background processes.
 */
void
tidy()
{
#ifndef HOST_OS_WINDOWS
    signal(SIGCHLD, SIG_DFL);
#endif
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
#ifdef HOST_OS_WINDOWS
		GenerateConsoleCtrlEvent(CTRL_C_EVENT, GetProcessId(i->_pid));
		Sleep(200);
		TerminateProcess(i->_pid, 0xFF);
		CloseHandle(i->_pid);
#else
		kill(i->_pid, SIGTERM);
#endif
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
#ifdef HOST_OS_WINDOWS
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    LARGE_INTEGER wt;

    hTimer = CreateWaitableTimer(&sa, TRUE, NULL);
    wt.QuadPart = -(10 * 1000 * 1000 * 10);
    SetWaitableTimer(hTimer, &wt, 0, die_wrapper, NULL, FALSE);
#else
    signal(SIGALRM, die);
    alarm(10);
#endif

    for (;;) {
	PID_T pid;
	vector<Command>::iterator i;

	if (commands.empty())
	    return;

#ifdef HOST_OS_WINDOWS
	/*
	 * Yes, this is lame.
	 */
	HANDLE awhandles[MAXIMUM_WAIT_OBJECTS];
	DWORD cnt;
	DWORD result;

	for (cnt = 0, i = commands.begin();
	     cnt < MAXIMUM_WAIT_OBJECTS && i != commands.end(); cnt++, i++) {
	    awhandles[cnt] = i->_pid;
	}
	result = WaitForMultipleObjectsEx(cnt, awhandles, FALSE, INFINITE,
					  FALSE);
	if (result <= WAIT_OBJECT_0 + cnt - 1) {
	    result -= WAIT_OBJECT_0;
	    pid = awhandles[result];
	} else
	   return;

#else /* !HOST_OS_WINDOWS */

	int status;
	pid = wait(&status);

#endif /* HOST_OS_WINDOWS */

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
#ifdef HOST_OS_WINDOWS
    atexit(cleanup_timer);
#endif
    atexit(tidy);

    /*
    ** Start all the background processes.
    */
    for (unsigned int i = 0; i < commands.size(); i++) {
#ifndef HOST_OS_WINDOWS
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
#endif /* !HOST_OS_WINDOWS */

	commands[i]._pid = xorp_spawn(commands[i]._command, output);

#ifndef HOST_OS_WINDOWS
	if (0 != sigprocmask(SIG_UNBLOCK, &set, 0)) {
	    cerr << "sigprockmask failed: " << strerror(errno) << endl;
	    exit(-1);
	}
 	sleep(1);
#else
	SleepEx(1 * 1000, FALSE);
#endif
	if ("" != commands[i]._wait_command) {
	    wait_command = commands[i]._wait_command;

#ifndef HOST_OS_WINDOWS
	    /*
	    ** Block SIGCHLD delivery until the xorp_spawn command has completed.
	    ** It seems on Linux the child can run to completion.
	    */
	    if (0 != sigprocmask(SIG_BLOCK, &set, 0)) {
		cerr << "sigprockmask failed: " << strerror(errno) << endl;
		exit(-1);
	    }
#endif /* !HOST_OS_WINDOWS */

	    wait_command_pid = xorp_spawn(commands[i]._wait_command.c_str(),
					  output);

#ifdef HOST_OS_WINDOWS
	    WaitForSingleObject((HANDLE)wait_command_pid, INFINITE);
#else
	    if (0 != sigprocmask(SIG_UNBLOCK, &set, 0)) {
		cerr << "sigprockmask failed: " << strerror(errno) << endl;
		exit(-1);
	    }

	    while (INVALID_PID != wait_command_pid) {
		pause();
	    }
#endif
	}
    }

    /*
    ** Wait five seconds for the commands that we have started to
    ** settle.
    */
#ifdef HOST_OS_WINDOWS
    SleepEx(5 * 1000, FALSE);
#else
//     sleep(5);
#endif

    /*
    ** Start the main script.
    */
    cpid = xorp_spawn(command, silent);
#ifdef HOST_OS_WINDOWS
    WaitForSingleObject((HANDLE)cpid, INFINITE);
#else
    for (;;)
	pause();
#endif

    return 0;
}
