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

#ifndef __POLICY_BACKEND_SET_MANAGER_HH__
#define __POLICY_BACKEND_SET_MANAGER_HH__

#include "policy/common/element_base.hh"
#include "policy/common/policy_exception.hh"
#include <string>
#include <map>


/**
 * @short Class that owns all sets. It resolves set names to ElemSet's.
 *
 * Ideally, if the contents of a set changes, a filter should not be
 * reconfigured, but only the sets. This is currently not the case, but there is
 * enough structure to allow it.
 */
class SetManager {
public:
    typedef map<string,Element*> SetMap;

    /**
     * @short Exception thrown when a set with an unknown name is requested.
     */
    class SetNotFound : public PolicyException {
    public:
	SetNotFound(const string& err) : PolicyException(err) {}
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

    // not impl
    SetManager(const SetManager&);
    SetManager& operator=(const SetManager&);
};

#endif // __POLICY_BACKEND_SET_MANAGER_HH__
