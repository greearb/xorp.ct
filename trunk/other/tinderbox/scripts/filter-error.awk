#
# A simple script to buffer the output of gmake and filter out the successful
# parts of a build.
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
