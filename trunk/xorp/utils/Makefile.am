## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/utils/Makefile.am,v 1.9 2003/10/17 01:44:28 hodson Exp $
##

SUBDIRS = . @FLOWER_MALLOC@

# -- Real Programs
bin_PROGRAMS	 =

# -- Test Programs
TESTS		 =

# -- No-install Programs
noinst_PROGRAMS	 = runit

# -- No-install Libraries
noinst_LIBRARIES =

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LIBRARIES)

# -- Program Sources
runit_SOURCES	      = runit.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = args.sh
EXTRA_DIST	+= bogon-be-gone.sed
EXTRA_DIST	+= bogon-be-gone.sh
EXTRA_DIST	+= ndbg
EXTRA_DIST	+= xorpc.sh
EXTRA_DIST	+= xrl_shell_lib.sh
EXTRA_DIST	+= xrl_wrappers_generator.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

