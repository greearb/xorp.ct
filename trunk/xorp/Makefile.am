## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/Makefile.am,v 1.2 2003/02/13 04:39:37 mjh Exp $
##

# --- Options
AUTOMAKE_OPTIONS	= foreign
ETAGS_ARGS		= --c++

# --- Libraries (directory ordering is important)
SUBDIRS	 = libxorp libcomm libxipc libproto mrt xrl cli policy mfea utils fea

# --- Applications (directory ordering should not matter)
SUBDIRS += bgp mld6igmp pim rtrmgr rib

# --- Conditional applications
SUBDIRS += @OSPFD@

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

# --- Stop compilation here if not using GNU make 
-include NON_EXISTANT_GNUMAKEFILE

