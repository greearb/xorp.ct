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

#ifndef __POLICY_COMMON_ELEMENT_BASE_HH__
#define __POLICY_COMMON_ELEMENT_BASE_HH__

#include <string>

/**
 * @short Basic object type used by policy engine.
 *
 * This element hierarchy is similar to XrlAtom's but exclusive to policy
 * components.
 */
class Element {
public:
    /**
     * Every element has a type which is representable by a string. This type is
     * later used by the policy semantic checker to ensure validity of policy
     * statements.
     *
     * @param type unique string representation of element type.
     */
    Element(const std::string& type) : _type(type) {}

    virtual ~Element() {}

    /**
     * Every element must be representable by a string. This is a requirement
     * to enable the policy manager to send elements to the backend filters via
     * XRL calls for example.
     *
     * @return string representation of the element.
     */
    virtual std::string str() const = 0;

    /**
     * @return string representation of element type.
     */
    const std::string& type() const { return _type; }


private:
    /**
     * Type must be unique as it is used by dispatcher for determining the
     * correct callback to execute.
     */
    const std::string _type;
};

#endif // __POLICY_COMMON_ELEMENT_BASE_HH__
