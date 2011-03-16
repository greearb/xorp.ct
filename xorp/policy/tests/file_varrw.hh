// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/policy/test/file_varrw.hh,v 1.14 2008/10/02 21:58:08 bms Exp $

#ifndef __POLICY_TEST_FILE_VARRW_HH__
#define __POLICY_TEST_FILE_VARRW_HH__

#include "policy/common/varrw.hh"
#include "policy/common/policy_exception.hh"
#include "policy/common/element_factory.hh"




class FileVarRW : public VarRW {
public:
    class Error : public PolicyException {
    public:
        Error(const char* file, size_t line, const string& init_why = "")   
            : PolicyException("Error", file, line, init_why) {}  
    };

    FileVarRW();
    ~FileVarRW();

    const Element&  read(const Id&);
    void	    write(const Id&, const Element&);
    void	    sync();
    void	    printVars();
    void	    load(const string& fname);
    void	    set_verbose(bool);

private:
    static const int MAX_TRASH = 666;

    bool doLine(const string& line);
    void clear_trash();

    const Element*  _map[VAR_MAX];
    ElementFactory  _ef;
    Element*	    _trash[MAX_TRASH];
    int		    _trashc;
    bool	    _verbose;
};

#endif // __POLICY_TEST_FILE_VARRW_HH__
