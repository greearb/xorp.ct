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

// $XORP: xorp/rtrmgr/util.hh,v 1.4 2004/05/26 19:05:03 hodson Exp $

#ifndef __RTRMGR_UTIL_HH__
#define __RTRMGR_UTIL_HH__

/**
 * Tokenize a string by breaking into separate strings when separator
 * character is encountered.
 *
 * @param s string to be tokenized.
 * @param sep separator to break string it.
 * @return list of tokens.
 */
list<string> split(const string& s, char sep);

/**
 * Attempt to find path of named executable.
 *
 * If the supplied string looks like a qualified path then this
 * function returns the directory component, otherwise it tries to
 * locate the named program name in the directories listed in the PATH
 * environment variable.
 *
 * @param progname program to find (typically the argv[0] supplied to main()).
 *
 * @return path of executable on success, empty string on failure.
 */
string find_exec_path_name(const char* progname);

/**
 * Initialize paths.
 *
 * This method attempts to determine where XORP is being run from and
 * initialize paths accordingly.  If the environment variable
 * XORP_ROOT is set, this overrides the path determination algorithm.
 *
 * This method should be called before any
 * of the following methods:
 *
 *   @li @ref xorp_binary_root_dir()
 *   @li @ref xorp_config_root_dir()
 *   @li @ref xorp_template_dir()
 *   @li @ref xorp_xrl_targets_dir()
 *   @li @ref xorp_boot_file()
 *
 * @param argv0 the argv[0] supplied to main().
 */
void xorp_path_init(const char* argv0);

/**
 * Return top-level directory of xorp binaries.
 *
 * @ref xorp_path_init() must be called before this method will return a sane
 * value.
 */
const string& xorp_binary_root_dir();

/**
 * Return top-level directory of xorp configuration files.
 *
 * @ref xorp_path_init() must be called before this method will return a sane
 * value.
 */
const string& xorp_config_root_dir();

/**
 * Return the path of the xorp templates directory given the xorp_root
 * path.
 *
 * @ref xorp_path_init() must be called before this method will return a sane
 * value.
 */
string xorp_template_dir();

/**
 * Return the path of the xrl targets directory given the xorp_root
 * path.
 *
 * @ref xorp_path_init() must be called before this method will return a sane
 * value.
 */
string xorp_xrl_targets_dir();

/**
 * Return path of boot config file to be used given the xorp_root path.
 *
 * @ref xorp_path_init() must be called before this method will return a sane
 * value.
 */
string xorp_boot_file();

/**
 * Return basename of binary.
 *
 * @param argv0 the argv[0] supplied to main().
 */
const char* xorp_basename(const char* argv0);

/**
 * Remove enclosing quotes from string.
 * @param s string that may have enclosing quotes.
 * @return string with quotes removed.
 */
string& unquote(string& s);

/**
 * Remove enclosing quotes from string value.
 * @param s string that may have enclosing quotes.
 * @return copy of string with quotes removed.
 */
string unquote(const string& s);

#endif // __RTRMGR_UTIL_HH__
