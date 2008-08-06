## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/Makefile.am,v 1.21 2007/04/18 01:27:41 pavlin Exp $
##

# --- Options
ETAGS_ARGS		= --c++

# --- Libraries and utilities (directory ordering is important)
SUBDIRS	 = libxorp libcomm libxipc libproto mrt xrl libfeaclient
SUBDIRS += cli policy utils

# --- Applications used by other applications (directory ordering is important)
SUBDIRS += fea rib

# --- Applications (directory ordering should not matter)
SUBDIRS += bgp fib2mrib mld6igmp ospf pim rip rtrmgr static_routes

# --- Tests that make use of protocol libraries
SUBDIRS += policy/test

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

