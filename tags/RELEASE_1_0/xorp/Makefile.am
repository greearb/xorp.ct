## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/Makefile.am,v 1.14 2004/02/18 00:14:02 pavlin Exp $
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
SUBDIRS += bgp fib2mrib mld6igmp pim rip rtrmgr static_routes

# --- Other (directory ordering should not matter)
SUBDIRS += etc contrib

# --- Conditional applications
SUBDIRS += @MIBS@

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

# --- Stop compilation here if not using GNU make
-include NON_EXISTANT_GNUMAKEFILE

