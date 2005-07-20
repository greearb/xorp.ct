// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP$"

#include "config.h"
#include "version_filters.hh"
#include "version_filter.hh"

const string VersionFilters::filter_import("filter_im");
const string VersionFilters::filter_sm("filter_sm");
const string VersionFilters::filter_ex("filter_ex");

VersionFilters::VersionFilters() : PolicyFilters(
				    new VersionFilter(filter_import),
				    new VersionFilter(filter_sm),
				    new VersionFilter(filter_ex))
{
}
