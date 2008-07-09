// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/policy/code_list.hh,v 1.7 2007/06/29 02:03:20 pavlin Exp $

#ifndef __POLICY_CODE_LIST_HH__
#define __POLICY_CODE_LIST_HH__


#include <string>
#include <list>

#include "code.hh"


/**
 * @short A collection of code fragments.
 */
class CodeList {
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

    // not impl
    CodeList(const CodeList&);
    CodeList& operator=(const CodeList&);
};

#endif // __POLICY_CODE_LIST_HH__
