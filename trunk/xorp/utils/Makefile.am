## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/utils/Makefile.am,v 1.5 2003/07/30 23:43:32 pavlin Exp $
##

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
runit_SOURCES	 = runit.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = bogon-be-gone.sed
EXTRA_DIST	+= bogon-be-gone.sh
EXTRA_DIST	+= ndbg
EXTRA_DIST	+= xorpc.sh
EXTRA_DIST	+= xrl_shell_lib.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

