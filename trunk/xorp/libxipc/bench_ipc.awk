#
# $XORP$
#

#
# An AWK script to process and print statistics of data collected
# by script bench_ipc.sh
#

/^XrlAtoms/ {
    if (n > 0)
	dump_stats();

    t = 0;	# total
    tsq = 0;	# total of squares
    n = 0;	# number of readings
    min = 10000000;
    max = 0;
    n_xrl = $5;
}

/^Received/ {
    x = $10;
    t = t + x;
    tsq = tsq + x * x;
    n++;
    if (x > max)
	max = x;
    if (x < min)
	min = x;
}

BEGIN {
    print "# Column 0 XrlAtoms per Xrl";
    print "# Column 1 Mean XRLs per sec";
    print "# Column 2 Std dev of XRLs per sec";
    print "# Column 3 Min XRLs per sec";
    print "# Column 4 Max XRLs per sec";
}

END {
    dump_stats()
}

function dump_stats() {
    m = t / n;
    msq = tsq / n;

    sigma = (msq - m * m) ** 0.5;

    print n_xrl, m, sigma, min, max;
}
