## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/Makefile.am,v 1.11 2003/08/06 21:42:18 hodson Exp $
##

# --- Options
AUTOMAKE_OPTIONS	= foreign
ETAGS_ARGS		= --c++

# --- Libraries and utilities (directory ordering is important)
SUBDIRS	 = libxorp libcomm libxipc libproto mrt xrl libfeaclient
SUBDIRS += cli policy utils

# --- Dual Libraries/Applications (directory ordering should not matter)
SUBDIRS	+= fea rib

# --- Applications (directory ordering should not matter)
SUBDIRS += bgp mld6igmp pim rip rtrmgr

# --- Other (directory ordering should not matter)
SUBDIRS += etc

# --- Conditional applications
SUBDIRS += @OSPFD@ @MIBS@

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

# --- Stop compilation here if not using GNU make
-include NON_EXISTANT_GNUMAKEFILE

