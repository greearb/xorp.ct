## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/utils/Makefile.am,v 1.7 2003/08/25 22:20:11 jcardona Exp $
##

# -- Real Programs
bin_PROGRAMS	 = 

# -- Test Programs
TESTS		 = test_flower_malloc.sh

# -- No-install Programs
noinst_PROGRAMS	 = runit flower_malloc.so test_flower_malloc

# -- No-install Libraries
noinst_LIBRARIES = 

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LIBRARIES)

# -- Program Sources
runit_SOURCES	      = runit.cc
flower_malloc_so_SOURCES = flower_malloc.c
flower_malloc_so_CFLAGS = -fpic -fPIC
flower_malloc_so_LDFLAGS = -shared -fPIC -fpic 
test_flower_malloc_SOURCES = test_flower_malloc.c

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = bogon-be-gone.sed
EXTRA_DIST	+= bogon-be-gone.sh
EXTRA_DIST	+= ndbg
EXTRA_DIST	+= xorpc.sh
EXTRA_DIST	+= xrl_shell_lib.sh
EXTRA_DIST	+= xrl_wrappers_generator.sh
EXTRA_DIST	+= FlowerCheck


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

