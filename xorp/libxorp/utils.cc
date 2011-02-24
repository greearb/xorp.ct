// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "xorp.h"
#include "c_format.hh"
#include "utils.hh"


#ifdef __WIN32__
int nonblock(int s, bool nb) {
    unsigned long lnb = !!nb;
    if (ioctlsocket(s, FIONBIO, &nb) == SOCKET_ERROR) {
	return -1
    }
    else {
	return 0;
    }
}
#endif


list<string>
split(const string& s, char ch)
{
    list<string> parts;
    string s2 = s;
    size_t ix;

    ix = s2.find(ch);
    while (ix != string::npos) {
	parts.push_back(s2.substr(0, ix));
	s2 = s2.substr(ix + 1, s2.size() - ix);
	ix = s2.find(ch);
    }
    if (!s2.empty())
	parts.push_back(s2);
    return parts;
}

string
strip_empty_spaces(const string& s)
{
    string res = s;

    // Strip the heading and trailing empty spaces
    while (!res.empty()) {
	size_t len = res.length();
	if ((res[0] == ' ') || (res[0] == '\t')) {
	    res = res.substr(1, len - 1);
	    continue;
	}
	if ((res[len - 1] == ' ') || (res[len - 1] == '\t')) {
	    res = res.substr(0, res.length() - 1);
	    continue;
	}
	break;
    }

    return res;
}

bool
has_empty_space(const string& s)
{
    string::size_type space;

    space = s.find(' ');
    if (space == string::npos)
	space = s.find('\t');

    if (space != string::npos)
	return (true);

    return (false);
}

const char*
xorp_basename(const char* argv0)
{
    const char* p = strrchr(argv0, PATH_DELIMITER_CHAR);
    if (p != NULL) {
	return p + 1;
    }
    return argv0;
}

FILE*
xorp_make_temporary_file(const string& tmp_dir,
			 const string& filename_template,
			 string& final_filename,
			 string& errmsg)
{
    char filename[MAXPATHLEN];
    list<string> cand_tmp_dirs;
    char* value;
    FILE* fp;

    if (filename_template.empty()) {
	errmsg = "Empty file name template";
	return (NULL);
    }

    //
    // Create the list of candidate temporary directories
    //

    // Get the values of environment variables
    value = getenv("TMPDIR");
    if (value != NULL)
	cand_tmp_dirs.push_back(value);

    // Argument "tmp_dir" if it is not an empty string
    if (! tmp_dir.empty())
	cand_tmp_dirs.push_back(tmp_dir);

    // The system-specific path of the directory designated for temporary files

    // The "P_tmpdir" directory if this macro is defined
#ifdef P_tmpdir
    cand_tmp_dirs.push_back(P_tmpdir);
#endif

    // A list of hard-coded directory names
    cand_tmp_dirs.push_back("/tmp");
    cand_tmp_dirs.push_back("/usr/tmp");
    cand_tmp_dirs.push_back("/var/tmp");

    //
    // Find the first directory that allows us to create the temporary file
    //
    list<string>::iterator iter;
    for (iter = cand_tmp_dirs.begin(); iter != cand_tmp_dirs.end(); ++iter) {
	string tmp_dir = *iter;
	if (tmp_dir.empty())
	    continue;
	// Remove the trailing '/' (or '\') from the directory name
	if (tmp_dir.substr(tmp_dir.size() - 1, 1) == PATH_DELIMITER_STRING)
	    tmp_dir.erase(tmp_dir.size() - 1);

	filename[0] = '\0';

	// Compose the temporary file name and try to create the file
	string tmp_filename = tmp_dir + PATH_DELIMITER_STRING +
			      filename_template + ".XXXXXX";
	snprintf(filename, sizeof(filename)/sizeof(filename[0]), "%s",
		 tmp_filename.c_str());

	int fd = mkstemp(filename);
	if (fd == -1)
	    continue;

	// Associate a stream with the file descriptor
	fp = fdopen(fd, "w+");
	if (fp == NULL) {
	    close(fd);
	    continue;
	}

	// Success
	final_filename = filename;
	return (fp);
    }

    errmsg = "Cannot find a directory to create the temporary file";
    return (NULL);
}
