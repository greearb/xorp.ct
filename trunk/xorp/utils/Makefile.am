## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/utils/Makefile.am,v 1.3 2003/05/07 21:58:13 pavlin Exp $
##

# -- Real Programs
bin_PROGRAMS = runit

# -- Test Programs

noinst_PROGRAMS =
noinst_LIBRARIES =

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LIBRARIES)

# -- Program Sources
runit_SOURCES	= runit.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	  = bogon-be-gone.sed
EXTRA_DIST	 += bogon-be-gone.sh
EXTRA_DIST	 += ndbg
EXTRA_DIST	 += xorpc.sh
EXTRA_DIST	 += xrl_shell_lib.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

