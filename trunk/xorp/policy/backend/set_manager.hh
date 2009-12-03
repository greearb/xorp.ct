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

// $XORP: xorp/policy/backend/set_manager.hh,v 1.9 2008/10/02 21:58:04 bms Exp $

#ifndef __POLICY_BACKEND_SET_MANAGER_HH__
#define __POLICY_BACKEND_SET_MANAGER_HH__

#include <string>
#include <map>

#include <boost/noncopyable.hpp>

#include "policy/common/element_base.hh"
#include "policy/common/policy_exception.hh"

/**
 * @short Class that owns all sets. It resolves set names to ElemSet's.
 *
 * Ideally, if the contents of a set changes, a filter should not be
 * reconfigured, but only the sets. This is currently not the case, but there is
 * enough structure to allow it.
 */
class SetManager :
    public boost::noncopyable
{
public:
    typedef map<string,Element*> SetMap;

    /**
     * @short Exception thrown when a set with an unknown name is requested.
     */
    class SetNotFound : public PolicyException {
    public:
        SetNotFound(const char* file, size_t line, const string& init_why = "")   
	  : PolicyException("SetNotFound", file, line, init_why) {}  
    };

    SetManager();
    ~SetManager();

    /**
     * Return the corresponding ElemSet for the requested set name.
     *
     * @return the ElemSet requested.
     * @param setid name of set wanted.
     */
    const Element& getSet(const string& setid) const;
   
    /**
     * Resplace all sets with the given ones.
     * Caller must not delete them.
     *
     * @param sets the new sets that should be used.
     */
    void replace_sets(SetMap* sets);

    /**
     * Zap all sets.
     */
    void clear();

private:
    SetMap* _sets;
};

#endif // __POLICY_BACKEND_SET_MANAGER_HH__
