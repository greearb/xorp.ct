#!/bin/sh

CONFIG="$(dirname $0)/config"
. $CONFIG

cd ${ROOTDIR}/xorp

( ./configure 1>&2 ) && ( gmake -k $* 1>&2 )


