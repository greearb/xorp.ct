## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/Makefile.am,v 1.5 2003/03/27 04:11:16 pavlin Exp $
##

# --- Options
AUTOMAKE_OPTIONS	= foreign
ETAGS_ARGS		= --c++

# --- Libraries (directory ordering is important)
SUBDIRS	 = libxorp libcomm libxipc libproto mrt xrl cli policy mfea

# --- Dual Libraries/Applications (directory ordering should not matter)
SUBDIRS	+= fea rib

# --- Applications (directory ordering should not matter)
SUBDIRS += bgp mld6igmp pim rip rtrmgr utils

# --- Conditional applications
SUBDIRS += @OSPFD@

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

# --- Stop compilation here if not using GNU make 
-include NON_EXISTANT_GNUMAKEFILE

