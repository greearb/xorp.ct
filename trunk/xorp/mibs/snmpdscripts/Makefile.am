## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/utils/Makefile.am,v 1.6 2003/08/01 02:11:51 pavlin Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = 
bin_SCRIPTS	 = getbgppeertable
bin_SCRIPTS	+= getbgpversion
bin_SCRIPTS	+= loadmib
bin_SCRIPTS	+= startsnmp
bin_SCRIPTS	+= stopsnmp
bin_SCRIPTS	+= unloadmib

# -- Test Programs
TESTS		 = 

# -- No-install Programs
noinst_PROGRAMS	 = 

# -- No-install Libraries
noinst_LIBRARIES = 

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LIBRARIES)

# -- Program Sources

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = README
EXTRA_DIST	+= getbgppeertable
EXTRA_DIST	+= getbgpversion
EXTRA_DIST	+= loadmib
EXTRA_DIST	+= snmp.conf
EXTRA_DIST	+= startsnmp.in
EXTRA_DIST	+= stopsnmp
EXTRA_DIST	+= unloadmib


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

