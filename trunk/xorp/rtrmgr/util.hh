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

// $XORP: xorp/rtrmgr/split.hh,v 1.2 2003/03/10 23:21:01 hodson Exp $

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

#endif // __RTRMGR_UTIL_HH__
