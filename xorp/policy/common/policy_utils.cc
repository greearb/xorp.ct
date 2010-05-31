// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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



#include "libxorp/xorp.h"

#ifdef HAVE_REGEX_H
#  include <regex.h>
#else // ! HAVE_REGEX_H
#  ifdef HAVE_PCRE_H
#    include <pcre.h>
#  endif
#  ifdef HAVE_PCREPOSIX_H
#    include <pcreposix.h>
#  endif
#endif // ! HAVE_REGEX_H

#include "policy_utils.hh"


namespace policy_utils {

void 
str_to_list(const string& in, list<string>& out)
{
    string::size_type pos1 = 0;	// beginning of token
    string::size_type pos2 = 0;  // end of token
    string::size_type len = in.length();
    string token;

    while(pos1 < len) {

	// find delimiter
        pos2 = in.find(",",pos1);

	// none found, so treat end of string as delim
        if(pos2 == string::npos) {
                token = in.substr(pos1,len-pos1);

                out.push_back(token);
                return;
        }
	
	// grab token [delimiter found].
        token = in.substr(pos1,pos2-pos1);
        out.push_back(token);
        pos1 = pos2+1;
    }
}

void 
str_to_set(const string& in, set<string>& out)
{
    list<string> tmp;

    // XXX: slow
    str_to_list(in,tmp);

    for(list<string>::iterator i = tmp.begin();
	i != tmp.end(); ++i)

	// since it is a set, dups will be removed.
	out.insert(*i);
}

void
read_file(const string& fname, string& out)
{
    char buff[4096];
    int rd;

    string err;

    // open file
    FILE* f = fopen(fname.c_str(),"r");
    if(!f) {
        err += "Unable to open file " + fname + ": ";
        err += strerror(errno);

        xorp_throw(PolicyUtilsErr, err);
    }

    buff[0] = 0;

    // read file
    while(!feof(f)) {
        rd = fread(buff,1,sizeof(buff)-1,f);
        if(rd == 0)
            break;
        if(rd < 0) {
            err += "Unable to read file " + fname + ": ";
            err += strerror(errno);

            fclose(f);
            xorp_throw(PolicyUtilsErr, err);
        }

	// append to content of file to out
        buff[rd] = 0;
        out += buff;
    }

    fclose(f);
    return;
}

unsigned 
count_nl(const char* x)
{
    const char* end = &x[strlen(x)];
    unsigned nl = 0;

    for(const char* ptr = x; ptr < end; ptr++)
	if(*ptr == '\n')
	    nl++;

    return nl;
}

bool
regex(const string& str, const string& reg)
{
    // compile the regex
    regex_t re;
    int res = regcomp(&re, reg.c_str(), REG_EXTENDED);
    
    if (res) {
	char tmp[128];
	string err;

	regerror(res, &re, tmp, sizeof(tmp));
	regfree(&re);

	err = "Unable to compile regex (" + reg;
	err += "): ";
	err += tmp;

	xorp_throw(PolicyUtilsErr, err);
    }

    // execute the regex [XXX: check for errors!!]
    bool result = !regexec(&re, str.c_str(), 0, 0, 0);
    regfree(&re);

    return result;
}

} // namespace
