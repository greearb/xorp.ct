// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
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

// $XORP: xorp/policy/visitor.hh,v 1.10 2008/08/06 08:17:07 abittau Exp $

#ifndef __POLICY_VISITOR_HH__
#define __POLICY_VISITOR_HH__

#include <string>

#include "policy/common/element_base.hh"

template<class T> class NodeAny;
typedef NodeAny<string>	NodeVar;

class NodeElem;
class NodeBin;
class NodeUn;
class NodeSet;
class NodeAssign;
class NodeAccept;
class NodeReject;
class NodeProto;
class Term;
class PolicyStatement;
class NodeNext;
class NodeSubr;

/**
 * @short Visitor pattern interface.
 *
 * Inspired by Alexandrescu.
 */
class Visitor {
public:
    virtual ~Visitor() {}

    virtual const Element* visit(NodeUn&) = 0;
    virtual const Element* visit(NodeBin&) = 0;
    virtual const Element* visit(NodeVar&) = 0;
    virtual const Element* visit(NodeAssign&) = 0;
    virtual const Element* visit(NodeSet&) = 0;
    virtual const Element* visit(NodeAccept&) = 0;
    virtual const Element* visit(NodeReject&) = 0;
    virtual const Element* visit(Term&) = 0;
    virtual const Element* visit(PolicyStatement&) = 0;
    virtual const Element* visit(NodeElem&) = 0;
    virtual const Element* visit(NodeProto&) = 0;
    virtual const Element* visit(NodeNext&) = 0;
    virtual const Element* visit(NodeSubr&) = 0;
};

#endif // __POLICY_VISITOR_HH__
