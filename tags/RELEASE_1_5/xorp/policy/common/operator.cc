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

#ident "$XORP: xorp/policy/common/operator.cc,v 1.5 2008/01/04 03:17:19 pavlin Exp $"

#include "libxorp/xorp.h"

#include "operator.hh"

// Initialization of static members.
Oper::Hash OpAnd::_hash = 0;
Oper::Hash OpOr::_hash = 0;
Oper::Hash OpXor::_hash = 0;
Oper::Hash OpNot::_hash = 0;

Oper::Hash OpEq::_hash = 0;
Oper::Hash OpNe::_hash = 0;
Oper::Hash OpLt::_hash = 0;
Oper::Hash OpGt::_hash = 0;
Oper::Hash OpLe::_hash = 0;
Oper::Hash OpGe::_hash = 0;

Oper::Hash OpAdd::_hash = 0;
Oper::Hash OpSub::_hash = 0;
Oper::Hash OpMul::_hash = 0;

Oper::Hash OpRegex::_hash = 0;
Oper::Hash OpCtr::_hash = 0;
Oper::Hash OpNEInt::_hash = 0;

Oper::Hash OpHead::_hash = 0;
