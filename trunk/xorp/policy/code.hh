// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP$

#ifndef __POLICY_CODE_HH__
#define __POLICY_CODE_HH__

#include "policy/common/filter.hh"
#include <string>
#include <set>

/**
 * @short This structure represents the intermediate language code.
 *
 * It contains the actual code for the policy, its target, and names of the sets
 * referenced. It also contains the policytags referenced.
 */
struct Code {
public:
    /**
     * @short A target represents where the code should be placed.
     *
     * A target consists of a protocol and a filter type. It identifies exactly
     * which filter of which protocol has to be configured with this code.
     */
    struct Target {
	string protocol;

	filter::Filter filter;

	Target() {}
	/**
	 * Construct a target [protocol/filter pair].
	 *
	 * @param p target protocol.
	 * @param f target filter.
	 */
	Target(const string& p, filter::Filter f) : protocol(p), filter(f) {}

	/**
	 * Operator to compare Targets. Needed for STL set storage.
	 *
	 * @return true if target is less than argument
	 * @param rhs target to compare with
	 */ 
	bool operator<(const Target& rhs) const;

	bool operator==(const Target& rhs) const;
	bool operator!=(const Target& rhs) const;

	/**
	 * @return string representation of target.
	 */
	string str() const;
    };

    typedef set<Target> TargetSet;

    /**
     * @return string representation of code.
     */
    string str();

    
    /**
     * Appends code to current code. It enables for chunks of code to be linked.
     *
     * @return reference update code.
     * @param rhs code to link.
     */
    Code& operator +=(const Code& rhs);


    Target _target;

    string _code;
    set<string> _sets;

    // if it is export filter code, these are the tags used by the code.
    typedef set<uint32_t> TagSet;

    // if it is an export filter code, these are source protocols which will
    // send routes to the target
    set<string> _source_protos;

    TagSet _tags;
};

#endif // __POLICY_CODE_HH__
