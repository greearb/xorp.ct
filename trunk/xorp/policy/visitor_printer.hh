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

// $XORP: xorp/policy/visitor_printer.hh,v 1.9 2008/10/02 21:58:01 bms Exp $

#ifndef __POLICY_VISITOR_PRINTER_HH__
#define __POLICY_VISITOR_PRINTER_HH__

#include "visitor.hh"
#include "policy_statement.hh"
#include "node.hh"

/**
 * @short This visitor will produce a human readable text stream from a policy.
 *
 * Useful for debugging and checking what the policy manager thinks polcies look
 * like.
 */
class VisitorPrinter : public Visitor {
public:
    /**
     * @param out stream which receives the text representation of policy.
     */
    VisitorPrinter(ostream& out);

    const Element* visit(NodeUn&);
    const Element* visit(NodeBin&);
    const Element* visit(NodeVar&);
    const Element* visit(NodeAssign&);
    const Element* visit(NodeSet&);
    const Element* visit(NodeAccept&);
    const Element* visit(NodeReject&);
    const Element* visit(Term&);
    const Element* visit(PolicyStatement&);
    const Element* visit(NodeElem&);
    const Element* visit(NodeProto&);
    const Element* visit(NodeNext&);
    const Element* visit(NodeSubr& node);

private:
    ostream& _out;
};

#endif // __POLICY_VISITOR_PRINTER_HH__
