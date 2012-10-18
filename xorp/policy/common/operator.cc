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
Oper::Hash OpDiv::_hash	    = HASH_OP_DIV;
Oper::Hash OpLShift::_hash  = HASH_OP_LSHIFT;
Oper::Hash OpRShift::_hash  = HASH_OP_RSHIFT;
Oper::Hash OpBitAnd::_hash  = HASH_OP_BITAND;
Oper::Hash OpBitOr::_hash   = HASH_OP_BITOR;
Oper::Hash OpBitXor::_hash   = HASH_OP_BITXOR;

Oper::Hash OpRegex::_hash   = HASH_OP_REGEX;
Oper::Hash OpCtr::_hash	    = HASH_OP_CTR;
Oper::Hash OpNEInt::_hash   = HASH_OP_NEINT;
Oper::Hash OpHead::_hash    = HASH_OP_HEAD;
