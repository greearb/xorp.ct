## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/mibs/snmpdscripts/Makefile.am,v 1.3 2004/11/12 02:44:44 bms Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = 
bin_SCRIPTS	 = getbgppeertable
bin_SCRIPTS	+= getbgpversion
bin_SCRIPTS	+= startsnmp
bin_SCRIPTS	+= stopsnmp

# -- Test Programs
TESTS		 = 

# -- No-install Programs
noinst_PROGRAMS	 = 

# -- Libraries
lib_LTLIBRARIES = 

# -- Libraries and library flags to link the binary programs
LDADD		 = $(lib_LTLIBRARIES)

# -- Program Sources

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = README
EXTRA_DIST	+= getbgppeertable
EXTRA_DIST	+= getbgpversion
EXTRA_DIST	+= snmp.conf
EXTRA_DIST	+= startsnmp.in
EXTRA_DIST	+= stopsnmp


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

