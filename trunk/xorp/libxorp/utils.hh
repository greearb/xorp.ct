// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/libxorp/utils.hh,v 1.7 2005/08/04 10:30:17 bms Exp $

#ifndef __LIBXORP_UTILS_HH__
#define __LIBXORP_UTILS_HH__

#include <list>
#include <vector>

#include "utility.h"

//
// Set of utilities
//

#define	PATH_CURDIR			"."
#define	PATH_PARENT			".."

#define	NT_PATH_DELIMITER_CHAR		'\\'
#define	NT_PATH_DELIMITER_STRING	"\\"
#define	NT_PATH_ENV_DELIMITER_CHAR	';'
#define	NT_PATH_ENV_DELIMITER_STRING	";"
#define	NT_PATH_DRIVE_DELIMITER_CH	':'
#define	NT_EXECUTABLE_SUFFIX		".exe"

// NT specific
#define	NT_PATH_UNC_PREFIX		"\\\\"
#define	NT_PATH_DRIVE_SUFFIX		":"

#define	UNIX_PATH_DELIMITER_CHAR	'/'
#define	UNIX_PATH_DELIMITER_STRING	"/"
#define	UNIX_PATH_ENV_DELIMITER_CHAR	':'
#define	UNIX_PATH_ENV_DELIMITER_STRING	":"
#define	UNIX_EXECUTABLE_SUFFIX		""

#ifdef	HOST_OS_WINDOWS
#define	PATH_DELIMITER_CHAR		NT_PATH_DELIMITER_CHAR
#define	PATH_DELIMITER_STRING		NT_PATH_DELIMITER_STRING
#define	PATH_ENV_DELIMITER_CHAR		NT_PATH_ENV_DELIMITER_CHAR
#define	PATH_ENV_DELIMITER_STRING	NT_PATH_ENV_DELIMITER_STRING
#define	EXECUTABLE_SUFFIX		NT_EXECUTABLE_SUFFIX
#else	// !HOST_OS_WINDOWS
#define	PATH_DELIMITER_CHAR		UNIX_PATH_DELIMITER_CHAR
#define	PATH_DELIMITER_STRING		UNIX_PATH_DELIMITER_STRING
#define	PATH_ENV_DELIMITER_CHAR		UNIX_PATH_ENV_DELIMITER_CHAR
#define	PATH_ENV_DELIMITER_STRING	UNIX_PATH_ENV_DELIMITER_STRING
#define	EXECUTABLE_SUFFIX		UNIX_EXECUTABLE_SUFFIX
#endif	// HOST_OS_WINDOWS

/*
 * Convert a UNIX style path to the platform's native path format.
 *
 * @param path the UNIX style path to be converted.
 * @return the converted path.
 */
inline string
unix_path_to_native(const string& unixpath)
{
#ifdef HOST_OS_WINDOWS
    string nativepath = unixpath;
    string::size_type n = 0;
    while (string::npos != (n = nativepath.find(UNIX_PATH_DELIMITER_CHAR, n))) {
        nativepath[n] = NT_PATH_DELIMITER_CHAR;
    }
    return (nativepath);
#else
    return string(unixpath);
#endif
}

/*
 * Determine if a provided native path string is an absolute path, or
 * possibly relative to a user's home directory under UNIX.
 *
 * @param path a path in native format to inspect.
 * @param homeok allow paths relative to a home directory to be regarded
 * as absolute paths by this function.
 * @return true if the path if satisfies the criteria for an absolute path.
 */
inline bool
is_absolute_path(const string& path, bool homeok = false)
{
    if (path.empty())
	return false;

#ifdef HOST_OS_WINDOWS
    if ((path.find(NT_PATH_UNC_PREFIX) == 0) ||
	((path.size() >= 2) && isalpha(path[0]) && path[1] == ':'))
        return true;
    return false;
    UNUSED(homeok);
#else
    if (path[0] == '/')
        return true;
    if (homeok && path[0] == '~')
        return true;
    return false;
#endif
}

/**
 * Template to delete a list of pointers, and the objects pointed to.
 * 
 * @param delete_list the list of pointers to objects to delete.
 */
template<class T> void
delete_pointers_list(list<T *>& delete_list)
{
    list<T *> tmp_list;
    
    // Swap the elements, so the original container never contains
    // entries that point to deleted elements.
    swap(tmp_list, delete_list);
    
    for (typename list<T *>::iterator iter = tmp_list.begin();
	 iter != tmp_list.end();
	 ++iter) {
	T *elem = (*iter);
	delete elem;
    }
    tmp_list.clear();
}

/**
 * Template to delete an array of pointers, and the objects pointed to.
 * 
 * @param delete_vector the vector of pointers to objects to delete.
 */
template<class T> void
delete_pointers_vector(vector<T *>& delete_vector)
{
    vector<T *> tmp_vector;
    // Swap the elements, so the original container never contains
    // entries that point to deleted elements.
    swap(tmp_vector, delete_vector);
    
    for (typename vector<T *>::iterator iter = tmp_vector.begin();
	 iter != tmp_vector.end();
	 ++iter) {
	T *elem = (*iter);
	delete elem;
    }
    tmp_vector.clear();
}

/**
 * Create a temporary file with unique name.
 * 
 * This function takes the given file name template, and adds a suffix
 * to it to create an unique file name in a chosen temporary directory.
 * The temporary directory is selected from the following list by choosing
 * the first directory that allows us the create the temporary file
 * (in the given order):
 * (a) The value of one of the following environment variables if defined
 *     (in the given order):
 *     - "TMPDIR"
 *     - "TEMP"   (Windows only)
 *     - "TMP"    (Windows only)
 * (b) Argument @ref tmp_dir if it is not an empty string.
 * (c) The system-specific path of the directory designated for
 *     temporary files:
 *     - GetTempPath() (Windows only)
 * (d) The "P_tmpdir" directory if this macro is defined (typically in
 *     the <stdio.h> include file).
 * (e) The first directory from the following list we can write to:
 *     - "C:\TEMP"  (Windows only)
 *     - "/tmp"     (UNIX only)
 *     - "/usr/tmp" (UNIX only)
 *     - "/var/tmp" (UNIX only)
 *
 * For example, if the file name template is "foo", and if the chosen
 * temporary directory is "/tmp", then the temporary file name would be
 * like "/tmp/foo.XXXXXX" where "XXXXXX" is alpha-numerical suffix
 * chosen automatically to compose the unique file name. The created file
 * has mode 0600 (i.e., readable and writable only by the owner).
 *
 * @param tmp_dir the preferred directory to store the file.
 * @param filename_template the file name template.
 * @param final_filename a return-by-reference argument that on success
 * returns the final name of the temporary file (including the directory name).
 * @param errmsg the error message (if error).
 * @return a file descriptor pointer for the temporary file (opened for
 * reading and writing) on success, otherwise NULL.
 */
FILE*	xorp_make_temporary_file(const string& tmp_dir,
				 const string& filename_template,
				 string& final_filename,
				 string& errmsg);

#endif // __LIBXORP_UTILS_HH__
