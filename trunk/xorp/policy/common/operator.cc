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

#ident "$XORP: xorp/policy/common/operator.cc,v 1.6 2008/07/23 05:11:26 pavlin Exp $"

#include "libxorp/xorp.h"

#include "operator.hh"

// Initialization of static members.
Oper::Hash OpAnd::_hash	    = HASH_OP_AND;
Oper::Hash OpOr::_hash	    = HASH_OP_OR;
Oper::Hash OpXor::_hash	    = HASH_OP_XOR;
Oper::Hash OpNot::_hash	    = HASH_OP_NOT;

Oper::Hash OpEq::_hash	    = HASH_OP_EQ;
Oper::Hash OpNe::_hash	    = HASH_OP_NE;
Oper::Hash OpLt::_hash	    = HASH_OP_LT;
Oper::Hash OpGt::_hash	    = HASH_OP_GT;
Oper::Hash OpLe::_hash	    = HASH_OP_LE;
Oper::Hash OpGe::_hash	    = HASH_OP_GE;

Oper::Hash OpAdd::_hash	    = HASH_OP_ADD;
Oper::Hash OpSub::_hash	    = HASH_OP_SUB;
Oper::Hash OpMul::_hash	    = HASH_OP_MUL;

Oper::Hash OpRegex::_hash   = HASH_OP_REGEX;
Oper::Hash OpCtr::_hash	    = HASH_OP_CTR;
Oper::Hash OpNEInt::_hash   = HASH_OP_NEINT;
Oper::Hash OpHead::_hash    = HASH_OP_HEAD;
