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

// $XORP: xorp/policy/visitor.hh,v 1.12 2008/10/02 21:58:01 bms Exp $

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
