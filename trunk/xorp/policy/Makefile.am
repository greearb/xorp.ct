## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/policy/Makefile.am,v 1.4 2003/05/07 21:58:12 pavlin Exp $
##

# -- Real Programs
bin_PROGRAMS	 = 

# -- Test Programs
check_PROGRAMS	 = test_policy
TESTS		 = $(check_PROGRAMS)

# -- No-install Programs
noinst_PROGRAMS  = 

# -- No-install Libraries
noinst_LIBRARIES	 = libpolicy.a

# -- Libraries and library flags to link the binary programs
LDADD			 = $(noinst_LIBRARIES)
LDADD			+= $(top_builddir)/libxorp/libxorp.a

# -- Program Sources
test_policy_SOURCES	 = test_policy.cc

# -- Library Sources
libpolicy_a_SOURCES  = policy_module.h
libpolicy_a_SOURCES += version.h
libpolicy_a_SOURCES += statement.hh
libpolicy_a_SOURCES += term.hh
libpolicy_a_SOURCES += from.hh
libpolicy_a_SOURCES += to.hh
libpolicy_a_SOURCES += then.hh
libpolicy_a_SOURCES += action.hh
libpolicy_a_SOURCES += policy_route.hh

libpolicy_a_SOURCES += statement.cc
libpolicy_a_SOURCES += term.cc
libpolicy_a_SOURCES += from.cc
libpolicy_a_SOURCES += to.cc
libpolicy_a_SOURCES += then.cc
libpolicy_a_SOURCES += action.cc
libpolicy_a_SOURCES += policy_route.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	  = 


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

