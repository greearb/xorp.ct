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

#ident "$XORP: xorp/libxorp/utils.cc,v 1.3 2005/07/08 01:27:13 atanu Exp $"

#include "xorp.h"
#include "c_format.hh"
#include "utils.hh"

FILE*
xorp_make_temporary_file(const string& tmp_dir,
			 const string& filename_template,
			 string& final_filename,
			 string& errmsg)
{
#ifdef HOST_OS_WINDOWS
    char dirname[MAXPATHLEN];
#endif // HOST_OS_WINDOWS
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

    // The system-specific path of the directory designated for temporary files
#ifdef HOST_OS_WINDOWS
    size_t size;
    size = GetTempPathA(sizeof(dirname), dirname);
    if (size >= sizeof(dirname)) {
	errmsg = c_format("Internal error: directory name buffer size is too "
			  "small (allocated %u required %u)",
			  XORP_UINT_CAST(sizeof(dirname)),
			  XORP_UINT_CAST(size + 1));
	return (NULL);
    }
    if (size != 0) {
	cand_tmp_dirs.push_back(dirname);
    }
#endif // HOST_OS_WINDOWS

    // The "P_tmpdir" directory if this macro is defined
#ifdef P_tmpdir
    cand_tmp_dirs.push_back(P_tmpdir);
#endif

    // A list of hard-coded directory names
#ifdef HOST_OS_WINDOWS
    cand_tmp_dirs.push_back("C:\\TEMP");
#else
    cand_tmp_dirs.push_back("/tmp");
    cand_tmp_dirs.push_back("/usr/tmp");
    cand_tmp_dirs.push_back("/var/tmp");
#endif

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

#ifdef HOST_OS_WINDOWS
	// Get the temporary filename and open the file
	snprintf(dirname, sizeof(dirname)/sizeof(dirname[0]), tmp_dir.c_str());
	if (GetTempFileNameA(dirname, filename_template.c_str(), 0,
	    filename) == 0)
	    continue;
	fp = fopen(filename, "w+");
	if (fp == NULL)
	    continue;

#else

	// Compose the temporary file name and try to create the file
	string tmp_filename = tmp_dir + PATH_DELIMITER_STRING +
			      filename_template + ".XXXXXX";
	snprintf(filename, sizeof(filename)/sizeof(filename[0]),
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
#endif // ! HOST_OS_WINDOWS

	// Success
	final_filename = filename;
	return (fp);
    }

    errmsg = "Cannot find a directory to create the temporary file";
    return (NULL);
}
