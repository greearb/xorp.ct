// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/policy/code.hh,v 1.9 2008/01/04 03:17:08 pavlin Exp $

#ifndef __POLICY_CODE_HH__
#define __POLICY_CODE_HH__

#include "policy/common/filter.hh"
#include <string>
#include <set>

/**
 * @short This class represents the intermediate language code.
 *
 * It contains the actual code for the policy, its target, and names of the
 * sets referenced. It also contains the policytags referenced.
 */
class Code {
public:
    /**
     * @short A target represents where the code should be placed.
     *
     * A target consists of a protocol and a filter type. It identifies exactly
     * which filter of which protocol has to be configured with this code.
     */
    class Target {
    public:
	/**
	 * Default constructor.
	 */
	Target() {}

	/**
	 * Construct a target [protocol/filter pair].
	 *
	 * @param p target protocol.
	 * @param f target filter.
	 */
	Target(const string& p, filter::Filter f) : _protocol(p), _filter(f) {}

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
	 * Get the protocol.
	 *
	 * @return the protocol.
	 */
	const string protocol() const { return _protocol; }

	/**
	 * Set the protocol.
	 *
	 * @param protocol the protocol name.
	 */
	void set_protocol(const string& protocol) { _protocol = protocol; }

	/**
	 * Get the filter type.
	 *
	 * @return the filter type.
	 */
	filter::Filter filter() const { return _filter; }

	/**
	 * Set the filter type.
	 *
	 * @param filter the filter type.
	 */
	void set_filter(const filter::Filter& filter) { _filter = filter; }

	/**
	 * @return string representation of target.
	 */
	string str() const;

    private:
	string		_protocol;
	filter::Filter	_filter;
    };

    typedef set<Target> TargetSet;

    // if it is export filter code, these are the tags used by the code.
    typedef set<uint32_t> TagSet;

    /**
     * Get the target.
     *
     * @return a reference to the target.
     */
    const Code::Target& target() const { return _target; }

    /**
     * Set the target.
     *
     * @param target the target.
     */
    void set_target(const Code::Target target) { _target = target; }

    /**
     * Set the target protocol.
     *
     * @param protocol the target protocol name.
     */
    void set_target_protocol(const string& protocol);

    /**
     * Set the target filter type.
     *
     * @param filter the target filter type.
     */
    void set_target_filter(const filter::Filter& filter);

    /**
     * Get the actual code.
     *
     * @return a reference to the actual code.
     */
    const string& code() const { return _code; }

    /**
     * Set the actual code.
     *
     * @param code the actual code.
     */
    void set_code(const string& code) { _code = code; }

    /**
     * Add to the actual code.
     *
     * @param code the code to add.
     */
    void add_code(const string& code) { _code += code; }

    /**
     * Get the names of the sets referenced by this code.
     *
     * @return a reference to the names of the sets referenced by this code.
     */
    const set<string> referenced_set_names() const {
	return _referenced_set_names;
    }

    /**
     * Set the names of the sets referenced by this code.
     *
     * @param set_names the names of the sets referenced by this code.
     */
    void set_referenced_set_names(const set<string>& set_names) {
	_referenced_set_names = set_names;
    }

    /**
     * Add the name of a set referenced by this code.
     *
     * @param set_name the name of the set referenced by this code.
     */
    void add_referenced_set_name(const string& set_name) {
	_referenced_set_names.insert(set_name);
    }

    /**
     * Remove the names of all sets referenced by this code.
     */
    void clear_referenced_set_names() { _referenced_set_names.clear(); }

    /*
     * Get the set of source protocols.
     *
     * @return a reference to the set of source protocols.
     */
    const set<string>& source_protocols() const { return _source_protocols; }

    /**
     * Add a source protocol.
     *
     * @param protocol the protocol to add.
     */
    void add_source_protocol(const string& protocol) {
	_source_protocols.insert(protocol);
    }

    /**
     * Get the set with all tags.
     *
     * @return a reference to the set with all tags.
     */
    const Code::TagSet& all_tags() const { return _all_tags; }

    /**
     * Get the set with the tags used for route redistribution to other
     * protocols.
     *
     * @return a reference to the set with tags used for route redistribution.
     */
    const Code::TagSet& redist_tags() const { return _redist_tags; }

    /**
     * Add a tag.
     *
     * @param tag the tag to add.
     * @param is_redist_tag if true, the tag is used for route redistribution
     * to other protocols.
     */
    void add_tag(uint32_t tag, bool is_redist_tag) {
	_all_tags.insert(tag);
	if (is_redist_tag)
	    _redist_tags.insert(tag);
    }

    /**
     * @return string representation of code.
     */
    string str();

    /**
     * Appends code to current code. It enables for chunks of code to be
     * linked.
     *
     * @return reference to the updated code.
     * @param rhs code to link.
     */
    Code& operator +=(const Code& rhs);

private:
    Target	_target;

    string	_code;
    set<string>	_referenced_set_names;

    // if it is an export filter code, these are source protocols which will
    // send routes to the target
    set<string>	_source_protocols;

    TagSet	_all_tags;
    TagSet	_redist_tags;	// The tags used for route redistribution
};

#endif // __POLICY_CODE_HH__
