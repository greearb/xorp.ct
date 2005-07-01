// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/devnotes/template.cc,v 1.5 2005/03/25 02:52:59 pavlin Exp $"

#include "xorp.h"
#include "utils.hh"

xsock_t
xorp_make_temporary_file(const string& tmp_dir,
			 const string& filename_template,
			 string& final_filename,
			 string& errmsg)
{
    list<string> cand_tmp_dirs;
    char* value;

    if (filename_template.empty()) {
	errmsg = "Empty file name template";
	return (XORP_BAD_SOCKET);
    }

    //
    // Create the list of candidate temporary directories
    //

    // Get the values of environment variables
    value = getenv("TMPDIR");
    if (value != NULL)
	cand_tmp_dirs.push_back(value);
#ifdef HOST_OS_WINDOWS
    value = getenv("TEMP");
    if (value != NULL)
	cand_tmp_dirs.push_back(value);
    value = getenv("TMP");
    if (value != NULL)
	cand_tmp_dirs.push_back(value);
#endif // HOST_OS_WINDOWS

    // Argument "tmp_dir" if it is not an empty string
    if (! tmp_dir.empty())
	cand_tmp_dirs.push_back(tmp_dir);

    // The "P_tmpdir" directory if this macro is defined
#ifdef P_tmpdir
    cand_tmp_dirs.push_back(P_tmpdir);
#endif

    // A list of hard-coded directory names
#ifdef HOST_OS_WINDOWS
    cand_tmp_dirs.push_back("C:\TEMP");
#endif
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
	// Remove the trailing '/' from the directory name
	if (tmp_dir[tmp_dir.size() - 1] == '/')
	    tmp_dir.erase(tmp_dir.size() - 1);

	// Compose the temporary file name and try to create the file
	char filename[MAXPATHLEN];
	string tmp_filename = tmp_dir + "/" + filename_template + ".XXXXXX";
	snprintf(filename, sizeof(filename)/sizeof(filename[0]),
		 tmp_filename.c_str());
	xsock_t s = mkstemp(filename);
	if (s == XORP_BAD_SOCKET)
	    continue;

	// Success
	final_filename = filename;
	return (s);
    }

    errmsg = "Cannot find a directory to create the temporary file";
    return (XORP_BAD_SOCKET);
}
