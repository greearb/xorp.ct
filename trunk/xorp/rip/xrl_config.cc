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

#ident "$XORP: xorp/devnotes/template.cc,v 1.1.1.1 2002/12/11 23:55:54 hodson Exp $"

#include "xrl_config.hh"

static const char* _rib_name = "rib";
static const char* _fea_name = "fea";

const char*
xrl_rib_name()
{
    return _rib_name;
}

const char*
xrl_fea_name()
{
    return _fea_name;
}

void
set_xrl_rib_name(const char* n)
{
    _rib_name = n;
}

void
set_xrl_fea_name(const char* n)
{
    _fea_name = n;
}

