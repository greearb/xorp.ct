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

// $XORP: xorp/policy/visitor_printer.hh,v 1.7 2008/08/06 08:17:07 abittau Exp $

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
