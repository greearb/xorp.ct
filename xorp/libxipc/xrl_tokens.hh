// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/libxipc/xrl_tokens.hh,v 1.12 2008/10/02 21:57:26 bms Exp $

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
