## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/utils/Makefile.am,v 1.8 2003/10/16 18:46:55 pavlin Exp $
##

# -- Real Programs
bin_PROGRAMS	 =

# -- Test Programs
TESTS		 = test_flower_malloc.sh

# -- No-install Programs
noinst_PROGRAMS	 = flower_malloc.so test_flower_malloc

# -- No-install Libraries
noinst_LIBRARIES =

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LIBRARIES)

# -- Program Sources
flower_malloc_so_SOURCES = flower_malloc.c
flower_malloc_so_CFLAGS = -fpic -fPIC
flower_malloc_so_LDFLAGS = -shared -fPIC -fpic
test_flower_malloc_SOURCES = test_flower_malloc.c

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = FlowerCheck


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

