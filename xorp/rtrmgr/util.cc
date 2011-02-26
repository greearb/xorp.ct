// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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



#include "rtrmgr_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/utility.h"
#include "libxorp/utils.hh"
#include "util.hh"

#ifdef	HOST_OS_WINDOWS
#include <io.h>
#ifdef _NO_OLDNAMES
#define	access(x,y)	_access(x,y)
#define	stat		_stat
#define	S_ISREG		_S_ISREG
#endif
#endif

static string s_cfg_root;
static string s_bin_root;
static string s_config_file;

/**
 * Find the directory of executable program.
 *
 * If the supplied string looks like a qualified path then this
 * function returns the directory component, otherwise it tries to
 * locate the named program name in the directories listed in the PATH
 * environment variable.
 * Note that in our search we do not consider the XORP root directory.
 *
 * @param program_name the name of the program to find.
 *
 * @return directory name of executable program on success, empty string
 * on failure.
 */
static string
find_executable_program_dir(const string& program_name)
{
    debug_msg("%s\n", program_name.c_str());

    if (program_name.size() >= MAXPATHLEN)
	return string("");		// Error: invalid program name

    //
    // Look for trailing slash in program_name
    //
    string::size_type slash = program_name.rfind(PATH_DELIMITER_CHAR);
    if (slash != string::npos) {
	string path = program_name.substr(0, slash);
	return path;
    }

    //
    // Go through the PATH environment variable and find the program location
    //
    string slash_progname(PATH_DELIMITER_STRING);
    slash_progname += program_name;
    slash_progname += EXECUTABLE_SUFFIX;
    const char* s = getenv("PATH");
    while (s != NULL && *s != '\0') {
	const char* e = strchr(s, PATH_ENV_DELIMITER_CHAR);
	string path;
	if (e != NULL) {
	    path = string(s, e);
	    s = e + 1;
	} else {
	    path = string(s);
	    s = NULL;
	}
	string complete_path = path + slash_progname;
	if (access(complete_path.c_str(), X_OK) == 0) {
	    return path;
	}
    }

    return string("");			// Error: nothing found
}

static string
xorp_real_path(const string& path)
{
    debug_msg("path: %s\n", path.c_str());

    char rp[MAXPATHLEN];

#ifdef HOST_OS_WINDOWS
    char *fp = NULL;
    if (GetFullPathNameA(path.c_str(), sizeof(rp), rp, &fp) > 0) {
	debug_msg("return %s\n", rp);
	return string(rp);
    }
#else
    const char* prp = realpath(path.c_str(), rp);
    if (prp != NULL) {
	debug_msg("return %s\n", prp);
	return string(prp);
    }
#endif

    // XLOG_WARNING("realpath(%s) failed.", path.c_str());
    debug_msg("return %s\n", path.c_str());
    return path;
}

void
xorp_path_init(const char* argv0)
{
    const char* xr = getenv("XORP_ROOT");
    if (xr != NULL) {
	s_bin_root = xr;
	s_cfg_root = xr;
	s_config_file = s_cfg_root + "/etc/xorp.conf";
	return;
    }

    string current_root = find_executable_program_dir(argv0) +
			  PATH_DELIMITER_STRING +
			  PATH_PARENT;
    current_root = xorp_real_path(current_root);

    debug_msg("current_root: %s\n", current_root.c_str());

    string build_root = xorp_real_path(XORP_BUILD_ROOT);
    debug_msg("build_root:   %s\n", build_root.c_str());
    if (current_root == build_root) {
	s_bin_root = build_root;
	s_cfg_root = xorp_real_path(XORP_SRC_ROOT);
	s_config_file = s_cfg_root + "/rtrmgr/xorp.conf";

	debug_msg("s_bin_root:   %s\n", s_bin_root.c_str());
	debug_msg("s_cfg_root:   %s\n", s_cfg_root.c_str());
	debug_msg("s_config_file:  %s\n", s_config_file.c_str());

	return;
    }

    string install_root = xorp_real_path(XORP_INSTALL_ROOT);
    s_bin_root = install_root;
    s_cfg_root = install_root;
    s_config_file = s_cfg_root + "/etc/xorp.conf";

    debug_msg("s_bin_root:   %s\n", s_bin_root.c_str());
    debug_msg("s_cfg_root:   %s\n", s_cfg_root.c_str());
    debug_msg("s_config_file:  %s\n", s_config_file.c_str());
}

const string&
xorp_binary_root_dir()
{
    return s_bin_root;
}

const string&
xorp_config_root_dir()
{
    return s_cfg_root;
}

string
xorp_module_dir()
{
    return s_cfg_root + string("/lib/xorp/sbin");
}

string
xorp_command_dir()
{
    return s_cfg_root + string("/lib/xorp/bin");
}

string
xorp_template_dir()
{
    return s_cfg_root + string("/share/xorp/templates");
}

string
xorp_xrl_targets_dir()
{
    return s_cfg_root + string("/share/xorp/xrl/targets");
}

string
xorp_config_file()
{
    return s_config_file;
}

string&
unquote(string& s)
{
    if (s.length() >= 2 && s[0] == '"' && s[s.size() - 1] == '"') {
	s = s.substr(1, s.size() - 2);
    }
    return s;
}

string
unquote(const string& s)
{
    if (s.length() >= 2 && s[0] == '"' && s[s.size() - 1] == '"') {
	return s.substr(1, s.size() - 2);
    }
    return s;
}

bool
is_quotable_string(const string& s)
{
    size_t i;

    for (i = 0; i < s.size(); i++) {
	if (! xorp_isalnum(s[i]))
	    return (true);
    }

    return (false);
}

string
find_executable_filename(const string& program_filename)
{
    string executable_filename;
    struct stat statbuf;

    if (program_filename.size() == 0) {
	return string("");			// Error
    }

    // Assume the path passed to us is a UNIX-style path.
    executable_filename = unix_path_to_native(program_filename);

    //
    // TODO: take care of the commented-out access() calls below (by BMS).
    //
    // Comment out the access() calls for now -- xorpsh does not
    // like them, when running under sudo -u xorp (euid?) and
    // as a result xorpsh fails to start up.
    // Consider checking for it in configure.in and shipping
    // our own if we can't find it on the system.
    //
    if (is_absolute_path(executable_filename)) {
	// Absolute path name
	if (stat(executable_filename.c_str(), &statbuf) == 0 &&
	    // access(executable_filename.c_str(), X_OK) == 0 &&
	    S_ISREG(statbuf.st_mode)) {
	    return executable_filename;
	}
	return string("");			// Error
    }

    // Relative path name
    string xorp_root_dir = xorp_binary_root_dir();

    list<string> path;
    path.push_back(xorp_command_dir());		// XXX FHS
    path.push_back(xorp_root_dir);

    // Expand path
    const char* p = getenv("PATH");
    if (p != NULL) {
	list<string> l2 = split(p, PATH_ENV_DELIMITER_CHAR);
	path.splice(path.end(), l2);
    }

    // Search each path component
    while (!path.empty()) {
	// Don't forget to append the executable suffix if needed.
	string full_path_executable = path.front() + PATH_DELIMITER_STRING +
				      executable_filename + EXECUTABLE_SUFFIX;
#ifdef HOST_OS_WINDOWS
	// Deal with any silly tricks which MSYS may have pulled on
	// us, like using UNIX-style slashes in a DOS-style path. Grr. -bms
	full_path_executable = unix_path_to_native(full_path_executable);
#endif
	if (stat(full_path_executable.c_str(), &statbuf) == 0 &&
	    // access(program_filename.c_str(), X_OK) == 0 &&
	    S_ISREG(statbuf.st_mode)) {
	    executable_filename = full_path_executable;
	    return executable_filename;
	}
	path.pop_front();
    }
    return string("");				// Error
}

void
find_executable_filename_and_arguments(const string& program_request,
				       string& executable_filename,
				       string& program_arguments)
{
    executable_filename = strip_empty_spaces(program_request);
    program_arguments = "";

    string::size_type space;
    space = executable_filename.find(' ');
    if (space == string::npos)
	space = executable_filename.find('\t');

    if (space != string::npos) {
	program_arguments = executable_filename.substr(space + 1);
	executable_filename = executable_filename.substr(0, space);
    }

    executable_filename = find_executable_filename(executable_filename);
    if (executable_filename.empty())
	program_arguments = "";
}
