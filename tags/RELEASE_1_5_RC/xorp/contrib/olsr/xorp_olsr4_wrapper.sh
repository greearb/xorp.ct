#!/bin/sh

# XXX assumes we are run from top level
WRAPPED_CMD="olsr/xorp_olsr4"
LOGFILE="/tmp/xorp_olsr4.log"
UHOME="/home/bms"
VALGRIND="$(which valgrind)"

VALGRIND_OPTS=" \
	--tool=memcheck					\
	--leak-check=yes				\
	--leak-resolution=high				\
	--num-callers=10				\
	--show-reachable=yes				\
	--demangle=no					\
	--suppressions=$UHOME/.valgrind/xorp.supp	\
	--error-limit=no				\
	--verbose					\
	"

#set -x

UNAME=$(uname)
if [ ${UNAME} = "FreeBSD" ]; then
	VALGRIND_OPTS="${VALGRIND_OPTS} \
		       --suppressions=$UHOME/.valgrind/fbsd.supp"
fi

VERS="$(${VALGRIND} --version)"
if [ $? -ne 0 ]; then
	# old valgrind
	VALGRIND_OPTS="${VALGRIND_OPTS} --logfile=${LOGFILE}"
else
	# new valgrind
	VALGRIND_OPTS="${VALGRIND_OPTS} --log-file=${LOGFILE}"
fi

if [ "${VALGRIND}" ]; then
	CMD="${VALGRIND} ${VALGRIND_OPTS} ${WRAPPED_CMD}"
else
	CMD="${WRAPPED_CMD}"
fi

exec ${CMD}
