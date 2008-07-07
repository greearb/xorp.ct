## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/utils/Makefile.am,v 1.13 2008/05/03 00:05:26 pavlin Exp $
##

SUBDIRS = .

# -- Real Programs
bin_PROGRAMS	 =

# -- Test Programs
TESTS		 =

# -- No-install Programs
noinst_PROGRAMS	 = runit

# -- Libraries and library flags to link the binary programs
LDADD		 =

# -- Program Sources
runit_SOURCES	      = runit.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = args.sh
EXTRA_DIST	+= attach_gdb
EXTRA_DIST	+= bogon-be-gone.sed
EXTRA_DIST	+= bogon-be-gone.sh
EXTRA_DIST	+= fig2dev-ps-anon.sed
EXTRA_DIST	+= gen-pathconf.sh.in
EXTRA_DIST	+= ndbg
EXTRA_DIST	+= rcsid2ident.sh
EXTRA_DIST	+= source_pathconf.sh
EXTRA_DIST	+= xorpc.sh
EXTRA_DIST	+= xrl_shell_lib.sh
EXTRA_DIST	+= xrl_wrappers_generator.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

