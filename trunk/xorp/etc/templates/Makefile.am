## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/etc/templates/Makefile.am,v 1.1 2003/07/31 00:34:25 pavlin Exp $
##

# Redefine the data installation directory
datadir = $(prefix)/$(subdir)

# -- Data
data_DATA	  = bgp.cmds bgp.tp
data_DATA	 += fea.cmds fea.tp
data_DATA	 += ospf.tp
data_DATA	 += rib.tp
data_DATA	 += snmp.tp
data_DATA	 += static_routes.tp

# -- Extra files and directories to add to the distribution
EXTRA_DIST	  = $(data_DATA)


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

