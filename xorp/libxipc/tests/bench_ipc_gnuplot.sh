#!/bin/sh

#
# $XORP$
#

#
# A script to plot the IPC performance measurements results.
#

# Terminal and label fonts
SET_TERM="set term post eps \"Times-Roman\" 23"
FONT="Times-Roman, 23"
SET_POINTSIZE="set pointsize 1.8"

# Default setup
XLABEL=""
YlABEL=""
SET_KEY="set key top right"


post_process_eps() {
#    sed 's/BoundingBox: 50 50 410 302/BoundingBox: 60 60 315 290/g' $OFILE >$OFILE.tmp
#    mv $OFILE.tmp $OFILE
}

# Plot functions
plot_eps() {
gnuplot <<EOF
$SET_TERM
$SET_POINTSIZE
set xlabel "$XLABEL" "$FONT"
set ylabel "$YLABEL" "$FONT"
set title  "$TITLE"  "$FONT"
set xrange $XRANGE
set yrange $YRANGE
$SET_KEY
set output "$OFILE"
set nologscale xy
set $LOGSCALE
$PLOT
EOF
post_process_eps
}


# Filter input
filter_input() {
awk '{if ($1 == "#") { print $0 } else { print $1" "$2" "$3} }' $1 > $2
}

# Setup various parameters
setup_vars() {

filter_input $stat2.dat $stat2.dat.gnuplot

IFILE2="$stat2.dat.gnuplot"

# Parameters
PAR_AVE2="using 1:2 t \"$NAME2\" w lp lt 1 lw 2 pt 4"
PAR_BAR2="t \"\" w yerrorbars lt 1 lw 2 pt 4"

}


# Common setup
XLABEL="Number of XRL arguments"
LOGSCALE="nologscale xy"
XRANGE="[*:*]"
YRANGE="[*:*]"
#SET_KEY="set key top right"
PLOT="plot \"$IFILE2\" $PAR_AVE2, \"$IFILE2\" $PAR_BAR2,"

stat2="tcp"
NAME2="TCP"
YLABEL="Performance (XRLs/sec)"
TITLE="XRL performance for various communication families"
setup_vars
OFILE="xrl_performance.eps"
PLOT="plot \"$IFILE2\" $PAR_AVE2, \"$IFILE2\" $PAR_BAR2"
plot_eps

exit 0
