// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/libxipc/xrl_tokens.cc,v 1.3 2003/03/10 23:20:29 hodson Exp $"

#include "xrl_tokens.hh"

const char* XrlToken::PROTO_TGT_SEP = "://";
const char* XrlToken::TGT_CMD_SEP = "/";
const char* XrlToken::CMD_ARGS_SEP = "?";
const char* XrlToken::ARG_ARG_SEP = "&";
const char* XrlToken::ARG_NT_SEP = ":";
const char* XrlToken::ARG_TV_SEP = "=";
const char* XrlToken::ARG_RARG_SEP = "->";
const char* XrlToken::LINE_CONT = "\\";
const char* XrlToken::LIST_SEP = ",";
