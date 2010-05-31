# Copyright (c) 2001-2009 XORP, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, Version 2, June
# 1991 as published by the Free Software Foundation. Redistribution
# and/or modification of this program under the terms of any other
# version of the GNU General Public License is not permitted.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
# see the GNU General Public License, Version 2, a copy of which can be
# found in the XORP LICENSE.gpl file.
# 
# XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
# http://xorp.net

# $XORP: other/tinderbox/scripts/filter-error.awk,v 1.9 2008/10/02 21:32:16 bms Exp $

#
# A simple AWK script to buffer the output of gmake and filter out the
# successful parts of a build.
#

BEGIN {
    depth = 0;			# Gmake spawn depth
    errlog[depth] = "";		# Build log (only printed if errors found).
    errcnt = 0;			# Number of errors seen to date.
}

function dump_error() {
    if (errlog[depth] != "")
	print errlog[depth];

    errlog[depth] = "";
}

/^PASS:/ {
    #
    # Running 'make check', test passed, print line so we can see which
    # test passed, and drop whatever stdout o/p there was.
    #
    print;
    errlog[depth]="";
    next;
}

/^FAIL:/ {
    #
    # Running 'make check', test failed, print line so we can see which
    # test failed, and dump whatever stdout o/p there was.
    #
    dump_error();
    print;
    next;
}

/^TINDERBOX:/ {
    #
    # A message from the tinderbox that should be printed.
    #
    print;
    next;
}

/gmake\[[0-9]+\]/ {
    print
    if ($2 == "Entering") {
	depth++;
	errlog[depth] = "";
    } else if ($2 == "Leaving") {
	depth--;
    } else if ($(NF-1) == "Error") {
	dump_error();
	errcnt++;
    } else if ($(NF) == "Terminated") {
	dump_error();
	errcnt++;
    }
    next;
}

/Stop.$/ {
    # Error with Makefile or related rather than compilation error
    dump_error();
    print;
    errcnt++;
    exit;
}

/^Connection to .* closed by remote host.$/ {
	# ssh died prematurely
	print;
	next;
}

// {
    if (depth == 0) print;
    errlog[depth] = (errlog[depth] $0 "\n");
}

END {
    exit(errcnt);
}
