/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2009 XORP, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, Version
 * 2.1, June 1999 as published by the Free Software Foundation.
 * Redistribution and/or modification of this program under the terms of
 * any other version of the GNU Lesser General Public License is not
 * permitted.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
 * see the GNU Lesser General Public License, Version 2.1, a copy of
 * which can be found in the XORP LICENSE.lgpl file.
 * 
 * XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
 * http://xorp.net
 */

#include "xorp_config.h"

#include "libxorp/xorp.h"
//#include "libxorp/debug.h"


#include <cassert>

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif
#ifdef HAVE_CXXABI_H
#include <cxxabi.h>
#endif

/*
 * Support for C/C++ symbolic backtraces, for runtime diagnostics.
 * Intended for use in both debug and production builds.
 */
#ifdef DEBUG_BACKTRACE
static const char **_xorp_backtrace_alloc();
static _xorp_backtrace_free(const char **data);
#endif

/**
 * @short Wrapper function for the platform's backtrace facilities,
 * if present.
 * @return pointer to an array of char*, allocated with malloc().
 */
extern "C"
const char **
xorp_backtrace_alloc()
{
#ifdef DEBUG_BACKTRACE
    return (_xorp_backtrace_alloc());
#else
    return (NULL);
#endif
}

extern "C"
void
xorp_backtrace_free(const char **data)
{
#ifdef DEBUG_BACKTRACE
    return (_xorp_backtrace_free());
#else
    return (NULL);
#endif
}

/**
 * Back-end implementation of runtime backtrace facility.
 *
 * @return pointer to an array of char* containing each backtrace line. The
 * storage must be freed with xorp_backtrace_free().
 *
 * XXX MUST be exception safe, apart from std::bad_alloc.
 *
 * Implementation notes:
 *  Currently it is implemented using the GLIBC-compatible backtrace()
 *  and backtrace_symbols() functions. On GLIBC systems, these are part
 *  of the libc library. On BSD systems, these are part of libexecinfo,
 *  a third-party implementation.
 *
 *  It is not guaranteed to produce correct results when the C/C++ runtime
 *  heap is corrupt, or when the stack is corrupt.
 *
 *  The output of backtrace() is an array of C strings containing
 *  function names and offsets. It will try to resolve the return addresses
 *  on the stack to symbols, using dladdr(), which is arch-independent.
 *  In some situations this might require building all executables with
 *  '-rdynamic', to get meaningful backtraces from stripped binaries.
 *
 *  It knows nothing about arguments or other stack frame entities.
 *  As most of the code here is C++, we need to post-process this output.
 *  This involves string processing, which in turn needs a working heap
 *  to store temporary allocations.
 *
 * FUTURE: This could be implemented using e.g. libunwind, libdwarf
 * to produce richer output (e.g. using separate symbol files
 * from the production shipping binaries, much like Microsoft's PDB format),
 * and this might insulate us from heap corruption.
 *
 * The approach taken really needs to be similar to that of a last-change
 * exception handler, to be useful in situations of catastrophic heap
 * corruption.
 *
 * XXX use of vector or array is a boost::scoped_array candidate.
 *     But we're in C and ABI land here, so stick to C.
 *     One malloc here is bad enough if the heap is toasted.
 *
 */
#ifdef DEBUG_BACKTRACE
static const int BACKTRACE_MAX = 128;

static const char **
_xorp_backtrace_alloc()
{
#if defined(HAVE_BACKTRACE) && defined(HAVE_BACKTRACE_SYMBOLS)
    int i, nentries;
    void **entries, **final;
    char **syms, **cp, **np;

    entries = calloc(BACKTRACE_MAX, sizeof(void *));
    assert(entries != NULL);

    nentries = backtrace(entries, BACKTRACE_MAX);
    if (nentries == 0) {
	free(entries);
	return (NULL);
    }

    // TODO skip first entry (it's this function).
    // We'll probably get inlined, so don't skip both.

    syms = backtrace_symbols(entries, nentries);
    assert(syms != NULL);

    /* Dispose of now-unused raw backtrace. */
    free(entries);

    /*
     * The C string vector returned by backtrace_symbols() is
     * allocated with malloc(), however, the symbol names themselves
     * are usually owned by the dynamic linker, and SHOULD NOT be freed.
     *
     * However, as we need to rewrite these names for C++ demangling,
     * and we'll be passing them around, we need to copy them. We also
     * need to delimit the array, as the caller doesn't see 'nentries'.
     * This allows us to have
     *
     * If the demangler is available from the C++ ABI, we do this
     * piecemeal. Otherwise, we reallocate upfront.
     *
     * XXX The libexecinfo implementation produces different output
     * than that of GLIBC. For now, let's produce consistent output,
     * and parse what we get.
     */
    final = calloc(nentries+1, sizeof(char *));
    assert(final != NULL);

#ifdef HAVE___CXA_DEMANGLE

    for (i = 0; i < ) {

    }

#else /* !HAVE___CXA_DEMANGLE */

    // TODO: munge the args in each line.
    for (i = 0, cp = syms, np = final; i < nentries; i++, cp++, final++)
	final = strdup(*cp);
	/* If out of heap space, all bets are off. */
	assert(*final != NULL);
    }
    *cp = NULL;

#endif /* HAVE___CXA_DEMANGLE */

    return (final);
#else /* ! (defined(HAVE_BACKTRACE) && defined(HAVE_BACKTRACE_SYMBOLS)) */

    return (NULL);
#endif /* defined(HAVE_BACKTRACE) && defined(HAVE_BACKTRACE_SYMBOLS) */
}

// XXX: CONSTIFY

static void
_xorp_backtrace_free(const char **data)
{
    char **cp;

    for (cp = (char **)data; cp != NULL; cp++)
	free(*cp);

    free(data);
}
#endif /* DEBUG_BACKTRACE */
