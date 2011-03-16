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

// $XORP: xorp/policy/code_list.hh,v 1.11 2008/10/02 21:57:57 bms Exp $

#ifndef __POLICY_CODE_LIST_HH__
#define __POLICY_CODE_LIST_HH__






#include "code.hh"

/**
 * @short A collection of code fragments.
 */
class CodeList :
    public NONCOPYABLE
{
public:
    /**
     * Initialize codelist.
     *
     * @param policy the policy name.
     */
    CodeList(const string& policy);
    ~CodeList();

    /**
     * Append code to the list.
     * Code is now owned by the code list.
     *
     * @param c code to append. Caller must not delete code.
     */
    void push_back(Code* c);

    /**
     * Obtain string representation of the code list.
     *
     * @return string representation of the code list.
     */
    string str() const;

    /**
     * Links all code in the code list to c.
     * The code is basically added to c.
     *
     * @param c code to link current code list to.
     */
    void link_code(Code& c) const;

    /**
     * Obtain the set of targets the code list has.
     *
     * @param targets argument is filled with targets the code list has.
     */
    void get_targets(Code::TargetSet& targets) const;

    /**
     * Obtain the set of targets of particular filter type the code list has.
     *
     * @param targets argument is filled with targets the code list has.
     * @param filter the filter type.
     */
    void get_targets(Code::TargetSet& targets,
		     const filter::Filter& filter) const;

    /**
     * Return the tags used by a certain protocol for route redistribution,
     * in the code list.
     *
     * @param protocol protocol caller wants tags of.
     * @param tagset filled with policytags used by protocol for route
     * redistribution.
     */
    void get_redist_tags(const string& protocol, Code::TagSet& tagset) const;

private:
    string _policy;		// The policy name

    typedef list<Code*> ListCode;

    ListCode _codes;
};

#endif // __POLICY_CODE_LIST_HH__
