## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/Makefile.am,v 1.42 2002/10/31 00:28:18 hodson Exp $
##

# --- Options
AUTOMAKE_OPTIONS	= foreign
ETAGS_ARGS		= --c++

# --- Libraries (directory ordering is important)
SUBDIRS	 = libxorp libcomm libxipc libproto mrt xrl cli mfea utils fea

# --- Applications (directory ordering should not matter)
SUBDIRS += bgp mld6igmp pim rtrmgr rib

# --- Conditional applications
SUBDIRS += @OSPFD@ @RIP@

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

# --- Stop compilation here if not using GNU make 
-include NON_EXISTANT_GNUMAKEFILE

