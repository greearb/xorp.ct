// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

// $XORP: xorp/libxipc/xrl_tokens.hh,v 1.4 2003/03/10 23:20:30 hodson Exp $

#ifndef __LIBXIPC_XRL_TOKENS_HH__
#define __LIBXIPC_XRL_TOKENS_HH__

struct XrlToken {
    // Protocol - Target separator
    static const char* PROTO_TGT_SEP;
    
    // Target - Command separator
    static const char* TGT_CMD_SEP;
    
    // Command - Arguments separator
    static const char* CMD_ARGS_SEP;

    // Argument-Argument separator
    static const char* ARG_ARG_SEP;

    // Argument Name-Type separator
    static const char* ARG_NT_SEP;

    // Argument Type-Value separator
    static const char* ARG_TV_SEP;

    // Input Argument list - Output argument list separator
    static const char* ARG_RARG_SEP;

    // Line Continuation
    static const char* LINE_CONT;

    // XrlAtomList item separator
    static const char* LIST_SEP;
};

#define TOKEN_BYTES(x) (strlen(x) + 1)

#endif // __LIBXIPC_XRL_TOKENS_HH__
