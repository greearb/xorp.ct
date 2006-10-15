/* /////////////////////////////////////////////////////////////////////////////
 * File:    glob.c
 *
 * Purpose: Definition of the glob() API functions for the Win32 platform.
 *
 * Created  13th November 2002
 * Updated: 17th February 2005
 *
 * Home:    http://synesis.com.au/software/
 *
 * Copyright 2002-2005, Matthew Wilson and Synesis Software
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer. 
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the names of Matthew Wilson and Synesis Software nor the names of
 *   any contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * ////////////////////////////////////////////////////////////////////////// */

#include "libxorp/xorp.h"

#ifdef HOST_OS_WINDOWS


#ifndef _SYNSOFT_DOCUMENTATION_SKIP_SECTION
# define _SYNSOFT_VER_C_GLOB_MAJOR      2
# define _SYNSOFT_VER_C_GLOB_MINOR      0
# define _SYNSOFT_VER_C_GLOB_REVISION   2
# define _SYNSOFT_VER_C_GLOB_EDIT       32
#endif /* !_SYNSOFT_DOCUMENTATION_SKIP_SECTION */

/* /////////////////////////////////////////////////////////////////////////////
 * Includes
 */

#include <windows.h>
#include <errno.h>
#include <stdlib.h>

#include "glob_win32.h"

#ifndef NUM_ELEMENTS
# define NUM_ELEMENTS(x)        (sizeof(x) / sizeof(0[x]))
#endif /* !NUM_ELEMENTS */

/* /////////////////////////////////////////////////////////////////////////////
 * Helper functions
 */

static char const *strrpbrk(char const *string, char const *strCharSet)
{
    char        *part   =   NULL;
    char const  *pch;

    for(pch = strCharSet; *pch; ++pch)
    {
        char    *p  =   strrchr(string, *pch);

        if(NULL != p)
        {
            if(NULL == part)
            {
                part = p;
            }
            else
            {
                if(part < p)
                {
                    part = p;
                }
            }
        }
    }

    return part;
}

/* /////////////////////////////////////////////////////////////////////////////
 * API functions
 */

/* It gives you back the matched contents of your pattern, so for Win32, the
 * directories must be included
 */

int glob(   char const  *pattern
        ,   int         flags
        , int         (*errfunc)(char const *, int)
        ,   glob_t      *pglob)
{
    int         result;
    char        szRelative[1 + _MAX_PATH];
    char const  *file_part  =   strrpbrk(pattern, "\\/");

    ((void)errfunc);

    if(NULL != file_part)
    {
        ++file_part;

        lstrcpyA(szRelative, pattern);
        szRelative[file_part - pattern] = '\0';
    }
    else
    {
        szRelative[0] = '\0';
    }

#if 0
    if(!GetFullPathNameA(pattern, NUM_ELEMENTS(szRelative), szRelative, &file_part))
    {
        result = GLOB_NOMATCH;
    }
    else
#endif /* 0 */
    {
        WIN32_FIND_DATAA    find_data;
        HANDLE              hFind = FindFirstFileA(pattern, &find_data);

        pglob->gl_pathc = 0;
        pglob->gl_pathv = NULL;

        if(hFind == INVALID_HANDLE_VALUE)
        {
            result = GLOB_NOMATCH;
        }
        else
        {
            int     cbCurr      =   0;
            size_t  cbAlloc     =   0;
            char    *buffer     =   NULL;
            int     cMatches    =   0;

            result = 0;

            do
            {
                int     cch;
                size_t  new_cbAlloc;

                if(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if(NULL != file_part)
                    {
                        /* Pattern must begin with '.' to match either dots directory */
                        if( *file_part != '.' &&
                            (0 == lstrcmpA(".", find_data.cFileName) ||
                            0 == lstrcmpA("..", find_data.cFileName)))
                        {
                            continue;
                        }
                    }

                    if(flags & GLOB_MARK)
                    {
                        lstrcatA(find_data.cFileName, "/");
                    }
                }
                else
                {
                    if(flags & GLOB_ONLYDIR)
                    {
                        // Skip all further actions, and get the next entry
                        continue;
                    }
                }

                cch =   lstrlenA(find_data.cFileName);
                if(NULL != file_part)
                {
                    cch +=  file_part - pattern;
                }

                new_cbAlloc = cbCurr + cch + 1;
                if(new_cbAlloc > cbAlloc)
                {
                    char    *new_buffer;

                    new_cbAlloc = (new_cbAlloc + 31) & ~(31);

                    new_buffer  = (char*)realloc(buffer, new_cbAlloc);

                    if(new_buffer == NULL)
                    {
                        result = GLOB_NOSPACE;
                        free(buffer);
                        buffer = NULL;
                        break;
                    }

                    buffer = new_buffer;
                    cbAlloc = new_cbAlloc;
                }

                lstrcpynA(buffer + cbCurr, szRelative, 1 + (file_part - pattern));
                lstrcatA(buffer + cbCurr, find_data.cFileName);
                cbCurr += cch + 1;

                ++cMatches;
            }
            while(FindNextFileA(hFind, &find_data));

            FindClose(hFind);

            if(result == 0)
            {
                char    *new_buffer =   (char*)realloc(buffer, cbAlloc + (1 + cMatches) * sizeof(char*));

                if(new_buffer == NULL)
                {
                    result = GLOB_NOSPACE;
                    free(buffer);
/*                     buffer = NULL; */
                }
                else
                {
                    char    **pp    = (char**)new_buffer;
                    char    **begin =   pp;
                    char    **end   =   begin + cMatches;
                    char    *next_str;

                    buffer = new_buffer;

                    memmove(new_buffer + (1 + cMatches) * sizeof(char*), new_buffer, cbAlloc);

                    if(flags & GLOB_NOSORT)
                    {
                        /* The way we need in order to test the removal of dots in the findfile_sequence. */
                        *end = NULL;
                        for(begin = pp, next_str = buffer + (1 + cMatches) * sizeof(char*); begin != end; --end)
                        {
                            *(end - 1) = next_str;

                            // Find the next string.
                            next_str += 1 + lstrlenA(next_str);
                        }
                    }
                    else
                    {
                        /* The normal way. */
                        for(begin = pp, next_str = buffer + (1 + cMatches) * sizeof(char*); begin != end; ++begin)
                        {
                            *begin = next_str;

                            // Find the next string.
                            next_str += 1 + lstrlenA(next_str);
                        }
                        *begin = NULL;
                    }

                    pglob->gl_pathc =   cMatches;
                    pglob->gl_matchc=   cMatches;
                    pglob->gl_offs  =   0;
                    pglob->gl_flags =   0;
                    pglob->gl_pathv =   pp;
                }
            }

            if(0 == cMatches)
            {
                result = GLOB_NOMATCH;
            }
        }
    }

    return result;
}

void globfree(glob_t *pglob)
{
    if(pglob != NULL)
    {
        free(pglob->gl_pathv);
        pglob->gl_pathv = NULL;
    }
}

/* ////////////////////////////////////////////////////////////////////////// */

#endif /* HOST_OS_WINDOWS */
