#!/bin/sh
#
# $XORP$
#
# Convert a XORP configuration file into the tabbing form that we are using
# for latex.

expand | sed -e 's/    /\\>/g' -e 's/{/\\&/' -e 's/}/\\&/' -e 's/$/\\\\/'
