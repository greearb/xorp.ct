// vim:set sts=4 ts=8:

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

#ident "$XORP$"

#include "filter.hh"

std::string
filter::filter2str(const filter::Filter& f) {
    switch(f) {
	case IMPORT:
	    return "Import";
	
	case EXPORT_SOURCEMATCH:
	    return "Export-SourceMatch";

	case EXPORT:
	    return "Export";
    }

    return "Unknown";
}
